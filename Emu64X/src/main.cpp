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
#include "Core/Scheduler.h"


#ifdef ESX_PLATFORM_WINDOWS
	#include <Windows.h>
	#undef ERROR
#endif // ESX_PLATFORM_WINDOWS

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include <iostream>
#include <sstream>

#include "optick.h"
#include "miniaudio.h"



using namespace esx;
#undef max
#undef min

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

	ma_device mAudioDevice;
	const U32 PRERENDERED_SIZE = 10;
	U32 mNumPrerendered = 0;
	String mCurrentGame = "";
};

struct HeadlessConfig {
	String romPath;
	String logPath = "emu64x_headless.log";
	U32 frames = 60;
	BIT valid = ESX_FALSE;
};

static String readNextArg(std::istringstream& iss) {
	String result;
	iss >> std::ws;
	if (iss.peek() == '"') {
		iss.get();
		std::getline(iss, result, '"');
	} else {
		iss >> result;
	}
	return result;
}

HeadlessConfig parseHeadlessArgs(LPSTR lpCmdLine) {
	HeadlessConfig cfg;
	std::istringstream iss(lpCmdLine);
	String token;

	while (iss >> token) {
		if (token == "--headless") {
			cfg.valid = ESX_TRUE;
		} else if (token == "--rom") {
			cfg.romPath = readNextArg(iss);
		} else if (token == "--log") {
			cfg.logPath = readNextArg(iss);
		} else if (token == "--frames") {
			String val = readNextArg(iss);
			if (!val.empty()) cfg.frames = std::stoul(val);
		}
	}

	if (cfg.romPath.empty()) cfg.valid = ESX_FALSE;
	return cfg;
}

int runHeadless(const HeadlessConfig& cfg) {
	LoggingSpecifications specs(cfg.logPath);
	LoggingSystem::Start(specs);

	ESX_CORE_LOG_INFO("=== Emu64X Headless Mode ===");
	ESX_CORE_LOG_INFO("ROM: {}", cfg.romPath);
	ESX_CORE_LOG_INFO("Log: {}", cfg.logPath);
	ESX_CORE_LOG_INFO("Frames: {}", cfg.frames);

	auto root = MakeShared<Bus>(ESX_TEXT("Root"));
	auto vr4300 = MakeShared<VR4300>();
	auto rdram = MakeShared<RDRAM>();
	auto rcp = MakeShared<RCP>();
	auto piExternalBus = MakeShared<PIExternalBus>();
	auto siExternalBus = MakeShared<SIExternalBus>("commons/bios/boot.rom");

	root->connectDevice(vr4300);
	vr4300->connectToBus(root);

	root->connectDevice(rdram);
	rdram->connectToBus(root);

	root->connectDevice(rcp);
	rcp->connectToBus(root);

	root->connectDevice(piExternalBus);
	piExternalBus->connectToBus(root);

	root->connectDevice(siExternalBus);
	siExternalBus->connectToBus(root);

	root->sortRanges();

	ESX_CORE_LOG_INFO("Loading ROM...");
	if (!std::filesystem::exists(cfg.romPath)) {
		ESX_CORE_LOG_FATAL("ROM file not found: {}", cfg.romPath);
		LoggingSystem::Shutdown();
		return 1;
	}
	piExternalBus->loadGame(cfg.romPath);
	String gameCode = piExternalBus->getGameCode();
	ESX_CORE_LOG_INFO("Game code: {}", gameCode);

	vr4300->reset();
	rdram->reset();
	piExternalBus->reset();
	siExternalBus->reset();

	ESX_CORE_LOG_INFO("Starting emulation for {} frames...", cfg.frames);

	U32 framesCompleted = 0;
	BIT running = ESX_TRUE;

	while (running && framesCompleted < cfg.frames) {
		BIT newFrameAvailable = ESX_FALSE;
		while (newFrameAvailable == ESX_FALSE) {
			while (Scheduler::HasEvents() == ESX_FALSE || vr4300->getClocks() < Scheduler::NextEvent().ClockTarget) {
				vr4300->clock();

				if (vr4300->getHalt()) {
					ESX_CORE_LOG_ERROR("CPU halted at frame {}, clocks {}", framesCompleted, vr4300->getClocks());
					running = ESX_FALSE;
					break;
				}
			}

			if (!running) break;

			if (Scheduler::HasEvents() && vr4300->getClocks() >= Scheduler::NextEvent().ClockTarget) {
				if (Scheduler::NextEvent().Type == SchedulerEventType::GPUFrameStart) {
					newFrameAvailable = ESX_TRUE;
				}

				Scheduler::ExecuteEvent();
				Scheduler::Progress();
			}
		}

		framesCompleted++;
		ESX_CORE_LOG_INFO("Frame {}/{} PC={:08x}h clocks={}", framesCompleted, cfg.frames, (U32)vr4300->mPC, vr4300->getClocks());
	}

	ESX_CORE_LOG_INFO("=== Headless run finished: {}/{} frames, {} total clocks ===", framesCompleted, cfg.frames, vr4300->getClocks());

	LoggingSystem::Shutdown();
	return 0;
}

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
	HeadlessConfig headlessCfg = parseHeadlessArgs(lpCmdLine);
	if (headlessCfg.valid) {
		return runHeadless(headlessCfg);
	}

	LoggingSpecifications specs(ESX_TEXT(""));
	LoggingSystem::Start(specs);

	ApplicationManager appManager;
	appManager.run(MakeShared<Emu64X>());

	LoggingSystem::Shutdown();
	return 0;
};