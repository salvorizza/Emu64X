#pragma once

#include "Base/Base.h"
#include "Base/Assert.h"

#include <optional>

namespace esx {

	constexpr Array<U32, 8> SEGS_MASKS = {
		//KUSEG:2048MB
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
		//KSEG0:512MB
		0x1FFFFFFF,
		//KSEG1:512MB
		0x1FFFFFFF,
		//KSSEG:1024MB
		0xFFFFFFFF,
		//KSEG3:1024MB
		0xFFFFFFFF
	};

	struct BusRange {
		U64 Start;
		U64 End;
		U64 Mask;

		BusRange()
			: Start(0),
			End(0),
			Mask(0)
		{}

		BusRange(U64 start, U64 sizeInBytes, U64 mask)
			:	Start(start),
				End(start + sizeInBytes),
				Mask(mask)
		{}
	};

	class Bus;

	class BusDevice {
	public:
		BusDevice(const StringView& name) : mName(name) {}
		virtual ~BusDevice() = default;

		virtual void init() {}

		virtual void clock(U64 clocks) {}

		virtual void writeLine(const StringView& busName, const StringView& lineName, BIT value) {}

		virtual void store(const StringView& busName, U32 address, U64 value) { ESX_CORE_LOG_ERROR("Device {} does not implement store64 at address {:08x}h", mName, address); }
		virtual void load(const StringView& busName, U32 address, U64& output) { ESX_CORE_LOG_ERROR("Device {} does not implement load64 at address {:08x}h", mName, address); }

		virtual void store(const StringView& busName, U32 address, U32 value) { ESX_CORE_LOG_ERROR("Device {} does not implement store32 at address {:08x}h", mName, address); }
		virtual void load(const StringView& busName, U32 address, U32& output) { ESX_CORE_LOG_ERROR("Device {} does not implement load32 at address {:08x}h", mName, address); }

		virtual void store(const StringView& busName, U32 address, U16 value) { ESX_CORE_LOG_ERROR("Device {} does not implement store16 at address {:08x}h", mName, address); }
		virtual void load(const StringView& busName, U32 address, U16& output) { ESX_CORE_LOG_ERROR("Device {} does not implement load16 at address {:08x}h", mName, address); }

		virtual void store(const StringView& busName, U32 address, U8 value) { ESX_CORE_LOG_ERROR("Device {} does not implement store8 at address {:08x}h", mName, address); }
		virtual void load(const StringView& busName, U32 address, U8& output) { ESX_CORE_LOG_ERROR("Device {} does not implement load8 at address {:08x}h", mName, address); }

		virtual void reset() { return; }

		virtual U8* getFastPointer(U32 address) { return nullptr; }

		const StringView& getName() const { return mName; }

		void connectToBus(const SharedPtr<Bus>& pBus);
		SharedPtr<Bus>& getBus(const StringView& busName);

	protected:
		void addRange(const StringView& busName, U64 start, U64 sizeInBytes, U64 mask);

	protected:
		UnorderedMap<StringView, SharedPtr<Bus>> mBusses;
		Vector<BusRange> mStoredRanges;

		StringView mName;
	};


	struct DeviceRangeData {
		BusRange Range = BusRange();
		SharedPtr<BusDevice> Device = {};

		DeviceRangeData(const BusRange& range, const SharedPtr<BusDevice>& device)
			: Range(range), Device(device)
		{}
	};


	using Interval = Pair<BusRange, SharedPtr<BusDevice>>;

	struct IntervalTreeNode {
		Interval interval;
		U64 maxEnd;
		IntervalTreeNode* left;
		IntervalTreeNode* right;

		IntervalTreeNode(const Interval& i) : interval(i), maxEnd(i.first.End), left(nullptr), right(nullptr) {}
	};

	constexpr size_t PageSize = KIBI(4);
	constexpr size_t PageTableSize = 0x10000;

	constexpr size_t NumPages = MIBI(4) / PageSize;
	constexpr size_t NumPagesPIF = (0x7C0 / PageSize) + 1;

	using Page = Span<U8>;
	using PageTable = Array<Page, PageTableSize>;

	class Bus {
	public:
		Bus(const StringView& name);
		~Bus() = default;

		void writeLine(const StringView& lineName, BIT value);

		void sortRanges();

		template<typename T>
		void store(U32 physicalAddress, T value) {
			storeIO<T>(physicalAddress, value);
		}

		template<typename T>
		T load(U32 physicalAddress) {
			return loadIO<T>(physicalAddress);
		}

		template<typename T>
		void storeIO(U32 physicalAddress, T value) {
			IntervalTreeNode* node = findRangeInIntervalTree(mIntervalTree, physicalAddress);

			auto& [busRange, device] = node->interval;
			if (node) {
				if (device) {
					device->store(mName, physicalAddress & busRange.Mask, value);
				}
				else {
					ESX_CORE_LOG_ERROR("Writing Address 0x{:08x}: not found at {} bytes", physicalAddress, sizeof(T));
				}
			} else {
				ESX_CORE_LOG_ERROR("Writing Address 0x{:08x}: not found at {} bytes", physicalAddress, sizeof(T));

			}
		}

		template<typename T>
		T loadIO(U32 physicalAddress) {
			T result = 0;

			IntervalTreeNode* node = findRangeInIntervalTree(mIntervalTree, physicalAddress);
			if (node) {
				auto& [busRange, device] = node->interval;
				if (device) {
					device->load(mName, physicalAddress & busRange.Mask, result);
				}
				else {
					ESX_CORE_LOG_ERROR("Reading Address 0x{:08x}: not found at {} bytes", physicalAddress, sizeof(T));
				}
			} else {
				ESX_CORE_LOG_ERROR("Reading Address 0x{:08x}: not found at {} bytes", physicalAddress, sizeof(T));
			}

			return result;
		}

		void connectDevice(const SharedPtr<BusDevice>& device);

		template<typename T>
		SharedPtr<T> getDevice(const StringView& name) {
			return std::dynamic_pointer_cast<T>(mDevices.at(name));
		}

		SharedPtr<BusDevice>& getDevice(const StringView& name) {
			return mDevices.at(name);
		}

		const StringView& getName() const { return mName; }

		void addRange(const StringView& deviceName, BusRange range);

		//TODO: Give a shit
		static U32 toPhysicalAddress(U32 address) { return address & 0x1FFFFFFF; }

		IntervalTreeNode* buildIntervalTree(const Vector<Interval>& intervals);
		IntervalTreeNode* findRangeInIntervalTree(IntervalTreeNode* root, uint32_t address);
	private:
		StringView mName;
		UnorderedMap<StringView, SharedPtr<BusDevice>> mDevices;
		Vector<Interval> mRanges;
		IntervalTreeNode* mIntervalTree = nullptr;

		PageTable mPageTableR, mPageTableW;
		Array<U8, 1> mNull;
	};


}