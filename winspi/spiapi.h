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

	void *new_spidevice(wchar_t *name, int chipSelectLine, int clockFrequency, int dataBitLength, int mode, int sharingMode);
	void delete_spidevice(void *device);
	void write_spidevice(void *device, char* data, unsigned int count);
	void read_spidevice(void *device, char* buffer, unsigned int length);
	void transfersequential_spidevice(void *device, char* data, unsigned int count, char* buffer, unsigned int length);
	void transferfullduplex_spidevice(void *device, char* data, unsigned int count, char* buffer, unsigned int length);
#ifdef __cplusplus
}
#endif

#define MODE0 0
#define MODE1 1
#define MODE2 2
#define MODE3 3

#define EXCLUSIVEMODE 0
#define SHAREDMODE 1