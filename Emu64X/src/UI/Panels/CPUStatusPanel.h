#pragma once

#include <Core/MIPS/VR4300/VR4300.h>

#include "Panel.h"

namespace esx {

	class CPUStatusPanel : public Panel {
	public:
		CPUStatusPanel();
		~CPUStatusPanel();

		void setInstance(const SharedPtr<VR4300>& pInstance) { mInstance = pInstance; }

	protected:
		virtual void onImGuiRender() override;

	private:
		SharedPtr<VR4300> mInstance;
	};

}