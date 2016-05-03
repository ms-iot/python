#include <collection.h>
#include "python.h"
#include "i2capi.h"

using namespace Windows::Foundation;
using namespace Windows::Devices;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Devices::I2c;
using namespace Platform;
using namespace Platform::Collections;
using namespace Microsoft::IoT::Lightning::Providers;
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

static I2cDevice^ GetDeviceInboxDriver(int index, I2cConnectionSettings^ settings) {
    if (index < 0) {
        PyErr_Format(PyExc_RuntimeError, "Could not find i2c  id: '%d'", index);
        return nullptr;
    }

    String^ deviceName = ref new String(L"I2C");
    deviceName += (index + 1).ToString();
    String^ querySyntax = I2cDevice::GetDeviceSelector(deviceName);
    auto asyncop = DeviceInformation::FindAllAsync(querySyntax);
    while (asyncop->Status != AsyncStatus::Completed) {
        if (asyncop->Status == AsyncStatus::Error) {
            PyErr_Format(PyExc_RuntimeError, "Could not find information for device '%S': %d", deviceName->Data(), asyncop->ErrorCode);
            return nullptr;
        }
        Sleep(50);
    }

    auto info = asyncop->GetResults();
    if (info == nullptr || info->Size == 0) {
        PyErr_Format(PyExc_RuntimeError, "Could not find information for device '%S'", deviceName->Data());
        return nullptr;
    }

    String^ id = info->GetAt(0)->Id;
    auto i2cop = I2cDevice::FromIdAsync(id, settings);
    while (i2cop->Status != AsyncStatus::Completed) {
        if (i2cop->Status == AsyncStatus::Error) {
            PyErr_Format(PyExc_RuntimeError, "Could get I2C device: %d", i2cop->ErrorCode);
            return nullptr;
        }
        Sleep(50);
    }

    auto i2cdevice = i2cop->GetResults();
    if (i2cdevice == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "Could not find I2C device specified");
        return nullptr;
    }

    return i2cdevice;
}

static I2cDevice^ GetDeviceLightning(int id, I2cConnectionSettings^ settings) {
    auto asyncop = I2cController::GetControllersAsync(LightningI2cProvider::GetI2cProvider());
    while (asyncop->Status != AsyncStatus::Completed) {
        if (asyncop->Status == AsyncStatus::Error) {
            PyErr_Format(PyExc_RuntimeError, "Could not find i2c controller: %d", asyncop->ErrorCode);
            return nullptr;
        }
        Sleep(50);
    }

    auto controllers = asyncop->GetResults();
    if (controllers == nullptr) {
        PyErr_Format(PyExc_RuntimeError, "Could not find i2c controller");
        return nullptr;
    }

    if (id < 0 || static_cast<unsigned int>(id) >= controllers->Size) {
        PyErr_Format(PyExc_RuntimeError, "Could not find i2c controller id: '%d'", id);
        return nullptr;
    }

    auto controller = controllers->GetAt(id);
    auto i2cdevice = controller->GetDevice(settings);
    if (i2cdevice == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "Could not find I2C device specified");
        return nullptr;
    }

    return i2cdevice;
}

static I2cDevice^ GetDevice(int id, I2cConnectionSettings^ settings) {
    if (LightningProvider::IsLightningEnabled) {
        return GetDeviceLightning(id, settings);
    } else {
        return GetDeviceInboxDriver(id, settings);
    }
}

static I2cConnectionSettings^ GetI2cConnectionSettings(int slaveAddress, int busSpeed, int sharingMode) {
    I2cConnectionSettings^ settings = ref new I2cConnectionSettings(slaveAddress);

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
        return nullptr;
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
        return nullptr;
    }

    return settings;
}

extern "C" {
    void enable_lightning_if_available() {
        if (LightningProvider::IsLightningEnabled) {
            LowLevelDevicesController::DefaultProvider = LightningProvider::GetAggregateProvider();
        }
    }

    void *new_i2cdevice(int id, int slaveAddress, int busSpeed, int sharingMode) {
        ComPtr<IInspectable> spInspectable = nullptr;
        try {
            I2cConnectionSettings^ settings = GetI2cConnectionSettings(slaveAddress, busSpeed, sharingMode);
            if (settings == nullptr) {
                return nullptr;
            }

            auto i2cdevice = GetDevice(id, settings);
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
        //auto transferResult = i2cDevice->WriteReadPartial(ArrayReference<unsigned char>(udata, count), ArrayReference<unsigned char>(ubuffer, length));
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
