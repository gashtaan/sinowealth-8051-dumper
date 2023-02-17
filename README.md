# Flash memory dumper for 8051-based SinoWealth MCUs
Beware, this project is rather POC than user-friendly tool. I successfully used it to dump firmware from SH68F881W MCU in Genesis Thor 300 keyboard, but it should work with other chips of the same type.

## How to build
The project should be compiled easily in Arduino IDE and uploaded to some ATmega328P-based board or chip directly. Before the build, check the chip configuration in `config.h` and update it if needed.

## Hot to use
Connect D2-D5 pins and power rail of your ATmega host to correspoding JTAG pins of SinoWealth MCU and power the host up. You should immediately observe messages (and hopefully dumped firmware) on host UART ports.

The host has to communicate with target MCU within few tens of milliseconds since powering up. Therefore if you use one of Arduino boards, I recommend you to change bootloader to Optiboot, to get rid of bootloader delay. If it's not enough, next thing to try, is to set SUT fuses of ATmega MCU to 4.1ms pre-delay (65ms is the default).
