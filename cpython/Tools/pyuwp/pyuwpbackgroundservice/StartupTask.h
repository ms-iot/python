#pragma once

#include "pch.h"

namespace pyuwpbackgroundservice
{
	[Windows::Foundation::Metadata::WebHostHidden]
    public ref class StartupTask sealed : public Windows::ApplicationModel::Background::IBackgroundTask
    {
    public:
        virtual void Run(Windows::ApplicationModel::Background::IBackgroundTaskInstance^ taskInstance);
	private:
		bool IsPythonDebuggerPresent(wchar_t* debugXmlString);
		void CheckPythonDebuggerPresence(Platform::Collections::Vector<Platform::String^>^ argumentsVector);
		int RunPy(Platform::Collections::Vector<Platform::String^>^ argumentsVector);
	private:
		Windows::ApplicationModel::Background::BackgroundTaskDeferral^ deferral;
    };
}
