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

#include "python.h"
#include "StdListener.h"

extern "C" {

    _Py_IDENTIFIER(stderr);
    _Py_IDENTIFIER(stdout);

    struct StdListenerObject
    {
        PyObject_HEAD               /* a pure abstract base class */
        std::function<void(Platform::String^)> target;
        PyObject *dict;
        int fd;
    };

    static void dealloc(StdListenerObject *obj)
    {
        Py_XDECREF(obj->dict);
        obj->target = nullptr;
        Py_TYPE(obj)->tp_free(obj);
    }

    static PyObject *StdListener_write(PyObject *self, PyObject *args, PyObject *kwargs)
    {
        PyObject *textObj = nullptr;
        if (!PyArg_UnpackTuple(args, "write", 1, 1, &textObj)) {
            return nullptr;
        }

        if (PyUnicode_Check(textObj)) {
            Py_INCREF(textObj);
        }
        else {
            textObj = PyUnicode_FromObject(textObj);
        }

        Py_ssize_t cch;
        auto text = PyUnicode_AsUnicodeAndSize(textObj, &cch);

        ((StdListenerObject*)self)->target(ref new Platform::String(text, cch));

        Py_DECREF(textObj);
        Py_RETURN_NONE;
    }

    static PyObject *StdListener_fileno(PyObject *self, PyObject *args, PyObject *kwargs)
    {
        return PyLong_FromLong(((StdListenerObject*)self)->fd);
    }

    static PyObject *StdListener_flush(PyObject *self, PyObject *args, PyObject *kwargs)
    {
        Py_RETURN_NONE;
    }

    static PyMethodDef StdListener_Methods[] = {
        { "write", (PyCFunction)StdListener_write, METH_VARARGS, NULL },
        { "fileno", (PyCFunction)StdListener_fileno, METH_VARARGS, NULL },
        { "flush", (PyCFunction)StdListener_flush, METH_VARARGS, NULL },
        { nullptr }
    };

    static PyTypeObject StdListenerType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "WindowsRuntimeStdListener",
        sizeof(StdListenerObject), 
        0,
        (destructor)dealloc,
        0,                                                  /* tp_print */
        0,                                                  /* tp_getattr */
        0,                                                  /* tp_setattr */
        0,                                                  /* tp_reserved */
        0,                                                  /* tp_repr */
        0,                                                  /* tp_as_number */
        0,                                                  /* tp_as_sequence */
        0,                                                  /* tp_as_mapping */
        0,                                                  /* tp_hash */
        0,                                                  /* tp_call */
        0,                                                  /* tp_str */
        0,                                                  /* tp_getattro */
        0,                                                  /* tp_setattro */
        0,                                                  /* tp_as_buffer */
        0,                                                  /* tp_flags */
        0,                                                  /* tp_doc */
        0,                                                  /* tp_traverse */
        0,                                                  /* tp_clear */
        0,                                                  /* tp_richcompare */
        0,                                                  /* tp_weaklistoffset */
        0,                                                  /* tp_iter */
        0,                                                  /* tp_iternext */
        StdListener_Methods,                                /* tp_methods */
        0,                                                  /* tp_members */
        0,                                                  /* tp_getset */
        0,                                                  /* tp_base */
        0,                                                  /* tp_dict */
        0,                                                  /* tp_descr_get */
        0,                                                  /* tp_descr_set */
        0,                                                  /* tp_dictoffset */
        0,                                                  /* tp_init */
        0,                                                  /* tp_alloc */
        0,                                                  /* tp_new */
        0,                                                  /* tp_free */
    };

    bool InitStdListeners()
    {
        StdListenerType.tp_dictoffset = offsetof(StdListenerObject, dict);
        StdListenerType.tp_methods = StdListener_Methods;

        if (PyType_Ready(&StdListenerType) < 0) {
            return false;
        }

        auto stdOut = StdListenerType.tp_alloc(&StdListenerType, 0);
        if (!stdOut) return false;
        ((StdListenerObject*)stdOut)->target = nullptr;
        ((StdListenerObject*)stdOut)->dict = PyDict_New();
        ((StdListenerObject*)stdOut)->fd = (int)stdout;

        auto stdErr = StdListenerType.tp_alloc(&StdListenerType, 0);
        if (!stdErr) { Py_DECREF(stdOut); return false; }
        ((StdListenerObject*)stdErr)->target = nullptr;
        ((StdListenerObject*)stdErr)->dict = PyDict_New();
        ((StdListenerObject*)stdOut)->fd = (int)stderr;

        auto encodingStr = PyUnicode_FromString("utf-16");
        if (!encodingStr ||
            PyObject_SetAttrString(stdOut, "encoding", encodingStr) < 0 ||
            PyObject_SetAttrString(stdErr, "encoding", encodingStr) < 0 ||
            PyObject_SetAttrString(stdOut, "newlines", Py_None) < 0 ||
            PyObject_SetAttrString(stdErr, "newlines", Py_None) < 0 ||
            PySys_SetObject("stdout", stdOut) < 0 ||
            PySys_SetObject("__stdout__", stdOut) < 0 ||
            _PySys_SetObjectId(&PyId_stdout, stdOut) < 0 ||
            PySys_SetObject("stderr", stdErr) < 0 ||
            PySys_SetObject("__stderr__", stdErr) < 0 ||
            _PySys_SetObjectId(&PyId_stderr, stdErr) < 0)
        {
            Py_DECREF(encodingStr);
            Py_DECREF(stdOut);
            Py_DECREF(stdErr);
            return false;
        }
        Py_DECREF(encodingStr);

        return true;
    }
}
std::function<void(Platform::String^)> SetStdOutCallback(std::function<void(Platform::String^)> fn)
{
    auto old = ((StdListenerObject*)PySys_GetObject("stdout"))->target;
    ((StdListenerObject*)PySys_GetObject("stdout"))->target = fn;
    return old;
}

std::function<void(Platform::String^)> SetStdErrCallback(std::function<void(Platform::String^)> fn)
{
    auto old = ((StdListenerObject*)PySys_GetObject("stderr"))->target;
    ((StdListenerObject*)PySys_GetObject("stderr"))->target = fn;
    return old;
}

