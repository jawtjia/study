#include <string.h> // To use strlen function
#include <stdarg.h> // Required for va_list, va_start and va_end
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "protocol.h"

static uint16_t crc16_table[256];

static void generate_crc16_table() {
    for (int i = 0; i < 256; i++) {
        uint16_t crc = i << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ CRC16_POLY;
            } else {
                crc <<= 1;
            }
        }
        crc16_table[i] = crc;
    }
}

static uint16_t crc16_update(uint16_t crc, uint8_t *data, uint16_t len) {
    for (uint16_t i=0; i<len; i++) {
        crc = (crc << 8) ^ crc16_table[(crc >> 8) ^ data[i]];
    }
    return crc;
}

static uint16_t crc16(uint8_t *data, uint16_t len) {
    return crc16_update(CRC16_INITIAL, data, len);
}

prtl_ret prtl_send_packet(uint16_t command, uint8_t *data, uint16_t length, send_data send_func, void *user) {
    if(length > MAX_UART_BUF_SIZE - HEAD_LEN - TAIL_LEN) {
        printf("Data length too long!\n");
        return PRTL_SEND_LEN_ERR;
    }

    printf("send data:");
    for (int i = 0; i < length; ++i) {
        printf("0x%02X ", data[i]);
    }
    printf("\n");
    
    uint8_t output_buffer[MAX_UART_BUF_SIZE] = {0};
    uint8_t head_tmp[COMMAND_LEN+LENGTH_POSITION] = {0};

    output_buffer[START_POSITION] = START_FLAG;  // Start of packet
    memcpy(head_tmp,&command,COMMAND_LEN);  // Command
    memcpy(head_tmp+COMMAND_LEN,&length,LENGTH_LEN);  // Length

    //printf("head_tmp:\n");
    //for (int i = 0; i < COMMAND_LEN + LENGTH_LEN; ++i) {
        // Send the packet over UART
        //printf("0x%02X ", head_tmp[i]);
    //}
    //printf("\n");

    //update output_buffer
    int buffer_index = START_FLAG_LEN;  // Start writing data after length field
    for (int i = START_FLAG_LEN; i < (START_FLAG_LEN + COMMAND_LEN + LENGTH_LEN); ++i) {
        //check buffer_index. because the head length is very smaller than the max length, so we do not need to check it.
        //if(buffer_index > (MAX_UART_BUF_SIZE-2)) {
        //    printf("Buf data too long head!\n");
        //   return PRTL_SEND_LEN_ERR;
        //}

        if (head_tmp[i-START_FLAG_LEN] == START_FLAG || head_tmp[i-START_FLAG_LEN] == END_FLAG || head_tmp[i-START_FLAG_LEN] == ESCAPE_FLAG) {
            // Escape the special characters
            output_buffer[buffer_index++] = ESCAPE_FLAG;
            output_buffer[buffer_index++] = head_tmp[i-START_FLAG_LEN] ^ ESCAPE_MASK;
        } else {
            output_buffer[buffer_index++] = head_tmp[i-START_FLAG_LEN];
        }
    }

    //printf("output_buffer:\n");
    //for (int i = 0; i < buffer_index; ++i) {
        // Send the packet over UART
        //printf("0x%02X ", output_buffer[i]);
    //}
    //printf("\n");

    // Start updating CRC starting with the command and length fields
    uint16_t crc = crc16_update(CRC16_INITIAL, head_tmp, COMMAND_LEN+LENGTH_LEN);
    
    //update output_buffer and crc
    for (int i = 0; i < length; ++i) {
        crc = crc16_update(crc, &data[i], 1);  // Update CRC for each byte

        //check buffer_index
        if(buffer_index > (MAX_UART_BUF_SIZE-2)) {
            printf("Buf data too long data!\n");
            return PRTL_SEND_LEN_ERR;
        }

        if (data[i] == START_FLAG || data[i] == END_FLAG || data[i] == ESCAPE_FLAG) {
            // Escape the special characters
            output_buffer[buffer_index++] = ESCAPE_FLAG;
            output_buffer[buffer_index++] = data[i] ^ ESCAPE_MASK;
        } else {
            output_buffer[buffer_index++] = data[i];
        }
    }

    //printf("output_buffer:\n");
    //for (int i = 0; i < buffer_index; ++i) {
        // Send the packet over UART
        //printf("0x%02X ", output_buffer[i]);
    //}
    //printf("\n");
    //printf("send scr:0x%02x\n",crc);

    //update output_buffer
    uint8_t crc_tmp[CRC_LEN] = {0};
    memcpy(crc_tmp,&crc,CRC_LEN);  // CRC
    for (int i = 0; i < CRC_LEN; ++i) {
        //check buffer_index
        if(buffer_index > (MAX_UART_BUF_SIZE-2)) {
            printf("Buf data too long crc!\n");
            return PRTL_SEND_LEN_ERR;
        }

        if (crc_tmp[i] == START_FLAG || crc_tmp[i] == END_FLAG || crc_tmp[i] == ESCAPE_FLAG) {
            // Escape the special characters
            output_buffer[buffer_index++] = ESCAPE_FLAG;
            output_buffer[buffer_index++] = crc_tmp[i] ^ ESCAPE_MASK;
        } else {
            output_buffer[buffer_index++] = crc_tmp[i];
        }
    }

    //check buffer_index
    if(buffer_index > (MAX_UART_BUF_SIZE - 1)) {
        printf("Buf data too long crc!\n");
        return PRTL_SEND_LEN_ERR;
    }
    output_buffer[buffer_index++] = END_FLAG;  // End of packet

    printf("send buffer:");
    for (int i = 0; i < buffer_index; ++i) {
        // Send the packet over UART
        printf("0x%02X ", output_buffer[i]);
    }
    printf("\n");

    return send_func(output_buffer, buffer_index, user);
}

prtl_recv *prtl_recv_param_init(int datasize,uint16_t headlen,uint16_t taillen,uint16_t singlelen,
									uint8_t payload_len_position,uint8_t payload_len_size,
									protocol_check_head prtl_check_head,protocol_check_crc prtl_check_crc,protocol_check_tail prtl_check_tail,
                                    protocol_handle prtl_handle,void *user)
{
	prtl_recv *p = NULL;
		
	if(datasize <= 0)
		return NULL;

	p = (prtl_recv *)malloc(sizeof(prtl_recv));
	if(NULL == p)
	{
		printf("prtl recv malloc fail\n");
		return NULL;
	}
	memset(p,0,sizeof(prtl_recv));

	p->datasize = datasize;
	p->data = (uint8_t *)malloc(datasize);
	if(NULL == p->data)
	{
		free(p);
		printf("prtl recv malloc data fail\n");
		return NULL;
	}
	memset(p->data,0,datasize);

	p->head_len = headlen;
	p->tail_len = taillen;
	p->single_len = singlelen;
	p->payload_len_position = payload_len_position,
	p->payload_len_size = payload_len_size,
	p->prtl_check_head = prtl_check_head;
	p->prtl_check_crc = prtl_check_crc;
    p->prtl_check_tail = prtl_check_tail;
	p->prtl_handle = prtl_handle;
	p->user = user;
    p->recheck = false;

	p->inited = true;

	return p;
}

void prtl_recv_param_deinit(prtl_recv *pdata)
{
	if(pdata && (pdata->inited))
	{
		if(pdata->data)
			free(pdata->data);
		if(pdata)
			free(pdata);
	}
	else
	{
		printf("pdata recv not init\n");
	}
}

void prtl_recv_data_reset(prtl_recv *pdata)
{
	if((pdata->inited)&&(pdata->datasize > 0)&&(NULL != pdata->data))
	{
		memset(pdata->data,0,pdata->datasize);
		pdata->recv_len = 0;
        pdata->recheck = false;
	}
	else
	{
		printf("pdata recv reset err\n");
	}
}

static int prtl_recv_payload_len(prtl_recv *recv)
{
	uint8_t *len_data = NULL;
	int len = 0;

	if((recv->payload_len_size <= 0)||(recv->payload_len_size > sizeof(int)))
	{
		printf("prtl payload len err %d\n",recv->payload_len_size);
		return PRTL_PRTL_MALLOC_ERR;
	}
	else
	{
		len_data = (uint8_t *)malloc(recv->payload_len_size+1);
		if(NULL == len_data)
		{
			printf("len data malloc fail\n");
			return PRTL_PRTL_MALLOC_ERR;
		}
		memset(len_data,0,recv->payload_len_size+1);
	}
	
	memcpy(len_data,recv->data+recv->payload_len_position,recv->payload_len_size);
    memcpy(&len,len_data,recv->payload_len_size);

	free(len_data);
	
	return len;
}

prtl_ret prtl_recv_data(uint8_t *data, int len, prtl_recv *recv)
{	
	int total_len = 0;

    //check init
	if(!recv->inited)
		return PRTL_PARAM_NOT_INIT;
	
    //check len
	if((len <= 0)||(len > (recv->datasize - recv->recv_len)))
	{
		printf("%s,len:%d,recv_len:%d\n",__func__,len,recv->recv_len);
		return PRTL_RECV_LEN_ERR;
	}

#if 0 //too much data, printf will case data drop!only debug!
	//debug
	{
		for(int i=0;i<len;i++)
		{
			printf("%02x ",data[i]);
		}
		printf("\n");
	}
#endif

    //copy data to recv->data
	memcpy(recv->data+recv->recv_len,data,len);
    recv->recv_len += len;

    //check escape, every time check all recv->data, because the data may be end with ESCAPE_FLAG.
    int i = recv->recv_len - len;
    if(recv->recheck == true)
    {
        i -= 1;
        if(i < 0)//if recv->recv_len - len - 1 < 0, it means recv->data is empty, so we don't need recheck.
            i = 0;
        recv->recheck = false;
    }

    for (; (i < recv->recv_len) && (i >= 0); ++i) {
        if(recv->data[i] == ESCAPE_FLAG)
        {
            if(i != (recv->recv_len - 1))
            {
                recv->data[i+1] ^= ESCAPE_MASK;
                memmove(recv->data+i,recv->data+i+1,recv->recv_len - i - 1);
                recv->recv_len--;
            }
            else
            {
                recv->recheck = true;//last byte is ESCAPE_FLAG, need recheck at next time.
                break;
            }
        }
    }

	RECV_DEAL_CONTINUE:
	if(recv->recv_len < recv->head_len)
	{
		return PRTL_NEED_MORE;
	}
	else
	{
		if(recv->prtl_check_head)
		{
			if(recv->prtl_check_head(recv,recv->user) != PRTL_OK) //check head err
			{
				printf("%s,head error\n",__func__);

                //error, clear one byte
				recv->recv_len--;
                if(recv->recv_len > 0)
                    memmove(recv->data,recv->data+1,recv->recv_len);
                //memset(recv->data+recv->recv_len,0x00,recv->datasize-recv->recv_len);//clear unuse bytes.

				goto RECV_DEAL_CONTINUE;
			}
		}
		
		int payload_len = prtl_recv_payload_len(recv);
		//printf("%s,payloadlen:%d\n",__func__,payload_len);
		
        //payload_len err
		if((payload_len < 0)||(payload_len > recv->single_len))
		{
			printf("%s,payload_len err %d\n",__func__,payload_len);

            //error, clear one byte
            recv->recv_len--;
            if(recv->recv_len > 0)
                memmove(recv->data,recv->data+1,recv->recv_len);
            //memset(recv->data+recv->recv_len,0x00,recv->datasize-recv->recv_len);//clear unuse bytes.

			goto RECV_DEAL_CONTINUE;
		}
			
		total_len = payload_len + recv->head_len + recv->tail_len;
        //printf("%s,total_len:%d\n",__func__,total_len);

		//total_len err
		if(total_len > recv->single_len)
		{
			printf("%s,total_len err %d\n",__func__,total_len);

            //error, clear one byte
			recv->recv_len--;
            if(recv->recv_len > 0)
                memmove(recv->data,recv->data+1,recv->recv_len);
            //memset(recv->data+recv->recv_len,0x00,recv->datasize-recv->recv_len);//clear unuse bytes.

			goto RECV_DEAL_CONTINUE;
		}
		
		if(recv->recv_len < total_len)
		{
			return PRTL_NEED_MORE;
		}
		else
		{
			int ret = PRTL_OK;

            if(recv->prtl_check_tail)
				ret = recv->prtl_check_tail(recv,total_len,recv->user);

            if(ret != PRTL_OK)//tail error
			{
                //error, clear one byte
				recv->recv_len--;
                if(recv->recv_len > 0)
                    memmove(recv->data,recv->data+1,recv->recv_len);
                //memset(recv->data+recv->recv_len,0x00,recv->datasize-recv->recv_len);//clear unuse bytes.

				goto RECV_DEAL_CONTINUE;
			}
			
            ret = PRTL_OK; //reset ret
			if(recv->prtl_check_crc)
				ret = recv->prtl_check_crc(recv,total_len,recv->user);
			
			if(ret == PRTL_OK)//CRC ok
			{
				if(recv->prtl_handle)
					recv->prtl_handle(recv->data,total_len,recv->user);
				recv->recv_len -= total_len;
				
				if(recv->recv_len > 0)
					memmove(recv->data,recv->data+total_len,recv->recv_len);
				//memset(recv->data+recv->recv_len,0x00,UART_DATA_RECV_MAX-recv->recv_len);//clear unuse bytes.

				goto RECV_DEAL_CONTINUE;
			}
			else //CRC err
			{
                //error, clear one byte
				recv->recv_len--;
                if(recv->recv_len > 0)
                    memmove(recv->data,recv->data+1,recv->recv_len);
                //memset(recv->data+recv->recv_len,0x00,recv->datasize-recv->recv_len);//clear unuse bytes.

				goto RECV_DEAL_CONTINUE;
			}
		}
	}
	
	return PRTL_NEED_MORE;//should not run here
}

static prtl_ret prtl_check_head(void *p,void *user)
{
    prtl_recv *recv = (prtl_recv *)p;
    
    if(recv->data[START_POSITION] != START_FLAG)
    {
        printf("prtl head err\n");
        return PRTL_HEAD_ERR;
    }
    
    return PRTL_OK;
}

static prtl_ret prtl_check_tail(void *p,int len,void *user)
{
    prtl_recv *recv = (prtl_recv *)p;
    
    if(recv->data[len-1] != END_FLAG)
    {
        printf("prtl tail err,%02x\n",recv->data[len-1]);
        return PRTL_TAIL_ERR;
    }
    
    return PRTL_OK;
}

static prtl_ret prtl_check_crc(void *p,int len,void *user)
{
    prtl_recv *recv = (prtl_recv *)p;
    uint16_t crc = 0;
    uint16_t crc_tmp = 0;
    
    memcpy(&crc,recv->data+len-END_FLAG_LEN-CRC_LEN,CRC_LEN);
    crc_tmp = crc16(recv->data+START_FLAG_LEN, len-START_FLAG_LEN-CRC_LEN-END_FLAG_LEN);

    //printf("crc check\n");
    //or(int i=0;i<len;i++)
    //{
    //    printf("%02x ",recv->data[i]);
    //}

    if(crc != crc_tmp)
    {
        printf("prtl crc err,%d,%d\n",crc_tmp,crc);
        return PRTL_CRC_ERR;
    }
    
    return PRTL_OK;
}

prtl_ret prtl_recv_data_ex(uint8_t *data, int len, prtl_recv *recv)
{
    //save data in recv->data and update recv->recv_len
    int total_len = 0;

    //check init
	if(!recv->inited)
		return PRTL_PARAM_NOT_INIT;
	
    //check len
	if((len <= 0)||(len > (recv->datasize - recv->recv_len)))
	{
		printf("%s,len:%d,recv_len:%d\n",__func__,len,recv->recv_len);
		return PRTL_RECV_LEN_ERR;
	}

    //copy data to recv->data
	memcpy(recv->data+recv->recv_len,data,len);
    recv->recv_len += len;
    
    //find start flag and end flag
    RECV_DEAL_CONTINUE:
    int start_flag_pos = -1;
    int end_flag_pos = -1;
    for(int i=0;i<recv->recv_len;i++)
    {
        if(recv->data[i] == START_FLAG)
        {
            start_flag_pos = i;
            //move data to start position
            if(start_flag_pos >= 0)
            {
                memmove(recv->data,recv->data+start_flag_pos,recv->recv_len-start_flag_pos);
                recv->recv_len -= start_flag_pos;
                i = 0;//if move data, we need to check data from start.
                start_flag_pos = 0;
                continue;//continue to check end flag
            }
        }
        else //delete data before start flag
        {
            if(start_flag_pos < 0)
            {
                recv->recv_len--;
                if(recv->recv_len > 0)
                    memmove(recv->data,recv->data+1,recv->recv_len);
                //memset(recv->data+recv->recv_len,0x00,recv->datasize-recv->recv_len);//clear unuse bytes.
                i--;
            }
        }

        //if found the start flag, then we find the end flag
        if(start_flag_pos >= 0)
        {
            if(recv->data[i] == END_FLAG)
            {
                end_flag_pos = i;
                break;
            }
        }
    }

    //if don't found start flag or end flag, return PRTL_NEED_MORE
    if((start_flag_pos < 0)||(end_flag_pos < 0))
    {
        if(recv->recv_len > recv->single_len)
        {
            printf("%s,recv_len err %d\n",__func__,recv->recv_len);
            //reset recv->data
            recv->recv_len = 0;
            return PRTL_RECV_LEN_ERR;
        }
        else
            return PRTL_NEED_MORE;
    }
    
    //if found both of start flag and end flag, then we check the data between start flag and end flag
    if(end_flag_pos > start_flag_pos)
    {
        //check escape and update recv->data
        for(int i=start_flag_pos;i<end_flag_pos;i++)
        {
            if(recv->data[i] == ESCAPE_FLAG)
            {
                if(i != (end_flag_pos - 1))
                {
                    recv->data[i+1] ^= ESCAPE_MASK;
                    memmove(recv->data+i,recv->data+i+1,end_flag_pos - i);
                    end_flag_pos--;
                }
                else//last-1 byte is ESCAPE_FLAG, it is CRC, so we need to move data to end_flag_pos+1
                {
                    //error, move data to end_flag_pos+1
                    recv->recv_len -= (end_flag_pos+1);
                    if(recv->recv_len > 0)
                        memmove(recv->data,recv->data+end_flag_pos+1,recv->recv_len);

                    goto RECV_DEAL_CONTINUE;
                }
            }
        }

        //check payload len
        int payload_len = prtl_recv_payload_len(recv);
        if((payload_len < 0)||(payload_len > recv->single_len))
        {
            printf("%s,payload_len err %d\n",__func__,payload_len);
            //error, move data to end_flag_pos+1
            recv->recv_len -= (end_flag_pos+1);
            if(recv->recv_len > 0)
                memmove(recv->data,recv->data+end_flag_pos+1,recv->recv_len);

            goto RECV_DEAL_CONTINUE;
        }
        
        //check total len
        total_len = payload_len + recv->head_len + recv->tail_len;
        if((total_len > recv->single_len)||(total_len != (end_flag_pos - start_flag_pos + 1)))
        {
            printf("%s,total_len err %d\n",__func__,total_len);
            //error, move data to end_flag_pos+1
            recv->recv_len -= (end_flag_pos+1);
            if(recv->recv_len > 0)
                memmove(recv->data,recv->data+end_flag_pos+1,recv->recv_len);

            goto RECV_DEAL_CONTINUE;
        }

        //check crc use prtl_check_crc
        if(recv->prtl_check_crc)
        {
            if(recv->prtl_check_crc(recv,total_len,recv->user) != PRTL_OK)
            {
                printf("%s,crc err\n",__func__);
                //error, move data to end_flag_pos+1
                recv->recv_len -= (end_flag_pos+1);
                if(recv->recv_len > 0)
                    memmove(recv->data,recv->data+end_flag_pos+1,recv->recv_len);

                goto RECV_DEAL_CONTINUE;
            }
        }

        //handle data
        if(recv->prtl_handle)
            recv->prtl_handle(recv->data,total_len,recv->user);

        //update recv->data
        recv->recv_len -= total_len;
        if(recv->recv_len > 0)
            memmove(recv->data,recv->data+total_len,recv->recv_len);
        //memset(recv->data+recv->recv_len,0x00,recv->datasize-recv->recv_len);//clear unuse bytes.

        goto RECV_DEAL_CONTINUE;
    }

    return PRTL_NEED_MORE;//should not run here
}

//test code start
static prtl_ret prtl_handle(uint8_t *p,int len,void *user)
{
    printf("prtl handle:");
    for (int i = 0; i < len; ++i) {
        // Send the packet over UART
        printf("0x%02X ", p[i]);
    }
    printf("\n");
}

static prtl_ret simulate_send_uart(uint8_t *data,int len,void *user)
{
    prtl_ret ret1 = PRTL_OK,ret2 = PRTL_OK;
    ret1 = prtl_recv_data(data,len,user);
    ret2 = prtl_recv_data_ex(data,len,user);
    if((ret1 != PRTL_NEED_MORE)||(ret2 != PRTL_NEED_MORE))
    {
        return PRTL_SEND_ERR;
    }
    return PRTL_OK;
}

void main(void)
{
    prtl_recv *uartdata_recv = NULL;

    generate_crc16_table();

    uartdata_recv = prtl_recv_param_init(MAX_UART_BUF_SIZE, HEAD_LEN, TAIL_LEN, MAX_UART_BUF_SIZE, LENGTH_POSITION, LENGTH_LEN, prtl_check_head, prtl_check_crc, prtl_check_tail, prtl_handle, NULL);
    
    uint8_t test_data[] = {0x7E,0x7E,0x7E,0xEC,0x4D,0x0A,0x00,0xB9,0x8F,0x6B,0xEB,0xA0,0x97,0x5B,0x40,0x6E,0x00,0xEA,0xEF,0x7F};
    prtl_recv_data(test_data,20,uartdata_recv);
    prtl_recv_data_ex(test_data,20,uartdata_recv);
    
    #if 1
    //get and printf system current timestamp
    int start_test_time = time(NULL);
    printf("start timestamp:%d\n",start_test_time);
    
    prtl_ret ret = PRTL_OK;
    int64_t count = 0;
    while((/*(ret == PRTL_SEND_LEN_ERR)||*/(ret == PRTL_OK)||(ret == PRTL_NEED_MORE))&&(count <= 1000000))
    {
        //generate random data with random length
        int len = rand()%20;
        uint8_t data[MAX_UART_BUF_SIZE] = {0};
        for(int i=0;i<len;i++)
        {
            data[i] = rand()%256;
        }
        //generate random command
        uint16_t command = rand()%65536;

        //teset prtl_send_packet
        ret = prtl_send_packet(command,data,len,simulate_send_uart,uartdata_recv);
        printf("prtl_send_packet ret:%d,count:%d\n",ret,count);
        count++;
    }

    int end_test_time = time(NULL);
    printf("end timestamp:%d,total test time:%d,count:%d,%f\n",end_test_time,end_test_time-start_test_time,count,(end_test_time-start_test_time)/count);
    #endif

    prtl_recv_param_deinit(uartdata_recv);
}
//test code end