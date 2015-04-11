#include "python.h"
#include "i2capi.h"
#include "constants.h"
#include "structmember.h"

#define VALIDATE_I2C(d) \
    if (!(d) || !((d)->ob_device)) { \
        PyErr_SetString(PyExc_ValueError, \
             "I2C device is not initialized"); \
        return NULL; \
    }

static const char moduledocstring[] = "I2C functionality of a Windows 10 IoT Core device";

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

PyDoc_STRVAR(read_doc,
    "read(count) -> bytes\n"
    "\n"
    "Reads from the device the specified number of bytes\n"
    "\n"
    "count=Number of bytes to read\n"
    );
static PyObject *py_i2cdevice_read(PyI2cDeviceObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = { "count", NULL };
	int* count = 1;
	PyBytesObject* result = NULL;

	VALIDATE_I2C(self);

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist, &count))
		return NULL;

	result = PyBytes_FromStringAndSize(NULL, count);

    Py_BEGIN_ALLOW_THREADS
    if (FAILURE == read_i2cdevice(self->ob_device, PyBytes_AsString(result), count)) {
        Py_DECREF(result);
        result = NULL;
    }
    Py_END_ALLOW_THREADS

	return result;
}

PyDoc_STRVAR(write_doc,
    "write(bytes)\n"
    "\n"
    "Writes the bytes to the device\n"
    "\n"
    "bytes=Byte array to be written to the device\n"
    );
static PyObject *py_i2cdevice_write(PyI2cDeviceObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = { "data", NULL };
	PyObject* data = NULL;
	PyBytesObject* bytes = NULL;
    PyObject* result = Py_None;

	VALIDATE_I2C(self);

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|", kwlist, &data))
		return NULL;

    bytes = PyBytes_FromObject(data);
	if (bytes == NULL)
		return NULL;

    Py_BEGIN_ALLOW_THREADS
    if (FAILURE == write_i2cdevice(self->ob_device, PyBytes_AsString(bytes), PyBytes_Size(bytes))) {
        result = NULL;
    }
    Py_END_ALLOW_THREADS

	return result;
}

PyDoc_STRVAR(writeread_doc,
    "writeread(bytes, count) -> bytes\n"
    "\n"
    "Writes the bytes to the device and reads the expected number of bytes\n"
    "\n"
    "bytes=Byte array to be written to the device\n"
    "\n",
    "count=Number of bytes expected to be read\n"
    );
static PyObject *py_i2cdevice_writeread(PyI2cDeviceObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = { "data", "count", NULL };
	PyObject* data = NULL;
	PyBytesObject* byteArray = NULL;
	int* count = 1;
    PyBytesObject* result = NULL;

	VALIDATE_I2C(self);

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|i", kwlist, &data, &count))
		return NULL;

	byteArray = PyBytes_FromObject(data);
	if (byteArray == NULL)
		return NULL;

	result = PyBytes_FromStringAndSize(NULL, count);

    Py_BEGIN_ALLOW_THREADS
    if (FAILURE == writeread_i2cdevice(self->ob_device, PyBytes_AsString(byteArray), PyBytes_Size(byteArray), PyBytes_AsString(result), count)) {
        Py_DECREF(result);
        result = NULL;
    }
    Py_END_ALLOW_THREADS

	return result;
}

PyDoc_STRVAR(deviceid_doc,
    "deviceid() -> string\n"
    "\n"
    "Retrieves the device id for the I2C device\n"
    "\n"
    );
static PyObject *
py_i2cdevice_deviceid(PyI2cDeviceObject *self, PyObject *args) {
    char id[_MAX_FNAME];
    PyObject* result = NULL;
    VALIDATE_I2C(self);

    Py_BEGIN_ALLOW_THREADS
    if (SUCCESS == get_deviceid_i2cdevice(self->ob_device, &id, _MAX_FNAME)) {
        result = PyUnicode_FromString(id);
    }
    Py_END_ALLOW_THREADS

    return result;
}

PyDoc_STRVAR(slaveaddress_doc,
    "slaveaddress() -> int\n"
    "\n"
    "Retrieves the slave address for the I2C device\n"
    "\n"
    );
static PyObject *py_i2cdevice_slaveaddress(PyI2cDeviceObject *self, PyObject *args)
{
	VALIDATE_I2C(self);
    PyObject* result = NULL;
    int val = 0;

    Py_BEGIN_ALLOW_THREADS
    if (SUCCESS == get_address_i2cdevice(self->ob_device, &val)) {
        result = PyLong_FromLong(val);
    }
    Py_END_ALLOW_THREADS

	return result;
}

PyDoc_STRVAR(busspeed_doc,
    "busspeed() -> int (FASTSPEED or STANDARDSPEED)\n"
    "\n"
    "Retrieves the bus speed for the I2C device\n"
    "\n"
    );
static PyObject *py_i2cdevice_busspeed(PyI2cDeviceObject *self, PyObject *args)
{
	VALIDATE_I2C(self);
    PyObject* result = NULL;
    int val = 0;

    Py_BEGIN_ALLOW_THREADS
    if (SUCCESS == get_busspeed_i2cdevice(self->ob_device, &val)) {
        result = PyLong_FromLong(val);
    }
    Py_END_ALLOW_THREADS

	return result;
}

PyDoc_STRVAR(sharingmode_doc,
    "sharingmode() -> int (EXCLUSIVEMODE or SHAREDMODE)\n"
    "\n"
    "Retrieves the sharing mode for the I2C device\n"
    "\n"
    );
static PyObject *py_i2cdevice_sharingmode(PyI2cDeviceObject *self, PyObject *args)
{
    VALIDATE_I2C(self);
    PyObject* result = NULL;
    int val = 0;

    Py_BEGIN_ALLOW_THREADS
    if (SUCCESS == get_sharingmode_i2cdevice(self->ob_device, &val)) {
        result = PyLong_FromLong(val);
    }
    Py_END_ALLOW_THREADS

    return result;
}

static PyMethodDef i2cdevice_methods[] = {
	{ "read", (PyCFunction)py_i2cdevice_read, METH_VARARGS | METH_KEYWORDS, read_doc },
	{ "write", (PyCFunction)py_i2cdevice_write, METH_VARARGS | METH_KEYWORDS, write_doc },
	{ "writeread", (PyCFunction)py_i2cdevice_writeread, METH_VARARGS | METH_KEYWORDS, writeread_doc },
    { "deviceid", (PyCFunction)py_i2cdevice_deviceid, METH_NOARGS, deviceid_doc },
	{ "slaveaddress", (PyCFunction)py_i2cdevice_slaveaddress, METH_NOARGS, slaveaddress_doc },
	{ "busspeed", (PyCFunction)py_i2cdevice_busspeed, METH_NOARGS, busspeed_doc },
	{ "sharingmode", (PyCFunction)py_i2cdevice_sharingmode, METH_NOARGS, sharingmode_doc },
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

PyDoc_STRVAR(i2cdevice_doc,
    "i2cdevice(friendlyname, slaveaddress, busspeed=STANDARDSPEED, sharingmode=EXCLUSIVEMODE) -> i2cdevice\n"
    "\n"
    "Creates a new instance of an I2C device");
static int
i2cdevice_init(PyI2cDeviceObject *self, PyObject *args, PyObject *kwds)
{
	static char *keywords[] = { "name", "slaveaddress", "busspeed", "sharingmode", NULL };
	PyObject *name = NULL, *tmp = NULL;
	int slaveAddress = 0;
	int sharingMode = EXCLUSIVEMODE;
	int busSpeed = STANDARDSPEED;
	wchar_t* nameString = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, 
		kwds, 
		"Oi|ii", 
		keywords, 
		&name, 
		&slaveAddress,
		&busSpeed,
		&sharingMode))
		return -1;

	nameString = PyUnicode_AsWideCharString(name, NULL);

    Py_BEGIN_ALLOW_THREADS
	self->ob_device = new_i2cdevice(nameString, slaveAddress, busSpeed, sharingMode);
    Py_END_ALLOW_THREADS
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
