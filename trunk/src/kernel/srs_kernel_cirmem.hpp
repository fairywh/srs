//
// Copyright (c) 2022 The SRS Authors
//
// SPDX-License-Identifier: MIT or MulanPSL-2.0
//

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
struct SrsCirmemHeader
{
    uint64_t max_nr;
    
    volatile uint64_t curr_nr;
    
    volatile uint64_t read_pos;
    volatile uint64_t read_pos_r;
    
    volatile uint64_t write_pos;
    volatile uint64_t write_pos_r;
};
#pragma pack()

class SrsCirMem {
public:
    SrsCirMem();

    ~SrsCirMem();
    
    int init(int shm_key, uint64_t mem_size = 0);

    /*
        ret:
        -1  full
        0   ok
        
    */
    int push(char *buffer, uint64_t size);
    /*
        ret:
        -1 no data
        0   ok
    */
    int pop(char * buffer, uint64_t &size);

    unsigned get_curr_nr() const {
        if(!info || !data) {// not initialize
            return 0;
        }
        return info->curr_nr;
    }

private:
    SrsCirMem(const SrsCirMem&);
    const SrsCirMem& operator=(const SrsCirMem&);

    SrsCirmemHeader *info;

    char* data;

    uint64_t get_writable_size() {
        return (info->read_pos_r + info->max_nr - info->write_pos - 1) % info->max_nr;
    }

    uint64_t get_readable_size() {
        return (info->write_pos_r + info->max_nr - info->read_pos) % info->max_nr;
    }
};


#endif