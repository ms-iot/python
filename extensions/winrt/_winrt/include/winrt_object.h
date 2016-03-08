#pragma once

#include <Inspectable.h>
#include "python.h"

#ifdef __cplusplus
extern "C" {
#endif
    /* The structure of PyWinRTObject.  This is also the base class of all WinRT Object.  
       This class is analogous to Platform::Object.  This provides functions like python's 
       __str__ function, which Platform::Object::ToString().  This type's desctructor (tp_dealloc)
       decrement the reference count of the IInspectable objec to ensure proper clean up. */
    typedef struct {
        PyObject_HEAD
        IInspectable *pInspectable;         /* The one and only IInspectable. */
        PyObject* dict;                     /* Generic dictionary to hold any auxiliary python objects.  For example, winrt event objects.
                                               This dictionary is not set with tp_dictoffset on purpose.  This gives the subclass better control
                                               of properties/attributes. */
        PyObject* weaklist;                 /* Weak pointer list - to enable use of weak pointers fo this type of object*/
    } PyWinRTObject;

#ifdef __cplusplus
}
#endif