/*
  Copyright (c) 2020 SDS librelab, All rights reserved.
  Copyright (c) 2014-2015 Arduino LLC.  All right reserved.
  Copyright (c) 2016 Justin Mattair. All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _VARIANT_BIGDOT_
#define _VARIANT_BIGDOT_

/* The definitions here need the MattairTech SAMD/SAML core >=1.6.8.
 * The format is different than the stock Arduino SAMD core,
 * which uses ARDUINO_SAMD_VARIANT_COMPLIANCE instead.
 */
#define MATTAIRTECH_ARDUINO_SAMD_VARIANT_COMPLIANCE 10608

/*----------------------------------------------------------------------------
 *        Clock Configuration
 *----------------------------------------------------------------------------*/

/** Master clock frequency (also Fcpu frequency) */
#define VARIANT_MCK		(48000000ul)

/** Frequency of the board main oscillator */
#define VARIANT_MAINOSC     (32768ul)

/* If CLOCKCONFIG_HS_CRYSTAL is defined, then HS_CRYSTAL_FREQUENCY_HERTZ
 * must also be defined with the external crystal frequency in Hertz.
 */
//#define HS_CRYSTAL_FREQUENCY_HERTZ	16000000UL

/* If the PLL is used (CLOCKCONFIG_32768HZ_CRYSTAL, or CLOCKCONFIG_HS_CRYSTAL
 * defined), then PLL_FRACTIONAL_ENABLED can be defined, which will result in
 * a more accurate 48MHz output frequency at the expense of increased jitter.
 */
#define PLL_FRACTIONAL_ENABLED

/* If both PLL_FAST_STARTUP and CLOCKCONFIG_HS_CRYSTAL are defined, the crystal
 * will be divided down to 1MHz - 2MHz, rather than 32KHz - 64KHz, before being
 * multiplied by the PLL. This will result in a faster lock time for the PLL,
 * however, it will also result in a less accurate PLL output frequency if the
 * crystal is not divisible (without remainder) by 1MHz. In this case, define
 * PLL_FRACTIONAL_ENABLED as well.
 */
//#define PLL_FAST_STARTUP

/* The fine calibration value for DFLL open-loop mode is defined here.
 * The coarse calibration value is loaded from NVM OTP (factory calibration values).
 */
#define NVM_SW_CALIB_DFLL48M_FINE_VAL     (512)

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "WVariant.h"
#include "sam.h"

#ifdef __cplusplus
#include "SERCOM.h"
#include "Uart.h"
#endif // __cplusplus

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*----------------------------------------------------------------------------
 *        Pins
 *----------------------------------------------------------------------------*/

// Number of pins defined in PinDescription array
#define NUM_PIN_DESCRIPTION_ENTRIES     (32u)

#define PINS_COUNT           NUM_PIN_DESCRIPTION_ENTRIES
#define NUM_DIGITAL_PINS     PINS_COUNT
#define NUM_ANALOG_INPUTS    (10u)
#define NUM_ANALOG_OUTPUTS   (2u)

#define analogInputToDigitalPin(p)  (p)

#define digitalPinToPort(P)        ( &(PORT->Group[g_APinDescription[P].ulPort]) )
#define digitalPinToBitMask(P)     ( 1 << g_APinDescription[P].ulPin )
//#define analogInPinToBit(P)        ( )
#define portOutputRegister(port)   ( &(port->OUT.reg) )
#define portInputRegister(port)    ( &(port->IN.reg) )
#define portModeRegister(port)     ( &(port->DIR.reg) )
#define digitalPinHasPWM(P)        ( (g_APinDescription[P].ulPinAttribute & PIN_ATTR_TIMER_PWM) == PIN_ATTR_TIMER_PWM )

/*
 * digitalPinToTimer(..) is AVR-specific and is not defined for SAMD
 * architecture. If you need to check if a pin supports PWM you must
 * use digitalPinHasPWM(..).
 *
 * https://github.com/arduino/Arduino/issues/1833
 */
// #define digitalPinToTimer(P)

/* LEDs
 * Big Dot onboard LED is on pin 6.
 * The RX and TX LEDs are not present.
 */
#define PIN_LED_13           (6u)
#define PIN_LED_RXL          (30u)
#define PIN_LED_TXL          (31u)
#define PIN_LED              PIN_LED_13
#define PIN_LED2             PIN_LED_RXL
#define PIN_LED3             PIN_LED_TXL
#define LED_BUILTIN          PIN_LED_13

/*
 * Analog pins
 */
#define PIN_A1               (2ul)
#define PIN_A2               (3ul)
#define PIN_A3               (4ul)
#define PIN_A4               (5ul)
#define PIN_A5               (6ul) // LED
#define PIN_A6               (7ul) // Batt level
#define PIN_DAC0             (2ul)
#define PIN_DAC1             (5ul)

static const uint8_t A1   = PIN_A1;
static const uint8_t A2   = PIN_A2;
static const uint8_t A3   = PIN_A3;
static const uint8_t A4   = PIN_A4;
static const uint8_t A5   = PIN_A5;
static const uint8_t A6   = PIN_A6;
static const uint8_t DAC0 = PIN_DAC0;
static const uint8_t DAC1 = PIN_DAC1;

/* Set ADC converter resolution, bits */
#define ADC_RESOLUTION      12

/* Set default analog voltage reference */
#define VARIANT_AR_DEFAULT  AR_DEFAULT

/* Reference voltage pins (define even if not enabled with PIN_ATTR_REF in the PinDescription table) */
#define REFA_PIN    (3ul)
#define REFB_PIN    (4ul)

/*
 *  Digital pins
 */
#define PIN_D1              (8ul)  // Bootloader active pin
#define PIN_D2              (9ul)
#define PIN_D3              (10ul) // Serial1: TX
#define PIN_D4              (11ul) // Serial1: RX
#define PIN_D5              (14ul) // RFM_reset
#define PIN_D6	       	    (15ul) // RFM_DIO0
#define PIN_D7	            (16ul) // SDA
#define PIN_D8	            (17ul) // SCL
#define PIN_D9              (18ul) // SPI_MOSI
#define PIN_D10             (19ul) // SPI_CLK
#define PIN_D11             (22ul) // SPI_MISO
#define PIN_D12             (23ul) // SPI_SS
#define PIN_D13             (26ul)

static const uint8_t D1   = PIN_D1;
static const uint8_t D2   = PIN_D2;
static const uint8_t D3   = PIN_D3;
static const uint8_t D4   = PIN_D4;
static const uint8_t D5   = PIN_D5;
static const uint8_t D6   = PIN_D6;
static const uint8_t D7   = PIN_D7;
static const uint8_t D8   = PIN_D8;
static const uint8_t D9   = PIN_D9;
static const uint8_t D10  = PIN_D10;
static const uint8_t D11  = PIN_D11;
static const uint8_t D12  = PIN_D12;
static const uint8_t D13  = PIN_D13;

// The ATN pin may be used in the future as the first SPI chip select.
// On boards that do not have the Arduino physical form factor, it can to set to any free pin.
#define PIN_ATN              (15ul)
static const uint8_t ATN = PIN_ATN;

/*
 * Serial interfaces
 */

// Serial1
#define PIN_SERIAL1_RX       (11ul)
#define PIN_SERIAL1_TX       (10ul)
#define PAD_SERIAL1_TX       (UART_TX_PAD_2)
#define PAD_SERIAL1_RX       (SERCOM_RX_PAD_3)

#define SERCOM_INSTANCE_SERIAL1       &sercom0

/*
 * SPI Interfaces
 */
#define SPI_INTERFACES_COUNT 1

#define PIN_SPI_MISO         (22u)
#define PIN_SPI_MOSI         (18u)
#define PIN_SPI_SCK          (19u)
#define PIN_SPI_SS           (23u)
#define PERIPH_SPI           sercom3
#define PAD_SPI_TX           SPI_PAD_2_SCK_3
#define PAD_SPI_RX           SERCOM_RX_PAD_0

static const uint8_t SS	  = PIN_SPI_SS;
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK  = PIN_SPI_SCK;

/*
 * Wire Interfaces
 */
#define WIRE_INTERFACES_COUNT 1

#define PIN_WIRE_SDA         (16u)
#define PIN_WIRE_SCL         (17u)
#define PERIPH_WIRE          sercom1
#define WIRE_IT_HANDLER      SERCOM1_Handler

static const uint8_t SDA = PIN_WIRE_SDA;
static const uint8_t SCL = PIN_WIRE_SCL;

/*
 * USB - Define PIN_USB_HOST_ENABLE to assert the defined pin to
 * PIN_USB_HOST_ENABLE_VALUE during startup. Leave undefined to disable this pin.
 */
#define PIN_USB_DM                      (24ul)
#define PIN_USB_DP                      (25ul)
//#define PIN_USB_HOST_ENABLE             (14ul)
#define PIN_USB_HOST_ENABLE_VALUE	HIGH

/*
 * I2S Interfaces
 */
#define I2S_INTERFACES_COUNT 1

#define I2S_DEVICE          0
#define I2S_CLOCK_GENERATOR 3
#define PIN_I2S_SD          (7u)
#define PIN_I2S_SCK         (10u)
#define PIN_I2S_FS          (11u)

#ifdef __cplusplus
}
#endif


/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#ifdef __cplusplus

/*	=========================
 *	===== SERCOM DEFINITION
 *	=========================
*/
extern SERCOM sercom0;
extern SERCOM sercom1;
extern SERCOM sercom2;
extern SERCOM sercom3;
extern SERCOM sercom4;
extern SERCOM sercom5;

extern Uart Serial1;

#endif

// These serial port names are intended to allow libraries and architecture-neutral
// sketches to automatically default to the correct port name for a particular type
// of use.  For example, a GPS module would normally connect to SERIAL_PORT_HARDWARE_OPEN,
// the first hardware serial port whose RX/TX pins are not dedicated to another use.
//
// SERIAL_PORT_MONITOR        Port which normally prints to the Arduino Serial Monitor
//
// SERIAL_PORT_USBVIRTUAL     Port which is USB virtual serial
//
// SERIAL_PORT_LINUXBRIDGE    Port which connects to a Linux system via Bridge library
//
// SERIAL_PORT_HARDWARE       Hardware serial port, physical RX & TX pins.
//
// SERIAL_PORT_HARDWARE_OPEN  Hardware serial ports which are open for use.  Their RX & TX
//                            pins are NOT connected to anything by default.
#define Serial                      SerialUSB
#define SERIAL_PORT_MONITOR         Serial
#define SERIAL_PORT_USBVIRTUAL      Serial

// Serial has no physical pins broken out, so it's not listed as HARDWARE port
#define SERIAL_PORT_HARDWARE        Serial1
#define SERIAL_PORT_HARDWARE_OPEN   Serial1

#endif /* _VARIANT_BIGDOT_ */
