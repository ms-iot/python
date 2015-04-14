#pragma once

#ifdef __cplusplus
extern "C" {
#endif
    int init_gpio(PyObject* module, void(*event_callback)(int, int));
    int setup_gpio_channel(int channel, int direction, int pull_up_down, int initial);
    int output_gpio_channel(int channel, int value);
    int input_gpio_channel(int channel, int *value);
    void cleanup_gpio_channel(int channel);
    void cleanup_gpio_channels();
    int enable_event_detect_gpio_channel(int channel, int debounce_timeout_ms, long long* token);
    int disable_event_detect_gpio_channel(int channel, long long token);
#ifdef __cplusplus
}
#endif

// Defines
#define SUCCESS 0
#define FAILURE -1

#define DRIVEMODE_IN  1
#define DRIVEMODE_OUT 0

#define PINVALUE_HIGH 1
#define PINVALUE_LOW  0

#define PUD_OFF  0
#define PUD_DOWN 1
#define PUD_UP   2

#define NO_EDGE      0
#define RISING_EDGE  1
#define FALLING_EDGE 2
#define BOTH_EDGE    3
