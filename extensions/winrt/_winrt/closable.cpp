#include "pch.h"
#include "winrt_common.h"
#include "Types.h"

#pragma warning(disable: 4483) // Allows us to use __identifier

using namespace Microsoft::WRL;

namespace PyClosable
{
    PyDoc_STRVAR(close_doc, "Releases system resources that are exposed by a Windows Runtime object.");
    static PyObject * close(PyObject *obj)
    {
        PyObject* const FailureValue = NULL;
        return ExceptionGuard([=]
        {
            PyWinRTObject *self = reinterpret_cast<PyWinRTObject*>(obj);
            auto thisType = Platform_cast<Platform::IDisposable>(self->pInspectable);
            thisType->__identifier("<Dispose>")();
            Py_RETURN_NONE;
        }, FailureValue);
    }

    PyDoc_STRVAR(enter_doc, "");
    static PyObject * enter(PyObject *self, PyObject *args, PyObject *kwds)
    {
        Py_INCREF(self);
        return self;
    }

    PyDoc_STRVAR(exit_doc, "Releases system resources that are exposed by a Windows Runtime object.");
    static PyObject * exit(PyObject *self, PyObject *args)
    {
        return PyObject_CallMethod(self, "Close", NULL);
    }

    static PyMethodDef methods[] = {
        { "__enter__",    (PyCFunction)enter, METH_NOARGS, enter_doc },
        { "__exit__",    (PyCFunction)exit, METH_VARARGS, exit_doc },
        { "Close",    (PyCFunction)close, METH_NOARGS, close_doc },
        { NULL,           NULL }              /* sentinel */
    };

    static const char type_name[] = "WinRT.Closable";
    static const char docstring[] = "Defines a method to release allocated resources.";

    PyTypeObject Type = {
        PyVarObject_HEAD_INIT(NULL, 0)
        type_name,                                                      /* tp_name */
        sizeof(PyWinRTObject),                                          /* tp_basicsize */
        0,                                                              /* tp_itemsize */
        0,                                                              /* tp_dealloc */
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
        0,                                                              /* tp_str */
        0,                                                              /* tp_getattro */
        0,                                                              /* tp_setattro */
        0,                                                              /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                       /* tp_flags */
        docstring,                                                      /* tp_doc */
        0,                                                              /* tp_traverse */
        0,                                                              /* tp_clear */
        0,                                                              /* tp_richcompare */
        0,                                                              /* tp_weaklistoffset */
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
        if (PyType_Ready(&PyClosable::Type) < 0)
        {
            return NULL;
        }

        return &Type;
    }
}

PyTypeObject* GetClosable_Type()
{
    static PyTypeObject* type;
    if (!type)
    {
        type = PyClosable::ReadyType();
    }

    return type;
}
