#include "python.h"
#include "spiapi.h"
#include "constants.h"
#include "structmember.h"

#define VALIDATE_SPI(d) \
    if (!(d) || !((d)->ob_device)) { \
        PyErr_SetString(PyExc_ValueError, \
             "SPI device is not initialized"); \
        return NULL; \
    }

PyDoc_STRVAR(spidevice_doc,
	"spidevice(deviceId=<deviceid>, chipselectline=<chipselectline>) -> spidevice\n");

static const char moduledocstring[] = "SPI functionality of a Windows \"Athens\" device using Python";

static void
spidevice_dealloc(PySpiDeviceObject *d) 
{
	delete_spidevice(d->ob_device);
	d->ob_device = NULL;
	Py_TYPE(d)->tp_free((PyObject*)d);
}

static PyObject *
spidevice_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PySpiDeviceObject *self;

	self = (PySpiDeviceObject*)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->ob_device = NULL;
	}

	return (PyObject*)self;
}

static PyObject *
py_spidevice_read(PySpiDeviceObject *self, PyObject *args, PyObject *kwargs)
{
	Py_RETURN_NONE;
}

static PyObject *
py_spidevice_write(PySpiDeviceObject *self, PyObject *args, PyObject *kwargs)
{
	Py_RETURN_NONE;
}

static PyMethodDef spidevice_methods[] = {
	{ "read", (PyCFunction)py_spidevice_read, METH_VARARGS | METH_KEYWORDS, "Reads data from SPI device" },
	{ "write", (PyCFunction)py_spidevice_write, METH_VARARGS | METH_KEYWORDS, "Writes data to SPI device" },
	{ NULL, NULL, 0, NULL }
};

static struct PyModuleDef winspimodule = {
	PyModuleDef_HEAD_INIT,
	"winspi",       // name of module
	moduledocstring,  // module documentation, may be NULL
	-1,               // size of per-interpreter state of the module, or -1 if the module keeps state in global variables.
};

static PyTypeObject spidevice_type = {
	PyVarObject_HEAD_INIT(0, 0)
	"winspi.spidevice",
	sizeof(PySpiDeviceObject),
	0
};

static int
spidevice_init(PySpiDeviceObject *self, PyObject *args, PyObject *kwds)
{
	static char *keywords[] = { "name", "chipselectline", "clockfrequency", "databitlength", "mode", "sharingmode", NULL };
	PyObject *name = NULL, *tmp = NULL;
	int chipselectline = 0;
	int clockfrequency = -1;
	int databitlength = -1;
	int mode = MODE0;
	int shareMode = EXCLUSIVEMODE;
	wchar_t* nameString = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, 
		kwds, 
		"Oi|iiii", 
		keywords, 
		&name, 
		&chipselectline,
		&clockfrequency,
		&databitlength,
		&mode,
		&shareMode))
		return -1;

	nameString = PyUnicode_AsWideCharString(name, NULL);

    Py_BEGIN_ALLOW_THREADS
	self->ob_device = new_spidevice(nameString, chipselectline, clockfrequency, databitlength, mode, shareMode);
    Py_END_ALLOW_THREADS
	if (self->ob_device == NULL)
		return -1;

	return 0;
}

PyMODINIT_FUNC 
PyInit_winspi(void)
{
	PyObject *module = NULL;

	// Initialize the device type
	spidevice_type.tp_dealloc = (destructor)spidevice_dealloc;
	spidevice_type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	spidevice_type.tp_doc = spidevice_doc;
	spidevice_type.tp_methods = spidevice_methods;
	spidevice_type.tp_init = (initproc)spidevice_init;
	spidevice_type.tp_alloc = PyType_GenericAlloc;
	spidevice_type.tp_new = spidevice_new;
	spidevice_type.tp_free = PyObject_Del;
	spidevice_type.tp_getattro = PyObject_GenericGetAttr;

	Py_TYPE(&spidevice_type) = &PyType_Type;

	if (PyType_Ready(&spidevice_type) < 0)
		return NULL;

	if ((module = PyModule_Create(&winspimodule)) == NULL)
		return NULL;

	Py_INCREF(&spidevice_type);
	PyModule_AddObject(module, "spidevice", (PyObject*)&spidevice_type);

	define_constants(module);

	return module;
}
