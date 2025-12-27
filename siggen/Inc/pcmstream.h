#ifndef INC_PCMSTREAM_H_
#define INC_PCMSTREAM_H_


#include "fs_broker.h" // fs
#include "fifo.h" //fifo
#include "taskscheduler.h" //task
#include <stdint.h>

typedef struct __attribute__((packed)) {
	char wave_header[4]; // Contains "WAVE"

	// Format Header
	char fmt_header[4]; // Contains "fmt " (includes trailing space)
	int32_t fmt_chunk_size; // Should be 16 for PCM
	int16_t audio_format; // Should be 1 for PCM. 3 for IEEE Float
	int16_t num_channels;
	int32_t sample_rate;
	int32_t byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Channel Sample
	int16_t sample_alignment; // num_channels * Bytes Per Channel Sample
	int16_t bit_depth; // Number of bits per sample

	// Data
	char data_header[4]; // Contains "data"
	int32_t data_bytes; // Number of bytes in data. Number of samples * num_channels * sample byte size
} wave_header_t;


typedef struct __attribute__((packed)) riff_header_s {
    // RIFF Header
    char riff_header[4]; // Contains "RIFF"
    uint32_t wav_size; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8

    wave_header_t wave;

    char bytes[0]; // Remainder of wave file is bytes
} riff_header_t;


int wav_read_header (fs_broker_t* fs, int fd, int* samplerate, int* channels, int* bytespersample, int* samples);
int wav_write_header (fs_broker_t* fs, int fd, int samplerate, int channels, int bytespersample, int samples);


int wavsrc_setup (int fd, fifo_t* out_stream);
int wavsink_setup (fifo_t* in_stream, int fd);

int nullsink_setup (fifo_t* in_stream);

int decfir_setup (fifo_t* in_stream, fifo_t* out_stream, int n, int bf);

int txmodem_setup (fifo_t* out_stream, int fs, int fc);
int rxmodem_setup (fifo_t* in_stream, int fc);

struct pcmsrc_context_s;
int pcmsrc_setup (fifo_t* out_stream, int fs, task_rc_t (*fn) (void*, uint16_t*), void (*cleanup) (void*), void* function_context);

#endif /* INC_PCMSTREAM_H_ */
