#include "python.h"
#include "spiapi.h"
#include "structmember.h"

static PyObject* mode0;
static PyObject* mode1;
static PyObject* mode2;
static PyObject* mode3;

static PyObject* exclusivemode;
static PyObject* sharedmode;

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

static const char moduledocstring[] = "SPI functionality of a Windows 10 IoT Core device";

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

PyDoc_STRVAR(read_doc,
    "read(count) -> bytes\n"
    "\n"
    "Reads from the device the specified number of bytes\n"
    "\n"
    "count=Number of bytes to read\n"
    );
static PyObject *
winspi_spidevice_read(PySpiDeviceObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "count", NULL };
    int count = 1;
    PyObject* result = NULL;

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

PyDoc_STRVAR(write_doc,
    "write(bytes)\n"
    "\n"
    "Writes the bytes to the device\n"
    "\n"
    "bytes=Byte array to be written to the device\n"
    );
static PyObject *
winspi_spidevice_write(PySpiDeviceObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "data", NULL };
    PyObject* data = NULL;
    PyObject* bytes = NULL;
    int writeresult = FAILURE;

    VALIDATE_SPIDEVICE(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|", kwlist, &data))
        return NULL;

    bytes = PyBytes_FromObject(data);
    if (bytes == NULL)
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    writeresult = write_spidevice(self->ob_device, PyBytes_AsString(bytes), PyBytes_Size(bytes));
    Py_END_ALLOW_THREADS

    if (writeresult == FAILURE) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(transfer_doc,
    "transfer(bytes, count) -> bytes\n"
    "\n"
    "Writes the bytes to the device and reads the expected number of bytes in full duplex\n"
    "\n"
    "bytes=Byte array to be written to the device\n"
    "\n"
    "count=Number of bytes expected to be read\n"
    );
static PyObject *
winspi_spidevice_transfer(PySpiDeviceObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "data", "count", NULL };
    PyObject* data = NULL;
    PyObject* byteArray = NULL;
    int count = 1;
    PyObject* result = NULL;

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

PyDoc_STRVAR(transfersequential_doc,
    "transfersequential(bytes, count) -> bytes\n"
    "\n"
    "Writes the bytes to the device and reads the expected number of bytes in sequential mode\n"
    "\n"
    "bytes=Byte array to be written to the device\n"
    "\n"
    "count=Number of bytes expected to be read\n"
    );
static PyObject *
winspi_spidevice_transfersequential(PySpiDeviceObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "data", "count", NULL };
    PyObject* data = NULL;
    PyObject* byteArray = NULL;
    int count = 1;
    PyObject* result = NULL;

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

PyDoc_STRVAR(deviceid_doc,
    "deviceid() -> string\n"
    "\n"
    "Retrieves the device id for the SPI device\n"
    "\n"
    );
static PyObject *
winspi_spidevice_deviceid(PySpiDeviceObject *self, PyObject *args) {
    char id[_MAX_FNAME];
    PyObject* result = NULL;
    VALIDATE_SPIDEVICE(self);

    Py_BEGIN_ALLOW_THREADS
    if (SUCCESS == get_deviceid_spidevice(self->ob_device, id, _MAX_FNAME)) {
        result = PyUnicode_FromString(id);
    }
    Py_END_ALLOW_THREADS

    return result;
}

PyDoc_STRVAR(csl_doc,
    "chipselectline() -> int\n"
    "\n"
    "Retrieves the chip select line for the SPI device\n"
    "\n"
    );
static PyObject *
winspi_spidevice_chipselectline(PySpiDeviceObject *self, PyObject *args) {
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

PyDoc_STRVAR(clockfrequency_doc,
    "clockfrequency() -> int\n"
    "\n"
    "Retrieves the clock frequency for the SPI device\n"
    "\n"
    );
static PyObject *
winspi_spidevice_clockfrequency(PySpiDeviceObject *self, PyObject *args) {
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

PyDoc_STRVAR(databitlength_doc,
    "databitlength() -> int\n"
    "\n"
    "Retrieves the data bit length for the SPI device\n"
    "\n"
    );
static PyObject *
winspi_spidevice_databitlength(PySpiDeviceObject *self, PyObject *args) {
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

PyDoc_STRVAR(mode_doc,
    "mode() -> int (MODE0,MODE1,MODE2 or MODE3)\n"
    "\n"
    "Retrieves the mode for the SPI device\n"
    "\n"
    );
static PyObject *
winspi_spidevice_mode(PySpiDeviceObject *self, PyObject *args) {
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

PyDoc_STRVAR(sharingmode_doc,
    "sharingmode() -> int (EXCLUSIVEMODE or SHAREDMODE)\n"
    "\n"
    "Retrieves the sharing mode for the SPI device\n"
    "\n"
    );
static PyObject *
winspi_spidevice_sharingmode(PySpiDeviceObject *self, PyObject *args) {
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

PyDoc_STRVAR(chipselectlinecount_doc,
    "chipselectlinecount() -> int\n"
    "\n"
    "Retrieves the chip select line count for the SPI bus\n"
    "\n"
    );
static PyObject *
winspi_spibusinfo_chipselectlinecount(PySpiBusInfoObject *self, PyObject *args) {
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

PyDoc_STRVAR(maxclockfrequency_doc,
    "maxclockfrequency() -> int\n"
    "\n"
    "Retrieves the max clock frequency for the SPI bus\n"
    "\n"
    );
static PyObject *
winspi_spibusinfo_maxclockfrequency(PySpiBusInfoObject *self, PyObject *args) {
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

PyDoc_STRVAR(minclockfrequency_doc,
    "minclockfrequency() -> int\n"
    "\n"
    "Retrieves the min clock frequency for the SPI bus\n"
    "\n"
    );
static PyObject *
winspi_spibusinfo_minclockfrequency(PySpiBusInfoObject *self, PyObject *args) {
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

PyDoc_STRVAR(supporteddatalengthbits_doc,
    "supporteddatalengthbits() -> list(int)\n"
    "\n"
    "Retrieves the supported data length bits for the SPI bus\n"
    "\n"
    );
static PyObject *
winspi_spibusinfo_supporteddatalengthbits(PySpiBusInfoObject *self, PyObject *args) {
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
    { "read", (PyCFunction)winspi_spidevice_read, METH_VARARGS | METH_KEYWORDS, read_doc },
    { "write", (PyCFunction)winspi_spidevice_write, METH_VARARGS | METH_KEYWORDS, write_doc },
    { "transfer", (PyCFunction)winspi_spidevice_transfer, METH_VARARGS | METH_KEYWORDS, transfer_doc },
    { "transfersequential", (PyCFunction)winspi_spidevice_transfersequential, METH_VARARGS | METH_KEYWORDS, transfersequential_doc },
    { "deviceid", (PyCFunction)winspi_spidevice_deviceid, METH_NOARGS, deviceid_doc },
    { "chipselectline", (PyCFunction)winspi_spidevice_chipselectline, METH_NOARGS, csl_doc },
    { "clockfrequency", (PyCFunction)winspi_spidevice_clockfrequency, METH_NOARGS, clockfrequency_doc },
    { "databitlength", (PyCFunction)winspi_spidevice_databitlength, METH_NOARGS, databitlength_doc },
    { "mode", (PyCFunction)winspi_spidevice_mode, METH_NOARGS, mode_doc },
    { "sharingmode", (PyCFunction)winspi_spidevice_sharingmode, METH_NOARGS, sharingmode_doc },
    { NULL, NULL, 0, NULL }
};

static PyMethodDef spibusinfo_methods[] = {
    { "chipselectlinecount", (PyCFunction)winspi_spibusinfo_chipselectlinecount, METH_NOARGS, chipselectlinecount_doc },
    { "maxclockfrequency", (PyCFunction)winspi_spibusinfo_maxclockfrequency, METH_NOARGS, maxclockfrequency_doc },
    { "minclockfrequency", (PyCFunction)winspi_spibusinfo_minclockfrequency, METH_NOARGS, minclockfrequency_doc },
    { "supporteddatalengthbits", (PyCFunction)winspi_spibusinfo_supporteddatalengthbits, METH_NOARGS, supporteddatalengthbits_doc },
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef winspi_module = {
    PyModuleDef_HEAD_INIT,
    "_winspi",       // name of module
    moduledocstring,  // module documentation, may be NULL
    -1,               // size of per-interpreter state of the module, or -1 if the module keeps state in global variables.
};

static PyTypeObject spidevice_type = {
    PyVarObject_HEAD_INIT(0, 0)
    "_winspi.spidevice",
    sizeof(PySpiDeviceObject),
    0
};

static PyTypeObject spibusinfo_type = {
    PyVarObject_HEAD_INIT(0, 0)
    "_winspi.spibusinfo",
    sizeof(PySpiBusInfoObject),
    0
};

PyDoc_STRVAR(spidevice_doc,
    "spidevice(id, chipselectline, clockfrequency=<default>, databitlength=<default>, mode=MODE0, sharingmode=EXCLUSIVEMODE) -> spidevice\n"
    "\n"
    "Creates a new instance of SPI device");
static int
spidevice_init(PySpiDeviceObject *self, PyObject *args, PyObject *kwds)
{
    static char *keywords[] = { "id", "chipselectline", "clockfrequency", "databitlength", "mode", "sharingmode", NULL };
    int id = 0;
    int chipselectline = 0;
    int clockfrequency = -1;
    int databitlength = -1;
    int mode = MODE0;
    int shareMode = EXCLUSIVEMODE;

    if (!PyArg_ParseTupleAndKeywords(args,
        kwds,
        "ii|iiii:spidevice",
        keywords,
        &id,
        &chipselectline,
        &clockfrequency,
        &databitlength,
        &mode,
        &shareMode))
        return -1;

    Py_BEGIN_ALLOW_THREADS
    self->ob_device = new_spidevice(id, chipselectline, clockfrequency, databitlength, mode, shareMode);
    Py_END_ALLOW_THREADS
    if (self->ob_device == NULL)
        return -1;

    return 0;
}

PyDoc_STRVAR(spibusinfo_doc,
    "spibusinfo(friendlyname) -> spibusinfo\n"
    "\n"
    "Creates a new instance of a SPI bus info object");
static int
spibusinfo_init(PySpiDeviceObject *self, PyObject *args, PyObject *kwds)
{
    static char *keywords[] = { "name", NULL };
    PyObject *name = NULL, *tmp = NULL;
    wchar_t* nameString = NULL;

    if (!PyArg_ParseTupleAndKeywords(args,
        kwds,
        "O:spibusinfo",
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
PyInit__winspi(void)
{
    PyObject *module = NULL;
    int lightning_enabled;

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

    // Initialize the bus info type
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

    if ((module = PyModule_Create(&winspi_module)) == NULL)
        return NULL;

    lightning_enabled = enable_lightning_if_available();

    Py_INCREF(&spidevice_type);
    PyModule_AddObject(module, "spidevice", (PyObject*)&spidevice_type);
    
    if (lightning_enabled) {
        // Lightning doesn't support spibusinfo
    } else {
        Py_INCREF(&spibusinfo_type);
        PyModule_AddObject(module, "spibusinfo", (PyObject*)&spibusinfo_type);
    }

    define_constants(module);

    return module;
}
