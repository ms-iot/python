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

#include <functional>

extern "C" {
    PyAPI_FUNC(bool) InitStdListeners();
}

PyAPI_FUNC(std::function<void(Platform::String^)>) SetStdOutCallback(std::function<void(Platform::String^)>);
PyAPI_FUNC(std::function<void(Platform::String^)>) SetStdErrCallback(std::function<void(Platform::String^)>);
