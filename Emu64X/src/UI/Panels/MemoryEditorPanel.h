#pragma once

#include <Base/Bus.h>
#include <Core/RDRAM.h>
#include <Core/SIExternalBus.h>

#include "Panel.h"

namespace esx {

	class MemoryEditorPanel : public Panel {
	public:
		MemoryEditorPanel();
		~MemoryEditorPanel();

		void setInstance(const SharedPtr<Bus>& pInstance) { mInstance = pInstance; }

	protected:
		virtual void onImGuiRender() override;

	private:
		SharedPtr<Bus> mInstance;
	};

}