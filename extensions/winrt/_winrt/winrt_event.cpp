#include "pch.h"
#include "event.h"
#include "Types.h"

using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::ApplicationModel::AppService;

namespace PyEvent
{
    typedef struct {
        PyObject_HEAD
        PyObject* callbacks;
        PyObject* pythonWrapper;
    } PyWinRTEvent;

#pragma endregion methods

    static int init(PyObject *obj, PyObject *args, PyObject *kwds)
    {
        const int FailureValue = -1;
        return ExceptionGuard([=]
        {
            PyWinRTEvent *self = reinterpret_cast<PyWinRTEvent*>(obj);
            PyObject* wrapper;
            if (!PyArg_ParseTuple(args, "O", &wrapper))
            {
                return FailureValue;
            }

            self->pythonWrapper = wrapper;
            wrapper = NULL;
            Py_IncRef(self->pythonWrapper);

            self->callbacks = PyList_New(NULL);
            if (!self->callbacks)
            {
                return FailureValue;
            }

            return 0;
        }, FailureValue);
    }

    static void dealloc(PyObject *obj)
    {
        PyWinRTEvent *self = (PyWinRTEvent*)obj;

        if (self->callbacks != NULL)
        {
            Py_DecRef(self->callbacks);
            self->callbacks = NULL;
        }

        if (self->pythonWrapper != NULL)
        {
            Py_DecRef(self->pythonWrapper);
            self->pythonWrapper = NULL;
        }

        Py_TYPE(self)->tp_free(self);
    }

    static PyObject* call(PyObject *obj, PyObject *args, PyObject *kw)
    {
        PyObject* const FailureValue = NULL;
        return ExceptionGuard([=]
        {
            PyWinRTEvent *self = reinterpret_cast<PyWinRTEvent*>(obj);

            // Create a local copy of the callbacks
            auto callbackCopyArg = PyObjectPtr::CreateAttach(PyTuple_Pack(1, self->callbacks));
            auto localCallbacks = PyObjectPtr::CreateAttach(PyObject_Call((PyObject*)&PyList_Type, callbackCopyArg.Get(), NULL));
            if (!localCallbacks)
            {
                return FailureValue;
            }

            // Iterate through the local callbacks and call them one by one
            Py_ssize_t length = PyList_Size(localCallbacks.Get());
            for (Py_ssize_t i = 0; i < length; i++)
            {
                auto callback = PyList_GetItem(localCallbacks.Get(), i); // borrow reference
                auto callResult = PyObject_Call(callback, args, kw);
                if (!callResult)
                {
                    return FailureValue;
                }
            }

            Py_RETURN_NONE;
        }, FailureValue);
    }

    static PyObject * Add(PyObject *obj, PyObject * callback)
    {
        PyObject* const FailureValue = NULL;
        return ExceptionGuard([=]
        {
            PyWinRTEvent *self = reinterpret_cast<PyWinRTEvent*>(obj);

            if (!PyCallable_Check(callback))
            {
                throw PythonException(PyExc_TypeError, L"the argument must be callable");
            }

            // Check to see if the callback is registered
            auto index = PyObjectPtr::CreateAttach(PyObject_CallMethod(self->callbacks, "index", "(O)", callback));
            if (index)
            {
                throw PythonException(PyExc_ValueError, L"the argument is already registered");
            }
            // Clear the error set by list.index()
            PyErr_Clear();

            if (PyList_Append(self->callbacks, callback) != 0)
            {
                return FailureValue;
            }

            Py_IncRef((PyObject*)self);
            return (PyObject*)self;
        }, FailureValue);
    }

    static PyObject * Remove(PyObject *obj, PyObject * callback)
    {
        PyObject* const FailureValue = NULL;
        return ExceptionGuard([=]
        {
            PyWinRTEvent *self = reinterpret_cast<PyWinRTEvent*>(obj);

            // Check to see if the callback is registered
            auto index = PyObjectPtr::CreateAttach(PyObject_CallMethod(self->callbacks, "index", "(O)", callback));
            if (!index)
            {
                PyErr_Clear();
                throw PythonException(PyExc_ValueError, L"the argument is already registered");
            }

            // remove the item
            auto removedItem = PyObjectPtr::CreateAttach(PyObject_CallMethod(self->callbacks, "pop", "(O)", index));

            Py_IncRef((PyObject*)self);
            return (PyObject*)self;
        }, FailureValue);
    }

    static PyMethodDef methods[] = {
        { "Add", Add, METH_O, "Register a new handler for this event.", },
        { "Remove", Remove, METH_O, "Unregister the handler for this event." },
        { NULL, NULL },
    };

    static const char type_name[] = "winrt.Event";
    static const char docstring[] = "Wrapper for WinRT events";

    PyTypeObject Type = {
        PyVarObject_HEAD_INIT(NULL, 0)
        type_name,                                                      /* tp_name */
        sizeof(PyWinRTEvent),                                           /* tp_basicsize */
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
        call,                                                           /* tp_call */
        0,                                                              /* tp_str */
        0,                                                              /* tp_getattro */
        0,                                                              /* tp_setattro */
        0,                                                              /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT,                                             /* tp_flags */
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
        init,                                                           /* tp_init */
        0,                                                              /* tp_alloc */
        PyType_GenericNew,                                              /* tp_new */
    };

    static PyTypeObject* ReadyType()
    {
        if (PyType_Ready(&Type) < 0)
        {
            return NULL;
        }

        return &Type;
    }
}

PyTypeObject* GetWinRTEvent_Type()
{
    static PyTypeObject* type;
    if (!type)
    {
        type = PyEvent::ReadyType();
    }

    return type;
}
