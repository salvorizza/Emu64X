#pragma once

#include <Core/MIPS/VR4300/VR4300.h>
#include <Core/MIPS/R4000/R4000.h>

#include "Panel.h"

namespace esx {

	class CPUStatusPanel : public Panel {
	public:
		CPUStatusPanel();
		~CPUStatusPanel();

		void setInstance(const SharedPtr<VR4300>& pInstance) { mInstance = pInstance; }
		void setInstance(const SharedPtr<R4000>& pInstance) { mInstanceR4000 = pInstance; }

	protected:
		virtual void onImGuiRender() override;

	private:
		SharedPtr<VR4300> mInstance;
		SharedPtr<R4000> mInstanceR4000;
	};

}