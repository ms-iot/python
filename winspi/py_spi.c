#include "python.h"
#include "spiapi.h"
#include "constants.h"
#include "structmember.h"

#define VALIDATE_SPIDEVICE(d) \
    if (!(d) || !((d)->ob_device)) { \
        PyErr_SetString(PyExc_ValueError, \
             "SPI device is not initialized"); \
        return NULL; \
    }

#define VALIDATE_SPIBUSINFO(d) \
    if (!(d) || !((d)->ob_businfo)) { \
        PyErr_SetString(PyExc_ValueError, \
             "SPI businfo is not initialized"); \
        return NULL; \
    }

PyDoc_STRVAR(spidevice_doc,
    "spidevice(friendlyname, chipselectline=<chipselectline>) -> spidevice\n");

PyDoc_STRVAR(spibusinfo_doc,
    "spibusinfo(friendlyname) -> spibusinfo\n");

static const char moduledocstring[] = "SPI functionality of a Windows 10 IoT Core device using Python";

static void
spidevice_dealloc(PySpiDeviceObject *d)
{
    delete_spidevice(d->ob_device);
    d->ob_device = NULL;
    Py_TYPE(d)->tp_free((PyObject*)d);
}

static void
spibusinfo_dealloc(PySpiBusInfoObject *b)
{
    delete_spibusinfo(b->ob_businfo);
    b->ob_businfo = NULL;
    Py_TYPE(b)->tp_free((PyObject*)b);
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
spibusinfo_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PySpiBusInfoObject *self;

    self = (PySpiBusInfoObject*)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->ob_businfo = NULL;
    }

    return (PyObject*)self;
}

static PyObject *
py_spidevice_read(PySpiDeviceObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "count", NULL };
    int* count = 1;
    PyBytesObject* result = NULL;

    VALIDATE_SPIDEVICE(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist, &count))
        return NULL;

    result = PyBytes_FromStringAndSize(NULL, count);

    Py_BEGIN_ALLOW_THREADS
    if (FAILURE == read_spidevice(self->ob_device, PyBytes_AsString(result), count)) {
        Py_DECREF(result);
        result = NULL;
    }
    Py_END_ALLOW_THREADS

    return result;
}

static PyObject *
py_spidevice_write(PySpiDeviceObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "data", NULL };
    PyObject* data = NULL;
    PyBytesObject* bytes = NULL;
    PyObject* result = Py_None;

    VALIDATE_SPIDEVICE(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|", kwlist, &data))
        return NULL;

    bytes = PyBytes_FromObject(data);
    if (bytes == NULL)
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    if (FAILURE == write_spidevice(self->ob_device, PyBytes_AsString(bytes), PyBytes_Size(bytes))) {
        result = NULL;
    }
    Py_END_ALLOW_THREADS

    return result;
}

static PyObject *
py_spidevice_transfer(PySpiDeviceObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "data", "count", NULL };
    PyObject* data = NULL;
    PyBytesObject* byteArray = NULL;
    int* count = 1;
    PyBytesObject* result = NULL;

    VALIDATE_SPIDEVICE(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|i", kwlist, &data, &count))
        return NULL;

    byteArray = PyBytes_FromObject(data);
    if (byteArray == NULL)
        return NULL;

    result = PyBytes_FromStringAndSize(NULL, count);

    Py_BEGIN_ALLOW_THREADS
    if (FAILURE == transferfullduplex_spidevice(self->ob_device, PyBytes_AsString(byteArray), PyBytes_Size(byteArray), PyBytes_AsString(result), count)) {
        result = NULL;
    }
    Py_END_ALLOW_THREADS

    return result;
}

static PyObject *
py_spidevice_transfersequential(PySpiDeviceObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "data", "count", NULL };
    PyObject* data = NULL;
    PyBytesObject* byteArray = NULL;
    int* count = 1;
    PyBytesObject* result = NULL;

    VALIDATE_SPIDEVICE(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|i", kwlist, &data, &count))
        return NULL;

    byteArray = PyBytes_FromObject(data);
    if (byteArray == NULL)
        return NULL;

    result = PyBytes_FromStringAndSize(NULL, count);

    Py_BEGIN_ALLOW_THREADS
    if (FAILURE == transfersequential_spidevice(self->ob_device, PyBytes_AsString(byteArray), PyBytes_Size(byteArray), PyBytes_AsString(result), count)) {
        result = NULL;
    }
    Py_END_ALLOW_THREADS

    return result;
}

static PyObject *
py_spidevice_deviceid(PySpiDeviceObject *self, PyObject *args) {
    char id[_MAX_FNAME];
    PyObject* result = NULL;
    VALIDATE_SPIDEVICE(self);

    Py_BEGIN_ALLOW_THREADS
    if (SUCCESS == get_deviceid_spidevice(self->ob_device, &id, _MAX_FNAME)) {
        result = PyUnicode_FromString(id);
    }
    Py_END_ALLOW_THREADS

    return result;
}

static PyObject *
py_spidevice_chipselectline(PySpiDeviceObject *self, PyObject *args) {
    int val = 0;
    PyObject* result = NULL;
    VALIDATE_SPIDEVICE(self);

    Py_BEGIN_ALLOW_THREADS
    if (SUCCESS == get_chipselectline_spidevice(self->ob_device, &val)) {
        result = PyLong_FromLong(val);
    }
    Py_END_ALLOW_THREADS

    return result;
}

static PyObject *
py_spidevice_clockfrequency(PySpiDeviceObject *self, PyObject *args) {
    int val = 0;
    PyObject* result = NULL;
    VALIDATE_SPIDEVICE(self);

    Py_BEGIN_ALLOW_THREADS
    if (SUCCESS == get_clockfrequency_spidevice(self->ob_device, &val)) {
        result = PyLong_FromLong(val);
    }
    Py_END_ALLOW_THREADS

    return result;
}

static PyObject *
py_spidevice_databitlength(PySpiDeviceObject *self, PyObject *args) {
    int val = 0;
    PyObject* result = NULL;
    VALIDATE_SPIDEVICE(self);

    Py_BEGIN_ALLOW_THREADS
    if (SUCCESS == get_databitlength_spidevice(self->ob_device, &val)) {
        result = PyLong_FromLong(val);
    }
    Py_END_ALLOW_THREADS

    return result;
}

static PyObject *
py_spidevice_mode(PySpiDeviceObject *self, PyObject *args) {
    int val = 0;
    PyObject* result = NULL;
    VALIDATE_SPIDEVICE(self);

    Py_BEGIN_ALLOW_THREADS
    if (SUCCESS == get_mode_spidevice(self->ob_device, &val)) {
        result = PyLong_FromLong(val);
    }
    Py_END_ALLOW_THREADS

    return result;
}

static PyObject *
py_spidevice_sharingmode(PySpiDeviceObject *self, PyObject *args) {
    int val = 0;
    PyObject* result = NULL;
    VALIDATE_SPIDEVICE(self);

    Py_BEGIN_ALLOW_THREADS
    if (SUCCESS == get_sharingmode_spidevice(self->ob_device, &val)) {
        result = PyLong_FromLong(val);
    }
    Py_END_ALLOW_THREADS

    return result;
}

static PyObject *
py_spibusinfo_chipselectlinecount(PySpiBusInfoObject *self, PyObject *args) {
    int val = 0;
    PyObject* result = NULL;
    VALIDATE_SPIBUSINFO(self);

    Py_BEGIN_ALLOW_THREADS;
    if (SUCCESS == get_chipselectlinecount_spibusinfo(self->ob_businfo, &val)) {
        result = PyLong_FromLong(val);
    }
    Py_END_ALLOW_THREADS;

    return result;
}

static PyObject *
py_spibusinfo_maxclockfrequency(PySpiBusInfoObject *self, PyObject *args) {
    int val = 0;
    PyObject* result = NULL;
    VALIDATE_SPIBUSINFO(self);

    Py_BEGIN_ALLOW_THREADS;
    if (SUCCESS == get_maxclockfrequency_spibusinfo(self->ob_businfo, &val)) {
        result = PyLong_FromLong(val);
    }
    Py_END_ALLOW_THREADS;

    return result;
}

static PyObject *
py_spibusinfo_minclockfrequency(PySpiBusInfoObject *self, PyObject *args) {
    int val = 0;
    PyObject* result = NULL;
    VALIDATE_SPIBUSINFO(self);

    Py_BEGIN_ALLOW_THREADS;
    if (SUCCESS == get_minclockfrequency_spibusinfo(self->ob_businfo, &val)) {
        result = PyLong_FromLong(val);
    }
    Py_END_ALLOW_THREADS;

    return result;
}

static PyObject *
py_spibusinfo_supporteddatalengthbits(PySpiBusInfoObject *self, PyObject *args) {
    int count = 0;
    PyObject* result = NULL;
    VALIDATE_SPIBUSINFO(self);

    Py_BEGIN_ALLOW_THREADS;
    if (SUCCESS == get_supporteddatalengthbits_spibusinfo(self->ob_businfo, 0, NULL, &count)) {
        int* entries = (int*)malloc(sizeof(int) * count);
        if (entries != NULL) {
            if (SUCCESS == get_supporteddatalengthbits_spibusinfo(self->ob_businfo, count, entries, &count)) {
                result = PyList_New(count);

                for (int i = 0; i < count; i++) {
                    PyList_SetItem(result, i, PyLong_FromLong(i));
                }
            }
        }
    }
    Py_END_ALLOW_THREADS;

    return result;
}

static PyMethodDef spidevice_methods[] = {
	{ "read", (PyCFunction)py_spidevice_read, METH_VARARGS | METH_KEYWORDS, "Reads data from SPI device" },
	{ "write", (PyCFunction)py_spidevice_write, METH_VARARGS | METH_KEYWORDS, "Writes data to SPI device" },
    { "transfer", (PyCFunction)py_spidevice_transfer, METH_VARARGS | METH_KEYWORDS, "Transfers full duplex to SPI device" },
    { "transfersequential", (PyCFunction)py_spidevice_transfersequential, METH_VARARGS | METH_KEYWORDS, "Transfers full duplex to SPI device" },
    { "deviceid", (PyCFunction)py_spidevice_deviceid, METH_NOARGS, "Gets device id for SPI device" },
    { "chipselectline", (PyCFunction)py_spidevice_chipselectline, METH_NOARGS, "Gets the device id" },
    { "clockfrequency", (PyCFunction)py_spidevice_clockfrequency, METH_NOARGS, "Gets the chip select line" },
    { "databitlength", (PyCFunction)py_spidevice_databitlength, METH_NOARGS, "Gets the clock frequency" },
    { "mode", (PyCFunction)py_spidevice_mode, METH_NOARGS, "Gets the device mode" },
    { "sharingmode", (PyCFunction)py_spidevice_sharingmode, METH_NOARGS, "Gets the device sharing mode" },
    { NULL, NULL, 0, NULL }
};

static PyMethodDef spibusinfo_methods[] = {
    { "chipselectlinecount", (PyCFunction)py_spibusinfo_chipselectlinecount, METH_VARARGS | METH_KEYWORDS, "Gets the chip select line count" },
    { "maxclockfrequency", (PyCFunction)py_spibusinfo_maxclockfrequency, METH_VARARGS | METH_KEYWORDS, "Gets the max clock frequency" },
    { "minclockfrequency", (PyCFunction)py_spibusinfo_minclockfrequency, METH_VARARGS | METH_KEYWORDS, "Gets the min clock frequency" },
    { "supporteddatalengthbits", (PyCFunction)py_spibusinfo_supporteddatalengthbits, METH_VARARGS | METH_KEYWORDS, "Gets the supported data length bits" },
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

static PyTypeObject spibusinfo_type = {
    PyVarObject_HEAD_INIT(0, 0)
    "winspi.spibusinfo",
    sizeof(PySpiBusInfoObject),
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

static int
spibusinfo_init(PySpiDeviceObject *self, PyObject *args, PyObject *kwds)
{
    static char *keywords[] = { "name", NULL };
    PyObject *name = NULL, *tmp = NULL;
    wchar_t* nameString = NULL;

    if (!PyArg_ParseTupleAndKeywords(args,
        kwds,
        "O",
        keywords,
        &name))
        return -1;

    nameString = PyUnicode_AsWideCharString(name, NULL);

    Py_BEGIN_ALLOW_THREADS
    self->ob_device = new_spibusinfo(nameString);
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

    // Initialize the device type
    spibusinfo_type.tp_dealloc = (destructor)spibusinfo_dealloc;
    spibusinfo_type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    spibusinfo_type.tp_doc = spibusinfo_doc;
    spibusinfo_type.tp_methods = spibusinfo_methods;
    spibusinfo_type.tp_init = (initproc)spibusinfo_init;
    spibusinfo_type.tp_alloc = PyType_GenericAlloc;
    spibusinfo_type.tp_new = spibusinfo_new;
    spibusinfo_type.tp_free = PyObject_Del;
    spibusinfo_type.tp_getattro = PyObject_GenericGetAttr;

    Py_TYPE(&spibusinfo_type) = &PyType_Type;

    if (PyType_Ready(&spibusinfo_type) < 0)
        return NULL;

    if ((module = PyModule_Create(&winspimodule)) == NULL)
		return NULL;

    Py_INCREF(&spidevice_type);
    PyModule_AddObject(module, "spidevice", (PyObject*)&spidevice_type);

    Py_INCREF(&spibusinfo_type);
    PyModule_AddObject(module, "spibusinfo", (PyObject*)&spibusinfo_type);

    define_constants(module);

	return module;
}
