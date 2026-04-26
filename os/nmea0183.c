#include "nmea0183.h"
#include "hal_plat.h" // t_malloc
#include <string.h> // strcmp
#include "globals.h"
#include "lex/lex.h"  // This is somewhat annoying


// this is how the (portable) lex reads a new byte
int nmea0183_lex_read (lex_instance_t *instance, int *b) {
	nmea0183_t *nmea0183 = (nmea0183_t*)instance->context; // context of lex is the parser
	*b = nmea0183->getchar(nmea0183);
	return 1;
}


// this is how lex prints an error message on this system
void nmea0183_lex_error (lex_instance_t *instance, int c, const char *str) {
}


// Constructor
nmea0183_t* nmea0183_create (char (*getchar) (nmea0183_t*), void *context) {
	nmea0183_t* instance;

	instance = (nmea0183_t*)t_malloc(sizeof(nmea0183_t));
	if (!instance) {
	    return (instance);
	}
	instance->getchar = getchar;
	instance->ctxt = context;

	return instance;
}


// Destructor
void nmea0183_destroy (nmea0183_t *instance) {
	if (!instance) {
		return;
	}
	t_free(instance);
}


int nmea0183_parse_ll (nmea0183_t *instance, lex_instance_t *lex, int *ii, int *ff) {
	const int resolution = 10000;
	int i = integer_value(lex);
	if (!lex_get(lex, T_INTEGER, NULL)) {
		return 0;
	}

	*ii = i / 100;
	*ff = (i % 100) * resolution;

	if (!lex_get(lex, T_FULLSTOP, NULL)) {
		return 0;
	}
	i = integer_value(lex);
	if (!lex_get(lex, T_INTEGER, NULL)) {
		return 0;
	}

	while (i > resolution) {
		i /= 10;
	}
	while (i && (i < ((resolution / 10) - 1))) {
		i *= 10;
	}
	*ff += i;

	*ff /= 6; // convert degree to frac

	return 1;
}


int nmea0183_parse_time (nmea0183_t *instance, lex_instance_t *lex) {
	int i = integer_value(lex);
	if (!lex_get(lex, T_INTEGER, NULL)) {
		return 0;
	}

	instance->hour = i / 10000;

	instance->min = i / 100;
	instance->min %= 100;

	instance->sec = i % 100;

	return 1;
}


int nmea0183_parse_gll (nmea0183_t *instance, lex_instance_t *lex) {

	if (!lex_get(lex, T_COMMA, NULL)) {
		return 0;
	}

	if (!nmea0183_parse_ll(instance, lex, &(instance->lat_i), &(instance->lat_f))) {
		return 0;
	}

	if (!lex_get(lex, T_COMMA, NULL)) {
		return 0;
	}
	if (lex_get(lex, T_IDENTIFIER, "S")) {
		instance->lat_i *= -1;
	} else if (!lex_get(lex, T_IDENTIFIER, "N")) {
		return 0;
	}
	if (!lex_get(lex, T_COMMA, NULL)) {
		return 0;
	}

	if (!nmea0183_parse_ll(instance, lex, &(instance->lon_i), &(instance->lon_f))) {
		return 0;
	}

	if (!lex_get(lex, T_COMMA, NULL)) {
		return 0;
	}
	if (lex_get(lex, T_IDENTIFIER, "W")) {
		instance->lon_i *= -1;
	} else if (!lex_get(lex, T_IDENTIFIER, "E")) {
		return 0;
	}

	if (!lex_get(lex, T_COMMA, NULL)) {
		return 0;
	}

	if (!nmea0183_parse_time(instance, lex)) {
		return 0;
	}

	return 1;
}


int nmea0183_update (nmea0183_t *instance) {
	int rc = 0;
	lex_instance_t *lex = lex_create(instance, 64, nmea0183_lex_read, nmea0183_lex_error, LEX_OCTAL_AS_INT);
	if (!lex) {
    	return 0;
	}
	lex_reset(lex);

	do {
		if (lex_get(lex, T_EOF, NULL)) {
			break;
		}
		if (lex_get(lex, T_DOLLAR, NULL)) {

			if (lex_get(lex, T_IDENTIFIER, "GNGLL")) {
				rc = nmea0183_parse_gll(instance, lex);
			} else if (lex_get(lex, T_IDENTIFIER, "GPGLL")) {
				rc = nmea0183_parse_gll(instance, lex);
			}
		}
		next_token(lex);
	} while (!rc);

	lex_destroy(lex);

	return rc;
}


