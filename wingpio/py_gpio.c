#include "python.h"
#include "gpioapi.h"
#include "constants.h"

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
static PyObject *py_setup_channel(PyObject *self, PyObject *args, PyObject *kwargs)
{
	int channel = -1;
	int direction;
	int i, chancount;
	PyObject *chanlist = NULL;
	PyObject *chantuple = NULL;
	PyObject *tempobj;
	int pud = PUD_OFF + PY_PUD_CONST_OFFSET;
	int initial = -1;
	static char *kwlist[] = { "channel", "direction", "pull_up_down", "initial", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oi|ii", kwlist, &chanlist, &direction, &pud, &initial))
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

	if (direction != INPUT && direction != OUTPUT) {
		PyErr_SetString(PyExc_ValueError, "An invalid direction was passed to setup()");
		return 0;
	}

	if (direction == OUTPUT)
		pud = PUD_OFF + PY_PUD_CONST_OFFSET;

	pud -= PY_PUD_CONST_OFFSET;
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
            PyErr_Format(PyExc_RuntimeError, "Setup of channel %d failed", channel);
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
            PyErr_Format(PyExc_RuntimeError, "Setup of channel %d failed", channel);
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
static PyObject *py_output_gpio(PyObject *self, PyObject *args)
{
    unsigned int gpio;
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

    if (!PyArg_ParseTuple(args, "OO", &chanlist, &valuelist))
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
        output_gpio_channel(channel, value);
        Py_RETURN_NONE;
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
            PyErr_Format(PyExc_RuntimeError, "Output to channel %d failed", channel);
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
static PyObject *py_input_gpio(PyObject *self, PyObject *args)
{
    unsigned int gpio;
    int channel;
    PyObject *value = NULL;
    int result = 0;

    if (!PyArg_ParseTuple(args, "i", &channel))
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
static PyObject *py_cleanup(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int i;
    int chancount = -666;
    int found = 0;
    int channel = -666;
    unsigned int gpio;
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

static const char moduledocstring[] = "GPIO functionality of a Windows 10 IoT Core device";

PyMethodDef win_gpio_methods[] = {
	{ "setup", (PyCFunction)py_setup_channel, METH_VARARGS | METH_KEYWORDS, setup_doc },
	{ "cleanup", (PyCFunction)py_cleanup, METH_VARARGS | METH_KEYWORDS, cleanup_doc },
	{ "output", py_output_gpio, METH_VARARGS, output_doc },
	{ "input", py_input_gpio, METH_VARARGS, input_doc },
	{ NULL, NULL, 0, NULL }
};

static struct PyModuleDef wingpiomodule = {
	PyModuleDef_HEAD_INIT,
	"wingpio",       // name of module
	moduledocstring,  // module documentation, may be NULL
	-1,               // size of per-interpreter state of the module, or -1 if the module keeps state in global variables.
    win_gpio_methods
};

PyMODINIT_FUNC PyInit_wingpio(void)
{
	PyObject *module = NULL;

	if ((module = PyModule_Create(&wingpiomodule)) == NULL)
		return NULL;

	define_constants(module);

    init_gpio(module);

	return module;
}
