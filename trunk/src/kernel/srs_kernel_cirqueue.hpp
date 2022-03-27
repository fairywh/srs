//
// Copyright (c) 2022 The SRS Authors
//
// SPDX-License-Identifier: MIT or MulanPSL-2.0
//

#ifndef SRS_KERNEL_CIRQUEUE_HPP
#define SRS_KERNEL_CIRQUEUE_HPP

#include <srs_core.hpp>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
#define SRS_CONST_MAX_BUFFER_SIZE     1 * 1024 * 1024 * 1024

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

class SrsCircleQueue {
public:
    SrsCircleQueue();

    ~SrsCircleQueue();
    
    srs_error_t init(int shm_key, uint64_t mem_size = 0);

    /*
        push the message to the queue
    */
    srs_error_t push(void *buffer, uint64_t size);

    /*
        get the message from the queue
        Error if empty, please use get_curr_size() to check before calling pop.
    */
    srs_error_t pop(void * buffer, uint64_t &size);

    unsigned get_curr_size() const {
        if (!info || !data) {
            return 0;
        }
        return info->curr_nr;
    }

private:
    SrsCircleQueue(const SrsCircleQueue&);
    const SrsCircleQueue& operator=(const SrsCircleQueue&);

    SrsCirmemHeader *info;

    void* data;

    uint64_t get_writable_size() {
        return (info->read_pos_r + info->max_nr - info->write_pos - 1) % info->max_nr;
    }

    uint64_t get_readable_size() {
        return (info->write_pos_r + info->max_nr - info->read_pos) % info->max_nr;
    }
};
#endif

