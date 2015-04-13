#include <ppltasks.h>
#include <collection.h>
#include "python.h"
#include "gpioapi.h"

using namespace concurrency;
using namespace Windows::Foundation;
using namespace Windows::Devices::Gpio;
using namespace Platform::Collections;

void OnPinValueChanged(GpioPin ^sender, GpioPinValueChangedEventArgs ^args);
int InitializeGpioStatics(void(*event_callback)(int, int));

static GpioController^ gpioController;
static Vector<GpioPin^>^ gpioPins;
static void(*gpioPinValueChangedCallback)(int, int);
static TypedEventHandler<GpioPin ^, GpioPinValueChangedEventArgs ^>^ gpioValueChangedEventHandler;

extern "C" {
    int init_gpio(PyObject* module, void (*event_callback)(int, int)) {
        int pinCount = InitializeGpioStatics(event_callback);

        if (pinCount != FAILURE) {
			PyModule_AddIntConstant(module, "pincount", pinCount);
        } else {
            PyErr_SetString(PyExc_TypeError, "Failed to get GPIO controller");
            return FAILURE;
        }

        return SUCCESS;
    }

    int setup_gpio_channel(int channel, int direction, int pull_up_down, int initial) {
        int ret = FAILURE;
        GpioPin^ pin = nullptr;
        GpioPinDriveMode driveMode;

        if (channel >= 0 && channel < (int)gpioPins->Size) {
            pin = gpioPins->GetAt(channel);

            if (pin == nullptr) {
                pin = gpioController->OpenPin(channel);
                gpioPins->SetAt(channel, pin);
            }

            if (direction == INPUT) {
                switch (pull_up_down) {
                case PUD_UP:
                    driveMode = GpioPinDriveMode::InputPullUp;
                    break;
                case PUD_DOWN:
                    driveMode = GpioPinDriveMode::InputPullDown;
                    break;
                default:
                    driveMode = GpioPinDriveMode::Input;
                    break;
                }
            }
            else {
                switch (pull_up_down) {
                case PUD_UP:
                    driveMode = GpioPinDriveMode::OutputStrongLowPullUp;
                    break;
                case PUD_DOWN:
                    driveMode = GpioPinDriveMode::OutputStrongHighPullDown;
                    break;
                default:
                    driveMode = GpioPinDriveMode::Output;
                    break;
                }
            }

            if (pin->IsDriveModeSupported(driveMode)) {
                ret = SUCCESS;

				pin->SetDriveMode(driveMode);

                if (pin->GetDriveMode() == GpioPinDriveMode::Output) {
                    ret = output_gpio_channel(channel, initial);
                }
            } else {
                PyErr_Format(PyExc_RuntimeError, "Invalid mode specified for pin %d", pin->PinNumber);
            }
        }

        return ret;
    }

    int output_gpio_channel(int channel, int value) {
        int ret = FAILURE;
        GpioPin^ pin = nullptr;

        if (channel >= 0 && channel < (int)gpioPins->Size) {
            pin = gpioPins->GetAt(channel);

            if (pin != nullptr && pin->GetDriveMode() == GpioPinDriveMode::Output) {
                if (value == HIGH) {
                    pin->Write(GpioPinValue::High);
                }
                else if (value == LOW) {
                    pin->Write(GpioPinValue::Low);
                }

                ret = SUCCESS;
            }
        }

        return ret;
    }

    int input_gpio_channel(int channel, int* value) {
        GpioPin^ pin = nullptr;
        int ret = FAILURE;

        if (channel >= 0 && channel < (int)gpioPins->Size) {
            pin = gpioPins->GetAt(channel);

            if (pin != nullptr) {
                auto pinValue = pin->Read();

                if (pinValue == GpioPinValue::High) {
                    *value = HIGH;
                }
                else if (pinValue == GpioPinValue::Low) {
                    *value = LOW;
                }

                ret = SUCCESS;
            }
        }

        return ret;
    }

    void cleanup_gpio_channel(int channel) {
        if (channel >= 0 && channel < (int)gpioPins->Size) {
            gpioPins->SetAt(channel, nullptr);
        }
    }

    void cleanup_gpio_channels() {
        for (auto i = 0; i < (int)gpioPins->Size; i++) {
            gpioPins->SetAt(i, nullptr);
        }
    }

    int enable_event_detect_gpio_channel(int channel, int debounce_timeout_ms, long long* event_token) {
        auto pin = gpioPins->GetAt(channel);
        TimeSpan debounceTimespan;

        if (pin == nullptr) {
            PyErr_SetString(PyExc_TypeError, "Channel is not setup to enable event detection");
            return FAILURE;
        }

        switch (pin->GetDriveMode())
        {
        case GpioPinDriveMode::Input:
        case GpioPinDriveMode::InputPullUp:
        case GpioPinDriveMode::InputPullDown:
            // Do nothing because pin is set up for input as expected
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "Channel is not setup for input");
            return FAILURE;
        }

        debounceTimespan.Duration = debounce_timeout_ms * 10000; // Translate to 100-nanoseconds

        pin->DebounceTimeout = debounceTimespan;

        auto token = pin->ValueChanged += gpioValueChangedEventHandler;

        *event_token = token.Value;

        return SUCCESS;
    }

    int disable_event_detect_gpio_channel(int channel, long long token) {
        auto pin = gpioPins->GetAt(channel);

        if (pin != nullptr) {
            EventRegistrationToken eventToken;

            eventToken.Value = token;

            try {
                pin->ValueChanged -= eventToken;
            } catch (Platform::Exception^ e) {
                PyErr_Format(PyExc_RuntimeError, "An unexpected error occured during event detection removal: %S", e->Message->Data());
                return FAILURE;
            }
        }

        return SUCCESS;
    }
}

int
InitializeGpioStatics(void(*event_callback)(int, int)) {
    int pinCount = FAILURE;
    gpioController = GpioController::GetDefault();
    gpioValueChangedEventHandler = ref new TypedEventHandler<GpioPin ^, GpioPinValueChangedEventArgs ^>(OnPinValueChanged);
    gpioPinValueChangedCallback = event_callback;

    if (gpioController != nullptr) {
        pinCount = (int)gpioController->PinCount;

        gpioPins = ref new Vector<GpioPin^>(pinCount+1);
    }

    return pinCount;
}

void 
OnPinValueChanged(GpioPin ^sender, GpioPinValueChangedEventArgs ^args) {
    if (sender != nullptr && args != nullptr && gpioPinValueChangedCallback != nullptr) {
        
        int edge = 0;

        switch (args->Edge) {
        case GpioPinEdge::FallingEdge:
            edge = FALLING_EDGE;
            break;
        case GpioPinEdge::RisingEdge:
            edge = RISING_EDGE;
            break;
        default:
            break;
        }

        gpioPinValueChangedCallback(sender->PinNumber, edge);
    }
}
