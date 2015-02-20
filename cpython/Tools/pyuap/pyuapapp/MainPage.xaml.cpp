//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "pythonwinrt.h"

#define STARTUP_FILE L"startupinfo.xml"

using namespace concurrency;

using namespace pyuapapp;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Data::Xml::Dom;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Documents;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Storage;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
{
	InitializeComponent();

    _workDoneColor = ref new SolidColorBrush(Windows::UI::Colors::CornflowerBlue);
    _errorTextColor = ref new SolidColorBrush(Windows::UI::Colors::Red);
    _dispatcher = Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher;
}

void MainPage::RunPythonScript(Object^ sender, RoutedEventArgs e)
{    
    auto installationLocation = Windows::ApplicationModel::Package::Current->InstalledLocation;

    task<StorageFile^> getStartupInfo(installationLocation->GetFileAsync(STARTUP_FILE));

    getStartupInfo.then([=](StorageFile^ storageFile)
    {
        task<XmlDocument^> getStartupInfoXml(XmlDocument::LoadFromFileAsync(storageFile));

        getStartupInfoXml.then([=](XmlDocument^ startupInfoXml)
        {
            Vector<String^>^ argumentsVector = ref new Vector<String^>();

            XmlNodeList^ argumentNodes = startupInfoXml->SelectNodes(L"StartupInfo/Argument");

            if (argumentNodes != nullptr)
            {
                argumentsVector->Append(L"Python.exe");

                // If debugging, add in the script for debug attachment
                if (IsDebuggerPresent())
                {
                    CheckPythonDebuggerPresence(argumentsVector);
                }

                for (unsigned int i = 0; i < argumentNodes->Length; i++)
                {
                    IXmlNode^ textNode = argumentNodes->GetAt(i)->FirstChild;
                    argumentsVector->Append((String^)textNode->NodeValue);
                }
            }

            task<int> runTask(create_async([=]() { return RunPy(argumentsVector); }));

            runTask.then([=](int retValue)
            {
                OutputDebugString(L"Python script completed execution: ");
                if (retValue == 0)
                    OutputDebugString(L"Success\r\n");
                else
                    OutputDebugString(L"Failure\r\n");

                auto grid = (Grid^)this->FindName(L"MainGrid");

                grid->Background = _workDoneColor;
            });
        });
    });
}

bool MainPage::IsPythonDebuggerPresent(wchar_t* debugXmlString)
{
    const DWORD pythonDebuggerExceptionCode = 0xEDCBA987;
    const DWORD argCount = 2;
    ULONG_PTR args[argCount];
    bool isPresent = false;

    args[0] = (ULONG_PTR)&isPresent;
    args[1] = (ULONG_PTR)debugXmlString;

    // Try to get information back from debugger
    __try
    {
        RaiseException(pythonDebuggerExceptionCode, 0, argCount, args);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    if (isPresent)
        OutputDebugString(L"Python debugger is present\r\n");
    else
        OutputDebugString(L"Python debugger is NOT present\r\n");

    return isPresent;
}

void MainPage::CheckPythonDebuggerPresence(Vector<String^>^ argumentsVector)
{
    wchar_t debugXmlString[4096] = { 0 };

    // If debugger present, read out the debug arguments to append and launch the application
    if (IsPythonDebuggerPresent(debugXmlString))
    {
        auto debugXml = ref new XmlDocument();
        XmlNodeList^ argumentNodes = nullptr;

        debugXml->LoadXml(ref new String(debugXmlString));

        argumentNodes = debugXml->SelectNodes("//arg");

        if (argumentNodes != nullptr)
        {
            for (unsigned int i = 0; i < argumentNodes->Length; i++)
            {
                auto textNode = argumentNodes->GetAt(i)->FirstChild;
                argumentsVector->Append((String^)textNode->NodeValue);
            }
        }
    }
}

void MainPage::StdErrWrite(String^ value)
{
    _dispatcher->RunAsync(
        Windows::UI::Core::CoreDispatcherPriority::Normal,
        ref new Windows::UI::Core::DispatchedHandler([=]()
    {
        WriteText(true, value);
    }, CallbackContext::Any));
}

void MainPage::StdOutWrite(String^ value)
{
    _dispatcher->RunAsync(
        Windows::UI::Core::CoreDispatcherPriority::Normal,
        ref new Windows::UI::Core::DispatchedHandler([=]()
    {
        WriteText(false, value);
    }, CallbackContext::Any));
}

void MainPage::WriteText(bool error, String^ value)
{
    auto block = (RichTextBlock^)this->FindName(L"PythonOut");

    if (block != nullptr)
    {
        auto para = (Paragraph^)block->Blocks->First()->Current;
        auto text = ref new Run();

        if (error)
        {
            text->Foreground = ref new SolidColorBrush(Windows::UI::Colors::Red);
        }

        text->Text = value;

        para->Inlines->Append(text);
    }
}

int MainPage::RunPy(Vector<String^>^ argumentsVector)
{
    auto writeStdOut = [this](Platform::String^ data) {
        this->StdOutWrite(data);
        fwprintf(stdout, data->Data());
    };
    auto writeStdErr = [this](Platform::String^ data) {
        this->StdErrWrite(data);
        fwprintf(stderr, data->Data());
    };

    return RunPython(writeStdOut, writeStdErr, argumentsVector);
}
