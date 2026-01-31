#ifndef _MODEM_H_
#define _MODEM_H_

#include <stdint.h>

#define MODEM_PKT_PAYLOAD_MAX (64)

typedef struct __attribute__((packed)) {
    uint8_t antipreamble; // Early packet verification
    uint8_t len;
    uint16_t crc;
} ofdm_pkt_header_t;


typedef union __attribute__((packed)) ofdm_packet_u {
    uint8_t byte[0];
    struct __attribute__((packed)) {
        ofdm_pkt_header_t h;
        char payload[MODEM_PKT_PAYLOAD_MAX];
    };
} ofdm_pkt_t;


int ofdm_packetize (ofdm_pkt_t* p, void *data, int len);
int ofdm_depacketize (ofdm_pkt_t* p, void **data);

int ofdm_txpkt (ofdm_pkt_t *p);
int ofdm_rxpkt (ofdm_pkt_t *p);




typedef struct __attribute__((packed)) {
    uint8_t preamble1;
    uint8_t preamble2;
    uint8_t len;
    uint16_t crc;
} bpsk_pkt_header_t;


typedef union __attribute__((packed)) bpsk_pkt_u {
    uint8_t byte[0];
    struct __attribute__((packed)) {
        bpsk_pkt_header_t h;
        char payload[MODEM_PKT_PAYLOAD_MAX];
    };
} bpsk_pkt_t;


int bpsk_packetize (bpsk_pkt_t* p, void *data, int len);
int bpsk_depacketize (bpsk_pkt_t* p, void **data);

int bpsk_txpkt (bpsk_pkt_t *p);
int bpsk_rxpkt (bpsk_pkt_t *p);




#endif
