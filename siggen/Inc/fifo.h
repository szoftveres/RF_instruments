#ifndef INC_FIFO_H_
#define INC_FIFO_H_

typedef struct fifo_s {
	char *buf;
	int length;
	int wordlength;
	int ip;
	int op;
	int data;
	int writers;
	int readers;
	struct fifo_s *next; // So that multiple of these can be made a list
} fifo_t;

fifo_t* fifo_create (int length, int wordlength);
void fifo_destroy (fifo_t *instance);
int fifo_isempty (fifo_t *instance);
int fifo_push (fifo_t* instance, void *c);
int fifo_pop (fifo_t* instance, void *c);
void fifo_push_or_sleep (fifo_t* instance, void *c);
void fifo_pop_or_sleep (fifo_t* instance, void *c);

#endif /* INC_FIFO_H_ */
