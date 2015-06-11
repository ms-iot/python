/*********************************************************

    msvcrtmodule.c

    A Python interface to the Microsoft Visual C Runtime
    Library, providing access to those non-portable, but
    still useful routines.

    Only ever compiled with an MS compiler, so no attempt
    has been made to avoid MS language extensions, etc...

    This may only work on NT or 95...

    Author: Mark Hammond and Guido van Rossum.
    Maintenance: Guido van Rossum.

***********************************************************/

#include "Python.h"
#include "malloc.h"
#include <io.h>
#include <conio.h>
#include <sys/locking.h>
#include <crtdbg.h>
#include <windows.h>

#ifdef _MSC_VER
#if _MSC_VER >= 1500 && _MSC_VER < 1600
#include <crtassem.h>
#elif _MSC_VER >= 1600
#include <crtversion.h>
#endif
#endif

/*[python input]
class Py_intptr_t_converter(CConverter):
    type = 'Py_intptr_t'
    format_unit = '"_Py_PARSE_INTPTR"'

class handle_return_converter(long_return_converter):
    type = 'Py_intptr_t'
    cast = '(void *)'
    conversion_fn = 'PyLong_FromVoidPtr'

class byte_char_return_converter(CReturnConverter):
    type = 'int'

    def render(self, function, data):
        data.declarations.append('char s[1];')
        data.return_value = 's[0]'
        data.return_conversion.append(
            'return_value = PyBytes_FromStringAndSize(s, 1);\n')

class wchar_t_return_converter(CReturnConverter):
    type = 'wchar_t'

    def render(self, function, data):
        self.declare(data)
        data.return_conversion.append(
            'return_value = PyUnicode_FromOrdinal(_return_value);\n')
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=6a54fc4e73d0b367]*/

/*[clinic input]
module msvcrt
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=f31a87a783d036cd]*/

#include "clinic/msvcrtmodule.c.h"

/*[clinic input]
msvcrt.heapmin

Minimize the malloc() heap.

Force the malloc() heap to clean itself up and return unused blocks
to the operating system. On failure, this raises OSError.
[clinic start generated code]*/

static PyObject *
msvcrt_heapmin_impl(PyModuleDef *module)
/*[clinic end generated code: output=464f866feb57c436 input=82e1771d21bde2d8]*/
{
#ifndef MS_UWP
    if (_heapmin() != 0)
        return PyErr_SetFromErrno(PyExc_IOError);
#endif

    Py_RETURN_NONE;
}
/*[clinic input]
msvcrt.locking

    fd: int
    mode: int
    nbytes: long
    /

Lock part of a file based on file descriptor fd from the C runtime.

Raises IOError on failure. The locked region of the file extends from
the current file position for nbytes bytes, and may continue beyond
the end of the file. mode must be one of the LK_* constants listed
below. Multiple regions in a file may be locked at the same time, but
may not overlap. Adjacent regions are not merged; they must be unlocked
individually.
[clinic start generated code]*/

static PyObject *
msvcrt_locking_impl(PyModuleDef *module, int fd, int mode, long nbytes)
/*[clinic end generated code: output=dff41e5e76d544de input=d9f13f0f6a713ba7]*/
{
    int err;

    Py_BEGIN_ALLOW_THREADS
    err = _locking(fd, mode, nbytes);
    Py_END_ALLOW_THREADS
    if (err != 0)
        return PyErr_SetFromErrno(PyExc_IOError);

    Py_RETURN_NONE;
}

/*[clinic input]
msvcrt.setmode -> long

    fd: int
    mode as flags: int
    /

Set the line-end translation mode for the file descriptor fd.

To set it to text mode, flags should be os.O_TEXT; for binary, it
should be os.O_BINARY.

Return value is the previous mode.
[clinic start generated code]*/

static long
msvcrt_setmode_impl(PyModuleDef *module, int fd, int flags)
/*[clinic end generated code: output=8c84e5b37c586d0d input=76e7c01f6b137f75]*/
{
    flags = _setmode(fd, flags);
    if (flags == -1)
        PyErr_SetFromErrno(PyExc_IOError);

    return flags;
}

/*[clinic input]
msvcrt.open_osfhandle -> long

    handle: Py_intptr_t
    flags: int
    /

Create a C runtime file descriptor from the file handle handle.

The flags parameter should be a bitwise OR of os.O_APPEND, os.O_RDONLY,
and os.O_TEXT. The returned file descriptor may be used as a parameter
to os.fdopen() to create a file object.
[clinic start generated code]*/

static long
msvcrt_open_osfhandle_impl(PyModuleDef *module, Py_intptr_t handle,
                           int flags)
/*[clinic end generated code: output=86bce32582c49c06 input=4d8516ed32db8f65]*/
{
    int fd;

    fd = _open_osfhandle(handle, flags);
    if (fd == -1)
        PyErr_SetFromErrno(PyExc_IOError);

    return fd;
}

/*[clinic input]
msvcrt.get_osfhandle -> handle

    fd: int
    /

Return the file handle for the file descriptor fd.

Raises IOError if fd is not recognized.
[clinic start generated code]*/

static Py_intptr_t
msvcrt_get_osfhandle_impl(PyModuleDef *module, int fd)
/*[clinic end generated code: output=376bff52586b55a6 input=c7d18d02c8017ec1]*/
{
    Py_intptr_t handle = -1;

    if (!_PyVerify_fd(fd)) {
        PyErr_SetFromErrno(PyExc_IOError);
    }
    else {
    _Py_BEGIN_SUPPRESS_IPH
        handle = _get_osfhandle(fd);
    _Py_END_SUPPRESS_IPH
        if (handle == -1)
            PyErr_SetFromErrno(PyExc_IOError);
    }

    return handle;
}

/* Console I/O */
/*[clinic input]
msvcrt.kbhit -> long

Return true if a keypress is waiting to be read.
[clinic start generated code]*/

static long
msvcrt_kbhit_impl(PyModuleDef *module)
/*[clinic end generated code: output=2b7293fcbe5cb24e input=e70d678a5c2f6acc]*/
{
    return _kbhit();
}

/*[clinic input]
msvcrt.getch -> byte_char

Read a keypress and return the resulting character as a byte string.

Nothing is echoed to the console. This call will block if a keypress is
not already available, but will not wait for Enter to be pressed. If the
pressed key was a special function key, this will return '\000' or
'\xe0'; the next call will return the keycode. The Control-C keypress
cannot be read with this function.
[clinic start generated code]*/

static int
msvcrt_getch_impl(PyModuleDef *module)
/*[clinic end generated code: output=199e3d89f49c166a input=37a40cf0ed0d1153]*/
{
#ifndef MS_UWP
    int ch;
#endif

#ifdef MS_UWP
    PyErr_SetString(PyExc_NotImplementedError, "Not supported in UWP Apps");
    return NULL;
#else
    Py_BEGIN_ALLOW_THREADS
    ch = _getch();
    Py_END_ALLOW_THREADS
    return ch;
#endif
}

/*[clinic input]
msvcrt.getwch -> wchar_t

Wide char variant of getch(), returning a Unicode value.
[clinic start generated code]*/

static wchar_t
msvcrt_getwch_impl(PyModuleDef *module)
/*[clinic end generated code: output=9d3762861328b1fe input=27b3dec8ad823d7c]*/
{
    wchar_t ch;

    Py_BEGIN_ALLOW_THREADS
    ch = _getwch();
    Py_END_ALLOW_THREADS
    return ch;
}

/*[clinic input]
msvcrt.getche -> byte_char

Similar to getch(), but the keypress will be echoed if possible.
[clinic start generated code]*/

static int
msvcrt_getche_impl(PyModuleDef *module)
/*[clinic end generated code: output=8aa369be6550068e input=43311ade9ed4a9c0]*/
{
#ifndef MS_UWP
    int ch;
#endif

#ifdef MS_UWP
    PyErr_SetString(PyExc_NotImplementedError, "Not supported in UWP Apps");
    return NULL;
#else
    Py_BEGIN_ALLOW_THREADS
    ch = _getche();
    Py_END_ALLOW_THREADS
    return ch;
#endif
}

/*[clinic input]
msvcrt.getwche -> wchar_t

Wide char variant of getche(), returning a Unicode value.
[clinic start generated code]*/

static wchar_t
msvcrt_getwche_impl(PyModuleDef *module)
/*[clinic end generated code: output=3693cf78e3ea0cf6 input=49337d59d1a591f8]*/
{
    wchar_t ch;

    Py_BEGIN_ALLOW_THREADS
    ch = _getwche();
    Py_END_ALLOW_THREADS
    return ch;
}

/*[clinic input]
msvcrt.putch

    char: char
    /

Print the byte string char to the console without buffering.
[clinic start generated code]*/

static PyObject *
msvcrt_putch_impl(PyModuleDef *module, char char_value)
/*[clinic end generated code: output=c05548b11554f36f input=ec078dd10cb054d6]*/
{
    _putch(char_value);
    Py_RETURN_NONE;
}

/*[clinic input]
msvcrt.putwch

    unicode_char: int(accept={str})
    /

Wide char variant of putch(), accepting a Unicode value.
[clinic start generated code]*/

static PyObject *
msvcrt_putwch_impl(PyModuleDef *module, int unicode_char)
/*[clinic end generated code: output=c216a73694ca73dd input=996ccd0bbcbac4c3]*/
{
    _putwch(unicode_char);
    Py_RETURN_NONE;

}

/*[clinic input]
msvcrt.ungetch

    char: char
    /

Opposite of getch.

Cause the byte string char to be "pushed back" into the
console buffer; it will be the next character read by
getch() or getche().
[clinic start generated code]*/

static PyObject *
msvcrt_ungetch_impl(PyModuleDef *module, char char_value)
/*[clinic end generated code: output=19a4cd3249709ec9 input=22f07ee9001bbf0f]*/
{

#ifdef MS_UWP
    PyErr_SetString(PyExc_NotImplementedError, "Not supported in UWP Apps");
    return NULL;
#else
    if (_ungetch(char_value) == EOF)
        return PyErr_SetFromErrno(PyExc_IOError);
    Py_RETURN_NONE;
#endif
}

/*[clinic input]
msvcrt.ungetwch

    unicode_char: int(accept={str})
    /

Wide char variant of ungetch(), accepting a Unicode value.
[clinic start generated code]*/

static PyObject *
msvcrt_ungetwch_impl(PyModuleDef *module, int unicode_char)
/*[clinic end generated code: output=1ee7674710322bd1 input=83ec0492be04d564]*/
{
    if (_ungetwch(unicode_char) == WEOF)
        return PyErr_SetFromErrno(PyExc_IOError);
    Py_RETURN_NONE;
}

#ifdef _DEBUG
/*[clinic input]
msvcrt.CrtSetReportFile -> long

    type: int
    file: int
    /

Wrapper around _CrtSetReportFile.

Only available on Debug builds.
[clinic start generated code]*/

static long
msvcrt_CrtSetReportFile_impl(PyModuleDef *module, int type, int file)
/*[clinic end generated code: output=8c3644fb2edfa808 input=bb8f721a604fcc45]*/
{
    return (long)_CrtSetReportFile(type, (_HFILE)file);
}

/*[clinic input]
msvcrt.CrtSetReportMode -> long

    type: int
    mode: int
    /

Wrapper around _CrtSetReportMode.

Only available on Debug builds.
[clinic start generated code]*/

static long
msvcrt_CrtSetReportMode_impl(PyModuleDef *module, int type, int mode)
/*[clinic end generated code: output=b407fbf8716a52b9 input=9319d29b4319426b]*/
{
    int res;

    res = _CrtSetReportMode(type, mode);
    if (res == -1)
        PyErr_SetFromErrno(PyExc_IOError);
    return res;
}

/*[clinic input]
msvcrt.set_error_mode -> long

    mode: int
    /

Wrapper around _set_error_mode.

Only available on Debug builds.
[clinic start generated code]*/

static long
msvcrt_set_error_mode_impl(PyModuleDef *module, int mode)
/*[clinic end generated code: output=62148adffa90867d input=046fca59c0f20872]*/
{
    return _set_error_mode(mode);
}
#endif /* _DEBUG */

/*[clinic input]
msvcrt.SetErrorMode

    mode: unsigned_int(bitwise=True)
    /

Wrapper around SetErrorMode.
[clinic start generated code]*/

static PyObject *
msvcrt_SetErrorMode_impl(PyModuleDef *module, unsigned int mode)
/*[clinic end generated code: output=544c60b085be79c6 input=d8b167258d32d907]*/
{
#ifndef MS_UWP
	unsigned int res;
#endif

#ifdef MS_UWP
    PyErr_SetString(PyExc_NotImplementedError, "Not supported in UWP Apps");
    return NULL;
#else
    res = SetErrorMode(mode);
    return PyLong_FromUnsignedLong(res);
#endif
}

/*[clinic input]
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=da39a3ee5e6b4b0d]*/

/* List of functions exported by this module */
static struct PyMethodDef msvcrt_functions[] = {
    MSVCRT_HEAPMIN_METHODDEF
    MSVCRT_LOCKING_METHODDEF
    MSVCRT_SETMODE_METHODDEF
    MSVCRT_OPEN_OSFHANDLE_METHODDEF
    MSVCRT_GET_OSFHANDLE_METHODDEF
#ifndef MS_UWP
    MSVCRT_KBHIT_METHODDEF
    MSVCRT_GETCH_METHODDEF
    MSVCRT_GETCHE_METHODDEF
    MSVCRT_PUTCH_METHODDEF
    MSVCRT_UNGETCH_METHODDEF
    MSVCRT_SETERRORMODE_METHODDEF
    MSVCRT_CRTSETREPORTFILE_METHODDEF
    MSVCRT_CRTSETREPORTMODE_METHODDEF
    MSVCRT_SET_ERROR_MODE_METHODDEF
#endif /* !MS_UWP */
    MSVCRT_GETWCH_METHODDEF
    MSVCRT_GETWCHE_METHODDEF
    MSVCRT_PUTWCH_METHODDEF
    MSVCRT_UNGETWCH_METHODDEF

    {NULL,                      NULL}
};


static struct PyModuleDef msvcrtmodule = {
    PyModuleDef_HEAD_INIT,
    "msvcrt",
    NULL,
    -1,
    msvcrt_functions,
    NULL,
    NULL,
    NULL,
    NULL
};

static void
insertint(PyObject *d, char *name, int value)
{
    PyObject *v = PyLong_FromLong((long) value);
    if (v == NULL) {
        /* Don't bother reporting this error */
        PyErr_Clear();
    }
    else {
        PyDict_SetItemString(d, name, v);
        Py_DECREF(v);
    }
}

PyMODINIT_FUNC
PyInit_msvcrt(void)
{
    int st;
    PyObject *d, *version;
    PyObject *m = PyModule_Create(&msvcrtmodule);
    if (m == NULL)
        return NULL;
    d = PyModule_GetDict(m);

    /* constants for the locking() function's mode argument */
    insertint(d, "LK_LOCK", _LK_LOCK);
    insertint(d, "LK_NBLCK", _LK_NBLCK);
    insertint(d, "LK_NBRLCK", _LK_NBRLCK);
    insertint(d, "LK_RLCK", _LK_RLCK);
    insertint(d, "LK_UNLCK", _LK_UNLCK);
#ifndef MS_UWP
    insertint(d, "SEM_FAILCRITICALERRORS", SEM_FAILCRITICALERRORS);
    insertint(d, "SEM_NOALIGNMENTFAULTEXCEPT", SEM_NOALIGNMENTFAULTEXCEPT);
    insertint(d, "SEM_NOGPFAULTERRORBOX", SEM_NOGPFAULTERRORBOX);
    insertint(d, "SEM_NOOPENFILEERRORBOX", SEM_NOOPENFILEERRORBOX);
#endif
#ifdef _DEBUG
    insertint(d, "CRT_WARN", _CRT_WARN);
    insertint(d, "CRT_ERROR", _CRT_ERROR);
    insertint(d, "CRT_ASSERT", _CRT_ASSERT);
    insertint(d, "CRTDBG_MODE_DEBUG", _CRTDBG_MODE_DEBUG);
    insertint(d, "CRTDBG_MODE_FILE", _CRTDBG_MODE_FILE);
    insertint(d, "CRTDBG_MODE_WNDW", _CRTDBG_MODE_WNDW);
    insertint(d, "CRTDBG_REPORT_MODE", _CRTDBG_REPORT_MODE);
    insertint(d, "CRTDBG_FILE_STDERR", (int)_CRTDBG_FILE_STDERR);
    insertint(d, "CRTDBG_FILE_STDOUT", (int)_CRTDBG_FILE_STDOUT);
    insertint(d, "CRTDBG_REPORT_FILE", (int)_CRTDBG_REPORT_FILE);
#endif

    /* constants for the crt versions */
    (void)st;
#ifdef _VC_ASSEMBLY_PUBLICKEYTOKEN
    st = PyModule_AddStringConstant(m, "VC_ASSEMBLY_PUBLICKEYTOKEN",
                                    _VC_ASSEMBLY_PUBLICKEYTOKEN);
    if (st < 0) return NULL;
#endif
#ifdef _CRT_ASSEMBLY_VERSION
    st = PyModule_AddStringConstant(m, "CRT_ASSEMBLY_VERSION",
                                    _CRT_ASSEMBLY_VERSION);
    if (st < 0) return NULL;
#endif
#ifdef __LIBRARIES_ASSEMBLY_NAME_PREFIX
    st = PyModule_AddStringConstant(m, "LIBRARIES_ASSEMBLY_NAME_PREFIX",
                                    __LIBRARIES_ASSEMBLY_NAME_PREFIX);
    if (st < 0) return NULL;
#endif

    /* constants for the 2010 crt versions */
#if defined(_VC_CRT_MAJOR_VERSION) && defined (_VC_CRT_MINOR_VERSION) && defined(_VC_CRT_BUILD_VERSION) && defined(_VC_CRT_RBUILD_VERSION)
    version = PyUnicode_FromFormat("%d.%d.%d.%d", _VC_CRT_MAJOR_VERSION,
                                                  _VC_CRT_MINOR_VERSION,
                                                  _VC_CRT_BUILD_VERSION,
                                                  _VC_CRT_RBUILD_VERSION);
    st = PyModule_AddObject(m, "CRT_ASSEMBLY_VERSION", version);
    if (st < 0) return NULL;
#endif

    return m;
}
