#include "python.h"
#include "i2capi.h"
#include "constants.h"
#include "structmember.h"

#define VALIDATE_I2C(d) \
    if (!(d) && !((d)->ob_device)) { \
        PyErr_SetString(PyExc_ValueError, \
             "I2C device is not initialized"); \
        return NULL; \
    }

PyDoc_STRVAR(i2cdevice_doc,
"i2cdevice(deviceId=<deviceid>, address=<address>) -> i2cdevice\n");

static const char moduledocstring[] = "I2C functionality of a Windows \"Athens\" device using Python";

static void
i2cdevice_dealloc(PyI2cDeviceObject *d) 
{
	delete_i2cdevice(d->ob_device);
	d->ob_device = NULL;
	Py_TYPE(d)->tp_free((PyObject*)d);
}

static PyObject *
i2cdevice_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyI2cDeviceObject *self;

	self = (PyI2cDeviceObject*)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->ob_device = NULL;
	}

	return (PyObject*)self;
}

static PyObject *py_i2cdevice_read(PyI2cDeviceObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = { "count", NULL };
	int* count = 1;
	PyByteArrayObject* result = NULL;

	VALIDATE_I2C(self);

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist, &count))
		return NULL;

	result = PyByteArray_FromStringAndSize(NULL, count);

	read_i2cdevice(self->ob_device, result->ob_bytes, count);

	return result;
}

static PyObject *py_i2cdevice_write(PyI2cDeviceObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = { "data", NULL };
	PyObject* data = NULL;
	PyByteArrayObject* byteArray = NULL;

	VALIDATE_I2C(self);

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|", kwlist, &data))
		return NULL;

	byteArray = PyBytes_FromObject(data);
	if (byteArray == NULL)
		return NULL;

	write_i2cdevice(self->ob_device, byteArray->ob_bytes, PyByteArray_Size(byteArray));

	Py_RETURN_NONE;
}

static PyObject *py_i2cdevice_writeread(PyI2cDeviceObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = { "data", "count", NULL };
	PyObject* data = NULL;
	PyByteArrayObject* byteArray = NULL;
	int* count = 1;
	PyByteArrayObject* result = NULL;

	VALIDATE_I2C(self);

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|i", kwlist, &data, &count))
		return NULL;

	byteArray = PyBytes_FromObject(data);
	if (byteArray == NULL)
		return NULL;

	result = PyByteArray_FromStringAndSize(NULL, count);

	writeread_i2cdevice(self->ob_device, byteArray->ob_bytes, PyByteArray_Size(byteArray), result->ob_bytes, count);

	return result;
}

static PyObject *py_i2cdevice_slaveaddress(PyI2cDeviceObject *self, PyObject *args)
{
	VALIDATE_I2C(self);

	long val = getaddress_i2cdevice(self->ob_device);

	if (val != -1) {
		return PyLong_FromLong(val);
	}

	return NULL;
}

static PyObject *py_i2cdevice_busspeed(PyI2cDeviceObject *self, PyObject *args)
{
	VALIDATE_I2C(self);

	long val = getbusspeed_i2cdevice(self->ob_device);

	if (val != -1) {
		return PyLong_FromLong(val);
	}

	return NULL;
}

static PyObject *py_i2cdevice_sharingmode(PyI2cDeviceObject *self, PyObject *args)
{
	VALIDATE_I2C(self);

	long val = getsharingmode_i2cdevice(self->ob_device);

	if (val != -1) {
		return PyLong_FromLong(val);
	}

	return NULL;
}

static PyMethodDef i2cdevice_methods[] = {
	{ "read", (PyCFunction)py_i2cdevice_read, METH_VARARGS | METH_KEYWORDS, "Reads data from I2C device" },
	{ "write", (PyCFunction)py_i2cdevice_write, METH_VARARGS | METH_KEYWORDS, "Writes data to I2C device" },
	{ "writeread", (PyCFunction)py_i2cdevice_writeread, METH_VARARGS | METH_KEYWORDS, "Writes data to and reads data from I2C device" },
	{ "slaveaddress", (PyCFunction)py_i2cdevice_slaveaddress, METH_NOARGS, "Returns slave address for I2C device" },
	{ "busspeed", (PyCFunction)py_i2cdevice_busspeed, METH_NOARGS, "Returns bus speed for I2C device" },
	{ "sharingmode", (PyCFunction)py_i2cdevice_sharingmode, METH_NOARGS, "Returns sharing mode for I2C device" },
	{ NULL, NULL, 0, NULL }
};

static struct PyModuleDef wini2cmodule = {
	PyModuleDef_HEAD_INIT,
	"wini2c",       // name of module
	moduledocstring,  // module documentation, may be NULL
	-1,               // size of per-interpreter state of the module, or -1 if the module keeps state in global variables.
};

static PyTypeObject i2cdevice_type = {
	PyVarObject_HEAD_INIT(0, 0)
	"wini2c.i2cdevice",
	sizeof(PyI2cDeviceObject),
	0
};

static int
i2cdevice_init(PyI2cDeviceObject *self, PyObject *args, PyObject *kwds)
{
	static char *keywords[] = { "name", "slaveaddress", "busspeed", "sharemode", NULL };
	PyObject *name = NULL, *tmp = NULL;
	int slaveAddress = 0;
	int shareMode = EXCLUSIVEMODE;
	int busSpeed = STANDARDSPEED;
	wchar_t* nameString = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, 
		kwds, 
		"Oi|ii", 
		keywords, 
		&name, 
		&slaveAddress,
		&busSpeed,
		&shareMode))
		return -1;

	nameString = PyUnicode_AsWideCharString(name, NULL);

	self->ob_device = new_i2cdevice(nameString, slaveAddress, busSpeed, shareMode);
	if (self->ob_device == NULL)
		return -1;

	return 0;
}

PyMODINIT_FUNC 
PyInit_wini2c(void)
{
	PyObject *module = NULL;

	// Initialize the device type
	i2cdevice_type.tp_dealloc = (destructor)i2cdevice_dealloc;
	i2cdevice_type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	i2cdevice_type.tp_doc = i2cdevice_doc;
	i2cdevice_type.tp_methods = i2cdevice_methods;
	i2cdevice_type.tp_init = (initproc)i2cdevice_init;
	i2cdevice_type.tp_alloc = PyType_GenericAlloc;
	i2cdevice_type.tp_new = i2cdevice_new;
	i2cdevice_type.tp_free = PyObject_Del;
	i2cdevice_type.tp_getattro = PyObject_GenericGetAttr;

	Py_TYPE(&i2cdevice_type) = &PyType_Type;

	if (PyType_Ready(&i2cdevice_type) < 0)
		return NULL;

	if ((module = PyModule_Create(&wini2cmodule)) == NULL)
		return NULL;

	Py_INCREF(&i2cdevice_type);
	PyModule_AddObject(module, "i2cdevice", (PyObject*)&i2cdevice_type);

	define_constants(module);

	return module;
}
