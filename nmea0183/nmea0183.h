#ifndef _NMEA0183_H_
#define _NMEA0183_H_



typedef struct nmea0183_s{
	int lat_i;
	int lat_f;

	int lon_i;
	int lon_f;

	int hour;
	int min;
	int sec;
	char (*getchar) (struct nmea0183_s*);
	void *ctxt;
} nmea0183_t;



nmea0183_t* nmea0183_create (char (*getchar) (nmea0183_t*), void* context);
void nmea0183_destroy (nmea0183_t *instance);
int nmea0183_update (nmea0183_t *instance);


#endif /* _NMEA0183_H_ */
