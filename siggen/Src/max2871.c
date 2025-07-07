#include "max2871.h"
#include <math.h> // math functions, double
#include <stdlib.h> // malloc


uint32_t max2871GetRegister (max2871_instance_t* instance, uint8_t reg) {
	return instance->registers[reg];
}

// 0 = Frac-N, 1 = Int-N
void max2871Set_INT (max2871_instance_t* instance, uint32_t j) {
	instance->registers[0] &= ~(0x1 << 31);
	instance->registers[0] |= j << 31;
}

// Integer Division Value
void max2871Set_N (max2871_instance_t* instance, uint32_t j) {
	instance->registers[0] &= ~(0xFFFF << 15);
	instance->registers[0] |= j << 15;
}

// Fractional Division Value
void max2871Set_FRAC (max2871_instance_t* instance, uint32_t j) {
	instance->registers[0] &= ~(0xFFF << 3);
	instance->registers[0] |= j << 3;
}

void max2871Set_CPOC (max2871_instance_t* instance, uint32_t j) {
	instance->registers[1] &= ~(0x1 << 31);
	instance->registers[1] |= j << 31;
}

// Charge Pump Linearity
void max2871Set_CPL (max2871_instance_t* instance, uint32_t j) {
	instance->registers[1] &= ~(0x3 << 29);
	instance->registers[1] |= j << 29;
}

// Charge Pump Test
void max2871Set_CPT (max2871_instance_t* instance, uint32_t j) {
	instance->registers[1] &= ~(0x3 << 27);
	instance->registers[1] |= j << 27;
}

// Phase Value
void max2871Set_P (max2871_instance_t* instance, uint32_t j) {
	instance->registers[1] &= ~(0xFFF << 15);
	instance->registers[1] |= j << 15;
}

// Fractional Modulus
void max2871Set_M (max2871_instance_t* instance, uint32_t j) {
	instance->registers[1] &= ~(0xFFF << 3);
	instance->registers[1] |= j << 3;
}

// Lock Detect Speed
void max2871Set_LDS (max2871_instance_t* instance, uint32_t j) {
	instance->registers[2] &= ~(0x1 << 31);
	instance->registers[2] |= j << 31;
}

// Low Noise / Spur Mode
void max2871Set_SDN (max2871_instance_t* instance, uint32_t j) {
	instance->registers[2] &= ~(0x3 << 29);
	instance->registers[2] |= j << 29;
}

// Mux Config
void max2871Set_MUX (max2871_instance_t* instance, uint32_t j) {
	instance->registers[2] &= ~(0x7 << 26);
	instance->registers[5] &= ~(0x1 << 18);
	instance->registers[2] |= (j & 0x7) << 26;
	instance->registers[5] |= ((j & 0x8) >> 3) << 18;
}

// Ref Doubler Mode
void max2871Set_DBR (max2871_instance_t* instance, uint32_t j) {
	instance->registers[2] &= ~(0x1 << 25);
	instance->registers[2] |= j << 25;
}

// Ref Div 2 Mode
void max2871Set_RDIV2 (max2871_instance_t* instance, uint32_t j) {
	instance->registers[2] &= ~(0x1 << 24);
	instance->registers[2] |= j << 24;
}

// Ref Divider Mode
void max2871Set_R (max2871_instance_t* instance, uint32_t j) {
	instance->registers[2] &= ~(0x3FF << 14);
	instance->registers[2] |= j << 14;
}

// Double Buffer
void max2871Set_REG4DB (max2871_instance_t* instance, uint32_t j) {
	instance->registers[2] &= ~(0x1 << 13);
	instance->registers[2] |= j << 13;
}

// Charge Pump Tristate Mode
void max2871Set_TRI (max2871_instance_t* instance, uint32_t j) {
	instance->registers[2] &= ~(0x1 << 4);
	instance->registers[2] |= j << 4;
}

// Lock Detect Function
void max2871Set_LDF (max2871_instance_t* instance, uint32_t j) {
	instance->registers[2] &= ~(0x1 << 8);
	instance->registers[2] |= j << 8;
}
// Lock Detect Precision
void max2871Set_LDP (max2871_instance_t* instance, uint32_t j) {
	instance->registers[2] &= ~(0x1 << 7);
	instance->registers[2] |= j << 7;
}

// Phase Detector Polarity
void max2871Set_PDP (max2871_instance_t* instance, uint32_t j) {
	instance->registers[2] &= ~(0x1 << 6);
	instance->registers[2] |= j << 6;
}

// Shutdown Mode
void max2871Set_SHDN (max2871_instance_t* instance, uint32_t j) {
	instance->registers[2] &= ~(0x1 << 5);
	instance->registers[2] |= j << 5;
}

// Charge Pump
void max2871Set_CP (max2871_instance_t* instance, uint32_t j) {
	instance->registers[2] &= ~(0xF << 9);
	instance->registers[2] |= j << 9;
}

// Counter Reset
void max2871Set_RST (max2871_instance_t* instance, uint32_t j) {
	instance->registers[2] &= ~(0x1 << 3);
	instance->registers[2] |= j << 3;
}

// VCO Selection
void max2871Set_VCO (max2871_instance_t* instance, uint32_t j) {
	instance->registers[3] &= ~(0x3F << 26);
	instance->registers[3] |= j << 26;
}

// VAS Shutdown
void max2871Set_VAS_SHDN (max2871_instance_t* instance, uint32_t j) {
	instance->registers[3] &= ~(0x1 << 25);
	instance->registers[3] |= j << 25;
}

// VAS Temp, also sets VAS_DLY
void max2871Set_VAS_TEMP (max2871_instance_t* instance, uint32_t j) {
	instance->registers[3] &= ~(0x1 << 24);
	instance->registers[3] |= 0 << 24;
	instance->registers[5] &= ~(0x3 << 29);
	instance->registers[5] |= j << 29;
	instance->registers[5] |= j << 30;
}

// Cycle Slip Mode
void max2871Set_CSM (max2871_instance_t* instance, uint32_t j) {
	instance->registers[3] &= ~(0x1 << 18);
	instance->registers[3] |= j << 18;
}

// Mute Delay Mode
void max2871Set_MUTEDEL (max2871_instance_t* instance, uint32_t j) {
	instance->registers[3] &= ~(0x1 << 17);
	instance->registers[3] |= j << 17;
}

// Clock Divider Mode
void max2871Set_CDM (max2871_instance_t* instance, uint32_t j) {
	instance->registers[3] &= ~(0x3 << 15);
	instance->registers[3] |= j << 15;
}

// Clock Divider Value
void max2871Set_CDIV (max2871_instance_t* instance, uint32_t j) {
	instance->registers[3] &= ~(0xFFF << 3);
	instance->registers[3] |= j << 3;
}

// Shutdown VCO LDO
void max2871Set_SDLDO (max2871_instance_t* instance, uint32_t j) {
	instance->registers[4] &= ~(0x1 << 28);
	instance->registers[4] |= j << 28;
}

// Shutdown VCO Divider
void max2871Set_SDDIV (max2871_instance_t* instance, uint32_t j) {
	instance->registers[4] &= ~(0x1 << 27);
	instance->registers[4] |= j << 27;
}

// Shutdown Ref Input
void max2871Set_SDREF (max2871_instance_t* instance, uint32_t j) {
	instance->registers[4] &= ~(0x1 << 26);
	instance->registers[4] |= j << 26;
}

// Band-Select MSBs
void max2871Set_BS (max2871_instance_t* instance, uint32_t j) {
	instance->registers[4] &= ~(0x3 << 24);
	instance->registers[4] &= ~(0xFF << 12);
	instance->registers[4] |= ((j & 0x300) >> 8) << 24;
	instance->registers[4] |= (j & 0xFF) << 12;
}

// VCO Feedback Mode
void max2871Set_FB (max2871_instance_t* instance, uint32_t j) {
	instance->registers[4] &= ~(0x1 << 23);
	instance->registers[4] |= j << 23;
}

// RFOUT_ Output Divider Mode
void max2871Set_DIVA (max2871_instance_t* instance, uint32_t j) {
	instance->registers[4] &= ~(0x7 << 20);
	instance->registers[4] |= j << 20;
}

// Shutdown VCO VCO
void max2871Set_SDVCO (max2871_instance_t* instance, uint32_t j) {
	instance->registers[4] &= ~(0x1 << 11);
	instance->registers[4] |= j << 11;
}

// RFOUT Mute Until Lock Detect
void max2871Set_MTLD (max2871_instance_t* instance, uint32_t j) {
	instance->registers[4] &= ~(0x1 << 10);
	instance->registers[4] |= j << 10;
}

// RFOUTB Output Path Select
void max2871Set_BDIV (max2871_instance_t* instance, uint32_t j) {
	instance->registers[4] &= ~(0x1 << 9);
	instance->registers[4] |= j << 9;
}

// RFOUTB Output Enable
void max2871Set_RFB_EN (max2871_instance_t* instance, uint32_t j) {
	instance->registers[4] &= ~(0x1 << 8);
	instance->registers[4] |= j << 8;
}
// RFOUTB Output Power
void max2871Set_BPWR (max2871_instance_t* instance, uint32_t j) {
	instance->registers[4] &= ~(0x3 << 6);
	instance->registers[4] |= j << 6;
}

// RFOUTA Output Enable
void max2871Set_RFA_EN (max2871_instance_t* instance, uint32_t j) {
	instance->registers[4] &= ~(0x1 << 5);
	instance->registers[4] |= j << 5;
}
// RFOUTA Output Power
void max2871Set_APWR (max2871_instance_t* instance, uint32_t j) {
	instance->registers[4] &= ~(0x3 << 3);
	instance->registers[4] |= j << 3;
}

// PLL Shutdown
void max2871Set_SDPLL (max2871_instance_t* instance, uint32_t j) {
	instance->registers[5] &= ~(0x1 << 25);
	instance->registers[5] |= j << 25;
}

// F01
void max2871Set_F01 (max2871_instance_t* instance, uint32_t j) {
	instance->registers[5] &= ~(0x1 << 24);
	instance->registers[5] |= j << 24;
}

// Lock Detect Function
void max2871Set_LD (max2871_instance_t* instance, uint32_t j) {
	instance->registers[5] &= ~(0x3 << 22);
	instance->registers[5] |= j << 22;
}

// Reserved Values
void max2871Set_Reserved (max2871_instance_t* instance) {
	instance->registers[3] &= ~(0x7F << 17);
	instance->registers[4] &= ~(0x1 << 10);
	instance->registers[5] &= ~(0x3F << 25);
}



// Writes registers 5 - 0 to MAX2871
void max2871WriteRegisters (max2871_instance_t* instance) {

	for (int i = 0; i < MAX2871_REGS; i++) {
		instance->register_write(max2871GetRegister(instance, i));
	}
}

//#define MOD (4095)
#define MOD (4000)


// Setup of the MAX2871 PLL, 50MHz, output off
max2871_instance_t* max2871_init (void (*register_write) (uint32_t), int (*check_ld) (void), void (*idle_wait) (void)) {

	max2871_instance_t* instance = (max2871_instance_t*) malloc(sizeof(max2871_instance_t));
	if (!instance) {
		return instance;
	}
	instance->register_write = register_write;
	instance->check_ld = check_ld;
	instance->idle_wait = idle_wait;

	instance->registers[0] = 0x0;
	instance->registers[1] = 0x1;
	instance->registers[2] = 0x2;
	instance->registers[3] = 0x3;
	instance->registers[4] = 0x8c; // Disable RFOUT_A and _B
	instance->registers[5] = 0x5;

	instance->idle_wait();

	// Initial writing of registers, with wait time

	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < MAX2871_REGS; i++) {
			instance->register_write(max2871GetRegister(instance, i));
			instance->idle_wait();
		}
		instance->idle_wait();
	}

	// Set initial register values
	max2871Set_INT(instance, 0); 		// Frac N
	max2871Set_N(instance, 83);		// Init to 50Mhz
	max2871Set_FRAC(instance, 1365);
	max2871Set_CPOC(instance, 0);
	max2871Set_CPL(instance, 0);
	max2871Set_CPT(instance, 0);
	max2871Set_P(instance, 1);
	max2871Set_M(instance, MOD);
	max2871Set_LDS(instance, 1);      // fPFD > 32MHz
	max2871Set_SDN(instance, 0);
	max2871Set_MUX(instance, 0xC); 	// Reg 6 Readback 0xC
	max2871Set_DBR(instance, 1);      //fPFD = fREF x [(1 + DBR) / (R x (1 + RDIV2))]
	max2871Set_RDIV2(instance, 0);
	max2871Set_R(instance, 1); 		// 40.0 MHz f_PFD
	max2871Set_REG4DB(instance, 0);
	max2871Set_CP(instance, 15);
	max2871Set_LDF(instance, 0);
	max2871Set_LDP(instance, 0);
	max2871Set_PDP(instance, 1);
	max2871Set_SHDN(instance, 0);
	max2871Set_TRI(instance, 0);
	max2871Set_RST(instance, 0);
	max2871Set_VCO(instance, 0);
	max2871Set_VAS_SHDN(instance, 0);
	max2871Set_VAS_TEMP(instance, 1);
	max2871Set_CSM(instance, 0);
	max2871Set_MUTEDEL(instance, 1);
	max2871Set_CDM(instance, 0);
	max2871Set_CDIV(instance, 400);   // 38 original XXX
	max2871Set_SDLDO(instance, 0);
	max2871Set_SDDIV(instance, 0);
	max2871Set_SDREF(instance, 0);
	max2871Set_BS(instance, 800);		//BS = fPFD / 50KHz
	max2871Set_FB(instance, 1);
	max2871Set_DIVA(instance, 6);
	max2871Set_SDVCO(instance, 0);
	max2871Set_MTLD(instance, 1);
	max2871Set_BDIV(instance, 0);
	max2871Set_RFB_EN(instance, 0);
	max2871Set_BPWR(instance, 0);
	max2871Set_RFA_EN(instance, 1);	// Begin with power on
	max2871Set_APWR(instance, 2);
	max2871Set_SDPLL(instance, 0);
	max2871Set_F01(instance, 0);
	max2871Set_LD(instance, 1);
	max2871Set_Reserved(instance);

	// Update info in struct

	// Send updated registers over SPI
	max2871WriteRegisters(instance);

	instance->idle_wait();

	return instance;
}


double max2871_freq (max2871_instance_t* instance, double khz) {

    int  diva = -1;
    double div = 0.0;
    double fPFD_khz = 40000.0; // 2x refosc


    if (khz < 23500.0) {
        return -1.0;
    } else if (khz <   46875.0) {
        diva = 7;
        div = 128.0;
    } else if (khz <   93750.0) {
        diva = 6;
        div = 64.0;
    } else if (khz <  187500.0) {
        diva = 5;
        div = 32.0;
    } else if (khz <  375000.0) {
        diva = 4;
        div = 16.0;
    } else if (khz <  750000.0) {
        diva = 3;
        div = 8.0;
    } else if (khz < 1500000.0) {
        diva = 2;
        div = 4.0;
    } else if (khz < 3000000.0) {
        diva = 1;
        div = 2.0;
    } else if (khz < 6000000.0) {
        diva = 0;
        div = 1.0;
    } else {
        return -1.0;
    }

    double n = khz * div / fPFD_khz;
    uint32_t N = (uint32_t) n;
    uint32_t F = round((n - N) * MOD);

	max2871Set_INT(instance, 0);
	max2871Set_N(instance, N);
	max2871Set_FRAC(instance, F);
	max2871Set_CPL(instance, 1);
	max2871Set_M(instance, MOD);
	max2871Set_LDF(instance, 0);
	max2871Set_DIVA(instance, diva);
	max2871Set_F01(instance, 1);

	max2871WriteRegisters(instance);

	while (!instance->check_ld()) {
		instance->idle_wait();
	}

    return fPFD_khz * (N + ((double)F / (double)MOD)) / div;
}



int max2871_rfa_power (max2871_instance_t* instance, int dbm) {
	uint8_t rfPower;
	switch (dbm) {
		case -4: rfPower = 0; break;
		case -1: rfPower = 1; break;
		case 2:  rfPower = 2; break;
		case 5:  rfPower = 3; break;
		default: return 0;
	}

	max2871Set_APWR(instance, rfPower);
	max2871WriteRegisters(instance);
	return 1;
}

int max2871_ld (max2871_instance_t* instance) {
	return instance->check_ld();
}

void max2871_rfa_out (max2871_instance_t* instance, int onoff) {
	max2871Set_RFA_EN(instance, (onoff & 0x01));
	max2871WriteRegisters(instance);
}



