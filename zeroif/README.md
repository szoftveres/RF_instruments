# General Coverage BPSK Transceiver, 400 MHz - 6 GHz

**[-->> RF board schematics <<--](schem.pdf)**

## BPSK Transmitter

The MAX2871 PLL has differential, individually driven outputs, meaning that each output line can be used in single-ended mode. Two (180° out of phase) lines of a differential pair is connected to the inputs of a SPDT RF switch, and the MCU is selecting between the two inputs, thereby creating a BPSK modulated RF signal at the output of the switch. For good I/Q balance at the highest carrier frequencies, the two RF PCB traces must be length-matched between the PLL and the SPDT RF switch.

## Zero-IF BPSK receiver

One of the most common problem with zero-IF receivers is the mixer DC offset and the theoretical necessity of DC coupling the baseband signal all the way from the mixer to the ADC through high-gain IF stages, which results in an inherent DC overload of the IF amplifier chain and the ADCs. [This very inspirational artcle](https://s53mv.s56g.net/archive/p024.pdf) describes a zero-IF receiver that has AC-coupled IF stages and subsequent AC-coupled digitizer. The main idea is that the transmitter makes sure that the received I and Q signals never linger at DC levels longer than just several symbols, by the means of scrambling (whitening) the transmitted signal, thereby enabling AC coupling in the receiver. Only the low cutoff frequency of the AC coupling must be chosen carefully.

Since the received signal has an inevitable carrier offset (Tx and Rx clock crystal frequency mismatch), a [Costas-loop](https://en.wikipedia.org/wiki/Costas_loop) de-rotates the baseband signal in the receiver. A Costas-loop has two stable states and is able to track slow phase drift (due to carrier mismatch) up to a certain slew-rate (and hence, up to a certain carrier mismatch). The loop changes state when the phase of the received signal abruptly changes by 180° (Tx symbol phase transition) that the Costas-loop cannot track. Hence, the preceding IF stages must support a high slew rate.

In this receiver, the Costas-loop can track +/- 10 kHz carrier offset; the IF stages have a low cutoff frequency of ~200 Hz and a max. bandwidth of 1 MHz, allowing very quick transitions. The total IF gain is ~40 dB.

Receiver Block Diagram ([DSP code](https://github.com/szoftveres/RF_instruments/blob/main/dsp_stm32H7/Src/bpsk_transceiver.c)):

![block_diagram](BPSK_Rx_block_diagram.png)
