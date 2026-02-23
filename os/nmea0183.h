#ifndef _NMEA0183_H_
#define _NMEA0183_H_



typedef struct {
	int lat_i;
	int lat_f;

	int lon_i;
	int lon_f;

	int hour;
	int min;
	int sec;
	char (*getchar) (void);
} nmea0183_t;



nmea0183_t* nmea0183_create (char (*getchar) (void));
void nmea0183_destroy (nmea0183_t *instance);
int nmea0183_update (nmea0183_t *instance);


#endif /* _NMEA0183_H_ */
