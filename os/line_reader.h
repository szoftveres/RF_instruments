#ifndef _LINE_READER_H_
#define _LINE_READER_H_


typedef struct line_reader_s {
	int linelen;
	void *context;
	char *line;
	char* (*getline) (struct line_reader_s*);
} line_reader_t;


line_reader_t* line_reader_create (int linelen, char* (*getline) (struct line_reader_s*), void *context);
void line_reader_destroy (line_reader_t *instance);


#endif /* OS_LINE_READER_H_ */
