#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "lambda_conversion.h"
#include "sampling.h"
#include "heater_control.h"
#include "max31855.h"
#include "fault.h"
#include "uart.h"

#include "tunerstudio.h"
#include "tunerstudio_io.h"
#include "wideband_board_config.h"

#ifdef UART_DEBUG
// just a reminder that we have either TS connectivity or this UART_DEBUG but not both

SerialConfig cfg = {
    .speed = 115200,
    .cr1 = 0,
    .cr2 = USART_CR2_STOP1_BITS | USART_CR2_LINEN,
    .cr3 = 0
};

static char printBuffer[200];

static THD_WORKING_AREA(waUartThread, 512);
static void UartThread(void*)
{
    chRegSetThreadName("UART debug");

    // in UART_DEBUG mode we only support Serial - this file name here has a bit of a confusing naming
    sdStart(&SD1, &cfg);

    while(true)
    {
        int ch;

        for (ch = 0; ch < AFR_CHANNELS; ch++) {
            float lambda = GetLambda(ch);
            int lambdaIntPart = lambda;
            int lambdaThousandths = (lambda - lambdaIntPart) * 1000;
            int batteryVoltageMv = GetInternalBatteryVoltage(ch) * 1000;
            int duty = GetHeaterDuty(ch) * 100;

            size_t writeCount = chsnprintf(printBuffer, 200,
                "[AFR%d]: %d.%03d DC: %4d mV AC: %4d mV ESR: %5d T: %4d C Ipump: %6d uA Vheater: %5d heater: %s (%d)\tfault: %s\r\n",
                ch,
                lambdaIntPart, lambdaThousandths,
                (int)(GetNernstDc(ch) * 1000.0),
                (int)(GetNernstAc(ch) * 1000.0),
                (int)GetSensorInternalResistance(ch),
                (int)GetSensorTemperature(ch),
                (int)(GetPumpNominalCurrent(ch) * 1000),
                batteryVoltageMv,
                describeHeaterState(GetHeaterState(ch)), duty,
                describeFault(GetCurrentFault(ch)));
            chnWrite(&SD1, (const uint8_t *)printBuffer, writeCount);
        }

#if (EGT_CHANNELS > 0)
        for (ch = 0; ch < EGT_CHANNELS; ch++) {
            size_t writeCount = chsnprintf(printBuffer, 200,
                "EGT[%d]: %d C (int %d C)\r\n",
                (int)getEgtDrivers()[ch].temperature,
                (int)getEgtDrivers()[ch].coldJunctionTemperature);
            chnWrite(&SD1, (const uint8_t *)printBuffer, writeCount);
        }
#endif /* EGT_CHANNELS > 0 */

        chThdSleepMilliseconds(100);
    }
}

#elif defined(TS_ENABLED)

#ifdef TS_PRIMARY_UART_PORT
static UartTsChannel primaryChannel(TS_PRIMARY_UART_PORT);
#endif

#ifdef TS_PRIMARY_SERIAL_PORT
static SerialTsChannel primaryChannel(TS_PRIMARY_SERIAL_PORT);
#endif

struct PrimaryChannelThread : public TunerstudioThread {
    PrimaryChannelThread() : TunerstudioThread("Primary TS Channel") { }

    TsChannelBase* setupChannel() {
        primaryChannel.start(TS_PRIMARY_BAUDRATE);

        return &primaryChannel;
    }
};

static PrimaryChannelThread primaryChannelThread;

#endif

void InitUart()
{
#ifdef UART_DEBUG
    chThdCreateStatic(waUartThread, sizeof(waUartThread), NORMALPRIO, UartThread, nullptr);
#elif defined(TS_ENABLED)
    primaryChannelThread.Start();
#endif
}
