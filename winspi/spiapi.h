#pragma once

#ifdef __cplusplus
extern "C" {
#endif
    typedef struct {
        PyObject_HEAD
        void *ob_device;
    } PySpiDeviceObject;

    typedef struct {
        PyObject_HEAD
        void *ob_businfo;
    } PySpiBusInfoObject;

    int enable_lightning_if_available();

    void *new_spidevice(int id, int chipSelectLine, int clockFrequency, int dataBitLength, int mode, int sharingMode);
    void delete_spidevice(void *device);

    void *new_spibusinfo(wchar_t *name);
    void delete_spibusinfo(void *businfo);

    int write_spidevice(void *device, char* data, unsigned int count);
    int read_spidevice(void *device, char* buffer, unsigned int length);
    int transfersequential_spidevice(void *device, char* data, unsigned int count, char* buffer, unsigned int length);
    int transferfullduplex_spidevice(void *device, char* data, unsigned int count, char* buffer, unsigned int length);
    int get_deviceid_spidevice(void* device, char* deviceIdBuffer, unsigned int length);
    int get_chipselectline_spidevice(void* device, int* csl);
    int get_clockfrequency_spidevice(void* device, int* clockfrequency);
    int get_databitlength_spidevice(void* device, int* databitlength);
    int get_mode_spidevice(void* device, int* mode);
    int get_sharingmode_spidevice(void* device, int* sharingmode);

    int get_chipselectlinecount_spibusinfo(void* businfo, int* chipselectlinecount);
    int get_maxclockfrequency_spibusinfo(void* businfo, int* maxclockfrequency);
    int get_minclockfrequency_spibusinfo(void* businfo, int* minclockfrequency);
    int get_supporteddatalengthbits_spibusinfo(void* businfo, int length, int* entries, int* count);

#ifdef __cplusplus
}
#endif

#define SUCCESS 0
#define FAILURE -1

#define MODE0 0
#define MODE1 1
#define MODE2 2
#define MODE3 3

#define EXCLUSIVEMODE 0
#define SHAREDMODE 1