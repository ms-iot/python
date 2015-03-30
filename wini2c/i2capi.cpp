#include <ppltasks.h>
#include <collection.h>
#include "python.h"
#include "constants.h"
#include "i2capi.h"

using namespace concurrency;
using namespace Windows::Devices::I2c;
using namespace Platform;
using namespace Platform::Collections;

extern "C" {
	void* new_i2cdevice(char *deviceId, int address) {
		I2cDevice^* newDevice = NULL;
		I2cConnectionSettings^ settings = ref new I2cConnectionSettings(address);
		std::string strDeviceId = deviceId;
		auto wstrDeviceId = std::wstring(strDeviceId.begin(), strDeviceId.end());

		create_task(I2cDevice::FromIdAsync(ref new String(wstrDeviceId.c_str()), settings)).then([&newDevice](I2cDevice^ device) {
			newDevice = new I2cDevice^();
			*newDevice = device;
		}).wait();

		return newDevice;
    }

	void delete_i2cdevice(void* device) {
		if (device != NULL) {
			I2cDevice^* realDevice = (I2cDevice^*)device;

			*realDevice = nullptr;

			delete realDevice;
		}
	}
}