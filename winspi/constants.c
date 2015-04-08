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
