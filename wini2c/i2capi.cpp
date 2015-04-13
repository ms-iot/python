#include <collection.h>
#include "python.h"
#include "i2capi.h"

using namespace Windows::Foundation;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Devices::I2c;
using namespace Platform;
using namespace Platform::Collections;
using namespace Microsoft::WRL;

#define I2CDEVICE_TOPOINTER(d) (reinterpret_cast<IInspectable*>((Object^)d))
#define I2CDEVICE_FROMPOINTER(p) ((I2cDevice^)reinterpret_cast<Object^>(reinterpret_cast<IInspectable*>(p)))
#define BEGIN_PYERR_EXCEPTION_WATCH try { 
#define END_PYERR_EXCEPTION_WATCH } catch (Exception^ exceptionDetails) { \
std::wstring wstrMessage(exceptionDetails->Message->Data()); \
std::string strMessage(wstrMessage.begin(), wstrMessage.end()); \
PyErr_SetString(PyExc_RuntimeError, strMessage.c_str()); \
}
#define VALIDATE_POINTER(p, errmsg) if (!(p)) { \
PyErr_SetString(PyExc_TypeError, errmsg); \
return FAILURE; \
}

extern "C" {
	void *new_i2cdevice(wchar_t *name, int slaveAddress, int busSpeed, int sharingMode) {
		ComPtr<IInspectable> spInspectable = nullptr;
        try {
            I2cConnectionSettings^ settings = ref new I2cConnectionSettings(slaveAddress);
            String^ deviceName = ref new String(name);
            String^ querySyntax = I2cDevice::GetDeviceSelector(deviceName);
			auto asyncop = DeviceInformation::FindAllAsync(querySyntax);
			while (asyncop->Status != AsyncStatus::Completed) {
				if (asyncop->Status == AsyncStatus::Error) {
					PyErr_Format(PyExc_RuntimeError, "Could not find information for device '%S': %d", name, asyncop->ErrorCode);
					return NULL;
				}
				Sleep(50);
			}
			auto info = asyncop->GetResults();
			if (info == nullptr || info->Size == 0) {
				PyErr_Format(PyExc_RuntimeError, "Could not find information for device '%S'", name);
				return NULL;
			}
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
                PyErr_Format(PyExc_TypeError, "Invalid I2C bus speed specified '%d'", busSpeed);
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
                PyErr_Format(PyExc_TypeError, "Invalid I2C sharing mode specified '%d'", sharingMode);
                return NULL;
            }

			auto i2cop = I2cDevice::FromIdAsync(id, settings);
			while (i2cop->Status != AsyncStatus::Completed) {
				if (i2cop->Status == AsyncStatus::Error) {
					PyErr_Format(PyExc_RuntimeError, "Could get I2C device: %d", i2cop->ErrorCode);
					return NULL;
				}
				Sleep(50);
			}

			auto i2cdevice = i2cop->GetResults();
            if (i2cdevice == nullptr) {
                PyErr_SetString(PyExc_RuntimeError, "Could not find I2C device specified");
                return NULL;
            }

            spInspectable = I2CDEVICE_TOPOINTER(i2cdevice);
        } catch (Exception^ e) {
            PyErr_Format(PyExc_RuntimeError, "An unexpected exception occurred during I2C device creation: %S", e->Message->Data());
            return NULL;
        }

		return spInspectable.Detach();
    }

	void delete_i2cdevice(void *device) {
		if (device != NULL) {
			ComPtr<IInspectable> realDevice;
			realDevice.Attach((IInspectable*)device);
		}
	}

	int write_i2cdevice(void *device, char* data, unsigned int count) {
        int ret = FAILURE;

        BEGIN_PYERR_EXCEPTION_WATCH

		I2cDevice^ i2cDevice = I2CDEVICE_FROMPOINTER(device);
		unsigned char* udata = reinterpret_cast<unsigned char*>(data);
		i2cDevice->Write(ArrayReference<unsigned char>(udata, count));
        ret = SUCCESS;

        END_PYERR_EXCEPTION_WATCH
            
        return ret;
	}

	int read_i2cdevice(void *device, char* buffer, unsigned int length) {
        int ret = FAILURE;

        BEGIN_PYERR_EXCEPTION_WATCH
            
        I2cDevice^ i2cDevice = I2CDEVICE_FROMPOINTER(device);
		unsigned char* ubuffer = reinterpret_cast<unsigned char*>(buffer);

		i2cDevice->Read(ArrayReference<unsigned char>(ubuffer, length));
        ret = SUCCESS;

        END_PYERR_EXCEPTION_WATCH

        return ret;
    }

	int writeread_i2cdevice(void *device, char* data, unsigned int count, char* buffer, unsigned int length) {
        int ret = FAILURE;

        BEGIN_PYERR_EXCEPTION_WATCH

        I2cDevice^ i2cDevice = I2CDEVICE_FROMPOINTER(device);
		unsigned char* udata = reinterpret_cast<unsigned char*>(data);
		unsigned char* ubuffer = reinterpret_cast<unsigned char*>(buffer);

		i2cDevice->WriteRead(ArrayReference<unsigned char>(udata, count), ArrayReference<unsigned char>(ubuffer, length));
        ret = SUCCESS;

        END_PYERR_EXCEPTION_WATCH

        return ret;
    }

    int get_deviceid_i2cdevice(void* device, char* deviceIdBuffer, unsigned int length) {
        int ret = FAILURE;

        VALIDATE_POINTER(deviceIdBuffer, "Data buffer is not valid");

        // Initialize
        deviceIdBuffer[0] = '\0';

        BEGIN_PYERR_EXCEPTION_WATCH

        I2cDevice^ deviceObj = I2CDEVICE_FROMPOINTER(device);

        auto deviceId = deviceObj->DeviceId;

        if (deviceId->Length() < length) {
            try {
                std::wstring wstrDeviceId(deviceId->Data());
                std::string strDeviceId(wstrDeviceId.begin(), wstrDeviceId.end());

                strncpy_s(deviceIdBuffer, length, strDeviceId.c_str(), _TRUNCATE);
            }
            catch (std::bad_alloc&) {
                PyErr_SetString(PyExc_RuntimeError, "Could not allocate string data for device id");
            }

            ret = SUCCESS;
        }
        else {
            PyErr_Format(PyExc_RuntimeError, "Buffer length '%d' was not large enough for id length '%d'", length, deviceId->Length());
        }

        END_PYERR_EXCEPTION_WATCH

        return ret;
    }
    
    int get_busspeed_i2cdevice(void *device, int* busspeed) {
        int ret = FAILURE;

        VALIDATE_POINTER(busspeed, "Pointer was not valid");

        BEGIN_PYERR_EXCEPTION_WATCH

		I2cDevice^ i2cDevice = I2CDEVICE_FROMPOINTER(device);

		switch (i2cDevice->ConnectionSettings->BusSpeed)
		{
		case I2cBusSpeed::FastMode:
			*busspeed = FASTSPEED;
            ret = SUCCESS;
            break;
		case I2cBusSpeed::StandardMode:
			*busspeed = STANDARDSPEED;			
            ret = SUCCESS;
            break;
        }

        END_PYERR_EXCEPTION_WATCH

		return ret;
	}

	int get_sharingmode_i2cdevice(void *device, int* sharingmode) {
        int ret = FAILURE;

        VALIDATE_POINTER(sharingmode, "Pointer was not valid");

        BEGIN_PYERR_EXCEPTION_WATCH

        I2cDevice^ i2cDevice = I2CDEVICE_FROMPOINTER(device);

		switch (i2cDevice->ConnectionSettings->SharingMode)
		{
		case I2cSharingMode::Exclusive:
			*sharingmode = EXCLUSIVEMODE;
            ret = SUCCESS;
            break;
		case I2cSharingMode::Shared:
			*sharingmode = SHAREDMODE;
            ret = SUCCESS;
            break;
        }

        END_PYERR_EXCEPTION_WATCH

        return ret;
    }

	int get_address_i2cdevice(void *device, int* slaveaddress) {
        int ret = FAILURE;

        VALIDATE_POINTER(slaveaddress, "Pointer was not valid");

        BEGIN_PYERR_EXCEPTION_WATCH

        I2cDevice^ i2cDevice = I2CDEVICE_FROMPOINTER(device);

		*slaveaddress = (int)i2cDevice->ConnectionSettings->SlaveAddress;

        ret = SUCCESS;

        END_PYERR_EXCEPTION_WATCH

        return ret;
    }
}