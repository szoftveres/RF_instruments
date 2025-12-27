#ifndef __MAX2871__H_
#define __MAX2871__H_

#include <stdint.h> // uint32


#define MAX2871_REGS (6)


typedef struct {
	uint32_t 	registers[MAX2871_REGS];
	uint32_t	F;
	uint32_t	N;
	uint32_t	M;
	uint32_t	diva;

	void (*register_write) (uint32_t);  // LE low, write a register, LE high
	int (*check_ld) (void);   			// ret: 1 = lock
	void (*idle_wait) (void); 			// LE high, and wait min. 30ms

} max2871_t;

max2871_t* max2871_create (void (*register_write) (uint32_t), int (*check_ld) (void), void (*idle_wait) (void));
void max2871_destroy (max2871_t* instance);
double max2871_freq (max2871_t* instance, double khz);
int max2871_rfa_power (max2871_t* instance, int dbm);
int max2871_ld (max2871_t* instance);
void max2871_rfa_out (max2871_t* instance, int onoff);


#endif
