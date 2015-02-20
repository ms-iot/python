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
#pragma once

#include "pyconfig.h"

#ifndef PyAPI_FUNC
#ifdef Py_BUILD_CORE
#define PyAPI_FUNC(RTYPE) __declspec(dllexport) RTYPE
#else
#define PyAPI_FUNC(RTYPE) __declspec(dllimport) RTYPE
#endif
#endif

PyAPI_FUNC(int) RunPython(
    std::function<void(Platform::String^)> stdOutFunc,
    std::function<void(Platform::String^)> stdErrFunc,
    Platform::Collections::Vector<Platform::String^>^ argumentsVector);