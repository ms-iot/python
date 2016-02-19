# Python for UWP

PythonUWP is based on a modified version of CPython 3.5 which supports running as a Windows 10 [UWP (Universal Windows Platform)](https://msdn.microsoft.com/en-us/library/windows/apps/dn894631.aspx) application.  Specifically, this supports running in a background application on Windows 10 IoT core.

Since there are a lot of differences between a win32 application and an background UWP application, naturally PythonUWP also inherited some of these differences.  This document tries to highlight some of them.  This is not an exhaustive list by any means.

### Input/Output
---------

As the name suggests, a background UWP application runs in the background and doesn't contain any user interface elements, specifically a console.  Thus, the standard outputs -- `sys.stdout`, `sys.__stdout__`, `sys.stderr`, and `sys.__stderr__` -- have been redirect to OutputDebugString() or the "debug output window" when running under PTVS  in Visual Studio (https://github.com/Microsoft/PTVS).

The standard inputs -- `sys.stdin` and `sys.__stdin__` -- are set to `None`.  Any attempts to read from it by calling `Input()` without redirecting the `sys.stdin` will result in a `RuntimeError`.  

As usual, the user can the `sys.stdin`, `sys.stdout`, and/or `sys.stderr` to file and/or sockets as needed.

### Environment variables
---------

UWP applications do not have the concept of environment variables.  Thus, Python for UWP wouldn't inherit any of usual environment variables from Windows such as `PATH`, `ComSpec`, `WinDir` etc.  To help ease this transition, PythonUWP creates a pseudo environment.  See example below:

|Key|Value|
|---|-----|
|'LOCALAPPDATA'| 'C:\\Data\\Users\\DefaultAccount\\AppData\\Local\\Packages\\\AppName\>\\LocalState'|
|'INSTALLPATH'|'C:\\Data\\Users\\DefaultAccount\\AppxLayouts\\\<AppName\>'|
|'APPDATA'|'C:\\Data\\Users\\DefaultAccount\\AppData\\Local\\Packages\\\<AppName\>\\RoamingState'|
|'TEMP'|'C:\\Data\\Users\\DefaultAccount\\AppData\\Local\\Packages\\\<AppName\>\\TempState'|
|'TMP'| 'C:\\Data\\Users\\DefaultAccount\\AppData\\Local\\Packages\\\AppName\>\\TempState'|
|'PATH'|''|

### File system restrictions
---------

UWP apps can access certain file system locations by default.  These locations are: 
* [Application install directory](https://msdn.microsoft.com/en-us/library/windows/apps/windows.applicationmodel.package.installedlocation.aspx) (Read only)
* [Application local data folder](https://msdn.microsoft.com/en-us/library/windows/apps/windows.storage.applicationdata.localfolder.aspx) (Read/Write)
* [Application roaming data folder](https://msdn.microsoft.com/en-us/library/windows/apps/windows.storage.applicationdata.roamingfolder.aspx)  (Read/Write)
* [Application temporary folder](https://msdn.microsoft.com/en-us/library/windows/apps/windows.storage.applicationdata.temporaryfolder.aspx) (Read/Write)

These folders are mapped various environment variables:

|Folder|Environment variable|
|---|-----|
|Application install directory|os.environ['InstallPath']|
|Application local data directory|os.environ['LocalAppData']|
|Application roaming data directory|os.environ['AppData']|
|Application temporary directory|os.environ['Temp'] or os.environ['Tmp']|

On startup, the current working directory is the "Application install directory" which is read only.  Data files can be written to application data folder or application temporary folder.  The current directory can be changed with the following code.

```python
# Change current directory to application temporary folder
import os
os.chdir(os.environ['TMP'])
```

### Default encoding
---------

For a win32 application, windows usually provides both narrow and wide version of the same API -- for example: `CreateFileA()` vs `CreateFileW()`.  However, an UWP application only have access to the 'wide' version of the API.  Thus, python for UWP will convert all (narrow) byte string filenames to unicode filenames using the encoding return by `sys.getfilesystemencoding()`.  By default, pythonUWP's `sys.filesystemencoding()` returns `utf-8` instead of `mbcs`.  

Thus, calling any file-related function in pythonUWP with an invalid utf-8 byte string will result in a `UnicodeDecodeError`.

Similarly, the `locale.getpreferredencoding()` will return `utf-8` instead of `cp1252`.

### Create Process
---------

Unfortunately, due to UWP's restrictions, `CreateProcess()`is not availiable. Thus, PythonUWP cannot create any process.  Because of this, `SubProcess` and any modules / functions that depends on `SubProcess` are disabled.  Using these disabled modules and / or functions will result in ImportError, NameError, or AttributeError.  At a high level, the following are disabled.

* `SubProcess`
* `MultiProcessing`
* `concurrent.futures.ProcessPoolExecutor` (Only supports `concurrent.future.ThreadPoolExecutor`)
* `asyncio.ProactorEventLoop` (Only supports `asyncio.SelectorEventLoop`)
* `http.server.CGIHTTPRequestHandler`
* `os.exec*()` and friends
* `os.spawn*()` and friends
* `os.popen()`
* `os.system()`
* `platform.popen()`

See appendix for more detail.

### Writing platform dependent code
---------

To aid in writing platform dependent code, PythonUWP changed the the values of sys.platform and os.name to `'uwp'` and `'uwp_os'` respectively.  For example:

```python
if sys.platform == 'win32':
	# win32 specific code here...
elif sys.platform == 'uwp':
    # UWP specific code here ...
```

### Appendix - Disabled Modules
---------
 

| Module Name | Description |
|-------------| ------------|
|distutils|The distutils package provides support for building and installing additional modules into a Python installation. The new modules may be either 100%-pure Python, or may be extension modules written in C, or may be collections of Python packages which include modules coded in both Python and C.|
|idlelib|IDLE is Python’s Integrated Development and Learning Environment.|
|lzma|This module provides classes and convenience functions for compressing and decompressing data using the LZMA compression algorithm. Also included is a file interface supporting the .xz and legacy .lzma file formats used by the xz utility, as well as raw compressed streams.|
|mmap|Memory-mapped file support|
|msilib|The msilib supports the creation of Microsoft Installer (.msi) files. Because these files often contain an embedded "cabinet" file (.cab), it also exposes an API to create CAB files. Support for reading .cab files is currently not implemented; read support for the .msi database is possible.|
|multiprocessing |multiprocessing is a package that supports spawning processes using an API similar to the threading module. The multiprocessingpackage offers both local and remote concurrency, effectively side-stepping the Global Interpreter Lock by using subprocesses instead of threads. Due to this, the multiprocessing module allows the programmer to fully leverage multiple processors on a given machine. It runs on both Unix and Windows.|
|sqlite|SQLite is a C library that provides a lightweight disk-based database that doesn’t require a separate server process and allows accessing the database using a nonstandard variant of the SQL query language. Some applications can use SQLite for internal data storage. It’s also possible to prototype an application using SQLite and then port the code to a larger database such as PostgreSQL or Oracle.|
|subprocess|The subprocess module allows you to spawn new processes, connect to their input/output/error pipes, and obtain their return codes.|
|tkinter|The tkinter package ("Tk interface") is the standard Python interface to the Tk GUI toolkit. Both Tk and tkinter are available on most Unix platforms, as well as on Windows systems. (Tk itself is not part of Python; it is maintained at ActiveState.)|
|venv|The venv module provides support for creating lightweight "virtual environments" with their own site directories, optionally isolated from system site directories. Each virtual environment has its own Python binary (allowing creation of environments with various Python versions) and can have its own independent set of installed Python packages in its site directories.|
|webbrowser|The webbrowser module provides a high-level interface to allow displaying Web-based documents to users.|
|winreg|These functions expose the Windows registry API to Python. Instead of using an integer as the registry handle, a handle object is used to ensure that the handles are closed correctly, even if the programmer neglects to explicitly close them.|

### Appendix - Disabled functions / classes
---------

* `asyncio.proactor_events.BaseProactorEventLoop`
* `asyncio.windows_events.ProactorEventLoop`
* `asyncio.windows_events.IocpProactor`
* `asyncio.IocpProactor`
* `asyncio.ProactorEventLoop`
* `http.server.CGIHTTPRequestHandler`
* `msvcrt.SetErrorMode()`
* `msvcrt.getch()`
* `msvcrt.getche()`
* `msvcrt.wch()`
* `msvcrt.wche()`
* `msvcrt.kbhit()`
* `msvcrt.putch()`
* `msvcrt.putwch()`
* `msvcrt.ungetch()`
* `msvcrt.ungetwch()`
* `os.execl()`
* `os.execle()`
* `os.execlp()`
* `os.execlpe()`
* `os.execv()`
* `os.execve()`
* `os.execvp()`
* `os.execvpe()`
* `os.get_terminal_size()`
* `os.getlogin()`
* `os.getppid()`
* `os.kill()`
* `os.link()`
* `os.pipe()`
* `os.popen()`
* `os.putenv()`
* `os.readlink()`
* `os.set_handle_inheritable()`
* `os.set_inheritable()`
* `os.spawnl()`
* `os.spawnle()`
* `os.spawnv()`
* `os.spawnv()`
* `os.spawnve()`
* `os.startfile()`
* `os.symlink()`
* `os.system()`
* `os.waitpid()`
* `platform.popen()`
* `socket.socket.set_inheritable()`
* `ssl.enum_crls()`
* `time.process_time()`
* `_winapi.Overlapped `
* `_winapi.ConnectNamePipe()`
* `_winapi.CreateJunction()`
* `_winapi.CreateNanedPipe()`
* `_winapi.CreatePipe()`
* `_winapi.CreateProcess()`
* `_winapi.ExitProcess()`
* `_winapi.GetCurrentProcess()`
* `_winapi.GetExitCodeProcess()`
* `_winapi.GetModuleFileName()`
* `_winapi.GetStdHandle()`
* `_winapi.GetVersion()`
* `_winapi.OpenProcess()`
* `_winapi.PeekNamedPipe()`
* `_winapi.SetNamedPipeHandleState()`
* `_winapi.TerminateProcess()`
* `_winapi.WaitNamedPipe()`


