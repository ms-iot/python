#include "pch.h"
#include "event.h"

using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::ApplicationModel::AppService;

namespace PyAppServiceResponse
{
#pragma region getset

    static PyObject * getStatus(PyObject *obj, void *closure)
    {
        PyObject* const FailureValue = NULL;
        return ExceptionGuard([=]
        {
            PyWinRTObject *self = reinterpret_cast<PyWinRTObject*>(obj);
            auto response = Platform_cast<AppServiceResponse>(self->pInspectable);
            auto appServiceConnectionStatus_type = GetTypeFromModule("WinRT.ApplicationModel.AppService", "AppServiceConnectionStatus");
            auto status = PyObjectPtr::CreateAttach(PyObject_CallFunction((PyObject*)appServiceConnectionStatus_type, "i", static_cast<int>(response->Status)));
            return status.Detach();
        }, FailureValue);
    }


    static PyObject * getMessage(PyObject *obj, void *closure)
    {
        PyObject* const FailureValue = NULL;
        return ExceptionGuard([=]
        {
            PyWinRTObject *self = reinterpret_cast<PyWinRTObject*>(obj);
            auto response = Platform_cast<AppServiceResponse>(self->pInspectable);
            return PyDict_FromPlatformValueSet(response->Message).Detach();
        }, FailureValue);
    }

    static PyGetSetDef getset[] = {
        { "Status",
        (getter)getStatus, NULL,
        "Gets the status for the response from the app service.",
        NULL },
        { "Message",
        getMessage, NULL,
        "Gets the message that the response from the app service contains.",
        NULL },
        { NULL }  /* Sentinel */
    };

#pragma endregion getset

#pragma region methods

    static PyMethodDef methods[] = {
        { "_CreateFromIInspectable", WinRTHelper<AppServiceResponse>::CreateFromInspectable, METH_CLASS | METH_VARARGS, NULL },
        { NULL, NULL },
    };

#pragma endregion methods

    static const char type_name[] = "winrt.ApplicationModel.AppService.AppServiceResponse";
    static const char docstring[] = "Represents a connection to the endpoint for an app service.";

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

PyTypeObject* GetAppServiceResponse_Type()
{
    static PyTypeObject* type;
    if (!type)
    {
        type = PyAppServiceResponse::ReadyType();
    }

    return type;
}
