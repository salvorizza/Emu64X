#include "UI/Application/ApplicationManager.h"

#include "UI/Panels/ViewportPanel.h"
#include "UI/Panels/CPUStatusPanel.h"
#include "UI/Panels/MemoryEditorPanel.h"
#include "UI/Panels/DisassemblerPanel.h"
#include "UI/Panels/ConsolePanel.h"
#include "UI/Panels/FileDialogPanel.h"

#include "UI/Graphics/SoftwareRenderer.h"
#include "UI/Graphics/BatchRenderer.h"
#include "UI/Window/FontAwesome5.h"
#include "UI/Window/ControllerManager.h"
#include "UI/Utils.h"
#include "UI/Application/LoopTimer.h"

#include "Utils/LoggingSystem.h"

#include "Base/Base.h"
#include "Base/Bus.h"
#include "Core/MIPS/VR4300/VR4300.h"
#include "Core/SIExternalBus.h"
#include "Core/RDRAM.h"
#include "Core/PIExternalBus.h"
#include "Core/RCP/RCP.h"


#ifdef ESX_PLATFORM_WINDOWS
	#include <Windows.h>
	#undef ERROR
#endif // ESX_PLATFORM_WINDOWS

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include <iostream>

#include "optick.h"
#include "miniaudio.h"



using namespace esx;
#undef max;
#undef min;

struct FPSCounter {
	LoopTimer Timer = {};
	I32 MaxFPS = 0;
	I32 MinFPS = 0;
	I32 CurrentFPS = 0;
	U64 NumSamples = 0;
	U64 SumFPS = 0;

	void Init() {
		Timer.init();
		MaxFPS = 0;
		MinFPS = 0;
		CurrentFPS = 0;
		NumSamples = 0;
		SumFPS = 0;
	}

	void Update() {
		Timer.update();
		NumSamples++;

		CurrentFPS = (I32)(1.0 / Timer.getDeltaTimeInSeconds());
		MaxFPS = std::max<I32>(MaxFPS, CurrentFPS);
		MinFPS = std::min<I32>(MinFPS, CurrentFPS);
		SumFPS += CurrentFPS;
	}

	I32 AvgFPS() {
		if (NumSamples == 0) return 0;
		return SumFPS / NumSamples;
	}
};

class Emu64XLogger : public Logger {
public:
	Emu64XLogger(const SharedPtr<ConsolePanel>& consolePanel)
		: Logger(ESX_TEXT("Core")),
			mConsolePanel(consolePanel)
	{}

	~Emu64XLogger() = default;

	virtual void Log(LogType type, const StringView& message) override {
		if ((I32)type < mLogLevel) return;

		auto& items = mConsolePanel->getInternalConsole().System().Items();
		if (items.size() > 1000) {
			//items.pop_front();
		}

		if (mConsolePanel) {
			switch (type)
			{
				case esx::LogType::Info:
					mConsolePanel->getInternalConsole().System().Log(csys::ItemType::INFO) << message;
					break;
				case esx::LogType::Trace:
					mConsolePanel->getInternalConsole().System().Log(csys::ItemType::LOG) << message;
					break;
				case esx::LogType::Warning:
					mConsolePanel->getInternalConsole().System().Log(csys::ItemType::WARNING) << message;
					break;
				case esx::LogType::Error:
					mConsolePanel->getInternalConsole().System().Log(csys::ItemType::ERROR) << message;
					break;
				case esx::LogType::Fatal:
					mConsolePanel->getInternalConsole().System().Log(csys::ItemType::ERROR) << message;
					break;
				default:
					break;
			}
		}
	}

	void SetLogLevel(LogType logLevel) {
		mLogLevel = (I32)logLevel;
	}
private:
	SharedPtr<ConsolePanel> mConsolePanel;
	I32 mLogLevel = -1;
};

class Emu64X : public Application {
public:
	Emu64X()
		: Application("Emu64X", "commons/icons/Logo.ico")
	{
	}

	~Emu64X() {
	}

	virtual void onSetup() override {
		mConsolePanel = MakeShared<ConsolePanel>();
		mLogger = MakeShared<Emu64XLogger>(mConsolePanel);
		LoggingSystem::SetCoreLogger(mLogger);

		//mLogger->SetLogLevel(LogType::Error);

		mCPUStatusPanel = MakeShared<CPUStatusPanel>();
		mDisassemblerPanel = MakeShared<DisassemblerPanel>();
		mMemoryEditorPanel = MakeShared<MemoryEditorPanel>();
		mBatchRenderer = MakeShared<BatchRenderer>();
		mSoftwareRenderer = MakeShared<SoftwareRenderer>();
		mViewportPanel = MakeShared<ViewportPanel>();
		mFileDialogPanel = MakeShared<FileDialogPanel>();

		mFileDialogPanel->setCurrentPath("commons/games");
		mFileDialogPanel->setOnFileSelectedCallback(std::bind(&Emu64X::onFileSelected, this, std::placeholders::_1));

		constexpr size_t t = MIBI(8);
		root = MakeShared<Bus>(ESX_TEXT("Root"));
		mVR4300 = MakeShared<VR4300>();
		mRDRAM = MakeShared<RDRAM>();
		mRCP = MakeShared<RCP>();
		mPIExternalBus = MakeShared<PIExternalBus>();
		mSIExternalBus = MakeShared<SIExternalBus>("commons/bios/boot.rom");

		root->connectDevice(mVR4300);
		mVR4300->connectToBus(root);

		root->connectDevice(mRDRAM);
		mRDRAM->connectToBus(root);

		root->connectDevice(mRCP);
		mRCP->connectToBus(root);

		root->connectDevice(mPIExternalBus);
		mPIExternalBus->connectToBus(root);

		root->connectDevice(mSIExternalBus);
		mSIExternalBus->connectToBus(root);

		root->sortRanges();

		mCPUStatusPanel->setInstance(mVR4300);
		mCPUStatusPanel->setInstance(mRCP->getCore());
		mDisassemblerPanel->setInstance(mVR4300);
		mDisassemblerPanel->setInstance(mRCP->getCore());
		mDisassemblerPanel->setBus(root);
		mMemoryEditorPanel->setInstance(root);

		mDisassemblerPanel->setAddressingFunc(DisassemblerProc::VR4300, [&]() {
			U32 baseAddress = 0x00000000;
			U32 adressingSize = 0x03F80000;
			if (ImGui::BeginTabBar("SelectDisassembleRom"))
			{
				if (ImGui::BeginTabItem("Instructions")) {
					baseAddress = 0x00000000;
					adressingSize = 0x03F80000;
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Bios")) {
					baseAddress = 0x1FC00000;
					adressingSize = 0x000007C0;
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("IMEM")) {
					baseAddress = 0x04001000;
					adressingSize = 0x00001000;
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("DMEM")) {
					baseAddress = 0x04000000;
					adressingSize = 0x00001000;
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
			return std::make_pair(baseAddress, adressingSize);
		});

		mDisassemblerPanel->setAddressingFunc(DisassemblerProc::RSP, [&]() {
			U32 baseAddress = 0x00000000;
			U32 adressingSize = 0x1000;
			if (ImGui::BeginTabBar("SelectDisassembleRom"))
			{
				if (ImGui::BeginTabItem("Instructions")) {
					baseAddress = 0x00000000;
					adressingSize = 0x1000;
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
			return std::make_pair(baseAddress, adressingSize);
		});

		mDisassemblerPanel->setDecodeFunc(DisassemblerProc::VR4300, [&](U32* physAddress) {
			esx::VR4300Instruction cpuInstruction;

			U32 opcode = mVR4300->getBus(ESX_TEXT("Root"))->load(*physAddress, 0, sizeof(U32));
			mVR4300->decode(cpuInstruction, opcode, *physAddress, ESX_TRUE);

			DisassemblerPanel::Instruction instruction;
			instruction.Address = *physAddress;
			instruction.Mnemonic = cpuInstruction.Mnemonic(mVR4300);

			return instruction;
		});

		mDisassemblerPanel->setDecodeFunc(DisassemblerProc::RSP, [&](U32* physAddress) {
			esx::R4000Instruction cpuInstruction;
			U32 addr = *physAddress;
			*physAddress += 0x04001000;

			U32 opcode = mRCP->SysADLoad(*physAddress, sizeof(U32));
			mRCP->getCore()->decode(cpuInstruction, opcode, addr, ESX_TRUE);

			DisassemblerPanel::Instruction instruction;
			instruction.Address = addr;
			instruction.Mnemonic = cpuInstruction.Mnemonic(mRCP->getCore());

			return instruction;
		});

		mBatchRenderer->Begin();
		mSoftwareRenderer->Begin();

		ma_device_config config = ma_device_config_init(ma_device_type_playback);
		config.playback.format = ma_format_s16;
		config.playback.channels = 2;
		config.sampleRate = 44100;
		config.dataCallback = audioCallback;
		config.pUserData = this;

		if (ma_device_init(NULL, &config, &mAudioDevice) != MA_SUCCESS) {
			ESX_CORE_LOG_ERROR("failed to init MiniAudio");
		}

		ma_device_start(&mAudioDevice);
		mNumPrerendered = PRERENDERED_SIZE;

		InputManager::Init();
		fpsCounter.Init();
	}


	void onFileSelected(const std::filesystem::path& filePath) {
		mPIExternalBus->loadGame(filePath.string());
		mCurrentGame = mPIExternalBus->getGameCode();
		hardReset();
	}

	static void audioCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
	{
		/*if (frameCount == 0) {
			return;
		}

		EmuStationXApp* pApp = (EmuStationXApp*)pDevice->pUserData;
		auto spu = pApp->spu;

		std::scoped_lock<std::mutex> lc(spu->mSamplesMutex);
		if (spu->mFrontBuffer.Complete() == ESX_FALSE) {
			return;
		}

		std::memcpy(pOutput, spu->mFrontBuffer.Batch.data(), sizeof(AudioFrame) * frameCount);*/
	}

	virtual void onUpdate() override {
		mDisassemblerPanel->onUpdate();


		mViewportPanel->setFrame(mBatchRenderer->getPreviousFrame());
		fpsCounter.Update();
	}

	virtual void onRender() override {
	}

	virtual void onCleanUp() override {
		ma_device_uninit(&mAudioDevice);
	}

	virtual void onImGuiRender(const SharedPtr<ImGuiManager>& pManager, const SharedPtr<Window>& pWindow) override {
		static bool p_open = true;

		mFileDialogPanel->setIconForExtension(".*", pManager->LoadIconResource("commons/icons/file.png"), "FILE");
		mFileDialogPanel->setFolderIcon(pManager->LoadIconResource("commons/icons/folder.png"));

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar;
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		window_flags |= ImGuiWindowFlags_NoTitleBar;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		ImGui::Begin("Emu64X", &p_open, window_flags);
		ImGui::PopStyleVar();

		if (ImGui::BeginMenuBar())
		{

			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open", "CTRL+M")) {
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Tools"))
			{
				if (ImGui::MenuItem("CPU Status", "CTRL+R")) mCPUStatusPanel->open();
				if (ImGui::MenuItem("Debugger", "CTRL+D")) mDisassemblerPanel->open();
				if (ImGui::MenuItem("Memory", "CTRL+M")) mMemoryEditorPanel->open();
				if (ImGui::MenuItem("Console", "CTRL+O")) mConsolePanel->open();
				if (ImGui::MenuItem("Save PPM", "CTRL+O")) mSoftwareRenderer->SaveToFile("vram.ppm");

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Emulation"))
			{
				if (ImGui::MenuItem("Play")) mDisassemblerPanel->onPlay();
				if (ImGui::MenuItem("Pause")) mDisassemblerPanel->onPause();
				if (ImGui::MenuItem("Hard Reset")) hardReset();

				ImGui::EndMenu();
			}

			ImGui::Text("Min FPS: %d", fpsCounter.MinFPS);
			ImGui::Text("Avg FPS: %d", fpsCounter.AvgFPS());
			ImGui::Text("Max FPS: %d", fpsCounter.MaxFPS);
			ImGui::Text("Current FPS: %d", fpsCounter.CurrentFPS);

			ImGui::TextUnformatted(mCurrentGame.c_str());

			ImGui::EndMenuBar();
		}


		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

		ImGui::End();

		mCPUStatusPanel->render(pManager);
		mMemoryEditorPanel->render(pManager);
		mDisassemblerPanel->render(pManager);
		mConsolePanel->render(pManager);
		mViewportPanel->render(pManager);
		mFileDialogPanel->render(pManager);
	}

	void hardReset() {
		fpsCounter.Init();
		mVR4300->reset();
		mRDRAM->reset();
		mPIExternalBus->reset();
		mSIExternalBus->reset();
		mConsolePanel->getInternalConsole().System().Items().clear();
		if (mDisassemblerPanel->getDebugState() == DebugState::Breakpoint) {
			mDisassemblerPanel->onPlay();
		}
	}

private:
	SharedPtr<Bus> root;

	SharedPtr<VR4300> mVR4300;
	SharedPtr<RDRAM> mRDRAM;
	SharedPtr<RCP> mRCP;
	SharedPtr<PIExternalBus> mPIExternalBus;
	SharedPtr<SIExternalBus> mSIExternalBus;
	FPSCounter fpsCounter;


	SharedPtr<CPUStatusPanel> mCPUStatusPanel;
	SharedPtr<DisassemblerPanel> mDisassemblerPanel;
	SharedPtr<MemoryEditorPanel> mMemoryEditorPanel;
	SharedPtr<ConsolePanel> mConsolePanel;
	SharedPtr<Emu64XLogger> mLogger;
	SharedPtr<SoftwareRenderer> mSoftwareRenderer;
	SharedPtr<BatchRenderer> mBatchRenderer;
	SharedPtr<ViewportPanel> mViewportPanel;
	SharedPtr<FileDialogPanel> mFileDialogPanel;
	glm::mat4 mProjectionMatrix;

	ma_device mAudioDevice;
	const U32 PRERENDERED_SIZE = 10;
	U32 mNumPrerendered = 0;
	String mCurrentGame = "";
};

int
#if !defined(_MAC)
	#if defined(_M_CEE_PURE)
		__clrcall
	#else
		WINAPI
	#endif
#else
	CALLBACK
#endif
WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd
) {
	LoggingSpecifications specs(ESX_TEXT(""));
	LoggingSystem::Start(specs);

	ApplicationManager appManager;
	appManager.run(MakeShared<Emu64X>());

	LoggingSystem::Shutdown();
	return 0;
};