# General Coverage BPSK Transceiver, 400 MHz - 6 GHz

![sys](sys.jpg)

**[-->> RF board schematics <<--](schem.pdf)**

## BPSK Transmitter

Instead of using a conventional mixer, a Qorvo QPC6324 SPDT RF switch (toggled by the MCU) is connected to one of the differential pair outputs of the main RF signal source (MAX2871 PLL), and selects between the in-phase or out-of-phase version of the RF signal, depending on the modulation bit (1 or 0). This primitive BPSK modulation method was chosen because of its simplicity and availability of cheap parts, as well as due to general unavailability of broadband, low-power- or low LO level upconverting mixers. For good I/Q balance at the highest carrier frequencies, the differential pair RF PCB traces must be length-matched between the RF source and the two inputs of the RF switch.

![rfclose](rfclose.jpg)

## Zero-IF receiver

Block Diagram ([DSP code](https://github.com/szoftveres/RF_instruments/blob/main/dsp_stm32H7/Src/bpsk_transceiver.c)):

![block_diagram](BPSK_Rx_block_diagram.png)

One of the most common problem with zero-IF receivers is the mixer DC offset and the theoretical necessity of DC coupling the baseband signal all the way from the mixer to the ADC, through high-gain IF stages, which results in an inherent DC overload of the IF amplifier chain and the ADCs. [This very inspirational artcle](https://s53mv.s56g.net/archive/p024.pdf) describes a zero-IF receiver that has AC-coupled IF stages and subsequent AC-coupled digitizer. The main idea is that as long as the transmitter makes sure that the received baseband I and Q signals have plenty of AC (and almost no DC) content, AC coupling in the receiving IF stages is allowed, assuming that the lowest frequencies of the baseband signal are still well above the AC low cutoff frequency of the IF chain. Adding AC content is done at the transmitter side by [scrambling (whitening)](https://en.wikipedia.org/wiki/Scrambler), which results in frequent alternating between 1 and -1 states of the baseband (IF) signal in the receiver. 

Since the received signal has an inevitable carrier offset (Tx and Rx clock crystal frequency mismatch), a [Costas-loop](https://en.wikipedia.org/wiki/Costas_loop) de-rotates the baseband signal in the receiver. A Costas-loop has two stable states and is able to track slow phase drift (due to carrier mismatch) up to a certain slew-rate (and hence, up to a certain carrier mismatch). The loop changes state when the phase of the received signal abruptly changes by 180° (Tx symbol phase transition) that the Costas-loop cannot track. Hence, the preceding IF stages must support a high slew rate. The integral of the NCO control signal of the Costas loop is directly related to the phase angle rotation speed of the incoming carrier, hence it can be direclty used to calculate the absolute Tx/Rx carrier offset, in Hz.

In this receiver, the Costas-loop can track +/- 10 kHz carrier offset; the IF stages have a low cutoff frequency of ~200 Hz and a max. bandwidth of 1 MHz, allowing very quick transitions. The total IF gain is ~40 dB.

The design of the DSP/MCU board is covered [here](https://github.com/szoftveres/RF_instruments/tree/main/dsp_stm32H7).

![dspboard](https://github.com/szoftveres/RF_instruments/blob/main/dsp_stm32H7/photo.jpg)


