#pragma once
#include "python.h"
#include "PyObjectPtr.h"

struct EnumMemberDescriptor
{
    char* name;
    int value;
};

template <size_t size>
PyObjectPtr CreateEnum(const char* enumTypeName, EnumMemberDescriptor(&members)[size], const char* moduleName)
{
    auto enumType = GetTypeFromModule("enum", "Enum");
    if (!enumType)
    {
        return nullptr;
    }

    auto dict = PyObjectPtr::CreateAttach(PyDict_New());
    if (!dict)
    {
        return nullptr;
    }

    for (size_t i = 0; i < size; i++)
    {
        auto value = PyObjectPtr::CreateAttach(PyLong_FromLong(static_cast<long>(members[i].value)));
        if (!value)
        {
            return nullptr;
        }

        if (PyDict_SetItemString(dict.Get(), members[i].name, value.Get()) != 0)
        {
            // Insert failed
            return nullptr;
        }
    }

    auto args = PyObjectPtr::CreateAttach(Py_BuildValue("(sO)", enumTypeName, dict.Get()));
    if (!args)
    {
        return nullptr;
    }

    auto kwds = PyObjectPtr::CreateAttach(PyDict_New());
    if (!kwds)
    {
        return nullptr;
    }

    auto pyModuleName = PyObjectPtr::CreateAttach(PyUnicode_FromString(moduleName));
    if (!pyModuleName)
    {
        return nullptr;
    }

    if (PyDict_SetItemString(kwds.Get(), "module", pyModuleName.Get()) != 0)
    {
        return nullptr;
    }

    return PyObjectPtr::CreateAttach(PyObject_Call((PyObject*)enumType, args.Get(), kwds.Get()));
}