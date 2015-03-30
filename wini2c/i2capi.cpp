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

extern "C" {
	void* new_i2cdevice(char *deviceId, int address) {
		ComPtr<IInspectable> newDevice = nullptr;
		I2cConnectionSettings^ settings = ref new I2cConnectionSettings(address);
		std::string strDeviceId = deviceId;
		auto wstrDeviceId = std::wstring(strDeviceId.begin(), strDeviceId.end());

		create_task(I2cDevice::FromIdAsync(ref new String(wstrDeviceId.c_str()), settings)).then(
				[&newDevice](I2cDevice^ device) {
			newDevice = reinterpret_cast<IInspectable*>((Object^)device);
		}).wait();

		return newDevice.Detach();
    }

	void delete_i2cdevice(void* device) {
		if (device != NULL) {
			ComPtr<IInspectable> realDevice;
			
			realDevice.Attach((IInspectable*)device);
		}
	}

	void write_i2cdevice(void* device, byte data) {
		if (device != NULL) {
			ComPtr<IInspectable> realDevice = (IInspectable*)device;
			I2cDevice^ i2cDevice = (I2cDevice^)reinterpret_cast<Object^>(realDevice.Get());
		}
	}
}