#include "pch.h"
#include "event.h"

using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::ApplicationModel::AppService;

namespace PyAppServiceClosedEventArgs
{
    typedef AppServiceClosedEventArgs thisType;
#pragma region getset

    static PyObject * getStatus(PyObject *obj, void *closure)
    {
        PyObject* const FailureValue = NULL;
        return ExceptionGuard([=]
        {
            PyWinRTObject *self = reinterpret_cast<PyWinRTObject*>(obj);
            auto args = Platform_cast<thisType>(self->pInspectable);
            auto appServiceClosedStatus_type = GetTypeFromModule("WinRT.ApplicationModel.AppService", "AppServiceClosedStatus");
            return PyObject_CallFunction((PyObject*)appServiceClosedStatus_type, "i", static_cast<int>(args->Status));
        }, FailureValue);
    }

    static PyGetSetDef getset[] = {
        { "Status", (getter)getStatus, NULL, "Status -> AppServiceClosedStatus. \nGets the status that was set when the endpoint for the app service was closed.", NULL },
        { NULL }  /* Sentinel */
    };

#pragma endregion getset

#pragma region methods

    static PyMethodDef methods[] = {
        { "_CreateFromIInspectable", WinRTHelper<thisType>::CreateFromInspectable, METH_CLASS | METH_VARARGS, NULL },
        { NULL, NULL },
    };

#pragma endregion methods

    static const char type_name[] = "WinRT.ApplicationModel.AppService.AppServiceClosedEventArgs";
    static const char docstring[] = "Provides data for the AppServiceConnection.ServiceClosed event that occurs when the other endpoint closes connection to the app service.";

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
        getset,                                                         /* tp_getset */
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
        auto winrt_object_type = GetTypeFromModule("WinRT", "Object");

        Type.tp_base = winrt_object_type;
        Type.tp_bases = PyTuple_Pack(1, winrt_object_type);

        if (PyType_Ready(&Type) < 0)
        {
            return NULL;
        }

        return &Type;
    }
}

PyTypeObject* GetAppServiceClosedEventArgs_Type()
{
    static PyTypeObject* type;
    if (!type)
    {
        type = PyAppServiceClosedEventArgs::ReadyType();
    }

    return type;
}
