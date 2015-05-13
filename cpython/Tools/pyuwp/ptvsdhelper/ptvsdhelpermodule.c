#include "python.h"
#include "pythonuwp.h"

PyDoc_STRVAR(ping_debugger_for_attach_doc,
    "ping_debugger_for_attach()\n"
    "\n"
    "Pings the debugger with SEH so PTVSD debugger can attach\n"
    );
// python function ping_debugger_for_attach()
static PyObject *
ptvsd_helper_ping_debugger_for_attach(PyObject *self, PyObject *args)
{
    ULONG_PTR isPresent = 0;
    ULONG_PTR arg = &isPresent;

    PingPythonDebugger(0xEDCBA988, &arg, 1);

    if (arg == 0)
        Py_RETURN_FALSE;
    else
        Py_RETURN_TRUE;
}

PyMethodDef ptvsdhelper_methods[] = {
    { "ping_debugger_for_attach", (PyCFunction)ptvsd_helper_ping_debugger_for_attach, METH_NOARGS, ping_debugger_for_attach_doc },
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef ptvsdhelper_module = {
    PyModuleDef_HEAD_INIT,
    "_ptvsdhelper",       // name of module
    "PTVSD Helper functionality of a Windows 10 IoT Core device",  // module documentation, may be NULL
    -1,               // size of per-interpreter state of the module, or -1 if the module keeps state in global variables.
    ptvsdhelper_methods
};

PyMODINIT_FUNC PyInit__ptvsdhelper(void)
{
    PyObject *module = NULL;

    if ((module = PyModule_Create(&ptvsdhelper_module)) == NULL)
        return NULL;

    return module;
}
