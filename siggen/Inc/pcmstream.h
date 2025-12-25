#ifndef INC_PCMSTREAM_H_
#define INC_PCMSTREAM_H_


#include "fifo.h" //fifo
#include "taskscheduler.h" //task
#include <stdint.h>



int nullsink_setup (fifo_t* in_stream);

int decfir_setup (fifo_t* in_stream, fifo_t* out_stream, int n, int bf);

int txmodem_setup (fifo_t* out_stream, int fs);
int rxmodem_setup (fifo_t* in_stream);

struct pcmsrc_context_s;
int pcmsrc_setup (fifo_t* out_stream, int fs, task_rc_t (*fn) (void*, uint16_t*), void (*cleanup) (void*), void* function_context);

#endif /* INC_PCMSTREAM_H_ */
