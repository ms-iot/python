#include "python.h"
#include "gpioapi.h"

// Callback structures
struct gpio_event_callback {
    PyObject *gpio_callback;
    struct gpio_event_callback *next;
};

struct gpio_event_handler_info {
    int pin_number;
    int pin_event_detected;
    int pin_event_edge;
    long long pin_event_token;
    struct gpio_event_callback *pin_event_callbacks;
    struct gpio_event_handler_info* next;
};

// Statics
static PyObject *high;
static PyObject *low;

static PyObject *input;
static PyObject *output;

static PyObject *pud_off;
static PyObject *pud_up;
static PyObject *pud_down;

static PyObject *rising_edge;
static PyObject *falling_edge;
static PyObject *both_edge;

static struct gpio_event_handler_info *event_handlers;

void 
define_gpio_constants(PyObject *module)
{
    high = Py_BuildValue("i", PINVALUE_HIGH);
    PyModule_AddObject(module, "HIGH", high);

    low = Py_BuildValue("i", PINVALUE_LOW);
    PyModule_AddObject(module, "LOW", low);

    output = Py_BuildValue("i", DRIVEMODE_OUT);
    PyModule_AddObject(module, "OUT", output);

    input = Py_BuildValue("i", DRIVEMODE_IN);
    PyModule_AddObject(module, "IN", input);

    pud_off = Py_BuildValue("i", PUD_OFF);
    PyModule_AddObject(module, "PUD_OFF", pud_off);

    pud_up = Py_BuildValue("i", PUD_UP);
    PyModule_AddObject(module, "PUD_UP", pud_up);

    pud_down = Py_BuildValue("i", PUD_DOWN);
    PyModule_AddObject(module, "PUD_DOWN", pud_down);

    rising_edge = Py_BuildValue("i", RISING_EDGE);
    PyModule_AddObject(module, "RISING", rising_edge);

    falling_edge = Py_BuildValue("i", FALLING_EDGE);
    PyModule_AddObject(module, "FALLING", falling_edge);

    both_edge = Py_BuildValue("i", BOTH_EDGE);
    PyModule_AddObject(module, "BOTH", both_edge);
}

// Callback method for when pin edges change
static void 
gpio_handle_pin_event(int pin_number, int pin_edge) {
    struct gpio_event_handler_info *handlers = event_handlers;
    PyGILState_STATE gil_state;

    while (handlers != NULL) {
        if (handlers->pin_number == pin_number) {
            if (handlers->pin_event_edge & pin_edge) {
                // Event matches edges we care about
                handlers->pin_event_detected = 1;

                struct gpio_event_callback *callbacks = handlers->pin_event_callbacks;

                while (callbacks != NULL) {
                    gil_state = PyGILState_Ensure();
                    PyObject *result = PyObject_CallFunction(callbacks->gpio_callback, "i", pin_number);
                    if (result == NULL && PyErr_Occurred()) {
                        PyErr_Print();
                        PyErr_Clear();
                    }
                    Py_XDECREF(result);
                    PyGILState_Release(gil_state);

                    callbacks = callbacks->next;
                }
            }
            break;
        }

        handlers = handlers->next;
    }
}

PyDoc_STRVAR(setup_doc,
"setup(channel(s), direction, pull_up_down=PUD_OFF, initial=None)\n"
"\n"
"Setup a GPIO channel or multiple GPIO channels for use.\n"
"channel(s) is a single channel pin number or list of channel\n"
"pins to setup.\n"
"\n"
"direction is either IN or OUT depending on how the pins are used\n"
"\n"
"pull_up_down is either PUD_OFF, PUD_UP or PUD_DOWN.\n"
"\n"
"initial is an initial value (HIGH or LOW) for the pin after setup\n");
// python function setup(channel(s), direction, pull_up_down=PUD_OFF, initial=None)
static PyObject *
wingpio_setup(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int channel = -1;
    int direction;
    int i, chancount;
    PyObject *chanlist = NULL;
    PyObject *chantuple = NULL;
    PyObject *tempobj;
    int pud = PUD_OFF;
    int initial = -1;
    static char *kwlist[] = { "channel", "direction", "pull_up_down", "initial", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oi|ii:setup", kwlist, &chanlist, &direction, &pud, &initial))
        return NULL;

    if (PyLong_Check(chanlist)) {
        channel = (int)PyLong_AsLong(chanlist);

        if (PyErr_Occurred())
            return NULL;
        chanlist = NULL;
    }
    else if PyList_Check(chanlist) {
        // do nothing
    }
    else if PyTuple_Check(chanlist) {
        chantuple = chanlist;
        chanlist = NULL;
    }
    else {
        // raise exception
        PyErr_SetString(PyExc_ValueError, "Channel must be an integer or list/tuple of integers");
        return NULL;
    }

    if (direction != DRIVEMODE_IN && direction != DRIVEMODE_OUT) {
        PyErr_SetString(PyExc_ValueError, "An invalid direction was passed to setup()");
        return 0;
    }

    if (direction == DRIVEMODE_OUT)
        pud = PUD_OFF;

    if (pud != PUD_OFF && pud != PUD_DOWN && pud != PUD_UP) {
        PyErr_SetString(PyExc_ValueError, "Invalid value for pull_up_down - should be either PUD_OFF, PUD_UP or PUD_DOWN");
        return NULL;
    }

    if (chanlist) {
        chancount = PyList_Size(chanlist);
    }
    else if (chantuple) {
        chancount = PyTuple_Size(chantuple);
    }
    else {
        if (FAILURE == setup_gpio_channel(channel, direction, pud, initial)) {
            return NULL;
        }
        Py_RETURN_NONE;
    }

    for (i = 0; i < chancount; i++) {
        if (chanlist) {
            if ((tempobj = PyList_GetItem(chanlist, i)) == NULL) {
                return NULL;
            }
        }
        else { // assume chantuple
            if ((tempobj = PyTuple_GetItem(chantuple, i)) == NULL) {
                return NULL;
            }
        }

        if (PyLong_Check(tempobj)) {
            channel = (int)PyLong_AsLong(tempobj);

            if (PyErr_Occurred())
                return NULL;
        }
        else {
            PyErr_SetString(PyExc_ValueError, "Channel must be an integer");
            return NULL;
        }

        if (FAILURE == setup_gpio_channel(channel, direction, pud, initial)) {
            return NULL;
        }
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(output_doc,
    "output(channels, values)\n"
    "\n"
    "Outputs to GPIO pins the corresponding values specified\n"
    "\n"
    "channels - A single channel or list of channel pins\n"
    "\n"
    "values - HIGH or LOW for every corresponding channel\n"
    );
// python function output(channel(s), value(s))
static PyObject *
wingpio_output(PyObject *self, PyObject *args)
{
    int channel = -1;
    int value = -1;
    int i;
    PyObject *chanlist = NULL;
    PyObject *valuelist = NULL;
    PyObject *chantuple = NULL;
    PyObject *valuetuple = NULL;
    PyObject *tempobj = NULL;
    int chancount = -1;
    int valuecount = -1;

    if (!PyArg_ParseTuple(args, "OO:output", &chanlist, &valuelist))
        return NULL;

    if (PyLong_Check(chanlist)) {
        channel = (int)PyLong_AsLong(chanlist);

        if (PyErr_Occurred())
            return NULL;
        chanlist = NULL;
    }
    else if (PyList_Check(chanlist)) {
        // do nothing
    }
    else if (PyTuple_Check(chanlist)) {
        chantuple = chanlist;
        chanlist = NULL;
    }
    else {
        PyErr_SetString(PyExc_ValueError, "Channel must be an integer or list/tuple of integers");
        return NULL;
    }

    if (PyLong_Check(valuelist)) {
        value = (int)PyLong_AsLong(valuelist);

        if (PyErr_Occurred())
            return NULL;
        valuelist = NULL;
    }
    else if (PyList_Check(valuelist)) {
        // do nothing
    }
    else if (PyTuple_Check(valuelist)) {
        valuetuple = valuelist;
        valuelist = NULL;
    }
    else {
        PyErr_SetString(PyExc_ValueError, "Value must be an integer/boolean or a list/tuple of integers/booleans");
        return NULL;
    }

    if (chanlist)
        chancount = PyList_Size(chanlist);
    if (chantuple)
        chancount = PyTuple_Size(chantuple);
    if (valuelist)
        valuecount = PyList_Size(valuelist);
    if (valuetuple)
        valuecount = PyTuple_Size(valuetuple);
    if ((chancount != -1 && chancount != valuecount && valuecount != -1) || (chancount == -1 && valuecount != -1)) {
        PyErr_SetString(PyExc_RuntimeError, "Number of channels != number of values");
        return NULL;
    }

    if (chancount == -1) {
        if (FAILURE == output_gpio_channel(channel, value)) {
            return NULL;
        }
    }

    for (i = 0; i < chancount; i++) {
        // get channel number
        if (chanlist) {
            if ((tempobj = PyList_GetItem(chanlist, i)) == NULL) {
                return NULL;
            }
        }
        else { // assume chantuple
            if ((tempobj = PyTuple_GetItem(chantuple, i)) == NULL) {
                return NULL;
            }
        }

        if (PyLong_Check(tempobj)) {
            channel = (int)PyLong_AsLong(tempobj);

            if (PyErr_Occurred())
                return NULL;
        }
        else {
            PyErr_SetString(PyExc_ValueError, "Channel must be an integer");
            return NULL;
        }

        // get value
        if (valuecount > 0) {
            if (valuelist) {
                if ((tempobj = PyList_GetItem(valuelist, i)) == NULL) {
                    return NULL;
                }
            }
            else { // assume valuetuple
                if ((tempobj = PyTuple_GetItem(valuetuple, i)) == NULL) {
                    return NULL;
                }
            }

            if (PyLong_Check(tempobj)) {
                value = (int)PyLong_AsLong(tempobj);

                if (PyErr_Occurred())
                    return NULL;
            }
            else {
                PyErr_SetString(PyExc_ValueError, "Value must be an integer or boolean");
                return NULL;
            }
        }

        if (FAILURE == output_gpio_channel(channel, value)) {
            return NULL;
        }
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(input_doc,
    "input(channel) -> HIGH/LOW\n"
    "\n"
    "Gets the current state of the GPIO pin specified\n"
    "\n"
    "channel=A single pin number\n"
    );
// python function value = input(channel)
static PyObject *
wingpio_input(PyObject *self, PyObject *args)
{
    int channel;
    PyObject *value = NULL;
    int result = 0;

    if (!PyArg_ParseTuple(args, "i:input", &channel))
        return NULL;

    if (SUCCESS == input_gpio_channel(channel, &result)) {
        value = Py_BuildValue("i", result);
    }

    return value;
}

PyDoc_STRVAR(cleanup_doc,
"cleanup(channel=None)\n"
"\n"
"Resets GPIO channels specified or all if none specified\n"
"\n"
"channel=A single channel or list of channels to cleanup\n"
);
// python function cleanup(channel=None)
static PyObject *
wingpio_cleanup(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int i;
    int chancount = -666;
    int found = 0;
    int channel = -666;
    PyObject *chanlist = NULL;
    PyObject *chantuple = NULL;
    PyObject *tempobj;
    static char *kwlist[] = { "channel", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &chanlist))
        return NULL;

    if (chanlist == NULL) {  // channel kwarg not set
                             // do nothing
    }
    else if (PyLong_Check(chanlist)) {
        channel = (int)PyLong_AsLong(chanlist);

        if (PyErr_Occurred())
            return NULL;
        chanlist = NULL;
    }
    else if (PyList_Check(chanlist)) {
        chancount = PyList_Size(chanlist);
    }
    else if (PyTuple_Check(chanlist)) {
        chantuple = chanlist;
        chanlist = NULL;
        chancount = PyTuple_Size(chantuple);
    }
    else {
        // raise exception
        PyErr_SetString(PyExc_ValueError, "Channel must be an integer or list/tuple of integers");
        return NULL;
    }

        if (channel == -666 && chancount == -666) {   // channel not set - cleanup everything
                                                      // clean up any /sys/class exports
            // set everything back to input
            cleanup_gpio_channels();
        }
        else if (channel != -666) {    // channel was an int indicating single channel
            cleanup_gpio_channel(channel);
        }
        else {  // channel was a list/tuple
            for (i = 0; i < chancount; i++) {
                if (chanlist) {
                    if ((tempobj = PyList_GetItem(chanlist, i)) == NULL) {
                        return NULL;
                    }
                }
                else { // assume chantuple
                    if ((tempobj = PyTuple_GetItem(chantuple, i)) == NULL) {
                        return NULL;
                    }
                }

                if (PyLong_Check(tempobj)) {
                    channel = (int)PyLong_AsLong(tempobj);

                    if (PyErr_Occurred())
                        return NULL;
                }
                else {
                    PyErr_SetString(PyExc_ValueError, "Channel must be an integer");
                    return NULL;
                }

                cleanup_gpio_channel(channel);
            }
        }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(add_event_detect_doc,
    "Enable edge detection events for a particular GPIO channel.\n"
    "channel = A pin number.\n"
    "edge = RISING, FALLING or BOTH\n"
    "[callback] = A callback function for the event (optional)\n"
    "[bouncetime] = Switch bounce timeout in ms for callback\n");
static PyObject *
wingpio_add_event_detect(PyObject *self, PyObject *args, PyObject *kwargs) {
    int channel = -1;
    int bouncetime = 0;
    PyObject *gpio_callback = NULL;
    int edge = 0;
    struct gpio_event_handler_info *handlers = event_handlers;
    struct gpio_event_handler_info* newhandler = NULL;
    long long token;
    int ret = FAILURE;
    static char *kwlist[] = { "channel", "edge", "callback", "bouncetime", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii|Oi:add_event_detect", kwlist, &channel, &edge, &gpio_callback, &bouncetime))
        return NULL;

    if (gpio_callback != NULL && !PyCallable_Check(gpio_callback)) {
        PyErr_SetString(PyExc_TypeError, "callback specified is not a valid callback function");
        return NULL;
    }

    if (bouncetime < 0) {
        PyErr_SetString(PyExc_TypeError, "bouncetime is not valid");
        return NULL;
    }

    switch (edge) {
    case FALLING_EDGE:
    case RISING_EDGE:
    case BOTH_EDGE:
        // Do nothing
        break;
    default:
        PyErr_SetString(PyExc_TypeError, "edge value is not FALLING, RISING or BOTH");
        break;
    }

    while (handlers) {
        if (handlers->pin_number == channel) {
            // Found an existing handler information already
            break;
        }

        handlers = handlers->next;
    }

    if (handlers != NULL) {
        PyErr_SetString(PyExc_TypeError, "pin already setup for event detection");
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS;
    ret = enable_event_detect_gpio_channel(channel, bouncetime, &token);
    Py_END_ALLOW_THREADS;

    if (ret == FAILURE) {
        return NULL;
    }

    newhandler = (struct gpio_event_handler_info*)malloc(sizeof(struct gpio_event_handler_info));
    if (newhandler == NULL) {
        PyErr_SetString(PyExc_MemoryError, "cannot allocate handler structure for pin edge events");
        return NULL;
    }

    newhandler->pin_number = channel;
    newhandler->pin_event_detected = 0;
    newhandler->pin_event_edge = edge;
    newhandler->pin_event_token = 0;
    newhandler->pin_event_callbacks = NULL;
    newhandler->next = NULL;

    // Add the callback if specified
    if (gpio_callback != NULL) {
        struct gpio_event_callback * new_event_callback = (struct gpio_event_callback *)malloc(sizeof(struct gpio_event_callback));

        if (new_event_callback == NULL) {
            free(newhandler);
            PyErr_SetString(PyExc_MemoryError, "cannot allocate event callback structure for pin edge events");
            return NULL;
        }

        Py_XINCREF(gpio_callback);

        new_event_callback->gpio_callback = gpio_callback;
        new_event_callback->next = NULL;

        newhandler->pin_event_callbacks = new_event_callback;
    }

    // Find a home for the new handler info structure
    handlers = event_handlers;
    while (handlers && handlers->next) {
        handlers = handlers->next;
    }

    if (handlers) {
        // Add to end of the handler list
        handlers->next = newhandler;
    }
    else {
        // This means there are no handlers, go ahead and just make this new handler the root
        event_handlers = newhandler;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(remove_event_detect_doc,
    "Remove edge detection for a particular GPIO channel\n"
    "channel = A pin number.\n");
static PyObject *
wingpio_remove_event_detect(PyObject *self, PyObject *args) {
    int channel = -1;
    int ret = FAILURE;
    struct gpio_event_handler_info *handler = event_handlers;
    struct gpio_event_handler_info *previous = NULL;
    struct gpio_event_callback *currentCallback = NULL;

    if (!PyArg_ParseTuple(args, "i:remove_event_detect", &channel))
        return NULL;

    while (handler) {
        if (handler->pin_number == channel) {
            // Found an existing handler information already
            break;
        }

        previous = handler;
        handler = handler->next;
    }

    if (handler == NULL) {
        PyErr_SetString(PyExc_TypeError, "pin not setup for event detection");
        return NULL;
    }

    // Remove current entry from the list
    if (previous != NULL) {
        previous->next = handler->next;
    } else {
        event_handlers = handler->next;
    }

    // Delete the memory for the handlers and callbacks
    currentCallback = handler->pin_event_callbacks;

    while (currentCallback) {
        struct gpio_event_callback* callback = currentCallback;
        
        Py_XDECREF(callback->gpio_callback);

        free(callback);

        currentCallback = currentCallback->next;
    }

    Py_BEGIN_ALLOW_THREADS;
    ret = disable_event_detect_gpio_channel(channel, handler->pin_event_token);
    Py_END_ALLOW_THREADS;

    free(handler);

    if (ret == FAILURE) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(event_detected_doc,
    "Returns True if an edge has occured on a given GPIO.  You need to enable edge detection using add_event_detect() first.\n"
    "channel = A pin number.\n");
static PyObject *
wingpio_event_detected(PyObject *self, PyObject *args) {
    int channel = -1;
    int ret = FAILURE;
    struct gpio_event_handler_info *handler = event_handlers;

    if (!PyArg_ParseTuple(args, "i:remove_event_detect", &channel))
        return NULL;

    while (handler) {
        if (handler->pin_number == channel) {
            // Found an existing handler information already
            break;
        }

        handler = handler->next;
    }

    if (handler == NULL) {
        PyErr_SetString(PyExc_TypeError, "pin number is not setup for edge detection");
        return NULL;
    }

    if (handler->pin_event_detected) {
        handler->pin_event_detected = 0;
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

PyDoc_STRVAR(add_event_callback_doc,
    "Add a callback for an event already defined using add_event_detect()\n"
    "channel = A pin number.\n"
    "callback = A callback function to be added.\n");
static PyObject *
wingpio_add_event_callback(PyObject *self, PyObject *args, PyObject *kwargs) {
    int channel = -1;
    PyObject *gpio_callback = NULL;
    int ret = FAILURE;
    struct gpio_event_handler_info *handler = event_handlers;
    struct gpio_event_callback *callback = NULL;
    struct gpio_event_callback *newcallback = NULL;
    static char *kwlist[] = { "channel", "callback", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iO", kwlist, &channel, &gpio_callback))
        return NULL;

    if (!PyCallable_Check(gpio_callback))
    {
        PyErr_SetString(PyExc_TypeError, "callback must be callable");
        return NULL;
    }

    while (handler) {
        if (handler->pin_number == channel) {
            // Found an existing handler information already
            break;
        }

        handler = handler->next;
    }

    if (handler == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "pin is not registered for edge detection");
        return NULL;
    }

    newcallback = malloc(sizeof(struct gpio_event_callback));
    newcallback->next = NULL;
    Py_INCREF(gpio_callback);
    newcallback->gpio_callback = gpio_callback;

    callback = handler->pin_event_callbacks;
    while (callback != NULL && callback->next != NULL) {
        callback = callback->next;
    }

    if (callback != NULL) {
        callback->next = newcallback;
    } else {
        handler->pin_event_callbacks = newcallback;
    }

    Py_RETURN_NONE;
}

PyMethodDef wingpio_methods[] = {
    { "setup", (PyCFunction)wingpio_setup, METH_VARARGS | METH_KEYWORDS, setup_doc },
    { "cleanup", (PyCFunction)wingpio_cleanup, METH_VARARGS | METH_KEYWORDS, cleanup_doc },
    { "output", wingpio_output, METH_VARARGS, output_doc },
    { "input", wingpio_input, METH_VARARGS, input_doc },
    { "add_event_detect", (PyCFunction)wingpio_add_event_detect, METH_VARARGS | METH_KEYWORDS, add_event_detect_doc },
    { "remove_event_detect", wingpio_remove_event_detect, METH_VARARGS, remove_event_detect_doc },
    { "event_detected", wingpio_event_detected, METH_VARARGS, event_detected_doc },
    { "add_event_callback", (PyCFunction)wingpio_add_event_callback, METH_VARARGS | METH_KEYWORDS, add_event_callback_doc },
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef wingpio_module = {
    PyModuleDef_HEAD_INIT,
    "_wingpio",       // name of module
    "GPIO functionality of a Windows 10 IoT Core device",  // module documentation, may be NULL
    -1,               // size of per-interpreter state of the module, or -1 if the module keeps state in global variables.
    wingpio_methods
};

PyMODINIT_FUNC PyInit__wingpio(void)
{
    PyObject *module = NULL;

    if ((module = PyModule_Create(&wingpio_module)) == NULL)
        return NULL;

    define_gpio_constants(module);

    if (FAILURE == init_gpio(module, gpio_handle_pin_event)) {
        _PyModule_Clear(module);
        return NULL;
    }

    return module;
}
