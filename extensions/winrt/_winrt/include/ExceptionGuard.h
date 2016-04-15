#pragma once
#include "Python.h"
#include "PyObjectPtr.h"

/* 
AllowThreadsContext - Helper object to allow the python interpreter to run on another
thread.  This releases the GIL on construction and re-acquire the GIL on destruction.
*/
class AllowThreadsContext
{
public:
    AllowThreadsContext()
    {
        _save = PyEval_SaveThread();
    }

    ~AllowThreadsContext()
    {
        PyEval_RestoreThread(_save);
    }
private:
    PyThreadState *_save;
};

class PythonException
{
public:
    /* Default constructor.  Used when the PyErr is already set.
       IE. when PyErr_Occured() return true. */
    PythonException()
        : exceptionType(nullptr), args(nullptr)
    {
        assert(PyErr_Occurred());
    }

    PythonException(PyObject* exceptionType, PyObjectPtr args)
        : exceptionType(exceptionType), args(args)
    { }

    PythonException(PyObject* exceptionType, const wchar_t * msg)
        : exceptionType(exceptionType), args(nullptr)
    {
        args.Attach(PyUnicode_FromWideChar(msg, -1));
    }

    static PythonException CreateFromFormat(PyObject* exceptionType, const wchar_t *format, const char* buildValueFormat, ...)
    {
        va_list va;
        va_start(va, buildValueFormat);
        auto formatObj = PyObjectPtr::CreateAttach(PyUnicode_FromWideChar(format, -1));
        auto args = PyObjectPtr::CreateAttach(Py_VaBuildValue(buildValueFormat, va));
        auto errorMessage = PyObjectPtr::CreateAttach(PyUnicode_Format(formatObj.Get(), args.Get()));
        va_end(va);
        return PythonException(exceptionType, errorMessage);
    }

    PyObject* exceptionType;
    PyObjectPtr args;
};

/*
ExceptionGuard() - Helper function to prevent any exceptions from being thrown into python's
"C" code.  This catches the C++ exception and raise a "python exception", essentially 
calls the PyErr* functions.  Only Platform::Exception and PythonException are converted.
*/
template <typename Callback>
auto ExceptionGuard(Callback callback, decltype(callback()) failureValue)
{
    try
    {
        return callback();
    }
    catch (Platform::Exception ^ e)
    {
        // assert that python exception is not already set
        assert(!PyErr_Occurred());

        auto msg = e->Message;
        auto pyMsg = PyUnicode_FromPlatformString(msg);
        PyErr_SetObject(PyExc_RuntimeError, pyMsg.Get());
        return failureValue;
    }
    catch (PythonException& e)
    {
        if (e.exceptionType != nullptr)
        {
            // assert that python exception is not already set
            assert(!PyErr_Occurred());
            PyErr_SetObject(e.exceptionType, e.args.Get());
        }
        return failureValue;
    }
}
