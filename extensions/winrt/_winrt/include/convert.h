#pragma once
#include "Python.h"
#include "Windows.Foundation.h"
#include <inspectable.h>

/*
Converts built-in types to PyObject and vice versa.  Note: A round-trip conversion doesn't necessary will result in the same type.

C++/cx Type                 Python Type             C++/cx Type
==========                  ==============          =============
bool                =>      PyBool          =>      bool
unsigned char       =>      PyBytes         =>      Array<byte>^
wchar_t             =>      PyUnicode       =>      Platform::String^
short               =>      PyLong          =>      Int64
int                 =>      PyLong          =>      Int64
long                =>      PyLong          =>      Int64
long long           =>      PyLong          =>      Int64
unsigned short      =>      PyLong          =>      Int64
unsigned int        =>      PyLong          =>      Int64
unsigned long       =>      PyLong          =>      Int64
unsigned long long  =>      PyLong          =>      Int64
float               =>      PyFloat         =>      double
double              =>      PyFloat         =>      double
Platform::String^   =>      PyUnicode       =>      Platform::String^

Notes: 
1. Since PyLong doesn't have a maximum value.  It is possible that a PyLong contains value
   that is not within [INT64_MIN, INT64_MAX].  In that case, an OverflowError is raised.

2. Array<T>^ where T is a type specified above is also supported.  Array<T>^ will be converted
   to PyList containing the corresponding python type above.  PyList can also be converted to
   Array<T>^ provided that the elements within the PyList are of the same type.
*/

/* Helper function to cast IInspectable to type T^.  Throws InvalidCastException() on error. */
template <typename T>
T^ Platform_cast(IInspectable* inspectable)
{
    Platform::Object^ obj = reinterpret_cast<Platform::Object^>(inspectable);
    return safe_cast<T^>(obj);
}

/* Converts a Platform::String into a PyObject (of type PyUnicode). */
PyObjectPtr PyUnicode_FromPlatformString(Platform::String^ str);

/* Converts a PyObject to a platform string.  Only PyUnicode and PyNone are accepted.  PyNone are converted to an empty Platform::String.
   All other PyObjects will throw.  */
Platform::String^ PlatformString_FromPyObject(PyObjectPtr obj);

/* Converts a PyObject into a boxed value-type (aka Platform::Object^).  Only built-in types are supported.  See the conversion table. */
Platform::Object^ PlatformBuiltInType_FromPyObject(PyObjectPtr obj);

/* Converts built-in types to PyObjectPtr.  These functions throw on error. */
PyObjectPtr PyObject_FromBuiltInType(bool val);
PyObjectPtr PyObject_FromBuiltInType(unsigned char val);
PyObjectPtr PyObject_FromBuiltInType(wchar_t val);
PyObjectPtr PyObject_FromBuiltInType(short val);
PyObjectPtr PyObject_FromBuiltInType(int val);;
PyObjectPtr PyObject_FromBuiltInType(long val);
PyObjectPtr PyObject_FromBuiltInType(long long val);
PyObjectPtr PyObject_FromBuiltInType(unsigned short val);
PyObjectPtr PyObject_FromBuiltInType(unsigned int val);
PyObjectPtr PyObject_FromBuiltInType(unsigned long val);
PyObjectPtr PyObject_FromBuiltInType(unsigned long long val);
PyObjectPtr PyObject_FromBuiltInType(float val);
PyObjectPtr PyObject_FromBuiltInType(double val);
PyObjectPtr PyObject_FromBuiltInType(HSTRING val);

/* Converts built-in arrays in to a list of PyObjects.  Throws on error. */
template <typename Type>
PyObjectPtr PyObject_FromBuiltInArray(Type* val, size_t size);

PyObjectPtr PyObject_FromPlatfromBuiltInType(Platform::Object^ obj);

Windows::Foundation::Collections::ValueSet^ PyDict_ToValueSet(PyObjectPtr dict);
PyObjectPtr PyDict_FromPlatformValueSet(Windows::Foundation::Collections::ValueSet^ valueSet);

#include "convert.inl"