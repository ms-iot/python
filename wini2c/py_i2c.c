#include "python.h"
#include "i2capi.h"
#include "constants.h"
#include "structmember.h"

PyDoc_STRVAR(i2cdevice_doc,
"i2cdevice(deviceId=<deviceid>, address=<address>) -> i2cdevice\n");

static const char moduledocstring[] = "I2C functionality of a Windows \"Athens\" device using Python";

static void
i2cdevice_dealloc(PyI2cDeviceObject *d) 
{
	delete_i2cdevice(d->i2cdevice);
	d->i2cdevice = NULL;
}

static PyObject *
i2cdevice_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *newObj;

	newObj = type->tp_alloc(type, 0);

	return newObj;
}

static PyMethodDef i2cdevice_methods[] = {
	{ NULL, NULL, 0, NULL }
};

static PyMemberDef i2cdevice_memberlist[] = {
	{0},
};

static PyMethodDef win_i2c_methods[] = {
	{ NULL, NULL, 0, NULL }
};

static struct PyModuleDef wini2cmodule = {
	PyModuleDef_HEAD_INIT,
	"wini2c",       // name of module
	moduledocstring,  // module documentation, may be NULL
	-1,               // size of per-interpreter state of the module, or -1 if the module keeps state in global variables.
	win_i2c_methods
};

static PyTypeObject i2cdevice_type = {
	PyVarObject_HEAD_INIT(0, 0)
	"wini2c.i2cdevice",
	sizeof(PyI2cDeviceObject),
	0
};

static int
i2cdevice_initobj(PyObject *self, PyObject *args, PyObject *kwds)
{
	PyI2cDeviceObject *d = (PyI2cDeviceObject *)self;
	static char *keywords[] = { "deviceId", "address", 0 };
	char *deviceId = NULL;
	int address = 0;

	if (!PyArg_ParseTupleAndKeywords(args, 
		kwds, 
		"|ziO:i2cdevice", 
		keywords, 
		&deviceId, 
		&address))
		return -1;

	d->i2cdevice = new_i2cdevice(deviceId, address);

	return 0;
}

PyMODINIT_FUNC PyInit_wini2c(void)
{
	PyObject *module = NULL;

	// Initialize the device type
	i2cdevice_type.tp_dealloc = (destructor)i2cdevice_dealloc;
	i2cdevice_type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	i2cdevice_type.tp_doc = i2cdevice_doc;
	i2cdevice_type.tp_methods = i2cdevice_methods;
	i2cdevice_type.tp_members = i2cdevice_memberlist;
	i2cdevice_type.tp_init = i2cdevice_initobj;
	i2cdevice_type.tp_alloc = PyType_GenericAlloc;
	i2cdevice_type.tp_new = i2cdevice_new;
	i2cdevice_type.tp_free = PyObject_Del;
	i2cdevice_type.tp_getattro = PyObject_GenericGetAttr;

	Py_TYPE(&i2cdevice_type) = &PyType_Type;

	if ((module = PyModule_Create(&wini2cmodule)) == NULL)
		return NULL;

	define_constants(module);

	return module;
}
