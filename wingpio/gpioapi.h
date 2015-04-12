#pragma once

#ifdef __cplusplus
extern "C" {
#endif
    void init_gpio(PyObject* module);
    int setup_gpio_channel(int channel, int direction, int pull_up_down, int initial);
    int output_gpio_channel(int channel, int value);
    int input_gpio_channel(int channel, int *value);
    void cleanup_gpio_channel(int channel);
    void cleanup_gpio_channels();
#ifdef __cplusplus
}
#endif

#define SUCCESS 0
#define FAILURE -1

#define SETUP_OK          0
#define SETUP_DEVMEM_FAIL 1
#define SETUP_MALLOC_FAIL 2
#define SETUP_MMAP_FAIL   3

#define INPUT  1 // is really 0 for control register!
#define OUTPUT 0 // is really 1 for control register!
#define ALT0   4

#define HIGH 1
#define LOW  0

#define PUD_OFF  0
#define PUD_DOWN 1
#define PUD_UP   2

#define NO_EDGE      0
#define RISING_EDGE  1
#define FALLING_EDGE 2
#define BOTH_EDGE    3
