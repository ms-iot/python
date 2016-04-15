#pragma once
#include "winrt_object.h"
#include "ExceptionGuard.h"
#include "PyObjectPtr.h"
#include "convert.h"

template <typename Type>
struct WinRTHelper
{
    static PyObject* tp_new(PyTypeObject *type)
    {
        return PyType_GenericNew(type, NULL, NULL);
    }

    static int InitFromInspectable(PyObject* obj, PyObject *args)
    {
        PyWinRTObject *self = reinterpret_cast<PyWinRTObject*>(obj);
        assert(!self->pInspectable);

        PyObject* pyLongVoidPtr;
        if (!PyArg_ParseTuple(args, "O", &pyLongVoidPtr))
            return -1;

        ComPtr<IInspectable> spInspectable = static_cast<IInspectable*>(PyLong_AsVoidPtr(pyLongVoidPtr));
        auto ptr = dynamic_cast<Type^>(reinterpret_cast<Platform::Object^>(spInspectable.Get()));
        if (!ptr)
        {
            // Wrong type!  Report error and return error
            PyErr_SetString(PyExc_RuntimeError, "Wrong type");
            return -1;
        }

        self->pInspectable = spInspectable.Detach();
        return 0;
    }

    static PyObject* CreateFromInspectable(PyObject* typeObj, PyObject* args)
    {
        PyTypeObject* type = reinterpret_cast<PyTypeObject*>(typeObj);
        PyObject* obj = WinRTHelper::tp_new(type);
        if (!obj)
            return NULL;

        if (WinRTHelper::InitFromInspectable(obj, args) == -1)
            return NULL;

        return obj;
    }
};

inline PyTypeObject* GetTypeFromModule(char* ModuleName, char* TypeName)
{
    auto module = PyObjectPtr::CreateAttach(PyImport_ImportModule(ModuleName));
    if (!module)
        return NULL;

    // 'dict' is a borrowed reference
    PyObject* dict = PyModule_GetDict(module.Get());
    if (!dict)
        return NULL;

    // 'object' is a borrowed reference
    PyObject* object = PyDict_GetItemString(dict, TypeName);
    if (!object || !PyType_Check(object))
        return NULL;

    return reinterpret_cast<PyTypeObject*>(object);
}

struct InitTypeTable
{
    const char* Name;
    PyTypeObject* (*GetTypeProc)();
};

class PythonEnsureGIL
{
public:
    PythonEnsureGIL()
    {
        state = PyGILState_Ensure();
    }

    ~PythonEnsureGIL()
    {
        PyGILState_Release(state);
    }
private:
    PyGILState_STATE state;
};
