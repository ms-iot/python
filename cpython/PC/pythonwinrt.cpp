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

#include "Python.h"
#include "Pythonrun.h"
#include "StdListener.h"

#include "pythonwinrt.h"

using namespace Platform;
using namespace Platform::Collections; 

int RunPython(
    std::function<void(String^)> stdOutFunc,
    std::function<void(String^)> stdErrFunc,
    Vector<String^>^ argumentsVector)
{
    int result = -1;

    Py_Initialize();

    // _set_invalid_parameter_handler();
    // _CrtSetReportMode()

    if (InitStdListeners())
    {
        SetStdOutCallback(stdOutFunc);
        SetStdErrCallback(stdErrFunc);
    }

    wchar_t** args = (wchar_t**)malloc(sizeof(wchar_t*)*(argumentsVector->Size));

    for (unsigned int i = 0; i < argumentsVector->Size; i++)
    {
        args[i] = (wchar_t*)argumentsVector->GetAt(i)->Data();
    }

    result = Py_Main(argumentsVector->Size, args);

    free(args);

    Py_Finalize();

    return result;
}
