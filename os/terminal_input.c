#include "terminal_input.h"
#include <stddef.h> //NULL
#include "hal_plat.h" // t_malloc


terminal_input_t* terminal_input_create (char (*getchar) (void), void (*putchar) (char), int echo, int linelen) {
	terminal_input_t* instance;

	instance = (terminal_input_t*)t_malloc(sizeof(terminal_input_t));
	if (!instance) {
		return instance;
	}
	instance->line = (char*)t_malloc(linelen);
	if (!instance->line) {
		t_free(instance);
		instance = NULL;
		return instance;
	}
	instance->linelen = linelen;

	instance->getchar = getchar;
	instance->putchar = putchar;
	instance->echo = echo;
	instance->pos = 0;
	instance->rp = -1;

	return instance;
}


void terminal_input_destroy (terminal_input_t *instance) {
	if (!instance) {
		return;
	}
	if (instance->line) {
		t_free(instance->line);
	}
	t_free(instance);
}


int consolefile_read_canonical (struct generic_file_s* thisfile, int b, char* buf) {

	terminal_input_t *this = (terminal_input_t *)(thisfile->context);

	while (1) {

		if (this->rp >= 0) { // Line is ready to be read
			int bytes = 0;
			while (b && (this->rp < this->pos)) {
				buf[bytes] = this->line[this->rp] ;
				bytes++;
				b--;
				this->rp++;
			}
			if (this->rp == this->pos) {
				this->pos = 0;
				this->rp = -1;
			}
			return bytes;
		}

		// Constructing the line
		int run = 1;

		while (run) {
			int printable = this->echo;
			char c = this->getchar();

			if (c == '\r') {
				c = '\n';
			}
			switch (c) {

			  case 0x04:	// Ctrl + D
				run = 0;
				printable = 0;
				break;

			  case '\b': // backspace
				if (this->pos > 0) {
					if (printable) {
						this->putchar('\b'); // backspace
						this->putchar(' ');
					}
					this->pos -= 1;
					//reader->line[this->pos] = '\0';
				} else {
					printable = 0;
				}
				break;

			  case '\n':
				run = 0;
			  default:
				if (c < 0x20 || c > 0x7E) {
					printable = (c == '\n');
				}
				this->line[this->pos++] = c;
				if ((this->pos + 1) >= (this->linelen - 1)) {
					this->pos -= 1;
					printable = 0;
				}
				//reader->line[this->pos] = '\0';
				break;
			}
			if (printable) {
				this->putchar(c);  // echo
			}
		}
		this->rp = 0;
	}
	return 0; // never reached
}


int consolefile_read_raw (struct generic_file_s* thisfile, int b, char* buf) {

	terminal_input_t *this = (terminal_input_t *)(thisfile->context);

	int bytes = 0;
	char c;
	while (b) {
		c = this->getchar();
		if (c == 0x04) { // EOF for now
			break;
		}
		buf[bytes] = c;
		bytes++;
		b--;
        if (c == '\n') { // line end
            break;
        }
	}
	return bytes;
}


int consolefile_write (struct generic_file_s* thisfile, int count, char* buf) {

	terminal_input_t *this = (terminal_input_t *)(thisfile->context);

	int b = 0;
	while (count) {
		this->putchar(buf[b]);
		b++;
		count--;
	}
	return b;
}

