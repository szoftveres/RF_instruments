#ifndef INC_TERMINAL_INPUT_H_
#define INC_TERMINAL_INPUT_H_

typedef struct terminal_input_s {
	int linelen;
	char *line;
	char (*getchar) (void);
} terminal_input_t;


terminal_input_t* terminal_input_create (char (*getchar) (void), int linelen);
void terminal_input_destroy (terminal_input_t *instance);
char* terminal_get_line (terminal_input_t* instance, const char* prompt, int echo);

#endif /* INC_TERMINAL_INPUT_H_ */
