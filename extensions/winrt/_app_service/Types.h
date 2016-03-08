#pragma once
#include "Python.h"

PyTypeObject* GetAppServiceConnection_Type();
PyTypeObject* GetAppServiceResponse_Type();
PyTypeObject* GetAppServiceResponseStatus_Type();
PyTypeObject* GetAppServiceClosedEventArgs_Type();
PyTypeObject* GetAppServiceRequestReceivedEventArgs_Type();
PyTypeObject* GetAppServiceRequest_Type();
PyTypeObject* GetAppServiceDeferral_Type();