# Proyecto: Control de Periféricos con I2C

Este proyecto implementa un sistema que permite la lectura de valores de ADC y el estado de GPIOs, así como la configuración de GPIOs, utilizando comunicación I2C. El código interactúa con un microcontrolador NXP S32K144 y usa ADC, SPI e I2C para gestionar diferentes funciones.

## Registros I2C

El microcontrolador está configurado como esclavo I2C, con varios registros accesibles para la lectura y configuración:

| **Registro**          | **Descripción**                                 | **Dirección** |
|-----------------------|-------------------------------------------------|---------------|
| `I2C_ADC1_REG`        | Lectura del valor del ADC en el canal 0         | `0x00`        |
| `I2C_ADC2_REG`        | Lectura del valor del ADC en el canal 1         | `0x01`        |
| `I2C_GPIO_STATE_REG`  | Lectura del estado lógico de los GPIOs de salida | `0x02`        |
| `I2C_GPIO_CONFIG_REG` | Configuración del nivel y polaridad de los GPIOs | `0x03`        |

### Detalles de `I2C_GPIO_CONFIG_REG`
- Byte 1: Configuración del nivel de los GPIOs (por SPI).
- Byte 2: Configuración de la polaridad de los GPIOs (por SPI).

## Compilación y Uso

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
