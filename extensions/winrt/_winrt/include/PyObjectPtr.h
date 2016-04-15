#pragma once
#include "python.h"

/*
Smart pointer wrapper around PyObject*.  This class doesn't throw exceptions.  Thus,
it is safe to use with or without ExceptionGuard().
*/
class PyObjectPtr
{
protected:
    PyObject *ptr_;

    void InternalAddRef() const throw()
    {
        if (ptr_ != nullptr)
        {
            Py_IncRef(ptr_);
        }
    }

    void InternalRelease() throw()
    {
        Py_DecRef(ptr_);
        ptr_ = nullptr;
    }

public:
#pragma region constructors
    PyObjectPtr() throw() : ptr_(nullptr)
    {
    }

    PyObjectPtr(decltype(__nullptr)) throw() : ptr_(nullptr)
    {
    }

    PyObjectPtr(_In_opt_ PyObject *other) throw() : ptr_(other)
    {
        InternalAddRef();
    }

    PyObjectPtr(const PyObjectPtr& other) throw() : ptr_(other.ptr_)
    {
        InternalAddRef();
    }

    PyObjectPtr(_Inout_ PyObjectPtr &&other) throw() : ptr_(nullptr)
    {
        if (this != reinterpret_cast<PyObjectPtr*>(&reinterpret_cast<unsigned char&>(other)))
        {
            Swap(other);
        }
    }
#pragma endregion

#pragma region destructor
    ~PyObjectPtr() throw()
    {
        InternalRelease();
    }
#pragma endregion

#pragma region assignment
    PyObjectPtr& operator=(decltype(__nullptr)) throw()
    {
        InternalRelease();
        return *this;
    }

    PyObjectPtr& operator=(_In_opt_ PyObject *other) throw()
    {
        if (ptr_ != other)
        {
            PyObjectPtr(other).Swap(*this);
        }
        return *this;
    }

    PyObjectPtr& operator=(const PyObjectPtr &other) throw()
    {
        if (ptr_ != other.ptr_)
        {
            PyObjectPtr(other).Swap(*this);
        }
        return *this;
    }

    PyObjectPtr& operator=(_Inout_ PyObjectPtr &&other) throw()
    {
        PyObjectPtr(static_cast<PyObjectPtr&&>(other)).Swap(*this);
        return *this;
    }
#pragma endregion

#pragma region modifiers
    void Swap(_Inout_ PyObjectPtr&& r) throw()
    {
        PyObject* tmp = ptr_;
        ptr_ = r.ptr_;
        r.ptr_ = tmp;
    }

    void Swap(_Inout_ PyObjectPtr& r) throw()
    {
        PyObject* tmp = ptr_;
        ptr_ = r.ptr_;
        r.ptr_ = tmp;
    }
#pragma endregion

    operator bool() const throw()
    {
        return Get() != nullptr ? true : false;
    }

    PyObject* Get() const throw()
    {
        return ptr_;
    }

    PyObject** GetAddressOf() throw()
    {
        return &ptr_;
    }

    PyObject* Detach() throw()
    {
        PyObject* ptr = ptr_;
        ptr_ = nullptr;
        return ptr;
    }

    void Attach(_In_opt_ PyObject* other) throw()
    {
        if (ptr_ == other)
        {
            return;
        }
        else if (ptr_ != nullptr)
        {
            InternalRelease();
        }

        ptr_ = other;
    }

    static PyObjectPtr CreateAttach(PyObject* other) throw()
    {
        PyObjectPtr ptr;
        ptr.Attach(other);
        return ptr;
    }
#pragma region python functions
    PyObjectPtr GetType() throw()
    {
        if (!ptr_)
        {
            return nullptr;
        }
        return PyObjectPtr::CreateAttach(PyObject_Type(this->Get()));
    }

    static PyObjectPtr CreateFromPlatformObject(PyTypeObject* type, Platform::Object^ winRTObject)
    {
        return CreateFromIInspectable(reinterpret_cast<PyObject*>(type), reinterpret_cast<IInspectable*>(winRTObject));
    }

    static PyObjectPtr CreateFromIInspectable(PyTypeObject* type, Microsoft::WRL::ComPtr<IInspectable> inspectable)
    {
        return CreateFromIInspectable(reinterpret_cast<PyObject*>(type), inspectable);
    }

    static PyObjectPtr CreateFromIInspectable(PyObject* type, Microsoft::WRL::ComPtr<IInspectable> inspectable)
    {
        auto pyLongVoidPtr = PyObjectPtr::CreateAttach(PyLong_FromVoidPtr(static_cast<void*>(inspectable.Get())));
        return PyObjectPtr::CreateAttach(PyObject_CallMethod(type, "_CreateFromIInspectable", "O", pyLongVoidPtr));
    }
#pragma endregion
};    // PyObjectPtr


class CoTaskMemoryHolder
{
public:
    CoTaskMemoryHolder(void* mem)
        : mem(mem)
    { }

    ~CoTaskMemoryHolder()
    {
        CoTaskMemFree(mem);
    }
private:
    void* mem;
};

class CoTaskMemoryHSTRINGHolder
{
public:
    CoTaskMemoryHSTRINGHolder(HSTRING* mem, size_t size)
        :mem(mem), size(size)
    {}

    ~CoTaskMemoryHSTRINGHolder()
    {
        for (size_t i = 0; i < size; i++)
        {
            ::WindowsDeleteString(mem[i]);
        }

        CoTaskMemFree(mem);
    }
private:
    HSTRING* mem;
    size_t size;
};