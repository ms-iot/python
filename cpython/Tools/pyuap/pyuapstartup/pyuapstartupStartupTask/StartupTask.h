#pragma once

#include "pch.h"

namespace pyuapstartupStartupTask
{
	[Windows::Foundation::Metadata::WebHostHidden]
    public ref class StartupTask sealed : public Windows::ApplicationModel::Background::IBackgroundTask
    {
    public:
        virtual void Run(Windows::ApplicationModel::Background::IBackgroundTaskInstance^ taskInstance);
    private:
        bool IsPythonDebuggerPresent(wchar_t* debugXmlString);
        void CheckPythonDebuggerPresence(Platform::Collections::Vector<Platform::String^>^ argumentsVector);
        void StdErrWrite(Platform::String^ value);
        void StdOutWrite(Platform::String^ value);
        int RunPy(Platform::Collections::Vector<Platform::String^>^ argumentsVector);
    private:
        Windows::ApplicationModel::Background::BackgroundTaskDeferral^ deferral;
    };
}
