//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace pyuapapp
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();
	private:
        void RunPythonScript(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs e);
        int RunPy(Platform::Collections::Vector<Platform::String^>^ argumentsVector);
        void StdOutWrite(Platform::String^ value);
        void StdErrWrite(Platform::String^ value);
        void WriteText(bool error, Platform::String^ value);
        void CheckPythonDebuggerPresence(Platform::Collections::Vector<Platform::String^>^ argumentsVector);
        bool IsPythonDebuggerPresent(wchar_t* debugXmlString);
	private:
        Windows::UI::Core::CoreDispatcher^ _dispatcher;
        Windows::UI::Xaml::Media::SolidColorBrush^ _workDoneColor;
        Windows::UI::Xaml::Media::SolidColorBrush^ _errorTextColor;
    };
}
