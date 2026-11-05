# Zero-IF BPSK transceiver, 400 MHz - 6 GHz

The most common problem with zero-IF receivers is the theoretical necessity of DC coupling of the IF stages, and the mixer DC offset, which inherently overloads a high-gain IF amplifier chain and subsequent ADCs. A very inspirational [artcle](https://s53mv.s56g.net/archive/p024.pdf) describes a zero-IF receiver that has AC-coupled IF stages and subsequent AC-coupled digitizer. The main idea is that if AC coupling is inevitable for avoiding DC overload and sufficient IF gain, the transmitter makes sure that the received I and Q signals never linger at DC levels longer than just several symbols, by the means of scrambling (whitening) the transmitted signal. By choosing a sufficiently low cutoff frequency for AC coupling, the IF stages can operate at high-gain without overload.

**[-->> RF board schematics <<--](schem.pdf)**

Functional Block Diagram:

![block_diagram](BPSK_Rx_block_diagram.png)

Receiver [DSP code](https://github.com/szoftveres/RF_instruments/blob/main/dsp_stm32H7/Src/bpsk_transceiver.c)

