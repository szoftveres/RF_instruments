#ifndef _LINE_READER_H_
#define _LINE_READER_H_


typedef struct line_reader_s {
	int linelen;
	void *context;
	char *line;
} line_reader_t;


line_reader_t* line_reader_create (int linelen, void *context);
void line_reader_destroy (line_reader_t *instance);


#endif /* OS_LINE_READER_H_ */
