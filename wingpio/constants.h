#pragma once

PyObject *high;
PyObject *low;

PyObject *input;
PyObject *output;

PyObject *pud_off;
PyObject *pud_up;
PyObject *pud_down;

PyObject *rising_edge;
PyObject *falling_edge;
PyObject *both_edge;

void define_constants(PyObject *module);
