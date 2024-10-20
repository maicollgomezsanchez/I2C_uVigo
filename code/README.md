# Proyecto: Control de Periféricos con I2C

Este proyecto implementa un sistema que permite la lectura de valores de ADC y el estado de GPIOs, así como la configuración de GPIOs, utilizando comunicación I2C. El código interactúa con un microcontrolador NXP S32K144 y usa ADC, SPI e I2C para gestionar diferentes funciones.

## Registros I2C

El microcontrolador está configurado como esclavo I2C, con varios registros accesibles para la lectura y configuración:

| **Registro**          | **Descripción**                                 | **Dirección** |
|-----------------------|-------------------------------------------------|---------------|
| `I2C_ADC1_REG`        | Lectura del valor del ADC en el canal 0         | `0x00`        |
| `I2C_ADC2_REG`        | Lectura del valor del ADC en el canal 1         | `0x01`        |
| `I2C_GPIO_STATE_REG`  | Lectura del estado lógico GPIO                  | `0x02`        |
| `I2C_GPIO_CONFIG_REG` | Configuración del nivel y direccion GPIO        | `0x03`        |

### Detalles de `I2C_GPIO_CONFIG_REG`
- Byte 1: Configuración de la direccion de los GPIOs (por SPI).
    1 para salida y 0 para entrada
- Byte 2: Configuración del nivel de los GPIOs (por SPI).
    1 para valor logico HIGH y 0 en valor logico LOW
por ejemplo: 0x03 0x0F 0x0A

    (0x0F) los primeros cuatro pines se configuran como salidas.
    (0x0A) El pin 3 y 1 estarán en HIGH y los demas pines en LOW.

## Pines Hardware
- UART RX pin 81
- UART TX pin 82
- I2C SDA pin 73
- I2C SCL pin 72
- ADC canal 1 pin 46 
- ADC canal 2 pin 45
- SPI SCK pin 48
- SPI SOUT pin 28
- SPI SIN pin 93
- SPI PCS pin 54
- Pines GPIO
    (PT0)  4 
           3
          71
          70
          69
          33
          32
    (PT7) 31

### Requisitos
- SDK de NXP S32K144
- Herramientas de desarrollo compatibles con el SDK (e.g., S32 Design Studio)

### Pasos de Compilación
1. Clona el repositorio o descarga el código fuente.
2. Abre el proyecto en S32 Design Studio.
3. Configura las rutas del SDK en tu entorno de desarrollo.
4. Compila el proyecto utilizando las opciones predeterminadas para el microcontrolador S32K144.

### Ejecución
1. Flashea el binario generado en la placa NXP S32K144.
2. Usa un maestro I2C para interactuar con los registros y obtener los valores de los ADCs o configurar los GPIOs.
3. Utiliza las direcciones I2C descritas anteriormente para enviar y recibir datos.
4. puedes usar UART para salida de consola usando los puertos mencionados a una velocidad de 9600, o usar la consola de depuracion.