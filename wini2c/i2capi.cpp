#include <ppltasks.h>
#include <collection.h>
#include "python.h"
#include "constants.h"
#include "i2capi.h"

using namespace concurrency;
using namespace Windows::Devices::I2c;
using namespace Platform;
using namespace Platform::Collections;
using namespace Microsoft::WRL;

#define I2CDEVICE_TOPOINTER(d) (reinterpret_cast<IInspectable*>((Object^)d))
#define I2CDEVICE_FROMPOINTER(p) ((I2cDevice^)reinterpret_cast<Object^>(reinterpret_cast<IInspectable*>(p)))

extern "C" {
	void *new_i2cdevice(wchar_t *name, int slaveAddress, int busSpeed, int sharingMode) {
		ComPtr<IInspectable> spInspectable = nullptr;
		I2cConnectionSettings^ settings = ref new I2cConnectionSettings(slaveAddress);
		String^ deviceName = ref new String(name);
        String^ querySyntax = I2cDevice::GetDeviceSelector(deviceName);
        auto info = create_task(Windows::Devices::Enumeration::DeviceInformation::FindAllAsync(querySyntax)).get();
        String^ id = info->GetAt(0)->Id;

		switch (busSpeed)
		{
		case FASTSPEED:
			settings->BusSpeed = I2cBusSpeed::FastMode;
			break;
		case STANDARDSPEED:
			settings->BusSpeed = I2cBusSpeed::StandardMode;
			break;
		default:
			return NULL;
		}

		switch (sharingMode)
		{
		case SHAREDMODE:
			settings->SharingMode = I2cSharingMode::Shared;
			break;
		case EXCLUSIVEMODE:
			settings->SharingMode = I2cSharingMode::Exclusive;
			break;
		default:
			return NULL;
		}

		auto getdevicetask = create_task(I2cDevice::FromIdAsync(id, settings));
		
        auto i2cdevice = getdevicetask.get();

        spInspectable = I2CDEVICE_TOPOINTER(i2cdevice);

		return spInspectable.Detach();
    }

	void delete_i2cdevice(void *device) {
		if (device != NULL) {
			ComPtr<IInspectable> realDevice;
			realDevice.Attach((IInspectable*)device);
		}
	}

	void write_i2cdevice(void *device, char* data, unsigned int count) {
		if (device != NULL) {
			I2cDevice^ i2cDevice = I2CDEVICE_FROMPOINTER(device);
			unsigned char* udata = reinterpret_cast<unsigned char*>(data);
			i2cDevice->Write(ArrayReference<unsigned char>(udata, count));
		}
	}

	void read_i2cdevice(void *device, char* buffer, unsigned int length) {
		if (device != NULL) {
			I2cDevice^ i2cDevice = I2CDEVICE_FROMPOINTER(device);
			unsigned char* ubuffer = reinterpret_cast<unsigned char*>(buffer);

			i2cDevice->Read(ArrayReference<unsigned char>(ubuffer, length));
		}
	}

	void writeread_i2cdevice(void *device, char* data, unsigned int count, char* buffer, unsigned int length) {
		if (device != NULL) {
			I2cDevice^ i2cDevice = I2CDEVICE_FROMPOINTER(device);
			unsigned char* udata = reinterpret_cast<unsigned char*>(data);
			unsigned char* ubuffer = reinterpret_cast<unsigned char*>(buffer);

			i2cDevice->WriteRead(ArrayReference<unsigned char>(udata, count), ArrayReference<unsigned char>(ubuffer, length));
		}
	}

	int getbusspeed_i2cdevice(void *device) {
		if (device != NULL) {
			I2cDevice^ i2cDevice = I2CDEVICE_FROMPOINTER(device);

			switch (i2cDevice->ConnectionSettings->BusSpeed)
			{
			case I2cBusSpeed::FastMode:
				return FASTSPEED;
			case I2cBusSpeed::StandardMode:
				return STANDARDSPEED;
			default:
				return -1;
			}
		}

		return -1;
	}

	int getsharingmode_i2cdevice(void *device) {
		if (device != NULL) {
			I2cDevice^ i2cDevice = I2CDEVICE_FROMPOINTER(device);

			switch (i2cDevice->ConnectionSettings->SharingMode)
			{
			case I2cSharingMode::Exclusive:
				return EXCLUSIVEMODE;
			case I2cSharingMode::Shared:
				return SHAREDMODE;
			default:
				return -1;
			}
		}

		return -1;
	}

	int getaddress_i2cdevice(void *device) {
		if (device != NULL) {
			I2cDevice^ i2cDevice = I2CDEVICE_FROMPOINTER(device);

			return (int)i2cDevice->ConnectionSettings->SlaveAddress;
		}

		return -1;
	}
}