#pragma once
#include "pch.h"
#include "winrt_common.h"
#include "Winstring.h"

using namespace Microsoft::WRL;
using namespace Platform;

namespace PyWinRTObjectImpl
{
    static void dealloc(PyObject *obj)
    {
        PyWinRTObject *self = (PyWinRTObject*)obj;
        if (self->weaklist != NULL)
            PyObject_ClearWeakRefs((PyObject*)self);

        if (self->dict != NULL)
        {
            Py_DecRef(self->dict);
            self->dict = NULL;
        }

        if (self->pInspectable)
        {
            self->pInspectable->Release();
            self->pInspectable = NULL;
        }

        Py_TYPE(self)->tp_free(self);
    }

    static PyObject * toString(PyObject *obj)
    {
        PyObject* const FailureValue = NULL;
        return ExceptionGuard([=]() 
        {
            auto self = reinterpret_cast<PyWinRTObject*>(obj);
            auto platformObj = reinterpret_cast<Platform::Object^>(self->pInspectable);
            auto str = platformObj->ToString();
            return PyUnicode_FromPlatformString(str).Detach();
        }, FailureValue);
    }

    PyDoc_STRVAR(GetRuntimeClassName_doc, "Gets the fully qualified name of the current Windows Runtime object.");
    static PyObject * GetRuntimeClassName(PyObject *obj, PyObject *unused)
    {
        auto self = reinterpret_cast<PyWinRTObject*>(obj);
        if (!self->pInspectable)
        {
            PyErr_SetString(PyExc_RuntimeError, "null object");
            return NULL;
        }

        Microsoft::WRL::Wrappers::HString hstr;
        HRESULT hr = self->pInspectable->GetRuntimeClassName(hstr.GetAddressOf());
        if (FAILED(hr))
        {
            switch (hr)
            {
            case E_OUTOFMEMORY:
                PyErr_SetString(PyExc_MemoryError, "cannot allocate memory");

            case E_ILLEGAL_METHOD_CALL:
                PyErr_SetString(PyExc_RuntimeError, "class factories or static interfaces do not have class names");
            default:
                PyErr_Format(PyExc_RuntimeError, "GetRuntimeClassName() failed.  HRESULT:%d", hr);
            }
            return NULL;
        }
        else
        {
            UINT32 length = 0;
            auto wstr = hstr.GetRawBuffer(&length);
            return PyUnicode_FromUnicode(wstr, length);
        }
    }

    static PyMethodDef methods[] = {
        { "_CreateFromIInspectable", WinRTHelper<Platform::Object>::CreateFromInspectable, METH_CLASS | METH_VARARGS, NULL },
        { "_GetRuntimeClassName", (PyCFunction)GetRuntimeClassName, METH_NOARGS, GetRuntimeClassName_doc },
        { NULL, NULL },
    };

    static const char type_name[] = "WinRT.Object";
    static const char docstring[] = "WinRT.Object - Provides functionality required for all Windows Runtime classes.";

    PyTypeObject Type = {
        PyVarObject_HEAD_INIT(NULL, 0)
        type_name,                                                      /* tp_name */
        sizeof(PyWinRTObject),                                          /* tp_basicsize */
        0,                                                              /* tp_itemsize */
        dealloc,                                                        /* tp_dealloc */
        0,                                                              /* tp_print */
        0,                                                              /* tp_getattr */
        0,                                                              /* tp_setattr */
        0,                                                              /* tp_reserved */
        0,                                                              /* tp_repr */
        0,                                                              /* tp_as_number */
        0,                                                              /* tp_as_sequence */
        0,                                                              /* tp_as_mapping */
        0,                                                              /* tp_hash  */
        0,                                                              /* tp_call */
        toString,                                                       /* tp_str */
        0,                                                              /* tp_getattro */
        0,                                                              /* tp_setattro */
        0,                                                              /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                       /* tp_flags */
        docstring,                                                      /* tp_doc */
        0,                                                              /* tp_traverse */
        0,                                                              /* tp_clear */
        0,                                                              /* tp_richcompare */
        offsetof(PyWinRTObject, weaklist),                              /* tp_weaklistoffset */
        0,                                                              /* tp_iter */
        0,                                                              /* tp_iternext */
        methods,                                                        /* tp_methods */
        0,                                                              /* tp_members */
        0,                                                              /* tp_getset */
        0,                                                              /* tp_base */
        0,                                                              /* tp_dict */
        0,                                                              /* tp_descr_get */
        0,                                                              /* tp_descr_set */
        0,                                                              /* tp_dictoffset */
        0,                                                              /* tp_init */
        0,                                                              /* tp_alloc */
        0,                                                              /* tp_new */
    };

    static PyTypeObject* ReadyType()
    {
        Py_TYPE(&Type) = &PyType_Type;

        if (PyType_Ready(&Type) < 0)
        {
            return NULL;
        }

        return &Type;
    }
}


PyTypeObject* GetWinRTObject_Type()
{
    static PyTypeObject* type;
    if (!type)
    {
        type = PyWinRTObjectImpl::ReadyType();
    }

    return type;
}
