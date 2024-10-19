#include "sdk_project_config.h"

volatile int exit_code = 0;

/* ADC configuration */
#define ADC_INSTANCE 0UL
#define ADC_CHANNEL_0 14U    // ADC channel 0 corresponds to pin PTC14
#define ADC_CHANNEL_1 15U    // ADC channel 1 corresponds to pin PTC15
#define ADC_NUM_CHANNELS 2U  // Number of ADC channels used
#define ADC_RESOLUTION 255   // ADC resolution of 8 bits
#define MIN_VOLTAGE 5        // Minimum measurable voltage
#define MAX_VOLTAGE 20       // Maximum measurable voltage
#define VOLTAGE_DIVISOR 0.2f // Voltage scaling factor for reading

/* SPI configuration */
#define SPI_BUFFER_SIZE 2U // SPI buffer size
#define SPI_TIMEOUT 10U    // SPI timeout in milliseconds

/* I2C configuration */
#define I2C_BUFF_SIZE 4U         // I2C buffer size for communication
#define I2C_REGISTER_ADDRESS 0U  // I2C address for the register to access
#define I2C_RESULT_ADRRES 1U     // Address in buffer for the result data
#define I2C_SIZE_SEND_1_BYTE 1U  // Size of data to send (1 byte)
#define I2C_ADC1_REG 0x00        // Register for ADC1 value
#define I2C_ADC2_REG 0x01        // Register for ADC2 value
#define I2C_GPIO_STATE_REG 0x02  // Register for GPIO state
#define I2C_GPIO_CONFIG_REG 0x03 // Register for GPIO configuration

typedef enum
{
    GPIO_LEVEL,   // GPIO level configuration
    GPIO_POLARITY // GPIO polarity configuration
} GPIO_mode_t;

/* Definition of the slave TX and RX buffers for I2C communication */
uint8_t slaveTxBuffer[I2C_BUFF_SIZE];
uint8_t slaveRxBuffer[I2C_BUFF_SIZE];

/* Function to get ADC value for a specified channel */
uint8_t GetADCValue(uint8_t channel)
{
    uint16_t result = 0U;
    switch (channel)
    {
    case ADC_CHANNEL_0:
        ADC_DRV_ConfigChan(ADC_INSTANCE, 0U, &adc_config_1_ChnConfig0); // Configure channel 0
        break;

    case ADC_CHANNEL_1:
        ADC_DRV_ConfigChan(ADC_INSTANCE, 0U, &adc_config_1_ChnConfig1); // Configure channel 1
        break;

    default:
        return 0U;
    }

    ADC_DRV_WaitConvDone(ADC_INSTANCE); // Wait for ADC conversion to complete

    ADC_DRV_GetChanResult(ADC_INSTANCE, 0U, &result); // Retrieve the result of the conversion
    // Scale the result to voltage range 5V - 20V
    int8_t voltage = (result * (MAX_VOLTAGE - MIN_VOLTAGE) * VOLTAGE_DIVISOR / ADC_RESOLUTION) + MIN_VOLTAGE;

    return (voltage < MIN_VOLTAGE) ? MIN_VOLTAGE : (voltage > MAX_VOLTAGE) ? MAX_VOLTAGE
                                                                           : voltage;
}

/* Function to get the GPIO status */
uint8_t getStatusGPIO(void)
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
void SPISetLevel(uint8_t data)
{
    // Only output pins can be modified
    if (g_pin_mux_InitConfigArr0->base == PORTD &&
        g_pin_mux_InitConfigArr0->direction == GPIO_OUTPUT_DIRECTION)
    {
        PINS_DRV_SetPins(PTD, data); // Set pin levels
        LPSPI_DRV_MasterTransferBlocking(INST_LPSPI_1,
                                         &data,
                                         NULL,
                                         SPI_BUFFER_SIZE,
                                         SPI_TIMEOUT); // Send data via SPI
    }
}

/* Function to set GPIO polarity via SPI */
void SPISetPolarity(uint8_t data)
{
    // Input or output direction for pins can be changed
    if (g_pin_mux_InitConfigArr0->base == PORTD)
    {
        PINS_DRV_SetPinsDirection(PTD, data); // Set pin direction
        LPSPI_DRV_MasterTransferBlocking(INST_LPSPI_1,
                                         &data,
                                         NULL,
                                         SPI_BUFFER_SIZE,
                                         SPI_TIMEOUT); // Send data via SPI
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
            slaveTxBuffer[I2C_RESULT_ADRRES] = GetADCValue(ADC_CHANNEL_0); // Get ADC1 value
            break;

        case I2C_ADC2_REG:
            slaveTxBuffer[I2C_RESULT_ADRRES] = GetADCValue(ADC_CHANNEL_1); // Get ADC2 value
            break;

        case I2C_GPIO_STATE_REG:
            slaveTxBuffer[I2C_RESULT_ADRRES] = getStatusGPIO(); // Get GPIO state
            break;

        default:
            break;
        }

        LPI2C_DRV_SlaveSetTxBuffer(instance, slaveTxBuffer, I2C_SIZE_SEND_1_BYTE); // Set TX buffer
        break;

    case I2C_SLAVE_EVENT_RX_FULL:
        if (slaveRxBuffer[I2C_REGISTER_ADDRESS] == I2C_GPIO_CONFIG_REG)
        {
            uint8_t level = slaveRxBuffer[1];
            SPISetLevel(level); // Set GPIO level via SPI

            uint8_t polarity = slaveRxBuffer[2];
            SPISetPolarity(polarity); // Set GPIO polarity via SPI
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
}

/* Initialize SPI communication */
void Init_SPI(void)
{
    LPSPI_DRV_MasterInit(INST_LPSPI_1, &lpspi_1State, &lpspi_0_MasterConfig0);
}

/* Initialize ADC converter and channels */
void Init_ADC(void)
{
    ADC_DRV_ConfigConverter(ADC_INSTANCE, &adc_config_1_ConvConfig0); // Configure the ADC
    ADC_DRV_AutoCalibration(ADC_INSTANCE);                            // Perform ADC calibration
    ADC_DRV_ConfigChan(ADC_INSTANCE, 0U, &adc_config_1_ChnConfig0);   // Configure channel 0 (PTC14)
    ADC_DRV_ConfigChan(ADC_INSTANCE, 0U, &adc_config_1_ChnConfig1);   // Configure channel 1 (PTC15)
}

int main(void)
{
    CLOCK_DRV_Init(&clockMan1_InitConfig0); // Initialize clock

    PINS_DRV_Init(NUM_OF_CONFIGURED_PINS0, g_pin_mux_InitConfigArr0); // Initialize GPIO pins

    Init_ADC(); // Initialize ADC
    Init_SPI(); // Initialize SPI
    Init_I2C(); // Initialize I2C

    for (;;)
    {
        if (exit_code != 0)
        {
            break;
        }
    }
    return exit_code;
}
