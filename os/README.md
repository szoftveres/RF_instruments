## Generic single-threaded OS library for small embedded systems

 * Command line interpreter
   * BASIC-like scripting language, syntax- and expression evaluation
   * Ability to store, save, load and run programs
   * Platform- and application specific commands
 * Custom minimal [FAT-like filesystem](https://github.com/szoftveres/RF_instruments/blob/main/os/fatsmall_fs.c)
 * File system abstraction layer integrates multiple filesystems under a single, unified programming interface
   * [FAT-like filesystem](https://github.com/szoftveres/RF_instruments/blob/main/os/fatsmall_fs.c)
   * STM32 SD FAT
   * Unix FS
 * Block device abstraction layer
   * RAM block
   * I2C EEPROM, SD Card, CF Card, Physical disk, etc..
 * DSP chain
   * DFT, IFT, OFDM modem, Downsampling filter, etc..

Uses:

 * [Stored-program 6 GHz RF signal generator](https://github.com/szoftveres/RF_instruments/tree/main/siggen)
 * [Unix runtime / development environment](https://github.com/szoftveres/RF_instruments/tree/main/unix)


