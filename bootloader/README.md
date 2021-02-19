# SAML21 Prototyping Notes

In this document, we describe the steps for setting up the SAML21 Xplained Pro dev board for testing purposes.

## Bootloader Installation

We must use a set of tools for writing the bootloader and firmware files to the SAML21 for prototyping purposes. Here are the steps to get it up and running:

**Dependencies**

- OpenOCD v.0.10.0 or superior
- [GCC-arm-none-eabi-7-2017-q4-major](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) (this is also included in the Arduino SAMD core with support for Big Dot, installable through the Arduino IDE "Boards Manager")
- [BOSSA](https://github.com/shumatech/BOSSA/releases)
- Debian 10 (or any other distro) w/ extra packages: 'libwxgtk3.0-dev' and 'libreadline-dev'

### Building the Bootloader

*Note: there are precompiled binaries included in the `bootloader` directory*

We need to build the bootloader for our board and our specific target MCU. Change to the directory `bootloader/ArduinoCore-samd-1.6.18-beta-b1/bootloaders/zero` and type:

```
$ BOARD_ID=XPLAINED_SAML21 MCU=SAML21J18B make all mostly_clean
```

After a few moments, you will have the bootloader (8K) with USB terminal support ready in the file `sam_ba_XPLAINED_SAML21_SAML21J18B.bin`.

### Runing OpenOCD

#### Using Atmel EDBG

Plug the USB cable on the 'debug USB' port and run OpenOCD w/ the configuration file included in the `openocd-conf` directory.

```
$ openocd -f openocd-SAML21.cfg
```
If everything goes as planned, you will see this output:

```
cortex_m reset_config sysresetreq
Info : CMSIS-DAP: SWD  Supported
Info : CMSIS-DAP: Interface Initialised (SWD)
Info : CMSIS-DAP: FW Version = 03.25.01B6
Info : SWCLK/TCK = 1 SWDIO/TMS = 1 TDI = 1 TDO = 1 nTRST = 0 nRESET = 1
Info : CMSIS-DAP: Interface ready
Info : clock speed 400 kHz
Info : SWD DPIDR 0x0bc11477
Info : at91saml21j18.cpu: hardware has 4 breakpoints, 2 watchpoints
```

Do not close this terminal or exit out of OpenOCD for the next steps. 

#### Using ATMEL-ICE

If you are privileged enough to have an ATMEL ICE programmer, you can use it to program the bootloader on a brand new chip. 

Please note that you can also use any STM32F103 that you might have laying around... 
In any case, here is how to do it with the super expensive programmer!

First, ensure the 'target chip' is set properly to match your chip in the `openocd-ATMEL-ICE.cfg` file.

```T
# target chip
set CHIPNAME at91saml21j18
source [find target/at91samdXX.cfg]
```

 Plug your programmer's SAM port to the "Cortex Debug" header of your board. Then, power your board via the target USB port with and run:

```
$ openocd -f openocd-ATMEL-ICE.cfg
```

This is the output you will see on your console:

```
Info : Listening on port 6666 for tcl connections
Info : Listening on port 4444 for telnet connections
Info : CMSIS-DAP: SWD  Supported
Info : CMSIS-DAP: JTAG Supported
Info : CMSIS-DAP: FW Version = 01.00.0021
Info : CMSIS-DAP: Serial# = J42700024764
Info : CMSIS-DAP: Interface Initialised (SWD)
Info : SWCLK/TCK = 1 SWDIO/TMS = 1 TDI = 1 TDO = 1 nTRST = 0 nRESET = 1
Info : CMSIS-DAP: Interface ready
Info : clock speed 400 kHz
Info : SWD DPIDR 0x0bc11477
Info : at91saml21j18.cpu: hardware has 4 breakpoints, 2 watchpoints
Info : starting gdb server for at91saml21j18.cpu on 3333
Info : Listening on port 3333 for gdb connections
```

Do not close this terminal or exit out of OpenOCD for the next steps.

### Write Bootloader

If you are dealing with a new chip, you will have to write the bootloader and set the fuses to protect it w/ OpenOCD. First connect to the chip:

```
$ telnet 0 4444 
```
Now you can write the bootloader:

```
reset halt
at91samd bootloader 0
program sam_ba_Generic_x21J_SAML21J18B.bin verify
at91samd bootloader 8192	# this command is optional, recommended to prevent BOSSA from erasing bootloader
reset
shutdown
```
The precompiled bootloader image for the SAML21J18B was obtained from the Mattair's repository. You can find more information on how to compile the bootloader image from the documentation of his fork.

If everything goes well, you are now ready to write your programs using the Arduino core framework to the target (SAML21). This is the output of a successful verification:

```
** Programming Started **
auto erase enabled
Info : SAMD MCU: SAML21J18B (256KB Flash, 32KB RAM)
wrote 16384 bytes from file /home/unixjazz/code/SAML21-xplained-board/bootloader-arduino/sam_ba_Generic_x21J_SAML21J18B.bin in 3.540651s (4.519 KiB/s)
** Programming Finished **
** Verify Started **
verified 4964 bytes in 0.181510s (26.707 KiB/s)
** Verified OK **
** Resetting Target **
```

You should verify if everything is fine by checking if the board is properly enumerated by running

```
# dmesg
```

This should return the following output:

```
usb 3-1.2: New USB device found, idVendor=0000, idProduct=0000, bcdDevice= 2.00
usb 3-1.2: New USB device strings: Mfr=1, Product=2, SerialNumber=0
cdc_acm 3-1.2:1.0: ttyACM0: USB ACM device
```

Voilà! Toolchain and programmer are now ready for embedded fun to start.

### Flash Your Firmware

You are now ready to upload an Arduino sketch (such as `usb-term-test.bin` under the `sketches` directory) to test the USB terminal.

If you plug your board using the 'target USB' port, it should enumerate and you should have access to it. Now, you can use any tool to program your device using the USB port. You can use any sketch that makes use of USB output to test.

## Using BOSSA

Once you have the bootloader installed, you can use the `bossa` GUI utility or the `bossac` command-line utility to flash firmware onto the chip.

### Compile BOSSA

Just run `make` and `make install` to get it all ready. The binaries will be on the folder 'bin/' (`bossa`: GUI, and `bossac`: command-line tool).

### Flashing the Sample Firmware

First, test that the bootloader is successfully installed by attempting to flash the demo firmware:

```
$ bossac -e -w -v -o 8192 -b usb-term-test.bin
```

If it is successful, then you should get the following message:

```
Erase flash

Done in 0.901 seconds
Write 11856 bytes to flash (186 pages)
[==============================] 100% (186/186 pages)
Done in 0.068 seconds
Verify 11856 bytes of flash
[==============================] 100% (186/186 pages)
Verify successful
Done in 0.096 seconds
Set boot flash true
```

If you get the message

```
Device unsupported
```

and the device is properly enumerated, then press the reset button twice to enter bootloader mode.

To test to see if the sample firmware is working correctly, first open a terminal and run:

```
$ screen /dev/ttyACM0
```

Or use your favorite serial monitor, which we assume is 'screen'.

__Happy hw hacking!__

-- Sign-off: LFRM, 02/18/2021
