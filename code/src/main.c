#include "sdk_project_config.h"
#include <stdio.h> // Include for semihosting log with printf
#include <string.h>

volatile int exit_code = 0;

/* ADC config */
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

/* UART config */
#define UART_BUFFER_SIZE 256U
#define UART_TIMEOUT 500U

typedef enum
{
    GPIO_LEVEL,
    GPIO_POLARITY
} GPIO_mode_t;

/* Slave TX and RX buffers definition */
uint8_t slaveTxBuffer[I2C_BUFF_SIZE];
uint8_t slaveRxBuffer[I2C_BUFF_SIZE];

/* Buffer used to receive data from the console */
uint8_t bufferUART[UART_BUFFER_SIZE];

/* Send the received data */
void UART_Send_Data(uint8_t *data)
{
    // print using semihost
    printf((char *)data);
    // send to  UART console
    LPUART_DRV_SendDataBlocking(INST_LPUART_1, data, strlen((char *)data), UART_TIMEOUT);
}

/* Function to get ADC value for a specified channel */
uint8_t ADC_Get_Value(uint8_t channel)
{
    uint16_t result = 0U;
    switch (channel)
    {
    case ADC_CHANNEL_0:
        ADC_DRV_ConfigChan(INST_ADC_CONFIG_1, 0U, &adc_config_1_ChnConfig0); // Configure channel 0
        break;

    case ADC_CHANNEL_1:
        ADC_DRV_ConfigChan(INST_ADC_CONFIG_1, 0U, &adc_config_1_ChnConfig1); // Configure channel 1
        break;

    default:
        return 0U;
    }

    ADC_DRV_WaitConvDone(INST_ADC_CONFIG_1); // Wait for ADC conversion to complete

    ADC_DRV_GetChanResult(INST_ADC_CONFIG_1, 0U, &result); // Retrieve the result of the conversion

    // Scale the result to voltage range 5V - 20V
    int8_t voltage = (result * (MAX_VOLTAGE - MIN_VOLTAGE) * VOLTAGE_DIVISOR / ADC_RESOLUTION) + MIN_VOLTAGE;

    return (voltage < MIN_VOLTAGE) ? MIN_VOLTAGE : (voltage > MAX_VOLTAGE) ? MAX_VOLTAGE
                                                                           : voltage;
}

/* Function to get the GPIO status */
uint8_t GPIO_Get_State(void)
{
    uint8_t result = 0;
    // Only output pins can be read
    if (g_pin_mux_InitConfigArr0->direction == GPIO_OUTPUT_DIRECTION)
    {
        result = (uint8_t)PINS_DRV_GetPinsOutput(PTD); // Get the output status of the pins
    }

    return result;
}

/* Function to set GPIO level via SPI */
void SPI_Set_Level(uint8_t data)
{
    // Only output pins can be modified
    if (g_pin_mux_InitConfigArr0->base == PORTD &&
        g_pin_mux_InitConfigArr0->direction == GPIO_OUTPUT_DIRECTION)
    {
        PINS_DRV_SetPins(PTD, data); // Set pin levels
        LPSPI_DRV_MasterTransferBlocking(INST_LPSPI_1, &data, NULL, SPI_BUFFER_SIZE, SPI_TIMEOUT); // Send data via SPI
    }
}

/* Function to set GPIO direcction via SPI */
void SPI_Set_Direcction(uint8_t data)
{
    // Input or output direction for pins can be changed
    if (g_pin_mux_InitConfigArr0->base == PORTD)
    {
        PINS_DRV_SetPinsDirection(PTD, data); // Set pin direction
        LPSPI_DRV_MasterTransferBlocking(INST_LPSPI_1, &data, NULL, SPI_BUFFER_SIZE, SPI_TIMEOUT); // Send data via SPI
    }
}

/* I2C Slave callback function for communication handling */
void lpi2c0_SlaveCallback0(i2c_slave_event_t slaveEvent, void *userData)
{
    uint32_t instance;
    instance = (uint32_t)userData;

    switch (slaveEvent)
    {
    case I2C_SLAVE_EVENT_RX_REQ:
        LPI2C_DRV_SlaveSetRxBuffer(instance, slaveRxBuffer, I2C_BUFF_SIZE); // Set RX buffer
        break;

    case I2C_SLAVE_EVENT_TX_REQ:
        switch (slaveRxBuffer[I2C_REGISTER_ADDRESS])
        {
        case I2C_ADC1_REG:
            slaveTxBuffer[I2C_RESULT_ADRRES] = ADC_Get_Value(ADC_CHANNEL_0); // Get ADC1 value
            sprintf((char *)bufferUART, "Sending ADC_1_value: %u\n", slaveTxBuffer[I2C_RESULT_ADRRES]);
            UART_Send_Data(bufferUART);

            LPI2C_DRV_SlaveSetTxBuffer(instance, slaveTxBuffer, I2C_SIZE_SEND_1_BYTE); // Set TX buffer
            break;

        case I2C_ADC2_REG:
            slaveTxBuffer[I2C_RESULT_ADRRES] = ADC_Get_Value(ADC_CHANNEL_1); // Get ADC2 value
            sprintf((char *)bufferUART, "Sending ADC_2_value: %u\n", slaveTxBuffer[I2C_RESULT_ADRRES]);
            UART_Send_Data(bufferUART);

            LPI2C_DRV_SlaveSetTxBuffer(instance, slaveTxBuffer, I2C_SIZE_SEND_1_BYTE); // Set TX buffer
            break;

        case I2C_GPIO_STATE_REG:
            slaveTxBuffer[I2C_RESULT_ADRRES] = GPIO_Get_State(); // Get GPIO state
            sprintf((char *)bufferUART, "Sending GPIO_state: %u\n", slaveTxBuffer[I2C_RESULT_ADRRES]);
            UART_Send_Data(bufferUART);

            LPI2C_DRV_SlaveSetTxBuffer(instance, slaveTxBuffer, I2C_SIZE_SEND_1_BYTE); // Set TX buffer
            break;

        default:
            break;
        }
        break;

    case I2C_SLAVE_EVENT_RX_FULL:
        if (slaveRxBuffer[I2C_REGISTER_ADDRESS] == I2C_GPIO_CONFIG_REG)
        {
            uint8_t level = slaveRxBuffer[1];
            sprintf((char *)bufferUART, "Configuring GPIO level to: %u\n", level);
            UART_Send_Data(bufferUART);
            SPI_Set_Level(level); // Set GPIO level via SPI

            uint8_t polarity = slaveRxBuffer[2];
            sprintf((char *)bufferUART, "Configuring GPIO polarity to: %u\n", polarity);
            UART_Send_Data(bufferUART);
            SPI_Set_Direcction(polarity); // Set GPIO polarity via SPI
        }
        break;

    case I2C_SLAVE_EVENT_TX_EMPTY:
    case I2C_SLAVE_EVENT_STOP:
    default:
        break;
    }
}

/* Initialize I2C communication */
void Init_I2C(void)
{
    lpi2c_slave_state_t lpi2c0SlaveState;
    lpi2c0_SlaveConfig0.callbackParam = (uint32_t *)INST_LPI2C0;
    /* Initialize LPI2C Slave with:
     * - Listening mode enabled
     * - Slave address set to 0x32
     * - Fast operating mode
     * - Configuration details handled via LPI2C components
     */
    LPI2C_DRV_SlaveInit(INST_LPI2C0, &lpi2c0_SlaveConfig0, &lpi2c0SlaveState);
    sprintf((char *)bufferUART, "I2C initialized\n");
    UART_Send_Data(bufferUART);
}

/* Initialize SPI communication */
void Init_SPI(void)
{
    LPSPI_DRV_MasterInit(INST_LPSPI_1, &lpspi_1State, &lpspi_0_MasterConfig0);
    sprintf((char *)bufferUART, "SPI initialized\n");
    UART_Send_Data(bufferUART);
}

/* Initialize ADC converter and channels */
void Init_ADC(void)
{
    ADC_DRV_ConfigConverter(INST_ADC_CONFIG_1, &adc_config_1_ConvConfig0); // Configure the ADC
    ADC_DRV_AutoCalibration(INST_ADC_CONFIG_1);                            // Perform ADC calibration
    ADC_DRV_ConfigChan(INST_ADC_CONFIG_1, 0U, &adc_config_1_ChnConfig0);   // Configure channel 0 (PTC14)
    ADC_DRV_ConfigChan(INST_ADC_CONFIG_1, 0U, &adc_config_1_ChnConfig1);   // Configure channel 1 (PTC15)
    sprintf((char *)bufferUART, "ADC initialized\n");
    UART_Send_Data(bufferUART);
}

/* Initialize UART */
void Init_UART(void)
{
    /* Initialize LPUART instance */
    LPUART_DRV_Init(INST_LPUART_1, &lpUartState1, &lpuart_1_InitConfig0);
    sprintf((char *)bufferUART, "UART initialized\n");
    UART_Send_Data(bufferUART);
}

int main(void)
{
    /* Initialize and configure clocks
     *     -    see clock manager component for details
     */
    CLOCK_SYS_Init(g_clockManConfigsArr, CLOCK_MANAGER_CONFIG_CNT,
                   g_clockManCallbacksArr, CLOCK_MANAGER_CALLBACK_CNT);
    CLOCK_SYS_UpdateConfiguration(0U, CLOCK_MANAGER_POLICY_AGREEMENT);

    PINS_DRV_Init(NUM_OF_CONFIGURED_PINS0, g_pin_mux_InitConfigArr0); // Initialize GPIO pins

    Init_UART(); // Initialize UART
    Init_ADC();  // Initialize ADC
    Init_SPI();  // Initialize SPI
    Init_I2C();  // Initialize I2C

    for (;;)
    {
        if (exit_code != 0)
        {
            break;
        }
    }
    return exit_code;
}
