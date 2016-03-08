#include "pch.h"
#include "event.h"
#include "Types.h"

using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::ApplicationModel::AppService;

namespace PyAppServiceConnection
{
    char ServiceClosedEventName[] = "ServiceClosed";
    char RequestReceivedEventName[] = "RequestReceived";

    static int init(PyObject *obj, PyObject *args, PyObject *kwds)
    {
        const int FailureValue = -1;
        return ExceptionGuard([=]
        {
            PyWinRTObject *self = reinterpret_cast<PyWinRTObject*>(obj);
            if (self->pInspectable)
            {
                self->pInspectable->Release();
                self->pInspectable = NULL;
            }

            if (!self->dict)
            {
                self->dict = PyDict_New();
            }

            auto thisObj = ref new AppServiceConnection();

            auto addServiceClosedHandler = [thisObj](TypedEventHandler<AppServiceConnection^, AppServiceClosedEventArgs^>^ handler) { thisObj->ServiceClosed += handler; };
            auto serviceClosedEvent = CreatePythonEvent<AppServiceConnection, AppServiceClosedEventArgs>(obj, (PyObject*)GetAppServiceClosedEventArgs_Type(), ServiceClosedEventName, addServiceClosedHandler);
            PyDict_SetItemString(self->dict, ServiceClosedEventName, serviceClosedEvent.Get());

            auto addRequestReceivedHandler = [thisObj](TypedEventHandler<AppServiceConnection^, AppServiceRequestReceivedEventArgs^>^ handler) { thisObj->RequestReceived += handler; };
            auto requestReceivedEvent = CreatePythonEvent<AppServiceConnection, AppServiceRequestReceivedEventArgs>(obj, (PyObject*)GetAppServiceRequestReceivedEventArgs_Type(), RequestReceivedEventName, addRequestReceivedHandler);
            PyDict_SetItemString(self->dict, RequestReceivedEventName, requestReceivedEvent.Get());

            self->pInspectable = reinterpret_cast<IInspectable*>(thisObj);
            self->pInspectable->AddRef();
            return 0;
        }, FailureValue);
    }

#pragma region getset

    static int setAppServiceName(PyObject *obj, PyObject *value, void *closure)
    {
        const int FailureValue = -1;
        return ExceptionGuard([=]
        {
            PyWinRTObject *self = reinterpret_cast<PyWinRTObject*>(obj);
            auto appServiceConnection = Platform_cast<AppServiceConnection>(self->pInspectable);
            auto str = PlatformString_FromPyObject(value);
            appServiceConnection->AppServiceName = str;
            return 0;

        }, FailureValue);
    }

    static PyObject * getAppServiceName(PyObject *obj, void *closure)
    {
        PyObject* const FailureValue = NULL;
        return ExceptionGuard([=]
        {
            PyWinRTObject *self = reinterpret_cast<PyWinRTObject*>(obj);
            auto appServiceConnection = Platform_cast<AppServiceConnection>(self->pInspectable);
            auto name = appServiceConnection->AppServiceName;
            return PyUnicode_FromPlatformString(name).Detach();
        }, FailureValue);
    }

    static int setPackageFamilyName(PyObject *obj, PyObject *value, void *closure)
    {
        const int FailureValue = -1;
        return ExceptionGuard([=]
        {
            PyWinRTObject *self = reinterpret_cast<PyWinRTObject*>(obj);
            auto appServiceConnection = Platform_cast<AppServiceConnection>(self->pInspectable);
            auto str = PlatformString_FromPyObject(value);
            appServiceConnection->PackageFamilyName = str;
            return 0;

        }, FailureValue);
    }

    static PyObject * getPackageFamilyName(PyObject *obj, void *closure)
    {
        PyObject* const FailureValue = NULL;
        return ExceptionGuard([=]
        {
            PyWinRTObject *self = reinterpret_cast<PyWinRTObject*>(obj);
            auto appServiceConnection = Platform_cast<AppServiceConnection>(self->pInspectable);
            auto name = appServiceConnection->PackageFamilyName;
            return PyUnicode_FromPlatformString(name).Detach();
        }, FailureValue);
    }

    static PyGetSetDef getset[] = {
        { "AppServiceName", getAppServiceName, setAppServiceName, "AppServiceName -> string. \nGets or sets the name of the app service to which you want to connect.", NULL},
        { "PackageFamilyName", getPackageFamilyName, setPackageFamilyName, "PackageFamilyName -> string. \nGets or sets the package family name for the package that contains the endpoint for the app service." , NULL},
        { ServiceClosedEventName, getEvent<ServiceClosedEventName>, NULL, "ServiceClosed -> WinRT.Event. \nOccurs when the other endpoint closes the connection to the app service. \nThe handler must take 2 positional parameters: AppServiceConnection and AppServiceClosedEventArgs", NULL},
        { RequestReceivedEventName, getEvent<RequestReceivedEventName>, NULL, "RequestReceived -> WinRT.Event. \nOccurs when a message arrives from the other endpoint of the app service connection. \nThe handler must take 2 positional parameters: AppServiceConnection and AppServiceRequestReceivedEventArgs", NULL},
        { NULL }  /* Sentinel */
    };

#pragma endregion getset

#pragma region utils
    
#pragma endregion utils

#pragma region methods
    static PyObject* Open(PyObject* obj)
    {
        PyObject* const FailureValue = NULL;
        PyWinRTObject *self = reinterpret_cast<PyWinRTObject*>(obj);
        return ExceptionGuard([=]
        {
            auto appServiceConnection = Platform_cast<AppServiceConnection>(self->pInspectable);
            AppServiceConnectionStatus openStatus;
            {
                AllowThreadsContext context;
                openStatus = concurrency::create_task(appServiceConnection->OpenAsync()).get();
            }
            auto appServiceConnectionStatus_type = GetTypeFromModule("WinRT.ApplicationModel.AppService", "AppServiceConnectionStatus");
            return PyObject_CallFunction((PyObject*)appServiceConnectionStatus_type, "i", static_cast<int>(openStatus));
        }, FailureValue);
    }

    static PyObject* SendMessage(PyObject *obj, PyObject *args)
    {
        PyObject* const FailureValue = NULL;
        return ExceptionGuard([=]
        {
            PyWinRTObject *self = reinterpret_cast<PyWinRTObject*>(obj);

            PyObject* dict;
            if (!PyArg_ParseTuple(args, "O", &dict))
                return FailureValue;

            auto valueSet = PyDict_ToValueSet(dict);
            assert(!PyErr_Occurred());
            auto appServiceConnection = Platform_cast<AppServiceConnection>(self->pInspectable);
            AppServiceResponse^ response;
            {
                AllowThreadsContext context;
                response = concurrency::create_task(appServiceConnection->SendMessageAsync(valueSet)).get();
            }
            return PyObjectPtr::CreateFromPlatformObject(GetAppServiceResponse_Type(), response).Detach();
        }, FailureValue);
    }


    static PyMethodDef methods[] = {
        { "Open", (PyCFunction)Open, METH_NOARGS, PyDoc_STR("Open() ->  AppServiceConnectionStatus.  Opens a connection to the endpoint for the app service.") },
        { "SendMessage", (PyCFunction)SendMessage, METH_VARARGS, PyDoc_STR("SendMessage(dict of { str : object } ) -> AppServiceResponse.  Sends a message to the other endpoint of the app service connection.") },
        { NULL, NULL },
    };

#pragma endregion methods

    static const char type_name[] = "winrt.ApplicationModel.AppService.AppServiceConnection";
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
        init,                                                           /* tp_init */
        0,                                                              /* tp_alloc */
        (newfunc)WinRTHelper<AppServiceConnection>::tp_new,             /* tp_new */
    };

    static PyTypeObject* ReadyType()
    {
        Py_TYPE(&Type) = &PyType_Type;
        auto winrt_object_type = GetTypeFromModule("WinRT", "Object");
        auto closable_type = GetTypeFromModule("WinRT", "Closable");

        Type.tp_base = winrt_object_type;
        PyAppServiceConnection::Type.tp_bases = PyTuple_Pack(2, closable_type, winrt_object_type);

        if (PyType_Ready(&Type) < 0)
        {
            return NULL;
        }

        return &Type;
    }
}

PyTypeObject* GetAppServiceConnection_Type()
{
    static PyTypeObject* type;
    if (!type)
    {
        type = PyAppServiceConnection::ReadyType();
    }

    return type;
}
