/*
Copyright (c) 2013-2015 Ben Croston

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

#include "Python.h"
#include "constants.h"
#include "spiapi.h"

void define_constants(PyObject *module) {
	mode0 = Py_BuildValue("i", MODE0);
	PyModule_AddObject(module, "MODE0", mode0);

	mode1 = Py_BuildValue("i", MODE1);
	PyModule_AddObject(module, "MODE1", mode1);

	mode2 = Py_BuildValue("i", MODE2);
	PyModule_AddObject(module, "MODE2", mode2);

	mode3 = Py_BuildValue("i", MODE3);
	PyModule_AddObject(module, "MODE3", mode3);

	exclusivemode = Py_BuildValue("i", EXCLUSIVEMODE);
	PyModule_AddObject(module, "EXCLUSIVEMODE", exclusivemode);

	sharedmode = Py_BuildValue("i", SHAREDMODE);
	PyModule_AddObject(module, "SHAREDMODE", sharedmode);
}
