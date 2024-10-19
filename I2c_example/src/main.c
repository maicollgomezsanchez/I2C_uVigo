#include "sdk_project_config.h"

volatile int exit_code = 0;

/* ADC config */
#define ADC_INSTANCE 0UL
#define ADC_CHANNEL_0 14U
#define ADC_CHANNEL_1 15U
#define ADC_NUM_CHANNELS 2U
#define ADC_RESOLUTION 255
#define MIN_VOLTAGE 5
#define MAX_VOLTAGE 20
#define VOLTAGE_DIVISOR 0.2f
/* SPI config */
#define SPI_BUFFER_SIZE 2U
#define SPI_TIMEOUT 10U
/* I2C config */
#define I2C_BUFF_SIZE 4U
#define I2C_REGISTER_ADDRESS 0U
#define I2C_RESULT_ADRRES 1U
#define I2C_SIZE_SEND_1_BYTE 1U
#define I2C_ADC1_REG 0x00
#define I2C_ADC2_REG 0x01
#define I2C_GPIO_STATE_REG 0x02
#define I2C_GPIO_CONFIG_REG 0x03

typedef enum
{
    GPIO_LEVEL,
    GPIO_POLARITY
} GPIO_mode_t;

/* Slave TX and RX buffers definition */
uint8_t slaveTxBuffer[I2C_BUFF_SIZE];
uint8_t slaveRxBuffer[I2C_BUFF_SIZE];

uint8_t GetADCValue(uint8_t channel)
{
    uint16_t result = 0U;
    switch (channel)
    {
    case ADC_CHANNEL_0:
        ADC_DRV_ConfigChan(ADC_INSTANCE, 0U, &adc_config_1_ChnConfig0);
        break;

    case ADC_CHANNEL_1:
        ADC_DRV_ConfigChan(ADC_INSTANCE, 0U, &adc_config_1_ChnConfig1);
        break;

    default:
        return 0U;
    }

    ADC_DRV_WaitConvDone(ADC_INSTANCE);

    ADC_DRV_GetChanResult(ADC_INSTANCE, 0U, &result);
    // read 0 to 3,3v scaled 5 to 20v
    int8_t voltage = (result * (MAX_VOLTAGE - MIN_VOLTAGE) * VOLTAGE_DIVISOR / ADC_RESOLUTION) + MIN_VOLTAGE;

    return (voltage < MIN_VOLTAGE) ? MIN_VOLTAGE : (voltage > MAX_VOLTAGE) ? MAX_VOLTAGE
                                                                           : voltage;
}

uint8_t getStatusGPIO(void)
{
    uint8_t result = 0;
    // only output can be readed
    if (g_pin_mux_InitConfigArr0->direction == GPIO_OUTPUT_DIRECTION)
    {
        result = (uint8_t)PINS_DRV_GetPinsOutput(PTD);
    }

    return result;
}

void SPISetLevel(uint8_t data)
{
    // only output can be changed
    if (g_pin_mux_InitConfigArr0->base == PORTD &&
        g_pin_mux_InitConfigArr0->direction == GPIO_OUTPUT_DIRECTION)
    {
        PINS_DRV_SetPins(PTD, data);
        LPSPI_DRV_MasterTransferBlocking(INST_LPSPI_1, &data, NULL, SPI_BUFFER_SIZE, SPI_TIMEOUT);
    }
}

void SPISetPolarity(uint8_t data)
{
    // changes input or output to pins
    if (g_pin_mux_InitConfigArr0->base == PORTD)
    {
        PINS_DRV_SetPinsDirection(PTD, data);
        LPSPI_DRV_MasterTransferBlocking(INST_LPSPI_1, &data, NULL, SPI_BUFFER_SIZE, SPI_TIMEOUT);
    }
}

void lpi2c0_SlaveCallback0(i2c_slave_event_t slaveEvent, void *userData)
{
    uint32_t instance;
    instance = (uint32_t)userData;

    switch (slaveEvent)
    {
    case I2C_SLAVE_EVENT_RX_REQ:
        LPI2C_DRV_SlaveSetRxBuffer(instance, slaveRxBuffer, I2C_BUFF_SIZE);
        break;

    case I2C_SLAVE_EVENT_TX_REQ:
        switch (slaveRxBuffer[I2C_REGISTER_ADDRESS])
        {
        case I2C_ADC1_REG:
            slaveTxBuffer[I2C_RESULT_ADRRES] = GetADCValue(ADC_CHANNEL_0);
            break;

        case I2C_ADC2_REG:
            slaveTxBuffer[I2C_RESULT_ADRRES] = GetADCValue(ADC_CHANNEL_1);
            break;

        case I2C_GPIO_STATE_REG:
            slaveTxBuffer[I2C_RESULT_ADRRES] = getStatusGPIO();
            break;

        default:
            break;
        }

        LPI2C_DRV_SlaveSetTxBuffer(instance, slaveTxBuffer, I2C_SIZE_SEND_1_BYTE);
        break;

    case I2C_SLAVE_EVENT_RX_FULL:
        if (slaveRxBuffer[I2C_REGISTER_ADDRESS] == I2C_GPIO_CONFIG_REG)
        {
            uint8_t level = slaveRxBuffer[1];
            SPISetLevel(level);

            uint8_t polarity = slaveRxBuffer[2];
            SPISetPolarity(polarity);
        }
        break;

    case I2C_SLAVE_EVENT_TX_EMPTY:
    case I2C_SLAVE_EVENT_STOP:
    default:
        break;
    }
}

void Init_I2C(void)
{
    lpi2c_slave_state_t lpi2c0SlaveState;
    lpi2c0_SlaveConfig0.callbackParam = (uint32_t *)INST_LPI2C0;
    /* Initialize LPI2C Slave configuration
     *  - Slave listening enabled
     *  - Slave address 0x32
     *  - Fast operating mode
     *  -   See LPI2C components for configuration details
     */
    LPI2C_DRV_SlaveInit(INST_LPI2C0, &lpi2c0_SlaveConfig0, &lpi2c0SlaveState);
}

void Init_SPI(void)
{
    LPSPI_DRV_MasterInit(INST_LPSPI_1, &lpspi_1State, &lpspi_0_MasterConfig0);
}

void Init_ADC(void)
{
    ADC_DRV_ConfigConverter(ADC_INSTANCE, &adc_config_1_ConvConfig0);
    ADC_DRV_AutoCalibration(ADC_INSTANCE);
    // ADC channel 0 port PTC14  pin_chip 46
    ADC_DRV_ConfigChan(ADC_INSTANCE, 0U, &adc_config_1_ChnConfig0);
    // ADC channel 1 port PTC15  pin_chip 45
    ADC_DRV_ConfigChan(ADC_INSTANCE, 0U, &adc_config_1_ChnConfig1);
}

int main(void)
{
    CLOCK_DRV_Init(&clockMan1_InitConfig0);

    PINS_DRV_Init(NUM_OF_CONFIGURED_PINS0, g_pin_mux_InitConfigArr0);

    Init_ADC();
    Init_SPI();
    Init_I2C();

    for (;;)
    {
        if (exit_code != 0)
        {
            break;
        }
    }
    return exit_code;
}
