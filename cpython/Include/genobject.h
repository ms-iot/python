
/* Generator object interface */

#ifndef Py_LIMITED_API
#ifndef Py_GENOBJECT_H
#define Py_GENOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

struct _frame; /* Avoid including frameobject.h */

typedef struct {
    PyObject_HEAD
    /* The gi_ prefix is intended to remind of generator-iterator. */

    /* Note: gi_frame can be NULL if the generator is "finished" */
    struct _frame *gi_frame;

    /* True if generator is being executed. */
    char gi_running;

    /* The code object backing the generator */
    PyObject *gi_code;

    /* List of weak reference. */
    PyObject *gi_weakreflist;

    /* Name of the generator. */
    PyObject *gi_name;

    /* Qualified name of the generator. */
    PyObject *gi_qualname;
} PyGenObject;

PyAPI_DATA(PyTypeObject) PyGen_Type;

#define PyGen_Check(op) PyObject_TypeCheck(op, &PyGen_Type)
#define PyGen_CheckExact(op) (Py_TYPE(op) == &PyGen_Type)

#define PyGen_CheckCoroutineExact(op) (PyGen_CheckExact(op) && \
                                       (((PyCodeObject*) \
                                           ((PyGenObject*)op)->gi_code) \
                                         ->co_flags & (CO_ITERABLE_COROUTINE | \
                                                       CO_COROUTINE)))

PyAPI_FUNC(PyObject *) PyGen_New(struct _frame *);
PyAPI_FUNC(PyObject *) PyGen_NewWithQualName(struct _frame *,
    PyObject *name, PyObject *qualname);
PyAPI_FUNC(int) PyGen_NeedsFinalizing(PyGenObject *);
PyAPI_FUNC(int) _PyGen_FetchStopIterationValue(PyObject **);
PyObject *_PyGen_Send(PyGenObject *, PyObject *);
PyAPI_FUNC(void) _PyGen_Finalize(PyObject *self);

PyObject *_PyGen_GetAwaitableIter(PyObject *o);

#ifdef __cplusplus
}
#endif
#endif /* !Py_GENOBJECT_H */
#endif /* Py_LIMITED_API */
