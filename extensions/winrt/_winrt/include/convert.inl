#pragma once
#include "Python.h"
#include "ExceptionGuard.h"
#include "Windows.Foundation.h"
#include <inspectable.h>

inline PyObjectPtr PyUnicode_FromPlatformString(Platform::String^ str)
{
    if (!str)
    {
        return PyObjectPtr::CreateAttach(PyUnicode_FromUnicode(NULL, 0));
    }

    return PyObjectPtr::CreateAttach(PyUnicode_FromUnicode(str->Data(), str->Length()));
}

inline Platform::String^ PlatformString_FromPyObject(PyObjectPtr obj)
{
    if (obj.Get() == Py_None)
    {
        // None is translated into Empty string
        return ref new Platform::String();
    }
    else if (PyUnicode_Check(obj.Get()))
    {
        Py_ssize_t length;
        auto wstr = PyUnicode_AsUnicodeAndSize(obj.Get(), &length);
        if (length <= UINT32_MAX)
        {
            return ref new Platform::String(wstr, static_cast<unsigned int>(length));
        }
        // fallthrough
    }

    throw PythonException::CreateFromFormat(PyExc_TypeError, L"cannot convert '%s' of type '%s' to string", "(OO)", obj.Get(), obj.GetType().Get());
}

inline __declspec(noreturn) void ThrowConversionError(Platform::Object^ obj)
{
    throw PythonException::CreateFromFormat(PyExc_TypeError, L"cannot convert '%s' of type '%s' to python type", "(uu)", obj->ToString()->Data(), obj->GetType()->FullName->Data());
}

inline __declspec(noreturn) void ThrowConversionError(PyObjectPtr obj)
{
    throw PythonException::CreateFromFormat(PyExc_TypeError, L"cannot convert '%s' of type '%s' to WinRT type", "(OO)", obj.Get(), obj.GetType().Get());
}


inline Platform::Object^ PlatformBuiltInType_FromPyObject(PyObjectPtr obj)
{
    if (obj.GetType().Get() == (PyObject*)&_PyNone_Type)
    {
        return nullptr;
    }
    else if (PyBool_Check(obj.Get()))
    {
        return obj.Get() == Py_True;
    }
    else if (PyUnicode_Check(obj.Get()))
    {
        return PlatformString_FromPyObject(obj.Get());
    }
    else if (PyLong_Check(obj.Get()))
    {
        int overflow = 0;
        Platform::Object^ value = PyLong_AsLongLong(obj.Get());
        // Check for overflow
        if (PyErr_Occurred())
        {
            throw PythonException();
        }
        return value;
    }
    else if (PyFloat_Check(obj.Get()))
    {
        Platform::Object^ value = PyFloat_AsDouble(obj.Get());
        return value;
    }
    else if (PyBytes_Check(obj.Get()))
    {
        Py_ssize_t size = PyBytes_Size(obj.Get());
        if (size > UINT32_MAX)
        {
            ThrowConversionError(obj);
        }
        return ref new Platform::Array<byte>((unsigned char*)PyBytes_AsString(obj.Get()), static_cast<unsigned int>(size));
    }
    else if (PyByteArray_Check(obj.Get()))
    {
        Py_ssize_t size = PyByteArray_Size(obj.Get());
        if (size > UINT32_MAX)
        {
            ThrowConversionError(obj);
        }
        return ref new Platform::Array<byte>((unsigned char*)PyByteArray_AsString(obj.Get()), static_cast<unsigned int>(size));
    }
    else if (PyList_Check(obj.Get()))
    {
        auto pySize = PyList_GET_SIZE(obj.Get());
        if (pySize == 0 || pySize > UINT_MAX)
        {
            ThrowConversionError(obj);
        }
        unsigned int size = static_cast<unsigned int>(pySize);

        PyObjectPtr firstItem = PyList_GetItem(obj.Get(), 0); // PyList_GetItem() returns borrowed reference
        auto itemType = firstItem.GetType();
        for (unsigned int i = 0; i < size; i++)
        {
            PyObjectPtr item = PyList_GetItem(obj.Get(), i); // PyList_GetItem() returns borrowed reference
            if (itemType.Get() != item.GetType().Get())
            {
                ThrowConversionError(obj);
            }
        }

        if (PyBool_Check(firstItem.Get()))
        {
            auto arr = ref new Platform::Array<bool>(size);
            for (unsigned int i = 0; i < size; i++)
            {
                arr[i] = (PyList_GetItem(obj.Get(), i) == Py_True);
            }
            return arr;
        }
        else if (PyUnicode_Check(firstItem.Get()))
        {
            auto arr = ref new Platform::Array<Platform::String^>(size);
            for (unsigned int i = 0; i < size; i++)
            {
                arr[i] = PlatformString_FromPyObject(PyList_GetItem(obj.Get(), i));
            }
            return arr;
        }
        else if (PyLong_Check(firstItem.Get()))
        {
            auto arr = ref new Platform::Array<long long>(size);
            for (unsigned int i = 0; i < size; i++)
            {
                arr[i] = PyLong_AsLongLong(PyList_GetItem(obj.Get(), i));
            }
            return arr;
        }
        else if (PyFloat_Check(firstItem.Get()))
        {
            auto arr = ref new Platform::Array<double>(size);
            for (unsigned int i = 0; i < size; i++)
            {
                arr[i] = PyFloat_AsDouble(PyList_GetItem(obj.Get(), i));
            }
            return arr;
        }
    }

    ThrowConversionError(obj);
}


inline PyObjectPtr PyObject_FromBuiltInType(bool val)
{
    auto result = PyObjectPtr::CreateAttach(PyBool_FromLong(val));
    if (!result)
    {
        throw PythonException();
    }
    return result;
}


inline PyObjectPtr PyObject_FromBuiltInType(unsigned char val)
{
    auto result = PyObjectPtr::CreateAttach(PyBytes_FromStringAndSize((char*)&val, 1));
    if (!result)
    {
        throw PythonException();
    }
    return result;
}


inline PyObjectPtr PyObject_FromBuiltInType(wchar_t val)
{
    auto result = PyObjectPtr::CreateAttach(PyUnicode_FromWideChar(&val, 1));
    if (!result)
    {
        throw PythonException();
    }
    return result;
}


inline PyObjectPtr PyObject_FromBuiltInType(short val)
{
    auto result = PyObjectPtr::CreateAttach(PyLong_FromLong(val));
    if (!result)
    {
        throw PythonException();
    }
    return result;
}


inline PyObjectPtr PyObject_FromBuiltInType(int val)
{
    auto result = PyObjectPtr::CreateAttach(PyLong_FromLong(val));
    if (!result)
    {
        throw PythonException();
    }
    return result;
}


inline PyObjectPtr PyObject_FromBuiltInType(long val)
{
    auto result = PyObjectPtr::CreateAttach(PyLong_FromLong(val));
    if (!result)
    {
        throw PythonException();
    }
    return result;
}


inline PyObjectPtr PyObject_FromBuiltInType(long long val)
{
    auto result = PyObjectPtr::CreateAttach(PyLong_FromLongLong(val));
    if (!result)
    {
        throw PythonException();
    }
    return result;
}


inline PyObjectPtr PyObject_FromBuiltInType(unsigned short val)
{
    auto result = PyObjectPtr::CreateAttach(PyLong_FromUnsignedLong(val));
    if (!result)
    {
        throw PythonException();
    }
    return result;
}


inline PyObjectPtr PyObject_FromBuiltInType(unsigned int val)
{
    auto result = PyObjectPtr::CreateAttach(PyLong_FromUnsignedLong(val));
    if (!result)
    {
        throw PythonException();
    }
    return result;
}


inline PyObjectPtr PyObject_FromBuiltInType(unsigned long val)
{
    auto result = PyObjectPtr::CreateAttach(PyLong_FromUnsignedLong(val));
    if (!result)
    {
        throw PythonException();
    }
    return result;
}


inline PyObjectPtr PyObject_FromBuiltInType(unsigned long long val)
{
    auto result = PyObjectPtr::CreateAttach(PyLong_FromUnsignedLongLong(val));
    if (!result)
    {
        throw PythonException();
    }
    return result;
}


inline PyObjectPtr PyObject_FromBuiltInType(float val)
{
    auto result = PyObjectPtr::CreateAttach(PyFloat_FromDouble((double)val));
    if (!result)
    {
        throw PythonException();
    }
    return result;
}


inline PyObjectPtr PyObject_FromBuiltInType(double val)
{
    auto result = PyObjectPtr::CreateAttach(PyFloat_FromDouble(val));
    if (!result)
    {
        throw PythonException();
    }
    return result;
}


inline PyObjectPtr PyObject_FromBuiltInType(HSTRING val)
{
    UINT32 length = 0;
    auto wstr = ::WindowsGetStringRawBuffer(val, &length);
    auto result = PyObjectPtr::CreateAttach(PyUnicode_FromUnicode(wstr, length));
    if (!result)
    {
        throw PythonException();
    }
    return result;
}

template <typename Type>
inline PyObjectPtr PyObject_FromBuiltInArray(Type* val, size_t size)
{
    auto retval = PyObjectPtr::CreateAttach(PyList_New(size));
    if (!retval)
    {
        throw PythonException();
    }

    for (size_t i = 0; i < size; i++)
    {
        auto item = PyObject_FromBuiltInType(val[i]);
        // PyList_SetItem() steals the item reference
        if (PyList_SetItem(retval.Get(), i, item.Detach()) != 0)
        {
            // Set item failed
            throw PythonException();
        }
    }

    return retval;
}


inline PyObjectPtr PyObject_FromPlatfromBuiltInType(Platform::Object^ obj)
{
    if (obj == nullptr)
    {
        return Py_None;
    }

    using namespace ::ABI::Windows::Foundation;
    using namespace ::Microsoft::WRL;
    ComPtr<IPropertyValue> spPropertyValue;
    ComPtr<IInspectable> spInspectable = reinterpret_cast<IInspectable*>(obj);
    if (FAILED(spInspectable.As<IPropertyValue>(&spPropertyValue)))
    {
        ThrowConversionError(obj);
    }

    PropertyType type;
    if (FAILED(spPropertyValue->get_Type(&type)))
    {
        ThrowConversionError(obj);
    }

    PyObjectPtr retval;

    switch (type)
    {
        case PropertyType::PropertyType_Boolean:
        {
            boolean val;
            if (FAILED(spPropertyValue->GetBoolean(&val)))
            {
                ThrowConversionError(obj);
            }
            return PyObject_FromBuiltInType(val != 0);
        }
        break;

        case PropertyType::PropertyType_Char16:
        {
            wchar_t val;
            if (FAILED(spPropertyValue->GetChar16(&val)))
            {
                ThrowConversionError(obj);
            }
            return PyObject_FromBuiltInType(val);
        }
        break;

        case PropertyType::PropertyType_Int16:
        {
            short val;
            if (FAILED(spPropertyValue->GetInt16(&val)))
            {
                ThrowConversionError(obj);
            }
            return PyObject_FromBuiltInType(val);
        }
        break;

        case PropertyType::PropertyType_Int32:
        {
            int val;
            if (FAILED(spPropertyValue->GetInt32(&val)))
            {
                ThrowConversionError(obj);
            }
            return PyObject_FromBuiltInType(val);
        }
        break;

        case PropertyType::PropertyType_Int64:
        {
            long long val;
            if (FAILED(spPropertyValue->GetInt64(&val)))
            {
                ThrowConversionError(obj);
            }
            return PyObject_FromBuiltInType(val);
        }
        break;

        case PropertyType::PropertyType_Single:
        {
            float val;
            if (FAILED(spPropertyValue->GetSingle(&val)))
            {
                ThrowConversionError(obj);
            }
            return PyObject_FromBuiltInType(val);
        }
        break;

        case PropertyType::PropertyType_Double:
        {
            double val;
            if (FAILED(spPropertyValue->GetDouble(&val)))
            {
                ThrowConversionError(obj);
            }
            return PyObject_FromBuiltInType(val);
        }
        break;

        case PropertyType::PropertyType_UInt16:
        {
            unsigned short val;
            if (FAILED(spPropertyValue->GetUInt16(&val)))
            {
                ThrowConversionError(obj);
            }
            return PyObject_FromBuiltInType(val);
        }
        break;

        case PropertyType::PropertyType_UInt32:
        {
            unsigned int val;
            if (FAILED(spPropertyValue->GetUInt32(&val)))
            {
                ThrowConversionError(obj);
            }
            return PyObject_FromBuiltInType(val);
        }
        break;

        case PropertyType::PropertyType_UInt64:
        {
            unsigned long long val;
            if (FAILED(spPropertyValue->GetUInt64(&val)))
            {
                ThrowConversionError(obj);
            }
            return PyObject_FromBuiltInType(val);
        }
        break;

        case PropertyType::PropertyType_String:
        {
            Microsoft::WRL::Wrappers::HString val;
            if (FAILED(spPropertyValue->GetString(val.GetAddressOf())))
            {
                ThrowConversionError(obj);
            }
            return PyObject_FromBuiltInType(val.Get());
        }
        break;

        case PropertyType::PropertyType_UInt8: // byte
        {
            unsigned char val;
            if (FAILED(spPropertyValue->GetUInt8(&val)))
            {
                ThrowConversionError(obj);
            }
            return PyObject_FromBuiltInType(val);
        }
        break;

        case PropertyType::PropertyType_BooleanArray:
        {
            UINT32 size;
            boolean* arr;
            if (FAILED(spPropertyValue->GetBooleanArray(&size, &arr)))
            {
                ThrowConversionError(obj);
            }

            CoTaskMemoryHolder cleanup(static_cast<void*>(arr));

            auto retval = PyObjectPtr::CreateAttach(PyList_New(size));
            if (!retval)
            {
                throw PythonException();
            }

            for (size_t i = 0; i < size; i++)
            {
                auto item = PyObject_FromBuiltInType(arr[i] != 0);
                // PyList_SetItem() steals the item reference
                if (PyList_SetItem(retval.Get(), i, item.Detach()) != 0)
                {
                    // Set item failed
                    throw PythonException();
                }
            }
            return retval;
        }
        break;

        case PropertyType::PropertyType_Char16Array:
        {
            UINT32 size;
            WCHAR* arr;
            if (FAILED(spPropertyValue->GetChar16Array(&size, &arr)))
            {
                ThrowConversionError(obj);
            }

            CoTaskMemoryHolder cleanup(static_cast<void*>(arr));
            return PyObject_FromBuiltInArray(arr, size);
        }
        break;

        case PropertyType::PropertyType_Int16Array:
        {
            UINT32 size;
            short* arr;
            if (FAILED(spPropertyValue->GetInt16Array(&size, &arr)))
            {
                ThrowConversionError(obj);
            }

            CoTaskMemoryHolder cleanup(static_cast<void*>(arr));
            return PyObject_FromBuiltInArray(arr, size);
        }
        break;

        case PropertyType::PropertyType_Int32Array:
        {
            UINT32 size;
            int* arr;
            if (FAILED(spPropertyValue->GetInt32Array(&size, &arr)))
            {
                ThrowConversionError(obj);
            }

            CoTaskMemoryHolder cleanup(static_cast<void*>(arr));
            return PyObject_FromBuiltInArray(arr, size);
        }
        break;

        case PropertyType::PropertyType_Int64Array:
        {
            UINT32 size;
            long long* arr;
            if (FAILED(spPropertyValue->GetInt64Array(&size, &arr)))
            {
                ThrowConversionError(obj);
            }

            CoTaskMemoryHolder cleanup(static_cast<void*>(arr));
            return PyObject_FromBuiltInArray(arr, size);
        }
        break;

        case PropertyType::PropertyType_SingleArray:
        {
            UINT32 size;
            float* arr;
            if (FAILED(spPropertyValue->GetSingleArray(&size, &arr)))
            {
                ThrowConversionError(obj);
            }

            CoTaskMemoryHolder cleanup(static_cast<void*>(arr));
            return PyObject_FromBuiltInArray(arr, size);
        }
        break;

        case PropertyType::PropertyType_DoubleArray:
        {
            UINT32 size;
            double* arr;
            if (FAILED(spPropertyValue->GetDoubleArray(&size, &arr)))
            {
                ThrowConversionError(obj);
            }

            CoTaskMemoryHolder cleanup(static_cast<void*>(arr));
            return PyObject_FromBuiltInArray(arr, size);
        }
        break;

        case PropertyType::PropertyType_UInt16Array:
        {
            UINT32 size;
            unsigned short* arr;
            if (FAILED(spPropertyValue->GetUInt16Array(&size, &arr)))
            {
                ThrowConversionError(obj);
            }

            CoTaskMemoryHolder cleanup(static_cast<void*>(arr));
            return PyObject_FromBuiltInArray(arr, size);
        }
        break;

        case PropertyType::PropertyType_UInt32Array:
        {
            UINT32 size;
            unsigned int* arr;
            if (FAILED(spPropertyValue->GetUInt32Array(&size, &arr)))
            {
                ThrowConversionError(obj);
            }

            CoTaskMemoryHolder cleanup(static_cast<void*>(arr));
            return PyObject_FromBuiltInArray(arr, size);
        }
        break;

        case PropertyType::PropertyType_UInt64Array:
        {
            UINT32 size;
            unsigned long long* arr;
            if (FAILED(spPropertyValue->GetUInt64Array(&size, &arr)))
            {
                ThrowConversionError(obj);
            }

            CoTaskMemoryHolder cleanup(static_cast<void*>(arr));
            return PyObject_FromBuiltInArray(arr, size);
        }
        break;

        case PropertyType::PropertyType_StringArray:
        {
            UINT32 size;
            HSTRING* arr;
            if (FAILED(spPropertyValue->GetStringArray(&size, &arr)))
            {
                ThrowConversionError(obj);
            }

            CoTaskMemoryHSTRINGHolder(arr, size);
            return PyObject_FromBuiltInArray(arr, size);
        }

        case PropertyType::PropertyType_UInt8Array: // byte
        {
            UINT32 size;
            unsigned char* arr;
            if (FAILED(spPropertyValue->GetUInt8Array(&size, &arr)))
            {
                ThrowConversionError(obj);
            }

            CoTaskMemoryHolder cleanup(static_cast<void*>(arr));
            return PyBytes_FromStringAndSize((char*)arr, size);
        }
    }
    ThrowConversionError(obj);
}

inline Windows::Foundation::Collections::ValueSet^ PyDict_ToValueSet(PyObjectPtr dict)
{
    if (!PyDict_Check(dict.Get()))
    {
        throw PythonException::CreateFromFormat(PyExc_TypeError, L"Cannot convert '%s' (type: '%s') to WinRT ValueSet", "(OO)", dict.Get(), dict.GetType().Get());
    }

    auto valueSet = ref new Windows::Foundation::Collections::ValueSet();
    auto keyValuePairs = PyObjectPtr::CreateAttach(PyDict_Items(dict.Get()));

    if (!keyValuePairs)
    {
        throw PythonException(); // Error already set
    }

    auto iterator = PyObjectPtr::CreateAttach(PyObject_GetIter(keyValuePairs.Get()));
    if (!iterator)
    {
        throw PythonException(); // Error already set
    }

    PyObjectPtr keyValuePair;
    while (keyValuePair = PyObjectPtr::CreateAttach(PyIter_Next(iterator.Get())))
    {
        PyObjectPtr key = PyTuple_GET_ITEM(keyValuePair.Get(), 0);
        PyObjectPtr value = PyTuple_GET_ITEM(keyValuePair.Get(), 1);

        auto platformKey = PlatformString_FromPyObject(key.Get());
        auto platformValue = PlatformBuiltInType_FromPyObject(value.Get());
        assert(!PyErr_Occurred());

        valueSet->Insert(platformKey, platformValue);
    }

    return valueSet;
}

inline PyObjectPtr PyDict_FromPlatformValueSet(Windows::Foundation::Collections::ValueSet^ valueSet)
{
    if (!valueSet)
    {
        return Py_None;
    }

    auto dict = PyObjectPtr::CreateAttach(PyDict_New());
    for (auto pair : valueSet)
    {
        auto key = PyUnicode_FromPlatformString(pair->Key);
        auto value = PyObject_FromPlatfromBuiltInType(pair->Value);
        if (PyObject_SetItem(dict.Get(), key.Get(), value.Get()) == -1)
        {
            // Insert failed.  Python exception is set by PyObject_SetItem()
            return nullptr;
        }
    }
    return dict;
}
