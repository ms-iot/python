#include "pch.h"
#include "winrt_common.h"
#include "Types.h"

static const char moduledocstring[] = "Windows Run-Time module";

static struct PyModuleDef winrt_module = {
    PyModuleDef_HEAD_INIT,
    "WinRT",          // name of module
    moduledocstring,  // module documentation, may be NULL
    -1,               // size of per-interpreter state of the module, or -1 if the module keeps state in global variables.
};

PyMODINIT_FUNC
PyInit__winrt(void)
{
    PyEval_InitThreads();

    InitTypeTable initTable[] = 
    {
        { "Object", GetWinRTObject_Type},
        { "Closable", GetClosable_Type },
        { "Event", GetWinRTEvent_Type },
        { NULL, NULL },
    };

    for (int i = 0; initTable[i].Name != NULL; i++)
    {
        if (!initTable[i].GetTypeProc || !initTable[i].GetTypeProc())
        {
            return NULL;
        }
    }

    PyObject *module = PyModule_Create(&winrt_module);
    if (!module)
        return NULL;

    for (int i = 0; initTable[i].Name != NULL; i++)
    {
        auto type = reinterpret_cast<PyObject*>(initTable[i].GetTypeProc());
        Py_INCREF(type);
        PyModule_AddObject(module, initTable[i].Name, type);
    }

    return module;
}
