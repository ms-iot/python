#pragma once

#ifdef __cplusplus
extern "C" {
#endif
    typedef struct {
        PyObject_HEAD
        void *ob_device;
    } PyI2cDeviceObject;

    void *new_i2cdevice(wchar_t *name, int slaveAddress, int busSpeed, int sharingMode);
    void delete_i2cdevice(void *device);
    int write_i2cdevice(void *device, char* data, unsigned int count);
    int read_i2cdevice(void *device, char* buffer, unsigned int length);
    int writeread_i2cdevice(void *device, char* data, unsigned int count, char* buffer, unsigned int length);

    int get_deviceid_i2cdevice(void* device, char* deviceIdBuffer, unsigned int length);
    int get_busspeed_i2cdevice(void *device, int* busspeed);
    int get_sharingmode_i2cdevice(void *device, int* sharingmode);
    int get_address_i2cdevice(void *device, int* slaveaddress);

#ifdef __cplusplus
}
#endif

#define SUCCESS 0
#define FAILURE -1

#define STANDARDSPEED 0
#define FASTSPEED 1

#define EXCLUSIVEMODE 0
#define SHAREDMODE 1
