#include "pch.h"
#include "StartupTask.h"
#include "Python.h"
#include "pythonuwp.h"

#define STARTUP_FILE L"startupinfo.json"

using namespace pyuwpbackgroundservice;

using namespace concurrency;
using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::ApplicationModel::Background;
using namespace Windows::Data::Json;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Documents;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

void StartupTask::Run(IBackgroundTaskInstance^ taskInstance)
{
	auto deferral = taskInstance->GetDeferral();

    InitializePython();
    
    auto installationLocation = Windows::ApplicationModel::Package::Current->InstalledLocation;

    task<StorageFile^> storageFileTask(installationLocation->GetFileAsync(STARTUP_FILE));

    auto storageFile = storageFileTask.get();

    task<String^> startupInfoTask(FileIO::ReadTextAsync(storageFile));

    auto startupInfoString = startupInfoTask.get();

    auto jsonContents = JsonValue::Parse(startupInfoString);

    auto jsonArray = jsonContents->GetArray();

    auto argumentsVector = ref new Vector<String^>();

	if (jsonContents != nullptr)
	{
		argumentsVector->Append(L"Python.exe");

		// If debugging, add in the script for debug attachment
		if (IsDebuggerPresent())
		{
			CheckPythonDebuggerPresence(argumentsVector);
		}

        for (auto jsonValue : jsonArray) {
            argumentsVector->Append(jsonValue->GetString());
        }
	}

	OutputDebugString(L"Python arguments:");
	for (auto arg : argumentsVector)
	{
		OutputDebugString(L" ");
		OutputDebugString(arg->Data());
	}
	OutputDebugString(L"\r\n");

	int retValue = RunPy(argumentsVector);

    OutputDebugString(L"Python script completed execution: ");
	if (retValue == 0)
		OutputDebugString(L"Success\r\n");
	else
		OutputDebugString(L"Failure\r\n");

    FinalizePython();

	deferral->Complete();
}

void StartupTask::CheckPythonDebuggerPresence(Vector<String^>^ argumentsVector)
{
	wchar_t debugCommand[4096] = { 0 };
    const DWORD argCount = 3;
    ULONG_PTR args[argCount];
    bool isPresent = false;

    args[0] = (ULONG_PTR)&isPresent;
    args[1] = (ULONG_PTR)debugCommand;
    args[2] = sizeof(debugCommand);

    PingPythonDebugger(0xEDCBA987, args, argCount);

	// If debugger present, read out the debug arguments to append and launch the application
	if (isPresent)
	{
        OutputDebugString(L"Python debugger is present\r\n");

        auto jsonDebugCmd = JsonValue::Parse(ref new String(debugCommand));

        auto jsonArray = jsonDebugCmd->GetArray();

        for (auto jsonValue : jsonArray)
        {
            argumentsVector->Append(jsonValue->GetString());
        }
	}
    else {
        OutputDebugString(L"Python debugger is NOT present\r\n");
    }
}

int StartupTask::RunPy(Vector<String^>^ argumentsVector)
{
	auto writeStdOut = [this](Platform::String^ data) {
		OutputDebugString(data->Data());
		fwprintf(stdout, data->Data());
	};
	auto writeStdErr = [this](Platform::String^ data) {
		OutputDebugString(data->Data());
		fwprintf(stderr, data->Data());
	};

	return RunPython(writeStdOut, writeStdErr, argumentsVector);
}
