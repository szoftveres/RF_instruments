## Generic single-threaded OS library for small embedded systems

This library was made to be used on small single-CPU embedded systems as the main human access and programming layer.

Features:
 * Command line interpreter
   * BASIC-like scripting language, syntax- and expression evaluation
   * Ability to store, save, load and run programs
   * Ability to add platform- and application specific commands
 * File system abstraction layer integrates different filesystems under a single, unified programming interface
   * [FAT-like filesystem](https://github.com/szoftveres/RF_instruments/blob/main/os/fatsmall_fs.c)
   * STM32 SD FAT
   * Unix FS
 * Block device abstraction layer
   * RAM block
   * I2C EEPROM, SD Card, CF Card, Physical disk, etc..
 * Chainable DSP functions
   * DFT, IFT, OFDM modem, Downsampling filter, MS WAV file read/write, etc..


Projects using this library:
 * [6 GHz RF signal generator](https://github.com/szoftveres/RF_instruments/tree/main/siggen)
 * [STM32H7 DSP borad / controller](https://github.com/szoftveres/RF_instruments/tree/main/dsp_stm32H7)
 * [Unix runtime / development environment](https://github.com/szoftveres/RF_instruments/tree/main/unix)

