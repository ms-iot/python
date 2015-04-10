#pragma once

#define PY_PUD_CONST_OFFSET 20
#define PY_EVENT_CONST_OFFSET 30

PyObject *high;
PyObject *low;
PyObject *input;
PyObject *output;
PyObject *pwm;
PyObject *serial;
PyObject *i2c;
PyObject *spi;
PyObject *unknown;
PyObject *board;
PyObject *bcm;
PyObject *pud_off;
PyObject *pud_up;
PyObject *pud_down;
PyObject *rising_edge;
PyObject *falling_edge;
PyObject *both_edge;

void define_constants(PyObject *module);
