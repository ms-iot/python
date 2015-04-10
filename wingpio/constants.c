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
}
