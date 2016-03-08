#pragma once
#include "winrt_common.h"
#include "Python.h"
#include "PyObjectPtr.h"

template <typename FirstType, typename SecondType>
ref class PythonDelegate
{
public:
    Windows::Foundation::TypedEventHandler<FirstType^, SecondType^>^ GetHandler()
    {
        return ref new Windows::Foundation::TypedEventHandler<FirstType^, SecondType^>(this, &PythonDelegate::EventHandler);
    }

    void EventHandler(FirstType^ first, SecondType^ second)
    {
        PythonEnsureGIL context;
        {
            PyTypeObject* secondType = reinterpret_cast<PyTypeObject*>(argType.Get());

            auto secondObject = PyObjectPtr::CreateFromPlatformObject(secondType, static_cast<Platform::Object^>(second));
            if (!secondObject)
            {
                PyErr_Clear();
                return;
            }

            // PyWeakref_GetObect() returns a borrowed reference
            PyObjectPtr eventSource = PyWeakref_GetObject(eventSourceWeak.Get());
            if (!eventSource || eventSource.Get() == Py_None)
            {
                // event source is gone.  Just return
                return;
            }

            auto callbacks = PyObjectPtr::CreateAttach(PyObject_GetAttrString(eventSource.Get(), eventName));
            if (!callbacks || !PyCallable_Check(callbacks.Get()))
            {
                return;
            }

            PyObject_CallFunction(callbacks.Get(), "OO", eventSource.Get(), secondObject.Get());
            if (PyErr_Occurred())
            {
                Py_FatalError("Exception throw out of event handler");
            }
        }
    }

internal:
    PythonDelegate(PyObjectPtr eventSource, PyObjectPtr argType, const char* eventName)
        : eventSourceWeak(), argType(argType), eventName(eventName)
    {
        if (!eventSource || !argType || !eventName)
        {
            auto msg = PyObjectPtr::CreateAttach(PyUnicode_FromWideChar(L"cannot be NULL", -1));
            throw PythonException(PyExc_ValueError, msg);
        }

        if (!PyType_Check(argType.Get()))
        {
            auto msg = PyObjectPtr::CreateAttach(PyUnicode_FromWideChar(L"argType must be Type", -1));
            throw PythonException(PyExc_TypeError, msg);
        }

        eventSourceWeak = PyObjectPtr::CreateAttach(PyWeakref_NewRef(eventSource.Get(), NULL));
        if (!eventSourceWeak)
        {
            auto msg = PyObjectPtr::CreateAttach(PyUnicode_FromWideChar(L"cannot create weak pointer", -1));
            throw PythonException(PyExc_TypeError, msg);
        }
    }

private:
    PyObjectPtr eventSourceWeak;
    PyObjectPtr argType;
    const char* eventName;
};


template <typename FirstType, typename SecondType>
PyObjectPtr CreatePythonEvent(PyObjectPtr eventSource, PyObjectPtr argType, const char* eventName, std::function<void (Windows::Foundation::TypedEventHandler<FirstType^, SecondType^>^)> addEvent)
{
    auto winrt_object_type = GetTypeFromModule("WinRT", "Object");
    auto winRTEventType = GetTypeFromModule("WinRT", "Event");
    if (!winrt_object_type || !winRTEventType)
    {
        Py_FatalError("Cannot get WinRT base types");
    }

    auto pythonDelegate = ref new PythonDelegate<FirstType, SecondType>(eventSource, argType, eventName);
    auto pyDelegate = PyObjectPtr::CreateFromPlatformObject(winrt_object_type, pythonDelegate);
    addEvent(pythonDelegate->GetHandler());

    return PyObjectPtr::CreateAttach(PyObject_CallFunction((PyObject*)winRTEventType, "(O)", pyDelegate.Get()));
}

template <const char* Name>
PyObject* getEvent(PyObject* obj, void * closure)
{
    PyObject* const FailureValue = NULL;
    return ExceptionGuard([=]
    {
        PyWinRTObject *self = reinterpret_cast<PyWinRTObject*>(obj);

        // PyDict_GetItemString() returns a borrowed reference
        PyObjectPtr retval = PyDict_GetItemString(self->dict, Name);
        return retval.Detach();
    }, FailureValue);
}