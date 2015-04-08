#include "Python.h"
#include "constants.h"
#include "i2capi.h"

void define_constants(PyObject *module) {
	fastspeed = Py_BuildValue("i", FASTSPEED);
	PyModule_AddObject(module, "FASTSPEED", fastspeed);

	standardspeed = Py_BuildValue("i", STANDARDSPEED);
	PyModule_AddObject(module, "STANDARDSPEED", standardspeed);

	exclusivemode = Py_BuildValue("i", EXCLUSIVEMODE);
	PyModule_AddObject(module, "EXCLUSIVEMODE", exclusivemode);

	sharedmode = Py_BuildValue("i", SHAREDMODE);
	PyModule_AddObject(module, "SHAREDMODE", sharedmode);
}
