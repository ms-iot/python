#include <ppltasks.h>
#include <collection.h>
#include "python.h"
#include "constants.h"
#include "gpioapi.h"

using namespace concurrency;
using namespace Windows::Foundation;
using namespace Windows::Devices::Gpio;
using namespace Platform::Collections;

void OnPinValueChanged(GpioPin ^sender, GpioPinValueChangedEventArgs ^args);

static GpioController^ gpioController;
static Vector<GpioPin^>^ gpioPins;
static Vector<int>^ fallingEventPins;
static Vector<int>^ risingEventPins;

extern "C" {
    void init_gpio(PyObject* module) {
        gpioPins = ref new Vector<GpioPin^>();
        fallingEventPins = ref new Vector<int>();
        risingEventPins = ref new Vector<int>();

		gpioController = GpioController::GetDefault();

        if (gpioController != nullptr) {
            int pinCount = (int)gpioController->PinCount;

            // Ensure the vector has the number of pins
            for (auto i = 0; i < pinCount; i++) {
                gpioPins->Append(nullptr);
            }

			PyModule_AddIntConstant(module, "pincount", pinCount);
		}
    }

    int setup_gpio_channel(int channel, int direction, int pull_up_down, int initial) {
        int ret = FAILURE;
        GpioPin^ pin = nullptr;
        GpioPinDriveMode driveMode;

        if (channel >= 0 && channel < (int)gpioPins->Size) {
            pin = gpioPins->GetAt(channel);

            if (pin == nullptr) {
                pin = gpioController->OpenPin(channel);
                pin->ValueChanged += ref new TypedEventHandler<GpioPin ^, GpioPinValueChangedEventArgs ^>(&OnPinValueChanged);
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
}

void OnPinValueChanged(GpioPin ^sender, GpioPinValueChangedEventArgs ^args)
{
    if (sender != nullptr) {
        unsigned int eventPinIndex = 0;

        if (args->Edge == GpioPinEdge::RisingEdge && risingEventPins->IndexOf(sender->PinNumber, &eventPinIndex)) {
            // TODO: Notify we hit rising edge
        }

        if (args->Edge == GpioPinEdge::FallingEdge && fallingEventPins->IndexOf(sender->PinNumber, &eventPinIndex)) {
            // TODO: Notify we hit falling edge
        }
    }
}
