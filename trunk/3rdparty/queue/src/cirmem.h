/*
 * wqueue
 * Copyright (c) 2022 fairywh
 */

#ifndef __EX_CIRQUEUE__H__
#define __EX_CIRQUEUE__H__



#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
#include "pthread.h"
#define MAX_BUFFER_SIZE     1 * 1024 * 1024 * 1024
#define USE_LOCAL_MEMORY    1
#pragma pack(1)
struct shm_cirmem_header
{
    uint64_t m_max_nr;
    
    volatile uint64_t m_curr_nr;
    
    volatile uint64_t m_ReadPos;
    volatile uint64_t m_ReadPos_r;
    
    volatile uint64_t m_WritePos;
    volatile uint64_t m_WritePos_r;
};
#pragma pack()

class cir_mem {
public:
    cir_mem() : m_info(NULL), m_pData(NULL) {
        
    }

    ~cir_mem() {
        if(m_pData) {
            free(m_pData - sizeof(shm_cirmem_header));
        }
    }
    
    int Init(int shm_key, uint64_t mem_size = 0) {
		// max 1G
        if(m_pData) {
            return 0;
        }
        if(mem_size > MAX_BUFFER_SIZE) {
            return -1;
        }

		void *pData = NULL;
        uint64_t shm_size = mem_size + sizeof(shm_cirmem_header);
        pData = malloc(shm_size);
        memset(pData, 0, shm_size);

        m_info = (shm_cirmem_header *)pData; 
        m_pData = (char *)pData + sizeof(shm_cirmem_header);
        m_info->m_max_nr = mem_size;

        return 0;
    }

    int Create(int shm_key, int memory_size) {
        return -21;
    }
    /*
        ret:
        -1  full
        0   ok
        
    */
    int push(char *buffer, uint64_t size) {
        if((!m_info) || (!m_pData)) {// not initialize
            return -2;
        }
        
        uint64_t Pos, NextPos;
        
        do {
            Pos = m_info->m_WritePos;
            
            NextPos = (Pos + size + 8) % m_info->m_max_nr;  // head is length
            
            //if (NextPos == m_info->m_ReadPos_r)
            if (GetWritableSize() < size + 8)
            {
                return -1;
            }
        } while(!__sync_bool_compare_and_swap(&m_info->m_WritePos, Pos, NextPos));

        if(NextPos > Pos) {
            *(uint64_t *)(m_pData + Pos) = size;
            memcpy(m_pData + Pos + 8, buffer, size);
        } else {
            if(Pos + 8 < m_info->m_max_nr) {
                *(uint64_t *)(m_pData + Pos) = size;
                memcpy(m_pData + Pos + 8, buffer, m_info->m_max_nr - Pos - 8);	// copy the first part of message
                memcpy(m_pData, buffer + m_info->m_max_nr - Pos - 8, NextPos);	// copy the remain
            } else {
                char temp_buffer[8] = {0};
                *(uint64_t *)temp_buffer = size;
                
                memcpy(m_pData + Pos, temp_buffer, m_info->m_max_nr - Pos); // copy the first part of message header
                memcpy(m_pData, temp_buffer + m_info->m_max_nr - Pos, 8 - (m_info->m_max_nr - Pos));
                memcpy(m_pData + 8 - (m_info->m_max_nr - Pos), buffer, size);  // copy data
            }
            
        }

        while(!__sync_bool_compare_and_swap(&m_info->m_WritePos_r, Pos, NextPos)) {
            sched_yield();
        }
        __sync_add_and_fetch(&m_info->m_curr_nr, 1);    // message num
        
        return 0;
    }

    /*
        ret:
        -1 no data
        0   ok
    */
    int pop(char * buffer, uint64_t &size) {   
        if((!m_info) || (!m_pData)) {// not initialize
            return -2;
        }
        
        uint64_t Pos, NextPos;
        do {
            Pos = m_info->m_ReadPos;
            if (Pos == m_info->m_WritePos_r) {
                return -1;
            }
            if(Pos + 8 < m_info->m_max_nr) {
                size = *(uint64_t *)(m_pData + Pos);  // the size of current block
            } else {
                char temp_buffer[8] = {0};
                memcpy(temp_buffer, m_pData + Pos, m_info->m_max_nr - Pos);
                memcpy(temp_buffer + m_info->m_max_nr - Pos, m_pData, 8 - (m_info->m_max_nr - Pos));
                size = *(uint64_t *)temp_buffer;
            }
            
            NextPos = (Pos + size + 8) % m_info->m_max_nr;
        } while(!__sync_bool_compare_and_swap(&m_info->m_ReadPos, Pos, NextPos));
        
        if(NextPos > Pos) {
            memcpy(buffer, m_pData + Pos + 8, size);
        } else {
            if(Pos + 8 < m_info->m_max_nr) {
                memcpy(buffer, m_pData + Pos + 8, m_info->m_max_nr - Pos - 8);
                memcpy(buffer + m_info->m_max_nr - Pos - 8, m_pData, NextPos);
            } else {
               memcpy(buffer, m_pData + 8 - (m_info->m_max_nr - Pos), size);
            }
            
        }
        while(!__sync_bool_compare_and_swap(&m_info->m_ReadPos_r, Pos, NextPos)) {
            sched_yield();
        }
        __sync_sub_and_fetch(&m_info->m_curr_nr, 1);

        return 0;
    }

	unsigned get_curr_nr() const {
        if(!m_info || !m_pData) {// not initialize
            return 0;
        }
        return m_info->m_curr_nr;
    }

	
    void PrintMem(uint64_t pos, unsigned int size) {
        if(size > 128 * 1024) {
            size = 128 * 1024;
        }

        for(unsigned int tmp = 0; tmp < size; tmp ++) {
            if(*((char *)m_pData + tmp) < 0)
            {
                char b = (*((char *)m_pData + tmp) >> 4)&0x0f;
                char c = *((char *)m_pData + tmp)&0x0f;

                printf("%x%x ", b,c);
            } else {
                printf("%02x ", *((char *)m_pData + tmp));
            }
        }
    }
private:
    cir_mem(const cir_mem&);
    const cir_mem& operator=(const cir_mem&);

	shm_cirmem_header *m_info;

    char* m_pData;

    uint64_t GetWritableSize() {
        return (m_info->m_ReadPos_r + m_info->m_max_nr - m_info->m_WritePos - 1) % m_info->m_max_nr;
    }

	uint64_t GetReadableSize() {
        return (m_info->m_WritePos_r + m_info->m_max_nr - m_info->m_ReadPos) % m_info->m_max_nr;
    }
};


#endif