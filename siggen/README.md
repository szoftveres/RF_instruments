## Programmable 6 GHz RF signal generator 

 * Frequency range: 23.5 MHz - 6 GHz
 * Level range: -5 dBm - -35 dBm

# [Schematics](https://github.com/szoftveres/RF_instruments/tree/main/siggen/schematics.pdf)

### Programming

The signal generator has a USB UART interface (FTDI FT230x) to issue commands to and an EEPROM to store level calibration data, configuration and program.

Frequency can be set by assigning a value in kHz to the `freq` resource variable, e.g. `freq = 915000` will set the frequency to 915 MHz, `freq += 1000` will increase the current frequency by 1 MHz. The `print` command can be used to print the value of an expression, e.g. `print freq` will print the current frequency value. Values can also come from variables, e.g. the `a = 200 + 10000 * 9`, `freq = a + 10000`, `b = freq` commands will result in the frequency  being set to 100.2 MHz, as well as the value of `b` being set to `100200`. Parentheses `(`, `)` can be used in expressions, e.g. `a = (150 + b) * freq`

Similarly, the RF output level can be set and retrieved by using the `level` resource variable.

Besides the resource variables, there are 26 general purpose variables (`a` - `z`) that can store 32-bit integer values.

RF output can be turned on or off with the `rfon` and `rfoff` commands.

The current configuration in can be printed out with the `cfg` command.

More commands can be placed in a single command line, separated by te `;` character; e.g. `rfon; sleep 200; rfoff` will briefly turn the output on for 200 ms.

Besides single-line commands, the device can store a short program, which can be saved to the EEPROM and recalled later. The program consists of numbered program lines and can be viewed by using the `list` command:

```
> list
 0 "rfoff"
 1 "freq = 900000"
 2 "freq += 1000"
 3 "rfon; sleep 200; rfoff"
 4 "if freq <= 930000 \"goto 2\""
 5 "rfoff"
 6 "end"
 7 " "
 8 " "
 9 " "
```
The above program will run a frequency sweep from 900 MHz to 930 MHz.

A program line can be edited by first typing the line number, followed by the program line in a C-string literal format, e.g. `1 "freq = 900000"`.
If a program line contains nested strings, the `\` character can be used before the nested double-quote characters, e.g. in lie `4 "if freq <= 930000 \"goto 2\""`, the `if` keyword also expects a program line string literal after the evaluated expression.








