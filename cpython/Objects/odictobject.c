/* Ordered Dictionary object implementation.

This implementation is necessarily explicitly equivalent to the pure Python
OrderedDict class in Lib/collections/__init__.py.  The strategy there
involves using a doubly-linked-list to capture the order.  We keep to that
strategy, using a lower-level linked-list.

About the Linked-List
=====================

For the linked list we use a basic doubly-linked-list.  Using a circularly-
linked-list does have some benefits, but they don't apply so much here
since OrderedDict is focused on the ends of the list (for the most part).
Furthermore, there are some features of generic linked-lists that we simply
don't need for OrderedDict.  Thus a simple custom implementation meets our
needs.  Alternatives to our simple approach include the QCIRCLE_*
macros from BSD's queue.h, and the linux's list.h.

Getting O(1) Node Lookup
------------------------

One invariant of Python's OrderedDict is that it preserves time complexity
of dict's methods, particularly the O(1) operations.  Simply adding a
linked-list on top of dict is not sufficient here; operations for nodes in
the middle of the linked-list implicitly require finding the node first.
With a simple linked-list like we're using, that is an O(n) operation.
Consequently, methods like __delitem__() would change from O(1) to O(n),
which is unacceptable.

In order to preserve O(1) performance for node removal (finding nodes), we
must do better than just looping through the linked-list.  Here are options
we've considered:

1. use a second dict to map keys to nodes (a la the pure Python version).
2. keep a simple hash table mirroring the order of dict's, mapping each key
   to the corresponding node in the linked-list.
3. use a version of shared keys (split dict) that allows non-unicode keys.
4. have the value stored for each key be a (value, node) pair, and adjust
   __getitem__(), get(), etc. accordingly.

The approach with the least performance impact (time and space) is #2,
mirroring the key order of dict's dk_enties with an array of node pointers.
While lookdict() and friends (dk_lookup) don't give us the index into the
array, we make use of pointer arithmetic to get that index.  An alternative
would be to refactor lookdict() to provide the index, explicitly exposing
the implementation detail.  We could even just use a custom lookup function
for OrderedDict that facilitates our need.  However, both approaches are
significantly more complicated than just using pointer arithmetic.

The catch with mirroring the hash table ordering is that we have to keep
the ordering in sync through any dict resizes.  However, that order only
matters during node lookup.  We can simply defer any potential resizing
until we need to do a lookup.

Linked-List Nodes
-----------------

The current implementation stores a pointer to the associated key only.
One alternative would be to store a pointer to the PyDictKeyEntry instead.
This would save one pointer de-reference per item, which is nice during
calls to values() and items().  However, it adds unnecessary overhead
otherwise, so we stick with just the key.

Linked-List API
---------------

As noted, the linked-list implemented here does not have all the bells and
whistles.  However, we recognize that the implementation may need to
change to accommodate performance improvements or extra functionality.  To
that end, We use a simple API to interact with the linked-list.  Here's a
summary of the methods/macros:

Node info:

* _odictnode_KEY(node)
* _odictnode_VALUE(od, node)
* _odictnode_PREV(node)
* _odictnode_NEXT(node)

Linked-List info:

* _odict_FIRST(od)
* _odict_LAST(od)
* _odict_EMPTY(od)
* _odict_FOREACH(od, node) - used in place of `for (node=...)`

For adding nodes:

* _odict_add_head(od, node)
* _odict_add_tail(od, node)
* _odict_add_new_node(od, key)

For removing nodes:

* _odict_pop_node(od, node, key)
* _odict_clear_node(od, node)
* _odict_clear_nodes(od, clear_each)

Others:

* _odict_initialize(od)
* _odict_find_node(od, key)
* _odict_keys_equal(od1, od2)

Used, but specific to the linked-list implementation:

* _odict_free_fast_nodes(od)

And here's a look at how the linked-list relates to the OrderedDict API:

============ === === ==== ==== ==== === ==== ===== ==== ==== === ==== === ===
method       key val prev next mem  1st last empty iter find add rmv  clr keq
============ === === ==== ==== ==== === ==== ===== ==== ==== === ==== === ===
__del__                                      ~                        X
__delitem__                    free                     ~        node
__eq__       ~                                                            X
__iter__     X            X
__new__                             X   X
__reduce__   X   ~                                 X
__repr__     X   X                                 X
__reversed__ X       X
__setitem__                                                  key
__sizeof__                     size          X
clear                          ~             ~                        X
copy         X   X                                 X
items        X   X        X
keys         X            X
move_to_end  X                      X   X               ~    h/t key
pop                            free                              key
popitem      X   X             free X   X                        node
setdefault       ~                                      ?    ~
values           X        X
============ === === ==== ==== ==== === ==== ===== ==== ==== === ==== === ===

__delitem__ is the only method that directly relies on finding an arbitrary
node in the linked-list.  Everything else is iteration or relates to the
ends of the linked-list.

Situation that Endangers Consistency
------------------------------------
Using a raw linked-list for OrderedDict exposes a key situation that can
cause problems.  If a node is stored in a variable, there is a chance that
the node may have been deallocated before the variable gets used, thus
potentially leading to a segmentation fault.  A key place where this shows
up is during iteration through the linked list (via _odict_FOREACH or
otherwise).

A number of solutions are available to resolve this situation:

* defer looking up the node until as late as possible and certainly after
  any code that could possibly result in a deletion;
* if the node is needed both before and after a point where the node might
  be removed, do a check before using the node at the "after" location to
  see if the node is still valid;
* like the last one, but simply pull the node again to ensure it's right;
* keep the key in the variable instead of the node and then look up the
  node using the key at the point where the node is needed (this is what
  we do for the iterators).

Another related problem, preserving consistent ordering during iteration,
is described below.  That one is not exclusive to using linked-lists.


Challenges from Subclassing dict
================================

OrderedDict subclasses dict, which is an unusual relationship between two
builtin types (other than the base object type).  Doing so results in
some complication and deserves further explanation.  There are two things
to consider here.  First, in what circumstances or with what adjustments
can OrderedDict be used as a drop-in replacement for dict (at the C level)?
Second, how can the OrderedDict implementation leverage the dict
implementation effectively without introducing unnecessary coupling or
inefficiencies?

This second point is reflected here and in the implementation, so the
further focus is on the first point.  It is worth noting that for
overridden methods, the dict implementation is deferred to as much as
possible.  Furthermore, coupling is limited to as little as is reasonable.

Concrete API Compatibility
--------------------------

Use of the concrete C-API for dict (PyDict_*) with OrderedDict is
problematic.  (See http://bugs.python.org/issue10977.)  The concrete API
has a number of hard-coded assumptions tied to the dict implementation.
This is, in part, due to performance reasons, which is understandable
given the part dict plays in Python.

Any attempt to replace dict with OrderedDict for any role in the
interpreter (e.g. **kwds) faces a challenge.  Such any effort must
recognize that the instances in affected locations currently interact with
the concrete API.

Here are some ways to address this challenge:

1. Change the relevant usage of the concrete API in CPython and add
   PyDict_CheckExact() calls to each of the concrete API funcions.
2. Adjust the relevant concrete API functions to explicitly accommodate
   OrderedDict.
3. As with #1, add the checks, but improve the abstract API with smart fast
   paths for dict and OrderedDict, and refactor CPython to use the abstract
   API.  Improvements to the abstract API would be valuable regardless.

Adding the checks to the concrete API would help make any interpreter
switch to OrderedDict less painful for extension modules.  However, this
won't work.  The equivalent C API call to `dict.__setitem__(obj, k, v)`
is 'PyDict_SetItem(obj, k, v)`.  This illustrates how subclasses in C call
the base class's methods, since there is no equivalent of super() in the
C API.  Calling into Python for parent class API would work, but some
extension modules already rely on this feature of the concrete API.

For reference, here is a breakdown of some of the dict concrete API:

========================== ============= =======================
concrete API               uses          abstract API
========================== ============= =======================
PyDict_Check                             PyMapping_Check
(PyDict_CheckExact)                      -
(PyDict_New)                             -
(PyDictProxy_New)                        -
PyDict_Clear                             -
PyDict_Contains                          PySequence_Contains
PyDict_Copy                              -
PyDict_SetItem                           PyObject_SetItem
PyDict_SetItemString                     PyMapping_SetItemString
PyDict_DelItem                           PyMapping_DelItem
PyDict_DelItemString                     PyMapping_DelItemString
PyDict_GetItem                           -
PyDict_GetItemWithError                  PyObject_GetItem
_PyDict_GetItemIdWithError               -
PyDict_GetItemString                     PyMapping_GetItemString
PyDict_Items                             PyMapping_Items
PyDict_Keys                              PyMapping_Keys
PyDict_Values                            PyMapping_Values
PyDict_Size                              PyMapping_Size
                                         PyMapping_Length
PyDict_Next                              PyIter_Next
_PyDict_Next                             -
PyDict_Merge                             -
PyDict_Update                            -
PyDict_MergeFromSeq2                     -
PyDict_ClearFreeList                     -
-                                        PyMapping_HasKeyString
-                                        PyMapping_HasKey
========================== ============= =======================


The dict Interface Relative to OrderedDict
==========================================

Since OrderedDict subclasses dict, understanding the various methods and
attributes of dict is important for implementing OrderedDict.

Relevant Type Slots
-------------------

================= ================ =================== ================
slot              attribute        object              dict
================= ================ =================== ================
tp_dealloc        -                object_dealloc      dict_dealloc
tp_repr           __repr__         object_repr         dict_repr
sq_contains       __contains__     -                   dict_contains
mp_length         __len__          -                   dict_length
mp_subscript      __getitem__      -                   dict_subscript
mp_ass_subscript  __setitem__      -                   dict_ass_sub
                  __delitem__
tp_hash           __hash__         _Py_HashPointer     ..._HashNotImpl
tp_str            __str__          object_str          -
tp_getattro       __getattribute__ ..._GenericGetAttr  (repeated)
                  __getattr__
tp_setattro       __setattr__      ..._GenericSetAttr  (disabled)
tp_doc            __doc__          (literal)           dictionary_doc
tp_traverse       -                -                   dict_traverse
tp_clear          -                -                   dict_tp_clear
tp_richcompare    __eq__           object_richcompare  dict_richcompare
                  __ne__
tp_weaklistoffset (__weakref__)    -                   -
tp_iter           __iter__         -                   dict_iter
tp_dictoffset     (__dict__)       -                   -
tp_init           __init__         object_init         dict_init
tp_alloc          -                PyType_GenericAlloc (repeated)
tp_new            __new__          object_new          dict_new
tp_free           -                PyObject_Del        PyObject_GC_Del
================= ================ =================== ================

Relevant Methods
----------------

================ =================== ===============
method           object              dict
================ =================== ===============
__reduce__       object_reduce       -
__sizeof__       object_sizeof       dict_sizeof
clear            -                   dict_clear
copy             -                   dict_copy
fromkeys         -                   dict_fromkeys
get              -                   dict_get
items            -                   dictitems_new
keys             -                   dictkeys_new
pop              -                   dict_pop
popitem          -                   dict_popitem
setdefault       -                   dict_setdefault
update           -                   dict_update
values           -                   dictvalues_new
================ =================== ===============


Pure Python OrderedDict
=======================

As already noted, compatibility with the pure Python OrderedDict
implementation is a key goal of this C implementation.  To further that
goal, here's a summary of how OrderedDict-specific methods are implemented
in collections/__init__.py.  Also provided is an indication of which
methods directly mutate or iterate the object, as well as any relationship
with the underlying linked-list.

============= ============== == ================ === === ====
method        impl used      ll uses             inq mut iter
============= ============== == ================ === === ====
__contains__  dict           -  -                X
__delitem__   OrderedDict    Y  dict.__delitem__     X
__eq__        OrderedDict    N  OrderedDict      ~
                                dict.__eq__
                                __iter__
__getitem__   dict           -  -                X
__iter__      OrderedDict    Y  -                        X
__init__      OrderedDict    N  update
__len__       dict           -  -                X
__ne__        MutableMapping -  __eq__           ~
__reduce__    OrderedDict    N  OrderedDict      ~
                                __iter__
                                __getitem__
__repr__      OrderedDict    N  __class__        ~
                                items
__reversed__  OrderedDict    Y  -                        X
__setitem__   OrderedDict    Y  __contains__         X
                                dict.__setitem__
__sizeof__    OrderedDict    Y  __len__          ~
                                __dict__
clear         OrderedDict    Y  dict.clear           X
copy          OrderedDict    N  __class__
                                __init__
fromkeys      OrderedDict    N  __setitem__
get           dict           -  -                ~
items         MutableMapping -  ItemsView                X
keys          MutableMapping -  KeysView                 X
move_to_end   OrderedDict    Y  -                    X
pop           OrderedDict    N  __contains__         X
                                __getitem__
                                __delitem__
popitem       OrderedDict    Y  dict.pop             X
setdefault    OrderedDict    N  __contains__         ~
                                __getitem__
                                __setitem__
update        MutableMapping -  __setitem__          ~
values        MutableMapping -  ValuesView               X
============= ============== == ================ === === ====

__reversed__ and move_to_end are both exclusive to OrderedDict.


C OrderedDict Implementation
============================

================= ================
slot              impl
================= ================
tp_dealloc        odict_dealloc
tp_repr           odict_repr
mp_ass_subscript  odict_ass_sub
tp_doc            odict_doc
tp_traverse       odict_traverse
tp_clear          odict_tp_clear
tp_richcompare    odict_richcompare
tp_weaklistoffset (offset)
tp_iter           odict_iter
tp_dictoffset     (offset)
tp_init           odict_init
tp_alloc          (repeated)
tp_new            odict_new
================= ================

================= ================
method            impl
================= ================
__reduce__        odict_reduce
__sizeof__        odict_sizeof
clear             odict_clear
copy              odict_copy
fromkeys          odict_fromkeys
items             odictitems_new
keys              odictkeys_new
pop               odict_pop
popitem           odict_popitem
setdefault        odict_setdefault
update            odict_update
values            odictvalues_new
================= ================

Inherited unchanged from object/dict:

================ ==========================
method           type field
================ ==========================
-                tp_free
__contains__     tp_as_sequence.sq_contains
__getattr__      tp_getattro
__getattribute__ tp_getattro
__getitem__      tp_as_mapping.mp_subscript
__hash__         tp_hash
__len__          tp_as_mapping.mp_length
__setattr__      tp_setattro
__str__          tp_str
get              -
================ ==========================


Other Challenges
================

Preserving Ordering During Iteration
------------------------------------
During iteration through an OrderedDict, it is possible that items could
get added, removed, or reordered.  For a linked-list implementation, as
with some other implementations, that situation may lead to undefined
behavior.  The documentation for dict mentions this in the `iter()` section
of http://docs.python.org/3.4/library/stdtypes.html#dictionary-view-objects.
In this implementation we follow dict's lead (as does the pure Python
implementation) for __iter__(), keys(), values(), and items().

For internal iteration (using _odict_FOREACH or not), there is still the
risk that not all nodes that we expect to be seen in the loop actually get
seen.  Thus, we are careful in each of those places to ensure that they
are.  This comes, of course, at a small price at each location.  The
solutions are much the same as those detailed in the `Situation that
Endangers Consistency` section above.


Potential Optimizations
=======================

* Allocate the nodes as a block via od_fast_nodes instead of individually.
  - Set node->key to NULL to indicate the node is not-in-use.
  - Add _odict_EXISTS()?
  - How to maintain consistency across resizes?  Existing node pointers
    would be invalidate after a resize, which is particularly problematic
    for the iterators.
* Use a more stream-lined implementation of update() and, likely indirectly,
  __init__().

*/

/* TODO

sooner:
- reentrancy (make sure everything is at a thread-safe state when calling
  into Python).  I've already checked this multiple times, but want to
  make one more pass.
- add unit tests for reentrancy?

later:
- make the dict views support the full set API (the pure Python impl does)
- implement a fuller MutableMapping API in C?
- move the MutableMapping implementation to abstract.c?
- optimize mutablemapping_update
- use PyObject_MALLOC (small object allocator) for odict nodes?
- support subclasses better (e.g. in odict_richcompare)

*/

#include "Python.h"
#include "structmember.h"
#include "dict-common.h"
#include <stddef.h>


typedef struct _odictnode _ODictNode;

/* PyODictObject */
struct _odictobject {
    PyDictObject od_dict;        /* the underlying dict */
    _ODictNode *od_first;        /* first node in the linked list, if any */
    _ODictNode *od_last;         /* last node in the linked list, if any */
    /* od_size and od_fast_nodes are managed by _odict_resize() */
    Py_ssize_t od_size;          /* hash table that mirrors the dict table */
    _ODictNode **od_fast_nodes;  /* managed by _odict_resize() */
    size_t od_state;             /* incremented whenever the LL changes */
    PyObject *od_inst_dict;      /* OrderedDict().__dict__ */
    PyObject *od_weakreflist;    /* holds weakrefs to the odict */
};


/* ----------------------------------------------
 * odict keys (a simple doubly-linked list)
 */

struct _odictnode {
    PyObject *key;
    Py_hash_t hash;
    _ODictNode *next;
    _ODictNode *prev;
};

#define _odictnode_KEY(node) \
    (node->key)
#define _odictnode_HASH(node) \
    (node->hash)
/* borrowed reference */
#define _odictnode_VALUE(node, od) \
    PyODict_GetItemWithError((PyObject *)od, _odictnode_KEY(node))
/* If needed we could also have _odictnode_HASH. */
#define _odictnode_PREV(node) (node->prev)
#define _odictnode_NEXT(node) (node->next)

#define _odict_FIRST(od) (((PyODictObject *)od)->od_first)
#define _odict_LAST(od) (((PyODictObject *)od)->od_last)
#define _odict_EMPTY(od) (_odict_FIRST(od) == NULL)
#define _odict_FOREACH(od, node) \
    for (node = _odict_FIRST(od); node != NULL; node = _odictnode_NEXT(node))


static void
_odict_free_fast_nodes(PyODictObject *od) {
    if (od->od_fast_nodes) {
        PyMem_FREE(od->od_fast_nodes);
    }
}

/* Return the index into the hash table, regardless of a valid node. */
static Py_ssize_t
_odict_get_index_hash(PyODictObject *od, PyObject *key, Py_hash_t hash)
{
    PyObject **value_addr = NULL;
    PyDictKeyEntry *ep;
    PyDictKeysObject *keys = ((PyDictObject *)od)->ma_keys;

    ep = (keys->dk_lookup)((PyDictObject *)od, key, hash, &value_addr);
    if (ep == NULL)
        return -1;
    /* We use pointer arithmetic to get the entry's index into the table. */
    return ep - keys->dk_entries;
}

/* Replace od->od_fast_nodes with a new table matching the size of dict's. */
static int
_odict_resize(PyODictObject *od) {
    Py_ssize_t size, i;
    _ODictNode **fast_nodes, *node;

    /* Initialize a new "fast nodes" table. */
    size = ((PyDictObject *)od)->ma_keys->dk_size;
    fast_nodes = PyMem_NEW(_ODictNode *, size);
    if (fast_nodes == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    for (i = 0; i < size; i++)
        fast_nodes[i] = NULL;

    /* Copy the current nodes into the table. */
    _odict_FOREACH(od, node) {
        i = _odict_get_index_hash(od, _odictnode_KEY(node),
                                  _odictnode_HASH(node));
        if (i < 0) {
            PyMem_FREE(fast_nodes);
            return -1;
        }
        fast_nodes[i] = node;
    }

    /* Replace the old fast nodes table. */
    _odict_free_fast_nodes(od);
    od->od_fast_nodes = fast_nodes;
    od->od_size = size;
    return 0;
}

/* Return the index into the hash table, regardless of a valid node. */
static Py_ssize_t
_odict_get_index(PyODictObject *od, PyObject *key)
{
    Py_hash_t hash;
    PyDictKeysObject *keys;

    assert(key != NULL);
    hash = PyObject_Hash(key);
    if (hash == -1)
        return -1;
    keys = ((PyDictObject *)od)->ma_keys;

    /* Ensure od_fast_nodes and dk_entries are in sync. */
    if (keys->dk_size != od->od_size) {
        int resize_res = _odict_resize(od);
        if (resize_res < 0)
            return -1;
    }

    return _odict_get_index_hash(od, key, hash);
}

static int
_odict_initialize(PyODictObject *od)
{
    od->od_state = 0;
    _odict_FIRST(od) = NULL;
    _odict_LAST(od) = NULL;
    return _odict_resize((PyODictObject *)od);
}

/* Returns NULL if there was some error or the key was not found. */
static _ODictNode *
_odict_find_node(PyODictObject *od, PyObject *key)
{
    Py_ssize_t index;

    if (_odict_EMPTY(od))
        return NULL;
    index = _odict_get_index(od, key);
    if (index < 0)
        return NULL;
    return od->od_fast_nodes[index];
}

static void
_odict_add_head(PyODictObject *od, _ODictNode *node)
{
    if (_odict_FIRST(od) == NULL) {
        _odictnode_PREV(node) = NULL;
        _odictnode_NEXT(node) = NULL;
        _odict_FIRST(od) = node;
        _odict_LAST(od) = node;
    }
    else {
        _odictnode_PREV(node) = NULL;
        _odictnode_NEXT(node) = _odict_FIRST(od);
        _odict_FIRST(od) = node;
        _odictnode_PREV(_odict_FIRST(od)) = node;
    }
    od->od_state++;
}

static void
_odict_add_tail(PyODictObject *od, _ODictNode *node)
{
    if (_odict_LAST(od) == NULL) {
        _odictnode_PREV(node) = NULL;
        _odictnode_NEXT(node) = NULL;
        _odict_FIRST(od) = node;
        _odict_LAST(od) = node;
    }
    else {
        _odictnode_PREV(node) = _odict_LAST(od);
        _odictnode_NEXT(node) = NULL;
        _odictnode_NEXT(_odict_LAST(od)) = node;
        _odict_LAST(od) = node;
    }
    od->od_state++;
}

/* adds the node to the end of the list */
static int
_odict_add_new_node(PyODictObject *od, PyObject *key)
{
    Py_hash_t hash;
    Py_ssize_t i;
    _ODictNode *node;

    Py_INCREF(key);
    hash = PyObject_Hash(key);
    if (hash == -1)
        return -1;

    i = _odict_get_index(od, key);
    if (i < 0) {
        if (!PyErr_Occurred())
            PyErr_SetObject(PyExc_KeyError, key);
        Py_DECREF(key);
        return -1;
    }
    else if (od->od_fast_nodes[i] != NULL) {
        /* We already have a node for the key so there's no need to add one. */
        Py_DECREF(key);
        return 0;
    }

    /* must not be added yet */
    node = (_ODictNode *)PyMem_MALLOC(sizeof(_ODictNode));
    if (node == NULL) {
        Py_DECREF(key);
        PyErr_NoMemory();
        return -1;
    }

    _odictnode_KEY(node) = key;
    _odictnode_HASH(node) = hash;
    _odict_add_tail(od, node);
    od->od_fast_nodes[i] = node;
    return 0;
}

/* Putting the decref after the free causes problems. */
#define _odictnode_DEALLOC(node) \
    do { \
        Py_DECREF(_odictnode_KEY(node)); \
        PyMem_FREE((void *)node); \
    } while (0)

/* Repeated calls on the same node are no-ops. */
static void
_odict_remove_node(PyODictObject *od, _ODictNode *node)
{
    if (_odict_FIRST(od) == node)
        _odict_FIRST(od) = _odictnode_NEXT(node);
    else if (_odictnode_PREV(node) != NULL)
        _odictnode_NEXT(_odictnode_PREV(node)) = _odictnode_NEXT(node);

    if (_odict_LAST(od) == node)
        _odict_LAST(od) = _odictnode_PREV(node);
    else if (_odictnode_NEXT(node) != NULL)
        _odictnode_PREV(_odictnode_NEXT(node)) = _odictnode_PREV(node);

    _odictnode_PREV(node) = NULL;
    _odictnode_NEXT(node) = NULL;
    od->od_state++;
}

static _ODictNode *
_odict_pop_node(PyODictObject *od, _ODictNode *node, PyObject *key)
{
    if (node == NULL) {
        node = _odict_find_node(od, key);
        if (node == NULL)
            return NULL;
    }
    _odict_remove_node(od, node);
    return node;
}

/* If someone calls PyDict_DelItem() directly on an OrderedDict, we'll
   get all sorts of problems here.  In PyODict_DelItem we make sure to
   call _odict_clear_node first.
 
   This matters in the case of colliding keys.  Suppose we add 3 keys:
   [A, B, C], where the hash of C collides with A and the next possible
   index in the hash table is occupied by B.  If we remove B then for C
   the dict's looknode func will give us the old index of B instead of
   the index we got before deleting B.  However, the node for C in
   od_fast_nodes is still at the old dict index of C.  Thus to be sure
   things don't get out of sync, we clear the node in od_fast_nodes
   *before* calling PyDict_DelItem.

   The same must be done for any other OrderedDict operations where
   we modify od_fast_nodes.
*/
static int
_odict_clear_node(PyODictObject *od, _ODictNode *node, PyObject *key)
{
    Py_ssize_t i;

    assert(key != NULL);
    if (_odict_EMPTY(od)) {
        /* Let later code decide if this is a KeyError. */
        return 0;
    }

    i = _odict_get_index(od, key);
    if (i < 0)
        return PyErr_Occurred() ? -1 : 0;

    if (node == NULL)
        node = od->od_fast_nodes[i];
    assert(node == od->od_fast_nodes[i]);
    if (node == NULL) {
        /* Let later code decide if this is a KeyError. */
        return 0;
    }

    // Now clear the node.
    od->od_fast_nodes[i] = NULL;
    _odict_remove_node(od, node);
    _odictnode_DEALLOC(node);
    return 0;
}

static void
_odict_clear_nodes(PyODictObject *od)
{
    _ODictNode *node, *next;

    if (!_odict_EMPTY(od)) {
        node = _odict_FIRST(od);
        while (node != NULL) {
            next = _odictnode_NEXT(node);
            _odictnode_DEALLOC(node);
            node = next;
        }
        _odict_FIRST(od) = NULL;
        _odict_LAST(od) = NULL;
    }

    _odict_free_fast_nodes(od);
    od->od_fast_nodes = NULL;
}

/* There isn't any memory management of nodes past this point. */
#undef _odictnode_DEALLOC

static int
_odict_keys_equal(PyODictObject *a, PyODictObject *b)
{
    _ODictNode *node_a, *node_b;

    node_a = _odict_FIRST(a);
    node_b = _odict_FIRST(b);
    while (1) {
        if (node_a == NULL && node_b == NULL)
            /* success: hit the end of each at the same time */
            return 1;
        else if (node_a == NULL || node_b == NULL)
            /* unequal length */
            return 0;
        else {
            int res = PyObject_RichCompareBool(
                                            (PyObject *)_odictnode_KEY(node_a),
                                            (PyObject *)_odictnode_KEY(node_b),
                                            Py_EQ);
            if (res < 0)
                return res;
            else if (res == 0)
                return 0;

            /* otherwise it must match, so move on to the next one */
            node_a = _odictnode_NEXT(node_a);
            node_b = _odictnode_NEXT(node_b);
        }
    }
}


/* ----------------------------------------------
 * OrderedDict mapping methods
 */

/* mp_ass_subscript: __setitem__() and __delitem__() */

static int
odict_mp_ass_sub(PyODictObject *od, PyObject *v, PyObject *w)
{
    if (w == NULL)
        return PyODict_DelItem((PyObject *)od, v);
    else
        return PyODict_SetItem((PyObject *)od, v, w);
}

/* tp_as_mapping */

static PyMappingMethods odict_as_mapping = {
    0,                                  /*mp_length*/
    0,                                  /*mp_subscript*/
    (objobjargproc)odict_mp_ass_sub,    /*mp_ass_subscript*/
};


/* ----------------------------------------------
 * OrderedDict methods
 */

/* __delitem__() */

PyDoc_STRVAR(odict_delitem__doc__, "od.__delitem__(y) <==> del od[y]");

/* __eq__() */

PyDoc_STRVAR(odict_eq__doc__,
"od.__eq__(y) <==> od==y.  Comparison to another OD is order-sensitive \n\
        while comparison to a regular mapping is order-insensitive.\n\
        ");

/* forward */
static PyObject * odict_richcompare(PyObject *v, PyObject *w, int op);

static PyObject *
odict_eq(PyObject *a, PyObject *b)
{
    return odict_richcompare(a, b, Py_EQ);
}

/* __init__() */

PyDoc_STRVAR(odict_init__doc__,
"Initialize an ordered dictionary.  The signature is the same as\n\
        regular dictionaries, but keyword arguments are not recommended because\n\
        their insertion order is arbitrary.\n\
\n\
        ");

/* forward */
static int odict_init(PyObject *self, PyObject *args, PyObject *kwds);

/* __iter__() */

PyDoc_STRVAR(odict_iter__doc__, "od.__iter__() <==> iter(od)");

static PyObject * odict_iter(PyODictObject *self);  /* forward */

/* __ne__() */

/* Mapping.__ne__() does not have a docstring. */
PyDoc_STRVAR(odict_ne__doc__, "");

static PyObject *
odict_ne(PyObject *a, PyObject *b)
{
    return odict_richcompare(a, b, Py_NE);
}

/* __repr__() */

PyDoc_STRVAR(odict_repr__doc__, "od.__repr__() <==> repr(od)");

static PyObject * odict_repr(PyODictObject *self);  /* forward */

/* __setitem__() */

PyDoc_STRVAR(odict_setitem__doc__, "od.__setitem__(i, y) <==> od[i]=y");

/* fromkeys() */

PyDoc_STRVAR(odict_fromkeys__doc__,
"OD.fromkeys(S[, v]) -> New ordered dictionary with keys from S.\n\
        If not specified, the value defaults to None.\n\
\n\
        ");

static PyObject *
odict_fromkeys(PyObject *cls, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"iterable", "value", 0};
    PyObject *seq;
    PyObject *value = Py_None;

    /* both borrowed */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:fromkeys", kwlist,
                                     &seq, &value)) {
        return NULL;
    }
    return _PyDict_FromKeys(cls, seq, value);
}

/* __sizeof__() */

/* OrderedDict.__sizeof__() does not have a docstring. */
PyDoc_STRVAR(odict_sizeof__doc__, "");

static PyObject *
odict_sizeof(PyODictObject *od)
{
    PyObject *pylong;
    Py_ssize_t res, temp;

    pylong = _PyDict_SizeOf((PyDictObject *)od);
    if (pylong == NULL)
        return NULL;
    res = PyLong_AsSsize_t(pylong);
    Py_DECREF(pylong);
    if (res == -1 && PyErr_Occurred())
        return NULL;

    res += sizeof(PyODictObject) - sizeof(PyDictObject);

    /* instance dict */
    pylong = _PyDict_SizeOf((PyDictObject *)od->od_inst_dict);
    if (pylong == NULL)
        return NULL;
    temp = PyLong_AsSsize_t(pylong);
    Py_DECREF(pylong);
    if (temp == -1 && PyErr_Occurred())
        return NULL;
    res += temp;

    res += sizeof(_ODictNode) * od->od_size;  /* od_fast_nodes */
    if (!_odict_EMPTY(od)) {
        res += sizeof(_ODictNode) * PyODict_SIZE(od);  /* linked-list */
    }
    return PyLong_FromSsize_t(res);
}

/* __reduce__() */

PyDoc_STRVAR(odict_reduce__doc__, "Return state information for pickling");

static PyObject *
odict_reduce(register PyODictObject *od)
{
    _Py_IDENTIFIER(__dict__);
    _Py_IDENTIFIER(__class__);
    PyObject *vars = NULL, *ns = NULL, *result = NULL, *cls = NULL;
    PyObject *items_iter = NULL, *items = NULL, *args = NULL;

    /* capture any instance state */
    vars = _PyObject_GetAttrId((PyObject *)od, &PyId___dict__);
    if (vars == NULL)
        goto Done;
    else {
        PyObject *empty, *od_vars, *iterator, *key;
        int ns_len;

        /* od.__dict__ isn't necessarily a dict... */
        ns = PyObject_CallMethod((PyObject *)vars, "copy", NULL);
        if (ns == NULL)
            goto Done;
        empty = PyODict_New();
        if (empty == NULL)
            goto Done;
        od_vars = _PyObject_GetAttrId((PyObject *)empty, &PyId___dict__);
        Py_DECREF(empty);
        if (od_vars == NULL)
            goto Done;
        iterator = PyObject_GetIter(od_vars);
        Py_DECREF(od_vars);
        if (iterator == NULL)
            goto Done;

        while ( (key = PyIter_Next(iterator)) ) {
            if (PyMapping_HasKey(ns, key) && PyMapping_DelItem(ns, key) != 0) {
                Py_DECREF(iterator);
                Py_DECREF(key);
                goto Done;
            }
            Py_DECREF(key);
        }
        Py_DECREF(iterator);
        if (PyErr_Occurred())
            goto Done;

        ns_len = PyObject_Length(ns);
        if (ns_len == -1)
            goto Done;
        if (!ns_len) {
            /* nothing novel to pickle in od.__dict__ */
            Py_CLEAR(ns);
        }
    }

    /* build the result */
    cls = _PyObject_GetAttrId((PyObject *)od, &PyId___class__);
    if (cls == NULL)
        goto Done;

    args = PyTuple_New(0);
    if (args == NULL)
        goto Done;

    items = PyObject_CallMethod((PyObject *)od, "items", NULL);
    if (items == NULL)
        goto Done;

    items_iter = PyObject_GetIter(items);
    if (items_iter == NULL)
        goto Done;

    result = PyTuple_Pack(5, cls, args, ns ? ns : Py_None, Py_None, items_iter);

Done:
    Py_XDECREF(vars);
    Py_XDECREF(ns);
    Py_XDECREF(cls);
    Py_XDECREF(args);
    Py_XDECREF(items);
    Py_XDECREF(items_iter);

    return result;
}

/* setdefault() */

PyDoc_STRVAR(odict_setdefault__doc__,
        "od.setdefault(k[,d]) -> od.get(k,d), also set od[k]=d if k not in od");

/* Skips __missing__() calls. */
static PyObject *
odict_setdefault(register PyODictObject *od, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"key", "default", 0};
    PyObject *key, *result = NULL;
    PyObject *failobj = Py_None;

    /* both borrowed */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:setdefault", kwlist,
                                     &key, &failobj)) {
        return NULL;
    }

    if (PyODict_CheckExact(od)) {
        result = PyODict_GetItemWithError(od, key);  /* borrowed */
        if (result == NULL) {
            if (PyErr_Occurred())
                return NULL;
            assert(_odict_find_node(od, key) == NULL);
            if (PyODict_SetItem((PyObject *)od, key, failobj) >= 0) {
                result = failobj;
                Py_INCREF(failobj);
            }
        }
        else {
            Py_INCREF(result);
        }
    }
    else {
        int exists = PySequence_Contains((PyObject *)od, key);
        if (exists < 0) {
            return NULL;
        }
        else if (exists) {
            result = PyObject_GetItem((PyObject *)od, key);
        }
        else if (PyObject_SetItem((PyObject *)od, key, failobj) >= 0) {
            result = failobj;
            Py_INCREF(failobj);
        }
    }

    return result;
}

/* pop() */

PyDoc_STRVAR(odict_pop__doc__,
"od.pop(k[,d]) -> v, remove specified key and return the corresponding\n\
        value.  If key is not found, d is returned if given, otherwise KeyError\n\
        is raised.\n\
\n\
        ");

/* forward */
static PyObject * _odict_popkey(PyObject *, PyObject *, PyObject *);

/* Skips __missing__() calls. */
static PyObject *
odict_pop(PyObject *od, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"key", "default", 0};
    PyObject *key, *failobj = NULL;

    /* borrowed */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:pop", kwlist,
                                     &key, &failobj)) {
        return NULL;
    }

    return _odict_popkey(od, key, failobj);
}

static PyObject *
_odict_popkey(PyObject *od, PyObject *key, PyObject *failobj)
{
    _ODictNode *node;
    PyObject *value = NULL;

    /* Pop the node first to avoid a possible dict resize (due to
       eval loop reentrancy) and complications due to hash collision
       resolution. */
    node = _odict_find_node((PyODictObject *)od, key);
    if (node == NULL) {
        if (PyErr_Occurred())
            return NULL;
    }
    else {
        int res = _odict_clear_node((PyODictObject *)od, node, key);
        if (res < 0) {
            return NULL;
        }
    }

    /* Now delete the value from the dict. */
    if (PyODict_CheckExact(od)) {
        if (node != NULL) {
            /* We could do PyDict_GetItem() and PyDict_DelItem() directly... */
            value = _PyDict_Pop((PyDictObject *)od, key, failobj);
        }
    }
    else {
        int exists = PySequence_Contains(od, key);
        if (exists < 0)
            return NULL;
        if (exists) {
            value = PyObject_GetItem(od, key);
            if (value != NULL) {
                if (PyObject_DelItem(od, key) == -1) {
                    Py_CLEAR(value);
                }
            }
        }
    }

    /* Apply the fallback value, if necessary. */
    if (value == NULL && !PyErr_Occurred()) {
        if (failobj) {
            value = failobj;
            Py_INCREF(failobj);
        }
        else {
            PyErr_SetObject(PyExc_KeyError, key);
        }
    }

    return value;
}

/* popitem() */

PyDoc_STRVAR(odict_popitem__doc__,
"od.popitem() -> (k, v), return and remove a (key, value) pair.\n\
        Pairs are returned in LIFO order if last is true or FIFO order if false.\n\
\n\
        ");

static PyObject *
odict_popitem(PyObject *od, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"last", 0};
    PyObject *key, *value, *item = NULL, *last = NULL;
    _ODictNode *node;
    int pos = -1;

    if (_odict_EMPTY((PyODictObject *)od)) {
        PyErr_SetString(PyExc_KeyError, "dictionary is empty");
        return NULL;
    }

    /* pull the item */

    /* borrowed */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:popitem", kwlist,
                                     &last)) {
        return NULL;
    }

    if (last != NULL) {
        int is_true;
        is_true = PyObject_IsTrue(last);
        if (is_true == -1)
            return NULL;
        pos = is_true ? -1 : 0;
    }
    if (pos == 0)
        node = _odict_FIRST((PyODictObject *)od);
    else
        node = _odict_LAST((PyODictObject *)od);

    key = _odictnode_KEY(node);
    Py_INCREF(key);
    value = _odict_popkey(od, key, NULL);
    if (value == NULL)
        return NULL;
    item = PyTuple_Pack(2, key, value);
    Py_DECREF(key);
    Py_DECREF(value);
    return item;
}

/* keys() */

/* MutableMapping.keys() does not have a docstring. */
PyDoc_STRVAR(odict_keys__doc__, "");

static PyObject * odictkeys_new(PyObject *od);  /* forward */

/* values() */

/* MutableMapping.values() does not have a docstring. */
PyDoc_STRVAR(odict_values__doc__, "");

static PyObject * odictvalues_new(PyObject *od);  /* forward */

/* items() */

/* MutableMapping.items() does not have a docstring. */
PyDoc_STRVAR(odict_items__doc__, "");

static PyObject * odictitems_new(PyObject *od);  /* forward */

/* update() */

/* MutableMapping.update() does not have a docstring. */
PyDoc_STRVAR(odict_update__doc__, "");

/* forward */
static PyObject * mutablemapping_update(PyObject *, PyObject *, PyObject *);

#define odict_update mutablemapping_update

/* clear() */

PyDoc_STRVAR(odict_clear__doc__,
             "od.clear() -> None.  Remove all items from od.");

static PyObject *
odict_clear(register PyODictObject *od)
{
    PyDict_Clear((PyObject *)od);
    _odict_clear_nodes(od);
    _odict_FIRST(od) = NULL;
    _odict_LAST(od) = NULL;
    if (_odict_resize(od) < 0)
        return NULL;
    Py_RETURN_NONE;
}

/* copy() */

PyDoc_STRVAR(odict_copy__doc__, "od.copy() -> a shallow copy of od");

static PyObject *
odict_copy(register PyODictObject *od)
{
    _ODictNode *node;
    PyObject *od_copy;

    if (PyODict_CheckExact(od))
        od_copy = PyODict_New();
    else
        od_copy = PyObject_CallFunctionObjArgs((PyObject *)Py_TYPE(od), NULL);
    if (od_copy == NULL)
        return NULL;

    if (PyODict_CheckExact(od)) {
        _odict_FOREACH(od, node) {
            PyObject *key = _odictnode_KEY(node);
            PyObject *value = _odictnode_VALUE(node, od);
            if (value == NULL) {
                if (!PyErr_Occurred())
                    PyErr_SetObject(PyExc_KeyError, key);
                goto fail;
            }
            if (PyODict_SetItem((PyObject *)od_copy, key, value) != 0)
                goto fail;
        }
    }
    else {
        _odict_FOREACH(od, node) {
            int res;
            PyObject *value = PyObject_GetItem((PyObject *)od,
                                               _odictnode_KEY(node));
            if (value == NULL)
                goto fail;
            res = PyObject_SetItem((PyObject *)od_copy,
                                   _odictnode_KEY(node), value);
            Py_DECREF(value);
            if (res != 0)
                goto fail;
        }
    }
    return od_copy;

fail:
    Py_DECREF(od_copy);
    return NULL;
}

/* __reversed__() */

PyDoc_STRVAR(odict_reversed__doc__, "od.__reversed__() <==> reversed(od)");

#define _odict_ITER_REVERSED 1
#define _odict_ITER_KEYS 2
#define _odict_ITER_VALUES 4

/* forward */
static PyObject * odictiter_new(PyODictObject *, int);

static PyObject *
odict_reversed(PyODictObject *od)
{
    return odictiter_new(od, _odict_ITER_KEYS|_odict_ITER_REVERSED);
}

/* move_to_end() */

PyDoc_STRVAR(odict_move_to_end__doc__,
"Move an existing element to the end (or beginning if last==False).\n\
\n\
        Raises KeyError if the element does not exist.\n\
        When last=True, acts like a fast version of self[key]=self.pop(key).\n\
\n\
        ");

static PyObject *
odict_move_to_end(PyODictObject *od, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"key", "last", 0};
    PyObject *key, *last = NULL;
    Py_ssize_t pos = -1;

    /* both borrowed */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:move_to_end", kwlist,
                                     &key, &last)) {
        return NULL;
    }
    if (_odict_EMPTY(od)) {
        PyErr_SetObject(PyExc_KeyError, key);
        return NULL;
    }
    if (last != NULL) {
        int is_true;
        is_true = PyObject_IsTrue(last);
        if (is_true == -1)
            return NULL;
        pos = is_true ? -1 : 0;
    }
    if (pos == 0) {
        /* Only move if not already the first one. */
        PyObject *first_key = _odictnode_KEY(_odict_FIRST(od));
        int not_equal = PyObject_RichCompareBool(key, first_key, Py_NE);
        if (not_equal == -1)
            return NULL;
        if (not_equal) {
            _ODictNode *node = _odict_pop_node(od, NULL, key);
            if (node != NULL) {
                _odict_add_head(od, node);
            }
            else {
                if (!PyErr_Occurred())
                    PyErr_SetObject(PyExc_KeyError, key);
                return NULL;
            }
        }
    }
    else if (pos == -1) {
        /* Only move if not already the last one. */
        PyObject *last_key = _odictnode_KEY(_odict_LAST(od));
        int not_equal = PyObject_RichCompareBool(key, last_key, Py_NE);
        if (not_equal == -1)
            return NULL;
        if (not_equal) {
            _ODictNode *node = _odict_pop_node(od, NULL, key);
            if (node != NULL) {
                _odict_add_tail(od, node);
            }
            else {
                if (!PyErr_Occurred())
                    PyErr_SetObject(PyExc_KeyError, key);
                return NULL;
            }
        }
    }
    Py_RETURN_NONE;
}


/* tp_methods */

static PyMethodDef odict_methods[] = {

    /* explicitly defined so we can align docstrings with
     * collections.OrderedDict */
    {"__delitem__",     (PyCFunction)odict_mp_ass_sub,  METH_NOARGS,
     odict_delitem__doc__},
    {"__eq__",          (PyCFunction)odict_eq,          METH_NOARGS,
     odict_eq__doc__},
    {"__init__",        (PyCFunction)odict_init,        METH_NOARGS,
     odict_init__doc__},
    {"__iter__",        (PyCFunction)odict_iter,        METH_NOARGS,
     odict_iter__doc__},
    {"__ne__",          (PyCFunction)odict_ne,          METH_NOARGS,
     odict_ne__doc__},
    {"__repr__",        (PyCFunction)odict_repr,        METH_NOARGS,
     odict_repr__doc__},
    {"__setitem__",     (PyCFunction)odict_mp_ass_sub,  METH_NOARGS,
     odict_setitem__doc__},
    {"fromkeys",        (PyCFunction)odict_fromkeys,
     METH_VARARGS | METH_KEYWORDS | METH_CLASS, odict_fromkeys__doc__},

    /* overridden dict methods */
    {"__sizeof__",      (PyCFunction)odict_sizeof,      METH_NOARGS,
     odict_sizeof__doc__},
    {"__reduce__",      (PyCFunction)odict_reduce,      METH_NOARGS,
     odict_reduce__doc__},
    {"setdefault",      (PyCFunction)odict_setdefault,
     METH_VARARGS | METH_KEYWORDS, odict_setdefault__doc__},
    {"pop",             (PyCFunction)odict_pop,
     METH_VARARGS | METH_KEYWORDS, odict_pop__doc__},
    {"popitem",         (PyCFunction)odict_popitem,
     METH_VARARGS | METH_KEYWORDS, odict_popitem__doc__},
    {"keys",            (PyCFunction)odictkeys_new,     METH_NOARGS,
     odict_keys__doc__},
    {"values",          (PyCFunction)odictvalues_new,   METH_NOARGS,
     odict_values__doc__},
    {"items",           (PyCFunction)odictitems_new,    METH_NOARGS,
     odict_items__doc__},
    {"update",          (PyCFunction)odict_update, METH_VARARGS | METH_KEYWORDS,
     odict_update__doc__},
    {"clear",           (PyCFunction)odict_clear,       METH_NOARGS,
     odict_clear__doc__},
    {"copy",            (PyCFunction)odict_copy,        METH_NOARGS,
     odict_copy__doc__},

    /* new methods */
    {"__reversed__",    (PyCFunction)odict_reversed,    METH_NOARGS,
     odict_reversed__doc__},
    {"move_to_end",     (PyCFunction)odict_move_to_end,
     METH_VARARGS | METH_KEYWORDS, odict_move_to_end__doc__},

    {NULL,              NULL}   /* sentinel */
};


/* ----------------------------------------------
 * OrderedDict members
 */

/* tp_members */

static PyMemberDef odict_members[] = {
    {"__dict__", T_OBJECT, offsetof(PyODictObject, od_inst_dict), READONLY},
    {0}
};


/* ----------------------------------------------
 * OrderedDict type slot methods
 */

/* tp_dealloc */

static void
odict_dealloc(PyODictObject *self)
{
    PyObject_GC_UnTrack(self);
    Py_TRASHCAN_SAFE_BEGIN(self);
    Py_XDECREF(self->od_inst_dict);
    if (self->od_weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *)self);

    _odict_clear_nodes(self);
    Py_TRASHCAN_SAFE_END(self);

    /* must be last */
    PyDict_Type.tp_dealloc((PyObject *)self);
};

/* tp_repr */

static PyObject *
odict_repr(PyODictObject *self)
{
    int i;
    const char *formatstr;
    _Py_IDENTIFIER(__class__);
    _Py_IDENTIFIER(__name__);
    Py_ssize_t count = -1;
    PyObject *pieces = NULL, *result = NULL, *cls = NULL;
    PyObject *classname = NULL, *format = NULL, *args = NULL;

    i = Py_ReprEnter((PyObject *)self);
    if (i != 0) {
        return i > 0 ? PyUnicode_FromString("...") : NULL;
    }

    if (PyODict_SIZE(self) == 0) {
        /* "OrderedDict()" */
        goto Finish;
    }

    if (PyODict_CheckExact(self)) {
        _ODictNode *node;
        pieces = PyList_New(PyODict_SIZE(self));
        if (pieces == NULL)
            goto Done;

        _odict_FOREACH(self, node) {
            PyObject *pair;
            PyObject *key = _odictnode_KEY(node);
            PyObject *value = _odictnode_VALUE(node, self);
            if (value == NULL) {
                if (!PyErr_Occurred())
                    PyErr_SetObject(PyExc_KeyError, key);
                goto Done;
            }
            pair = PyTuple_Pack(2, key, value);
            if (pair == NULL)
                goto Done;

            PyList_SET_ITEM(pieces, ++count, pair);  /* steals reference */
        }
    }
    else {
        PyObject *items = PyObject_CallMethod((PyObject *)self, "items", NULL);
        if (items == NULL)
            goto Done;
        pieces = PySequence_List(items);
        Py_DECREF(items);
        if(pieces == NULL)
            goto Done;
    }

Finish:
    cls = _PyObject_GetAttrId((PyObject *)self, &PyId___class__);
    if (cls == NULL)
        goto Done;
    classname = _PyObject_GetAttrId(cls, &PyId___name__);
    if (classname == NULL)
        goto Done;

    if (pieces == NULL) {
        formatstr = "%s()";
        args = PyTuple_Pack(1, classname);
    }
    else {
        formatstr = "%s(%r)";
        args = PyTuple_Pack(2, classname, pieces);
    }
    if (args == NULL)
        goto Done;

    format = PyUnicode_InternFromString(formatstr);
    if (format == NULL)
        goto Done;

    result = PyUnicode_Format(format, args);
Done:
    Py_XDECREF(pieces);
    Py_XDECREF(cls);
    Py_XDECREF(classname);
    Py_XDECREF(format);
    Py_XDECREF(args);
    Py_ReprLeave((PyObject *)self);
    return result;
};

/* tp_doc */

PyDoc_STRVAR(odict_doc,
        "Dictionary that remembers insertion order");

/* tp_traverse */

static int
odict_traverse(PyODictObject *od, visitproc visit, void *arg)
{
    Py_VISIT(od->od_inst_dict);
    Py_VISIT(od->od_weakreflist);
    return PyDict_Type.tp_traverse((PyObject *)od, visit, arg);
}

/* tp_clear */

static int
odict_tp_clear(PyODictObject *od)
{
    PyObject *res;
    Py_CLEAR(od->od_inst_dict);
    Py_CLEAR(od->od_weakreflist);
    res = odict_clear(od);
    if (res == NULL)
        return -1;
    Py_DECREF(res);
    return 0;
}

/* tp_richcompare */

static PyObject *
odict_richcompare(PyObject *v, PyObject *w, int op)
{
    if (!PyODict_Check(v) || !PyDict_Check(w)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (op == Py_EQ || op == Py_NE) {
        PyObject *res, *cmp;
        int eq;

        cmp = PyDict_Type.tp_richcompare(v, w, op);
        if (cmp == NULL)
            return NULL;
        if (!PyODict_Check(w))
            return cmp;
        if (op == Py_EQ && cmp == Py_False)
            return cmp;
        if (op == Py_NE && cmp == Py_True)
            return cmp;
        Py_DECREF(cmp);

        /* Try comparing odict keys. */
        eq = _odict_keys_equal((PyODictObject *)v, (PyODictObject *)w);
        if (eq < 0)
            return NULL;

        res = (eq == (op == Py_EQ)) ? Py_True : Py_False;
        Py_INCREF(res);
        return res;
    } else {
        Py_RETURN_NOTIMPLEMENTED;
    }
};

/* tp_iter */

static PyObject *
odict_iter(PyODictObject *od)
{
    return odictiter_new(od, _odict_ITER_KEYS);
};

/* tp_init */

static int
odict_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *res;
    Py_ssize_t len = PyObject_Length(args);

    if (len == -1)
        return -1;
    if (len > 1) {
        char *msg = "expected at most 1 arguments, got %d";
        PyErr_Format(PyExc_TypeError, msg, len);
        return -1;
    }

    /* __init__() triggering update() is just the way things are! */
    res = odict_update(self, args, kwds);
    if (res == NULL) {
        return -1;
    } else {
        Py_DECREF(res);
        return 0;
    }
};

/* tp_new */

static PyObject *
odict_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *od = PyDict_Type.tp_new(type, args, kwds);
    if (od != NULL) {
        if (_odict_initialize((PyODictObject *)od) < 0)
            return NULL;
        ((PyODictObject *)od)->od_inst_dict = PyDict_New();
        ((PyODictObject *)od)->od_weakreflist = NULL;
    }
    return od;
}

/* PyODict_Type */

PyTypeObject PyODict_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "collections.OrderedDict",                  /* tp_name */
    sizeof(PyODictObject),                      /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)odict_dealloc,                  /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)odict_repr,                       /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    &odict_as_mapping,                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    odict_doc,                                  /* tp_doc */
    (traverseproc)odict_traverse,               /* tp_traverse */
    (inquiry)odict_tp_clear,                    /* tp_clear */
    (richcmpfunc)odict_richcompare,             /* tp_richcompare */
    offsetof(PyODictObject, od_weakreflist),    /* tp_weaklistoffset */
    (getiterfunc)odict_iter,                    /* tp_iter */
    0,                                          /* tp_iternext */
    odict_methods,                              /* tp_methods */
    odict_members,                              /* tp_members */
    0,                                          /* tp_getset */
    &PyDict_Type,                               /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    offsetof(PyODictObject, od_inst_dict),      /* tp_dictoffset */
    (initproc)odict_init,                       /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    (newfunc)odict_new,                         /* tp_new */
    0,                                          /* tp_free */
};


/* ----------------------------------------------
 * the public OrderedDict API
 */

PyObject *
PyODict_New(void) {
    return odict_new(&PyODict_Type, NULL, NULL);
};

int
PyODict_SetItem(PyObject *od, PyObject *key, PyObject *value) {
    int res = PyDict_SetItem(od, key, value);
    if (res == 0) {
        res = _odict_add_new_node((PyODictObject *)od, key);
        /* XXX Revert setting the value on the dict? */
    }
    return res;
};

int
PyODict_DelItem(PyObject *od, PyObject *key) {
    int res = _odict_clear_node((PyODictObject *)od, NULL, key);
    if (res < 0)
        return -1;
    return PyDict_DelItem(od, key);
};


/* -------------------------------------------
 * The OrderedDict views (keys/values/items)
 */

typedef struct {
    PyObject_HEAD
    int kind;
    PyODictObject *di_odict;
    Py_ssize_t di_size;
    size_t di_state;
    PyObject *di_current;
    PyObject *di_result; /* reusable result tuple for iteritems */
} odictiterobject;

static void
odictiter_dealloc(odictiterobject *di)
{
    _PyObject_GC_UNTRACK(di);
    Py_XDECREF(di->di_odict);
    Py_XDECREF(di->di_current);
    if (di->kind & (_odict_ITER_KEYS | _odict_ITER_VALUES)) {
        Py_DECREF(di->di_result);
    }
    PyObject_GC_Del(di);
}

static int
odictiter_traverse(odictiterobject *di, visitproc visit, void *arg)
{
    Py_VISIT(di->di_odict);
    Py_VISIT(di->di_current);  /* A key could be any type, not just str. */
    Py_VISIT(di->di_result);
    return 0;
}

/* In order to protect against modifications during iteration, we track
 * the current key instead of the current node. */
static PyObject *
odictiter_nextkey(odictiterobject *di)
{
    PyObject *key = NULL;
    _ODictNode *node;
    int reversed = di->kind & _odict_ITER_REVERSED;

    if (di->di_odict == NULL)
        return NULL;
    if (di->di_current == NULL)
        goto done;  /* We're already done. */

    /* Check for unsupported changes. */
    if (di->di_odict->od_state != di->di_state) {
        PyErr_SetString(PyExc_RuntimeError,
                        "OrderedDict mutated during iteration");
        goto done;
    }
    if (di->di_size != PyODict_SIZE(di->di_odict)) {
        PyErr_SetString(PyExc_RuntimeError,
                        "OrderedDict changed size during iteration");
        di->di_size = -1; /* Make this state sticky */
        return NULL;
    }

    /* Get the key. */
    node = _odict_find_node(di->di_odict, di->di_current);
    if (node == NULL) {
        /* Must have been deleted. */
        Py_CLEAR(di->di_current);
        return NULL;
    }
    key = di->di_current;

    /* Advance to the next key. */
    node = reversed ? _odictnode_PREV(node) : _odictnode_NEXT(node);
    if (node == NULL) {
        /* Reached the end. */
        di->di_current = NULL;
    }
    else {
        di->di_current = _odictnode_KEY(node);
        Py_INCREF(di->di_current);
    }

    return key;

done:
    Py_CLEAR(di->di_odict);
    return key;
}

static PyObject *
odictiter_iternext(odictiterobject *di)
{
    PyObject *value;
    PyObject *key = odictiter_nextkey(di);  /* new reference */

    if (key == NULL)
        return NULL;

    /* Handle the keys case. */
    if (! (di->kind & _odict_ITER_VALUES)) {
        return key;
    }

    /* Handle the items case. */
    if (di->kind & _odict_ITER_KEYS) {
        PyObject *result = di->di_result;

        value = PyODict_GetItem((PyObject *)di->di_odict, key);  /* borrowed */
        if (value == NULL) {
            if (!PyErr_Occurred())
                PyErr_SetObject(PyExc_KeyError, key);
            Py_DECREF(key);
            goto done;
        }
        Py_INCREF(value);

        if (result->ob_refcnt == 1) {
            /* not in use so we can reuse it
             * (the common case during iteration) */
            Py_INCREF(result);
            Py_DECREF(PyTuple_GET_ITEM(result, 0));  /* borrowed */
            Py_DECREF(PyTuple_GET_ITEM(result, 1));  /* borrowed */
        }
        else {
            result = PyTuple_New(2);
            if (result == NULL) {
                Py_DECREF(key);
                Py_DECREF(value);
                goto done;
            }
        }

        PyTuple_SET_ITEM(result, 0, key);  /* steals reference */
        PyTuple_SET_ITEM(result, 1, value);  /* steals reference */

        return result;
    }
    /* Handle the values case. */
    else {
        value = PyODict_GetItem((PyObject *)di->di_odict, key);
        Py_DECREF(key);
        if (value == NULL) {
            if (!PyErr_Occurred())
                PyErr_SetObject(PyExc_KeyError, key);
            goto done;
        }
        Py_INCREF(value);
        return value;
    }

done:
    Py_CLEAR(di->di_current);
    Py_CLEAR(di->di_odict);
    return NULL;
}

/* No need for tp_clear because odictiterobject is not mutable. */

PyDoc_STRVAR(reduce_doc, "Return state information for pickling");

static PyObject *
odictiter_reduce(odictiterobject *di)
{
    PyObject *list, *iter;

    list = PyList_New(0);
    if (!list)
        return NULL;

    /* iterate the temporary into a list */
    for(;;) {
        PyObject *element = odictiter_iternext(di);
        if (element) {
            if (PyList_Append(list, element)) {
                Py_DECREF(element);
                Py_DECREF(list);
                return NULL;
            }
            Py_DECREF(element);
        }
        else {
            /* done iterating? */
            break;
        }
    }
    if (PyErr_Occurred()) {
        Py_DECREF(list);
        return NULL;
    }
    iter = _PyObject_GetBuiltin("iter");
    if (iter == NULL) {
        Py_DECREF(list);
        return NULL;
    }
    return Py_BuildValue("N(N)", iter, list);
}

static PyMethodDef odictiter_methods[] = {
    {"__reduce__", (PyCFunction)odictiter_reduce, METH_NOARGS, reduce_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyODictIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "odict_iterator",                         /* tp_name */
    sizeof(odictiterobject),                  /* tp_basicsize */
    0,                                        /* tp_itemsize */
    /* methods */
    (destructor)odictiter_dealloc,            /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_reserved */
    0,                                        /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash */
    0,                                        /* tp_call */
    0,                                        /* tp_str */
    PyObject_GenericGetAttr,                  /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,  /* tp_flags */
    0,                                        /* tp_doc */
    (traverseproc)odictiter_traverse,         /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    PyObject_SelfIter,                        /* tp_iter */
    (iternextfunc)odictiter_iternext,         /* tp_iternext */
    odictiter_methods,                        /* tp_methods */
    0,
};

static PyObject *
odictiter_new(PyODictObject *od, int kind)
{
    odictiterobject *di;
    _ODictNode *node;
    int reversed = kind & _odict_ITER_REVERSED;

    di = PyObject_GC_New(odictiterobject, &PyODictIter_Type);
    if (di == NULL)
        return NULL;

    if (kind & (_odict_ITER_KEYS | _odict_ITER_VALUES)){
        di->di_result = PyTuple_Pack(2, Py_None, Py_None);
        if (di->di_result == NULL) {
            Py_DECREF(di);
            return NULL;
        }
    }
    else
        di->di_result = NULL;

    di->kind = kind;
    node = reversed ? _odict_LAST(od) : _odict_FIRST(od);
    di->di_current = node ? _odictnode_KEY(node) : NULL;
    Py_XINCREF(di->di_current);
    di->di_size = PyODict_SIZE(od);
    di->di_state = od->od_state;
    di->di_odict = od;
    Py_INCREF(od);

    _PyObject_GC_TRACK(di);
    return (PyObject *)di;
}

/* keys() */

static PyObject *
odictkeys_iter(_PyDictViewObject *dv)
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return odictiter_new((PyODictObject *)dv->dv_dict,
            _odict_ITER_KEYS);
}

static PyObject *
odictkeys_reversed(_PyDictViewObject *dv)
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return odictiter_new((PyODictObject *)dv->dv_dict,
            _odict_ITER_KEYS|_odict_ITER_REVERSED);
}

static PyMethodDef odictkeys_methods[] = {
    {"__reversed__", (PyCFunction)odictkeys_reversed, METH_NOARGS, NULL},
    {NULL,          NULL}           /* sentinel */
};

PyTypeObject PyODictKeys_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "odict_keys",                             /* tp_name */
    0,                                        /* tp_basicsize */
    0,                                        /* tp_itemsize */
    0,                                        /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_reserved */
    0,                                        /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash */
    0,                                        /* tp_call */
    0,                                        /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    0,                                        /* tp_flags */
    0,                                        /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    (getiterfunc)odictkeys_iter,              /* tp_iter */
    0,                                        /* tp_iternext */
    odictkeys_methods,                        /* tp_methods */
    0,                                        /* tp_members */
    0,                                        /* tp_getset */
    &PyDictKeys_Type,                         /* tp_base */
};

static PyObject *
odictkeys_new(PyObject *od)
{
    return _PyDictView_New(od, &PyODictKeys_Type);
}

/* items() */

static PyObject *
odictitems_iter(_PyDictViewObject *dv)
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return odictiter_new((PyODictObject *)dv->dv_dict,
            _odict_ITER_KEYS|_odict_ITER_VALUES);
}

static PyObject *
odictitems_reversed(_PyDictViewObject *dv)
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return odictiter_new((PyODictObject *)dv->dv_dict,
            _odict_ITER_KEYS|_odict_ITER_VALUES|_odict_ITER_REVERSED);
}

static PyMethodDef odictitems_methods[] = {
    {"__reversed__", (PyCFunction)odictitems_reversed, METH_NOARGS, NULL},
    {NULL,          NULL}           /* sentinel */
};

PyTypeObject PyODictItems_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "odict_items",                            /* tp_name */
    0,                                        /* tp_basicsize */
    0,                                        /* tp_itemsize */
    0,                                        /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_reserved */
    0,                                        /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash */
    0,                                        /* tp_call */
    0,                                        /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    0,                                        /* tp_flags */
    0,                                        /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    (getiterfunc)odictitems_iter,             /* tp_iter */
    0,                                        /* tp_iternext */
    odictitems_methods,                       /* tp_methods */
    0,                                        /* tp_members */
    0,                                        /* tp_getset */
    &PyDictItems_Type,                        /* tp_base */
};

static PyObject *
odictitems_new(PyObject *od)
{
    return _PyDictView_New(od, &PyODictItems_Type);
}

/* values() */

static PyObject *
odictvalues_iter(_PyDictViewObject *dv)
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return odictiter_new((PyODictObject *)dv->dv_dict,
            _odict_ITER_VALUES);
}

static PyObject *
odictvalues_reversed(_PyDictViewObject *dv)
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return odictiter_new((PyODictObject *)dv->dv_dict,
            _odict_ITER_VALUES|_odict_ITER_REVERSED);
}

static PyMethodDef odictvalues_methods[] = {
    {"__reversed__", (PyCFunction)odictvalues_reversed, METH_NOARGS, NULL},
    {NULL,          NULL}           /* sentinel */
};

PyTypeObject PyODictValues_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "odict_values",                           /* tp_name */
    0,                                        /* tp_basicsize */
    0,                                        /* tp_itemsize */
    0,                                        /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_reserved */
    0,                                        /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash */
    0,                                        /* tp_call */
    0,                                        /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    0,                                        /* tp_flags */
    0,                                        /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    (getiterfunc)odictvalues_iter,            /* tp_iter */
    0,                                        /* tp_iternext */
    odictvalues_methods,                      /* tp_methods */
    0,                                        /* tp_members */
    0,                                        /* tp_getset */
    &PyDictValues_Type,                       /* tp_base */
};

static PyObject *
odictvalues_new(PyObject *od)
{
    return _PyDictView_New(od, &PyODictValues_Type);
}


/* ----------------------------------------------
   MutableMappping implementations

Mapping:

============ ===========
method       uses
============ ===========
__contains__ __getitem__
__eq__       items
__getitem__  +
__iter__     +
__len__      +
__ne__       __eq__
get          __getitem__
items        ItemsView
keys         KeysView
values       ValuesView
============ ===========

ItemsView uses __len__, __iter__, and __getitem__.
KeysView uses __len__, __iter__, and __contains__.
ValuesView uses __len__, __iter__, and __getitem__.

MutableMapping:

============ ===========
method       uses
============ ===========
__delitem__  +
__setitem__  +
clear        popitem
pop          __getitem__
             __delitem__
popitem      __iter__
             _getitem__
             __delitem__
setdefault   __getitem__
             __setitem__
update       __setitem__
============ ===========
*/

static int
mutablemapping_add_pairs(PyObject *self, PyObject *pairs)
{
    PyObject *pair, *iterator, *unexpected;
    int res = 0;

    iterator = PyObject_GetIter(pairs);
    if (iterator == NULL)
        return -1;
    PyErr_Clear();

    while ((pair = PyIter_Next(iterator)) != NULL) {
        /* could be more efficient (see UNPACK_SEQUENCE in ceval.c) */
        PyObject *key = NULL, *value = NULL;
        PyObject *pair_iterator = PyObject_GetIter(pair);
        if (pair_iterator == NULL)
            goto Done;

        key = PyIter_Next(pair_iterator);
        if (key == NULL) {
            if (!PyErr_Occurred())
                PyErr_SetString(PyExc_ValueError,
                                "need more than 0 values to unpack");
            goto Done;
        }

        value = PyIter_Next(pair_iterator);
        if (value == NULL) {
            if (!PyErr_Occurred())
                PyErr_SetString(PyExc_ValueError,
                                "need more than 1 value to unpack");
            goto Done;
        }

        unexpected = PyIter_Next(pair_iterator);
        if (unexpected != NULL) {
            Py_DECREF(unexpected);
            PyErr_SetString(PyExc_ValueError,
                            "too many values to unpack (expected 2)");
            goto Done;
        }
        else if (PyErr_Occurred())
            goto Done;

        res = PyObject_SetItem(self, key, value);

Done:
        Py_DECREF(pair);
        Py_XDECREF(pair_iterator);
        Py_XDECREF(key);
        Py_XDECREF(value);
        if (PyErr_Occurred())
            break;
    }
    Py_DECREF(iterator);

    if (res < 0 || PyErr_Occurred() != NULL)
        return -1;
    else
        return 0;
}

static PyObject *
mutablemapping_update(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int res = 0;
    Py_ssize_t len = (args != NULL) ? PyObject_Size(args) : 0;

    /* first handle args, if any */
    if (len < 0) /* PyObject_Size raised an exception. */
        return NULL;

    if (len > 1) {
        char *msg = "update() takes at most 1 positional argument (%d given)";
        PyErr_Format(PyExc_TypeError, msg, len);
        return NULL;
    }

    if (len == 1) {
        PyObject *other = PyTuple_GET_ITEM(args, 0);  /* borrowed reference */
        if (other == NULL)
            return NULL;
        Py_INCREF(other);
        if (PyObject_HasAttrString(other, "items")) {  /* never fails */
            PyObject *items = PyMapping_Items(other);
            Py_DECREF(other);
            if (items == NULL)
                return NULL;
            res = mutablemapping_add_pairs(self, items);
            Py_DECREF(items);
            if (res == -1)
                return NULL;
        }
        else if (PyObject_HasAttrString(other, "keys")) {  /* never fails */
            PyObject *keys, *iterator, *key;
            keys = PyObject_CallMethod(other, "keys", NULL);
            if (keys == NULL) {
                Py_DECREF(other);
                return NULL;
            }
            iterator = PyObject_GetIter(keys);
            Py_DECREF(keys);
            if (iterator == NULL) {
                Py_DECREF(other);
                return NULL;
            }
            while (res == 0 && (key = PyIter_Next(iterator))) {
                PyObject *value = PyObject_GetItem(other, key);
                if (value != NULL) {
                    res = PyObject_SetItem(self, key, value);
                    Py_DECREF(value);
                }
                else {
                    res = -1;
                }
                Py_DECREF(key);
            }
            Py_DECREF(other);
            Py_DECREF(iterator);
            if (res != 0 || PyErr_Occurred())
                return NULL;
        }
        else {
            res = mutablemapping_add_pairs(self, other);
            Py_DECREF(other);
            if (res != 0)
                return NULL;
        }
    }

    /* now handle kwargs */
    len = (kwargs != NULL) ? PyObject_Size(kwargs) : 0;
    if (len < 0) /* PyObject_Size raised an exception. */
        return NULL;
    if (len > 0) {
        PyObject *items;
        if (!PyMapping_Check(kwargs)) {
            PyErr_SetString(PyExc_TypeError, "expected mapping for kwargs");
            return NULL;
        }
        items = PyMapping_Items(kwargs);
        if (items == NULL)
            return NULL;
        res = mutablemapping_add_pairs(self, items);
        Py_DECREF(items);
        if (res == -1)
            return NULL;
    }

    Py_RETURN_NONE;
}
