## Programmable 6 GHz RF signal generator 

 * Frequency range: 23.5 MHz - 6 GHz
 * Level range: -5 dBm - -35 dBm

# [Schematics](https://github.com/szoftveres/RF_instruments/tree/main/siggen/schematics.pdf)

### Programming

The signal generator has a USB UART interface (FTDI FT230x) to issue commands to and an EEPROM to store level calibration data, configuration and program.

Frequency can be set by assigning value to the `freq` resource variable, in kHz; e.g. `freq 915000` will set the frequency to 915 MHz, `freq += 1000` will increase the current frequency by 1 MHz. The `print` command can be used to print the value of an expression, e.g. `print freq` will print the current frequency value. Values can also come from variables, e.g. the `a = 200 + 10000 * 9`, `freq = a + 10000`, `b = freq` commands will result in the frequency  being set to 100.2 MHz, as well as the value of `b` being set to `100200`. Parentheses `(` and `)` can be used in expressions, e.g. `a = (150 + b) * freq`

Similarly, the RF output level can be set and retrieved by using the `level` resource variable.

Besides the resource variables, there are 26 general purpose variables (`a` - `z`) that can store 32-bit integer values.

