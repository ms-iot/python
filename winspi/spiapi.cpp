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
#define SPIBUSINFO_TOPOINTER(d) (reinterpret_cast<IInspectable*>((Object^)d))
#define SPIBUSINFO_FROMPOINTER(p) ((SpiBusInfo^)reinterpret_cast<Object^>(reinterpret_cast<IInspectable*>(p)))
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
            PyErr_Format(PyExc_TypeError, "Invalid SPI mode specified '%d'", mode);
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
            PyErr_Format(PyExc_TypeError, "Invalid SPI sharing mode specified '%d'", sharingMode);
            return NULL;
        }

        auto getdevicetask = create_task(SpiDevice::FromIdAsync(id, settings));

        auto spidevice = getdevicetask.get();

        spInspectable = SPIDEVICE_TOPOINTER(spidevice);

        return spInspectable.Detach();
    }

    void *new_spibusinfo(wchar_t *name) {
        ComPtr<IInspectable> spInspectable = nullptr;
        String^ deviceName = ref new String(name);
        String^ querySyntax = SpiDevice::GetDeviceSelector(deviceName);
        auto info = create_task(Windows::Devices::Enumeration::DeviceInformation::FindAllAsync(querySyntax)).get();
        String^ id = info->GetAt(0)->Id;

        auto spibusinfo = SpiDevice::GetBusInfo(id);

        spInspectable = SPIBUSINFO_TOPOINTER(spibusinfo);

        return spInspectable.Detach();
    }

    void delete_spidevice(void *device) {
        if (device != NULL) {
            ComPtr<IInspectable> realDevice;
            realDevice.Attach((IInspectable*)device);
        }
    }

    void delete_spibusinfo(void *businfo) {
        if (businfo != NULL) {
            ComPtr<IInspectable> realBusInfo;
            realBusInfo.Attach((IInspectable*)businfo);
        }
    }

    int write_spidevice(void *device, char* data, unsigned int count) {
        int ret = FAILURE;

        BEGIN_PYERR_EXCEPTION_WATCH
        SpiDevice^ deviceObj = SPIDEVICE_FROMPOINTER(device);
        unsigned char* udata = reinterpret_cast<unsigned char*>(data);
        deviceObj->Write(ArrayReference<unsigned char>(udata, count));
        ret = SUCCESS;
        END_PYERR_EXCEPTION_WATCH

        return ret;
	}

	int read_spidevice(void *device, char* buffer, unsigned int length) {
        int ret = FAILURE;

        BEGIN_PYERR_EXCEPTION_WATCH
        SpiDevice^ deviceObj = SPIDEVICE_FROMPOINTER(device);
        unsigned char* ubuffer = reinterpret_cast<unsigned char*>(buffer);
        deviceObj->Read(ArrayReference<unsigned char>(ubuffer, length));
        ret = SUCCESS;
        END_PYERR_EXCEPTION_WATCH

        return ret;
	}

	int transfersequential_spidevice(void *device, char* data, unsigned int count, char* buffer, unsigned int length) {
        int ret = FAILURE;

        BEGIN_PYERR_EXCEPTION_WATCH
        SpiDevice^ deviceObj = SPIDEVICE_FROMPOINTER(device);
		unsigned char* udata = reinterpret_cast<unsigned char*>(data);
		unsigned char* ubuffer = reinterpret_cast<unsigned char*>(buffer);

		deviceObj->TransferSequential(ArrayReference<unsigned char>(udata, count), ArrayReference<unsigned char>(ubuffer, length));
        ret = SUCCESS;
        END_PYERR_EXCEPTION_WATCH

        return ret;
    }

	int transferfullduplex_spidevice(void *device, char* data, unsigned int count, char* buffer, unsigned int length) {
        int ret = FAILURE;

        BEGIN_PYERR_EXCEPTION_WATCH
        SpiDevice^ deviceObj = SPIDEVICE_FROMPOINTER(device);
		unsigned char* udata = reinterpret_cast<unsigned char*>(data);
		unsigned char* ubuffer = reinterpret_cast<unsigned char*>(buffer);

		deviceObj->TransferFullDuplex(ArrayReference<unsigned char>(udata, count), ArrayReference<unsigned char>(ubuffer, length));
        ret = SUCCESS;
        END_PYERR_EXCEPTION_WATCH

        return ret;
    }

    int get_deviceid_spidevice(void* device, char* deviceIdBuffer, unsigned int length) {
        int ret = FAILURE;

        VALIDATE_POINTER(deviceIdBuffer, "Data buffer is not valid");

        // Initialize
        deviceIdBuffer[0] = '\0';

        BEGIN_PYERR_EXCEPTION_WATCH

        SpiDevice^ deviceObj = SPIDEVICE_FROMPOINTER(device);

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
        } else {
            PyErr_Format(PyExc_RuntimeError, "Buffer length '%d' was not large enough for id length '%d'", length, deviceId->Length());
        }
        END_PYERR_EXCEPTION_WATCH

        return ret;
    }

    int get_chipselectline_spidevice(void* device, int* csl) {
        int ret = FAILURE;

        VALIDATE_POINTER(csl, "Pointer is not valid");

        *csl = 0;

        BEGIN_PYERR_EXCEPTION_WATCH
        SpiDevice^ deviceObj = SPIDEVICE_FROMPOINTER(device);

        *csl = deviceObj->ConnectionSettings->ChipSelectLine;
        ret = SUCCESS;
        END_PYERR_EXCEPTION_WATCH

        return ret;
    }

    int get_clockfrequency_spidevice(void* device, int* clockfrequency) {
        int ret = FAILURE;

        VALIDATE_POINTER(clockfrequency, "Pointer is not valid");

        *clockfrequency = 0;

        BEGIN_PYERR_EXCEPTION_WATCH
        SpiDevice^ deviceObj = SPIDEVICE_FROMPOINTER(device);

        *clockfrequency = deviceObj->ConnectionSettings->ClockFrequency;
        ret = SUCCESS;
        END_PYERR_EXCEPTION_WATCH

        return ret;
    }

    int get_databitlength_spidevice(void* device, int* databitlength) {
        int ret = FAILURE;

        VALIDATE_POINTER(databitlength, "Pointer is not valid");

        *databitlength = 0;

        BEGIN_PYERR_EXCEPTION_WATCH
        SpiDevice^ deviceObj = SPIDEVICE_FROMPOINTER(device);

        *databitlength = deviceObj->ConnectionSettings->DataBitLength;
        ret = SUCCESS;
        END_PYERR_EXCEPTION_WATCH

        return ret;
    }

    int get_mode_spidevice(void* device, int* mode) {
        int ret = FAILURE;

        VALIDATE_POINTER(mode, "Pointer is not valid");

        *mode = 0;

        BEGIN_PYERR_EXCEPTION_WATCH
        SpiDevice^ deviceObj = SPIDEVICE_FROMPOINTER(device);

        *mode = (int)deviceObj->ConnectionSettings->Mode;
        ret = SUCCESS;
        END_PYERR_EXCEPTION_WATCH

        return ret;
    }

    int get_sharingmode_spidevice(void* device, int* sharingmode) {
        int ret = FAILURE;

        VALIDATE_POINTER(sharingmode, "Pointer is not valid");

        *sharingmode = 0;

        BEGIN_PYERR_EXCEPTION_WATCH
        SpiDevice^ deviceObj = SPIDEVICE_FROMPOINTER(device);

        *sharingmode = (int)deviceObj->ConnectionSettings->SharingMode;
        ret = SUCCESS;
        END_PYERR_EXCEPTION_WATCH

        return ret;
    }

    int get_chipselectlinecount_spibusinfo(void* businfo, int* chipselectlinecount) {
        int ret = FAILURE;

        VALIDATE_POINTER(chipselectlinecount, "Pointer is not valid");

        *chipselectlinecount = 0;

        BEGIN_PYERR_EXCEPTION_WATCH;
        SpiBusInfo^ busInfoObj = SPIBUSINFO_FROMPOINTER(businfo);

        *chipselectlinecount = busInfoObj->ChipSelectLineCount;
        ret = SUCCESS;

        END_PYERR_EXCEPTION_WATCH;

        return ret;
    }

    int get_maxclockfrequency_spibusinfo(void* businfo, int* maxclockfrequency) {
        int ret = FAILURE;

        VALIDATE_POINTER(maxclockfrequency, "Pointer is not valid");

        *maxclockfrequency = 0;

        BEGIN_PYERR_EXCEPTION_WATCH;
        SpiBusInfo^ busInfoObj = SPIBUSINFO_FROMPOINTER(businfo);

        *maxclockfrequency = busInfoObj->MaxClockFrequency;
        ret = SUCCESS;

        END_PYERR_EXCEPTION_WATCH;

        return ret;
    }

    int get_minclockfrequency_spibusinfo(void* businfo, int* minclockfrequency) {
        int ret = FAILURE;

        VALIDATE_POINTER(minclockfrequency, "Pointer is not valid");

        *minclockfrequency = 0;

        BEGIN_PYERR_EXCEPTION_WATCH;
        SpiBusInfo^ busInfoObj = SPIBUSINFO_FROMPOINTER(businfo);

        *minclockfrequency = busInfoObj->MinClockFrequency;
        ret = SUCCESS;

        END_PYERR_EXCEPTION_WATCH;

        return ret;
    }

    int get_supporteddatalengthbits_spibusinfo(void* businfo, int length, int** entries, int* count) {
        int ret = FAILURE;

        VALIDATE_POINTER(count, "Pointer is not valid");
        *count = 0;

        BEGIN_PYERR_EXCEPTION_WATCH;
        SpiBusInfo^ busInfoObj = SPIBUSINFO_FROMPOINTER(businfo);

        *count = busInfoObj->SupportedDataBitLengths->Size;

        if (entries != NULL && length < *count) {
            PyErr_SetString(PyExc_RuntimeError, "Spaces for entries is less than total number of entries");
        } else {
            for (int i = 0; i < *count; i++) {
                (*entries)[i] = busInfoObj->SupportedDataBitLengths->GetAt(i);
            }
            ret = SUCCESS;
        }

        END_PYERR_EXCEPTION_WATCH;

        return ret;
    }
}