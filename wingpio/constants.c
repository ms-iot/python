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
#include "gpioapi.h"

void define_constants(PyObject *module)
{
	high = Py_BuildValue("i", HIGH);
	PyModule_AddObject(module, "HIGH", high);

	low = Py_BuildValue("i", LOW);
	PyModule_AddObject(module, "LOW", low);

	output = Py_BuildValue("i", OUTPUT);
	PyModule_AddObject(module, "OUT", output);

	input = Py_BuildValue("i", INPUT);
	PyModule_AddObject(module, "IN", input);

	pud_off = Py_BuildValue("i", PUD_OFF + PY_PUD_CONST_OFFSET);
	PyModule_AddObject(module, "PUD_OFF", pud_off);

	pud_up = Py_BuildValue("i", PUD_UP + PY_PUD_CONST_OFFSET);
	PyModule_AddObject(module, "PUD_UP", pud_up);

	pud_down = Py_BuildValue("i", PUD_DOWN + PY_PUD_CONST_OFFSET);
	PyModule_AddObject(module, "PUD_DOWN", pud_down);

	version = Py_BuildValue("s", "0.5.11");
	PyModule_AddObject(module, "VERSION", version);
}
