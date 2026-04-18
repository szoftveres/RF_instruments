#ifndef INC_TERMINAL_INPUT_H_
#define INC_TERMINAL_INPUT_H_


#include "line_reader.h"


typedef struct terminal_input_s {
	char (*getchar) (void);
} terminal_input_t;


terminal_input_t* terminal_input_create (char (*getchar) (void));
void terminal_input_destroy (terminal_input_t *instance);
char* terminal_get_line (line_reader_t* reader);

#endif /* INC_TERMINAL_INPUT_H_ */
