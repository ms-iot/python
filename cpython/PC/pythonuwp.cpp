/* ****************************************************************************
 *
 * Copyright (c) Microsoft Corporation. 
 *
 * This source code is subject to terms and conditions of the Apache License, Version 2.0. A 
 * copy of the license can be found in the License.html file at the root of this distribution. If 
 * you cannot locate the Apache License, Version 2.0, please send an email to 
 * vspython@microsoft.com. By using this source code in any fashion, you are agreeing to be bound 
 * by the terms of the Apache License, Version 2.0.
 *
 * You must not remove this notice, or any other, from this software.
 *
 * ***************************************************************************/

#include <collection.h>
#include <ppltasks.h>

#include "Python.h"
#include "Pythonrun.h"
#include "StdListener.h"

#include "pythonuwp.h"

using namespace Platform;
using namespace Platform::Collections; 

bool GetPythonDebuggerParams(
    DWORD dwExceptionCode,
    wchar_t* debugCommand);

void OutputDebugTimeString(PCWSTR message);

void InitializePython()
{
    OutputDebugTimeString(L"Starting initialize");
    Py_Initialize();
    OutputDebugTimeString(L"Finished initialize");
}

void FinalizePython()
{
    Py_Finalize();
}

int RunPython(
    std::function<void(String^)> stdOutFunc,
    std::function<void(String^)> stdErrFunc,
    Vector<String^>^ argumentsVector)
{
    int result = -1;

    // _set_invalid_parameter_handler();
    // _CrtSetReportMode()

    if (InitStdListeners())
    {
        SetStdOutCallback(stdOutFunc);
        SetStdErrCallback(stdErrFunc);
    }

    bool GetPythonDebuggerParams(
        DWORD dwExceptionCode,
        wchar_t* debugCommand);

    wchar_t** args = (wchar_t**)malloc(sizeof(wchar_t*)*(argumentsVector->Size));

    for (unsigned int i = 0; i < argumentsVector->Size; i++)
    {
        args[i] = (wchar_t*)argumentsVector->GetAt(i)->Data();
    }

    OutputDebugTimeString(L"Starting Py_Main...");
    result = Py_Main(argumentsVector->Size, args);

    free(args);

    return result;
}

void PingPythonDebugger(
    unsigned int dwExceptionCode,
    ULONG_PTR * pArguments,
    unsigned int nArguments)
{
    concurrency::task<void> raiseDebuggerExceptionTask(concurrency::create_async([=] {
        // Try to get information back from debugger
        __try
        {
            OutputDebugTimeString(L"Starting exception...");
            RaiseException(dwExceptionCode, 0, nArguments, pArguments);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            OutputDebugTimeString(L"finished exception.");
        }
    }));

    raiseDebuggerExceptionTask.wait();
}

void OutputDebugTimeString(PCWSTR message)
{
    time_t ltime;
    wchar_t timeBuf[256];

    time(&ltime);

    _wctime_s(timeBuf, sizeof(timeBuf) / sizeof(timeBuf[0]), &ltime);
    
    timeBuf[wcsnlen_s(timeBuf, sizeof(timeBuf) / sizeof(timeBuf[0])) - 1] = 0;

    OutputDebugString(timeBuf);
    OutputDebugString(L": ");
    OutputDebugString(message);
    OutputDebugString(L"\n");
}