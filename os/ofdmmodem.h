#ifndef _OFDMMODEM_H_
#define _OFDMMODEM_H_

#include <stdint.h>

#define OFDM_PKT_PAYLOAD_MAX (64)

typedef struct __attribute__((packed)) {
    uint8_t antipreamble; // To create a sharp boundary
    uint8_t len;
    uint16_t crc;
} ofdm_pkt_header_t;


typedef union __attribute__((packed)) ofdm_packet_u {
    uint8_t byte[0];
    struct __attribute__((packed)) {
        ofdm_pkt_header_t h;
        char payload[OFDM_PKT_PAYLOAD_MAX];
    };
} ofdm_pkt_t;


int ofdm_packetize (ofdm_pkt_t* p, void *data, int len);
int ofdm_depacketize (ofdm_pkt_t* p, void **data);

int ofdm_txpkt (int fs, ofdm_pkt_t *p);
int ofdm_rxpkt (int fs, ofdm_pkt_t *p, int* ampl);



#endif
