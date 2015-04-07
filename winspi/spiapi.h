/*
Copyright (c) 2012-2013 Ben Croston

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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

    void *new_spidevice(wchar_t *name, int chipSelectLine, int clockFrequency, int dataBitLength, int mode, int sharingMode);
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
    int get_supporteddatalengthbits_spibusinfo(void* businfo, int length, int** entries, int* count);

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