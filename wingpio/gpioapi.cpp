#include <ppltasks.h>
#include <collection.h>
#include "python.h"
#include "constants.h"
#include "gpioapi.h"

using namespace concurrency;
using namespace Windows::Devices::Gpio;
using namespace Platform::Collections;

static GpioController^ gpioController;
static Vector<GpioPin^>^ gpioPins;

extern "C" {
    void init_gpio(PyObject* module) {
        gpioPins = ref new Vector<GpioPin^>();

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

        if (channel >= 0 && channel < (int)gpioPins->Size) {
            pin = gpioPins->GetAt(channel);

            if (pin == nullptr) {
                pin = gpioController->OpenPin(channel);
                gpioPins->SetAt(channel, pin);
            }

            if (direction == INPUT) {
                pin->SetDriveMode(GpioPinDriveMode::Input);
            }
            else {
                pin->SetDriveMode(GpioPinDriveMode::Output);
            }

            ret = SUCCESS;

            if (pin->GetDriveMode() == GpioPinDriveMode::Output) {
                ret = output_gpio_channel(channel, initial);
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