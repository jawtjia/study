/**
 *@file     protocol.h
 *@date     2017-05-02
 *@author   
 *@brief    
 */   

#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

//CRC-16/CCITT
#define CRC16_POLY 0x1021
#define CRC16_INITIAL 0xFFFF

#define MAX_UART_BUF_SIZE 256
#define START_FLAG 0x7E
#define END_FLAG 0x7F
#define ESCAPE_FLAG 0x7D
#define ESCAPE_MASK 0x20

//START(1BYTES) + COMMAND(2BYTES) + PAYLOAD_LENGTH(2BYTES) + PAYLOAD + CRC(2BYTES) + END(1BYTES)
#define START_FLAG_LEN 1
#define START_POSITION 0
#define COMMAND_LEN 2
#define COMMAND_POSITION 1
#define LENGTH_LEN 2
#define LENGTH_POSITION 3
#define PAYLOAD_POSITION 5
#define HEAD_LEN 5

#define CRC_LEN 2
#define END_FLAG_LEN 1
#define TAIL_LEN 3

typedef enum
{
    PRTL_OK = 0,
	PRTL_NEED_MORE = 1,
	PRTL_PARAM_NOT_INIT = -1,
	PRTL_RECV_LEN_ERR = -2,
	PRTL_PRTL_LEN_ERR = -3,
	PRTL_PRTL_CHECK_ERR = -4,
	PRTL_PRTL_MALLOC_ERR = -5,
    PRTL_HEAD_ERR = -6,
    PRTL_PARAM_ERR = -7,
    PRTL_CRC_ERR = -8,
    PRTL_TAIL_ERR = -9,
    PRTL_SEND_LEN_ERR = -10,
    PRTL_SEND_ERR = -11,
}prtl_ret;

typedef prtl_ret (*send_data)(uint8_t *,int,void *);
typedef prtl_ret (*protocol_check_head)(void *,void *);
typedef prtl_ret (*protocol_check_crc)(void *,int,void *);
typedef prtl_ret (*protocol_check_tail)(void *,int,void *);
typedef prtl_ret (*protocol_handle)(uint8_t *,int,void *);

typedef struct
{
	int datasize;
	uint8_t *data;
	int recv_len;
	uint16_t head_len;
	uint16_t tail_len;
	uint16_t single_len;
	uint8_t payload_len_position;
	uint8_t payload_len_size;
	protocol_check_head prtl_check_head; //check head
	protocol_check_crc prtl_check_crc; //check crc
    protocol_check_tail prtl_check_tail; //check tail
	protocol_handle prtl_handle;
	bool inited;
	void *user;
    bool recheck;
}prtl_recv;

prtl_recv *prtl_recv_param_init(int datasize,uint16_t headlen,uint16_t taillen,uint16_t singlelen,
									uint8_t payload_len_position,uint8_t payload_len_size,
									protocol_check_head prtl_check_head,protocol_check_crc prtl_check_crc,protocol_check_tail prtl_check_tail,
                                    protocol_handle prtl_handle,void *user);

void prtl_recv_param_deinit(prtl_recv *pdata);

void prtl_recv_data_reset(prtl_recv *pdata);

prtl_ret prtl_recv_data(uint8_t *data, int len, prtl_recv *recv);

prtl_ret prtl_recv_data_ex(uint8_t *data, int len, prtl_recv *recv);

prtl_ret prtl_send_packet(uint16_t command, uint8_t *data, uint16_t length, send_data send_func, void *user);

#endif // __PROTOCOL_H