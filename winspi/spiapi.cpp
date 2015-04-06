#include <ppltasks.h>
#include <collection.h>
#include "python.h"
#include "constants.h"
#include "spiapi.h"

using namespace concurrency;
using namespace Windows::Devices::Spi;
using namespace Platform;
using namespace Platform::Collections;
using namespace Microsoft::WRL;

#define SPIDEVICE_TOPOINTER(d) (reinterpret_cast<IInspectable*>((Object^)d))
#define SPIDEVICE_FROMPOINTER(p) ((SpiDevice^)reinterpret_cast<Object^>(reinterpret_cast<IInspectable*>(p)))

extern "C" {
	void *new_spidevice(wchar_t *name, int chipSelectLine, int clockFrequency, int dataBitLength, int mode, int sharingMode) {
		ComPtr<IInspectable> spInspectable = nullptr;
		SpiConnectionSettings^ settings = ref new SpiConnectionSettings(chipSelectLine);
		String^ deviceName = ref new String(name);
        String^ querySyntax = SpiDevice::GetDeviceSelector(deviceName);
        auto info = create_task(Windows::Devices::Enumeration::DeviceInformation::FindAllAsync(querySyntax)).get();
        String^ id = info->GetAt(0)->Id;

		if (clockFrequency != -1) {
			settings->ClockFrequency = clockFrequency;
		}

		if (dataBitLength != -1) {
			settings->DataBitLength = dataBitLength;
		}

		switch (mode)
		{
		case MODE0:
			settings->Mode = SpiMode::Mode0;
			break;
		case MODE1:
			settings->Mode = SpiMode::Mode1;
			break;
		case MODE2:
			settings->Mode = SpiMode::Mode2;
			break;
		case MODE3:
			settings->Mode = SpiMode::Mode3;
			break;
		default:
			return NULL;
		}

		switch (sharingMode)
		{
		case SHAREDMODE:
			settings->SharingMode = SpiSharingMode::Shared;
			break;
		case EXCLUSIVEMODE:
			settings->SharingMode = SpiSharingMode::Exclusive;
			break;
		default:
			return NULL;
		}

		auto getdevicetask = create_task(SpiDevice::FromIdAsync(id, settings));
		
        auto spidevice = getdevicetask.get();

        spInspectable = SPIDEVICE_TOPOINTER(spidevice);

		return spInspectable.Detach();
    }

	void delete_spidevice(void *device) {
		if (device != NULL) {
			ComPtr<IInspectable> realDevice;
			realDevice.Attach((IInspectable*)device);
		}
	}

	void write_spidevice(void *device, char* data, unsigned int count) {
		if (device != NULL) {
			SpiDevice^ deviceObj = SPIDEVICE_FROMPOINTER(device);
			unsigned char* udata = reinterpret_cast<unsigned char*>(data);
			deviceObj->Write(ArrayReference<unsigned char>(udata, count));
		}
	}

	void read_spidevice(void *device, char* buffer, unsigned int length) {
		if (device != NULL) {
			SpiDevice^ deviceObj = SPIDEVICE_FROMPOINTER(device);
			unsigned char* ubuffer = reinterpret_cast<unsigned char*>(buffer);

			deviceObj->Read(ArrayReference<unsigned char>(ubuffer, length));
		}
	}

	void transfersequential_spidevice(void *device, char* data, unsigned int count, char* buffer, unsigned int length) {
		if (device != NULL) {
			SpiDevice^ deviceObj = SPIDEVICE_FROMPOINTER(device);
			unsigned char* udata = reinterpret_cast<unsigned char*>(data);
			unsigned char* ubuffer = reinterpret_cast<unsigned char*>(buffer);

			deviceObj->TransferSequential(ArrayReference<unsigned char>(udata, count), ArrayReference<unsigned char>(ubuffer, length));
		}
	}

	void transferfullduplex_spidevice(void *device, char* data, unsigned int count, char* buffer, unsigned int length) {
		if (device != NULL) {
			SpiDevice^ deviceObj = SPIDEVICE_FROMPOINTER(device);
			unsigned char* udata = reinterpret_cast<unsigned char*>(data);
			unsigned char* ubuffer = reinterpret_cast<unsigned char*>(buffer);

			deviceObj->TransferFullDuplex(ArrayReference<unsigned char>(udata, count), ArrayReference<unsigned char>(ubuffer, length));
		}
	}
}