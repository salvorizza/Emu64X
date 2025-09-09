#include "VideoInterface.h"

#include "../RCP.h"

namespace esx {

	VideoInterface::VideoInterface(RCP* rcp)
		: mRCP(rcp)
	{
	}

	VideoInterface::~VideoInterface()
	{
	}

	void VideoInterface::init()
	{
	}

	void VideoInterface::clock(U64 clocks)
	{
	}

	void VideoInterface::store(U32 address, U32 value)
	{
		switch (address) {
			case 0x04400000: {
				VI_CTRL.write(value);
				break;
			}
			case 0x0440000C: {
				VI_V_INTR.write(value);
				break;
			}
			case 0x04400010: {
				mRCP->clearInterrupt(InterruptType::VI);
				VI_V_CURRENT.write(value);
				break;
			}
			case 0x04400024: {
				VI_H_VIDEO.write(value);
				break;
			}
			default: {
				ESX_CORE_LOG_WARNING("{} - Store to address 0x{:08x} with value 0x{:08x} not implemented yet", mName, address, value);
				break;
			}
		}
	}

	U32 VideoInterface::load(U32 address)
	{
		switch (address) {
			case 0x04400000: {
				return VI_CTRL.read();
			}
			case 0x0440000C: {
				return VI_V_INTR.read();
			}
			case 0x04400010: {
				return VI_V_CURRENT.read();
			}
			case 0x04400024: {
				return VI_H_VIDEO.read();
			}
			default: {
				ESX_CORE_LOG_WARNING("{} - Load from address 0x{:08x} not implemented yet", mName, address);
				break;
			}
		}
	}

	void VideoInterface::reset()
	{
	}
}