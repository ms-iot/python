#include "pch.h"
#include "enum.h"
#include "winrt_object.h"
#include "Types.h"

extern PyTypeObject PyAppServiceConnection_Type;

static const char moduleName[] = "Windows.ApplicationModel.AppService";
static const char moduledocstring[] = "Windows Run-Time module containing classes for using app services.";

static struct PyModuleDef app_service_module = {
    PyModuleDef_HEAD_INIT,
    moduleName,          // name of module
    moduledocstring,  // module documentation, may be NULL
    -1,               // size of per-interpreter state of the module, or -1 if the module keeps state in global variables.
};

static bool DefineModuleEnums(PyObject * module)
{
    using namespace Windows::ApplicationModel::AppService;

    EnumMemberDescriptor appServiceClosedStatusMembers[] =
    {
        { "Completed",                  static_cast<int>(AppServiceClosedStatus::Completed) },
        { "Canceled",                   static_cast<int>(AppServiceClosedStatus::Canceled) },
        { "ResourceLimitsExceeded",     static_cast<int>(AppServiceClosedStatus::ResourceLimitsExceeded) },
        { "Unknown",                    static_cast<int>(AppServiceClosedStatus::Unknown) },
    };
    auto pyAppServiceClosedStatus = CreateEnum("AppServiceClosedStatus", appServiceClosedStatusMembers, moduleName);

    EnumMemberDescriptor appServiceConnectionStatusMembers[] =
    {
        { "Success",                    static_cast<int>(AppServiceConnectionStatus::Success) },
        { "AppNotInstalled",            static_cast<int>(AppServiceConnectionStatus::AppNotInstalled) },
        { "AppUnavailable",             static_cast<int>(AppServiceConnectionStatus::AppUnavailable) },
        { "AppServiceUnavailable",      static_cast<int>(AppServiceConnectionStatus::AppServiceUnavailable) },
        { "Unknown",                    static_cast<int>(AppServiceConnectionStatus::Unknown) },
    };
    auto pyAppServiceConnectionStatus = CreateEnum("AppServiceConnectionStatus", appServiceClosedStatusMembers, moduleName);

    EnumMemberDescriptor appServiceResponseStatusMembers[] =
    {
        { "Success",                    static_cast<int>(AppServiceResponseStatus::Success) },
        { "Failure",                    static_cast<int>(AppServiceResponseStatus::Failure) },
        { "ResourceLimitsExceeded",     static_cast<int>(AppServiceResponseStatus::ResourceLimitsExceeded) },
        { "Unknown",                    static_cast<int>(AppServiceResponseStatus::Unknown) },
    };
    auto pyAppServiceResponseStatus = CreateEnum("AppServiceResponseStatus", appServiceResponseStatusMembers, moduleName);

    if (!pyAppServiceClosedStatus || !pyAppServiceConnectionStatus || !pyAppServiceResponseStatus)
        return false;

    // PyModule_AddObject steals the reference
    PyModule_AddObject(module, "AppServiceClosedStatus", pyAppServiceClosedStatus.Detach());
    PyModule_AddObject(module, "AppServiceConnectionStatus", pyAppServiceConnectionStatus.Detach());
    PyModule_AddObject(module, "AppServiceResponseStatus", pyAppServiceResponseStatus.Detach());
    return true;
}

PyMODINIT_FUNC
PyInit__app_service(void)
{
    PyObject *module = NULL;

    InitTypeTable initTable[] =
    {
        { "AppServiceClosedEventArgs", GetAppServiceClosedEventArgs_Type },
        { "AppServiceResponse", GetAppServiceResponse_Type },
        { "AppServiceDeferral", GetAppServiceDeferral_Type },
        { "AppServiceConnection", GetAppServiceConnection_Type },
        { NULL, NULL },
    };

    for (int i = 0; initTable[i].Name != NULL; i++)
    {
        if (!initTable[i].GetTypeProc || !initTable[i].GetTypeProc())
        {
            return NULL;
        }
    }

    if ((module = PyModule_Create(&app_service_module)) == NULL)
        return NULL;

    for (int i = 0; initTable[i].Name != NULL; i++)
    {
        auto type = reinterpret_cast<PyObject*>(initTable[i].GetTypeProc());
        Py_INCREF(type);
        PyModule_AddObject(module, initTable[i].Name, type);
    }

    if (!DefineModuleEnums(module))
    {
        return NULL;
    }

    return module;
}
