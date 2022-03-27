//
// Copyright (c) 2013-2021 The SRS Authors
//
// SPDX-License-Identifier: MIT or MulanPSL-2.0
//

#include <srs_kernel_cirqueue.hpp>

#include <srs_kernel_error.hpp>

using namespace std;

SrsCircleQueue::SrsCircleQueue() : info(NULL), data(NULL)
{
    
}

SrsCircleQueue::~SrsCircleQueue()
{
    if (data) {
        free(data - sizeof(SrsCirmemHeader));
    }
}

srs_error_t SrsCircleQueue::init(int shm_key, uint64_t mem_size)
{
    srs_error_t err = srs_success;
    // max 1G
    if (data) {
        return err;
    }
    if (mem_size > SRS_CONST_MAX_BUFFER_SIZE) {
        return srs_error_new(ERROR_QUEUE_INIT, "size is too large");
    }

    uint64_t shm_size = mem_size + sizeof(SrsCirmemHeader);
    data = malloc(shm_size);
    memset(data, 0, shm_size);

    info = (SrsCirmemHeader *)data; 
    data += sizeof(SrsCirmemHeader);
    info->max_nr = mem_size;

    return err;
}

srs_error_t SrsCircleQueue::push(void *buffer, uint64_t size)
{
    srs_error_t err = srs_success;
    if ((!info) || (!data)) {// not initialize
        return srs_error_new(ERROR_QUEUE_PUSH, "queue init failed");
    }
    
    uint64_t current_pos, next_pos;
    
    do {
        current_pos = info->write_pos;
        
        next_pos = (current_pos + size + 8) % info->max_nr;  // head is length

        if (get_writable_size() < size + 8) {
            return srs_error_new(ERROR_QUEUE_PUSH, "queue is full");
        }
    } while (!__sync_bool_compare_and_swap(&info->write_pos, current_pos, next_pos));

    if (next_pos > current_pos) {
        *(uint64_t *)(data + current_pos) = size;
        memcpy(data + current_pos + 8, buffer, size);
    } else {
        if(current_pos + 8 < info->max_nr) {
            *(uint64_t *)(data + current_pos) = size;
            memcpy(data + current_pos + 8, buffer, info->max_nr - current_pos - 8);  // copy the first part of message
            memcpy(data, buffer + info->max_nr - current_pos - 8, next_pos);  // copy the remain
        } else {
            char temp_buffer[8] = {0};
            *(uint64_t *)temp_buffer = size;
            
            memcpy(data + current_pos, temp_buffer, info->max_nr - current_pos); // copy the first part of message header
            memcpy(data, temp_buffer + info->max_nr - current_pos, 8 - (info->max_nr - current_pos));
            memcpy(data + 8 - (info->max_nr - current_pos), buffer, size);  // copy data
        }
        
    }

    while (!__sync_bool_compare_and_swap(&info->write_pos_r, current_pos, next_pos)) {
        sched_yield();
    }
    __sync_add_and_fetch(&info->curr_nr, 1);    // message num add 1
    
    return err;
}

srs_error_t SrsCircleQueue::pop(void * buffer, uint64_t &size) {   
    srs_error_t err = srs_success;
    if ((!info) || (!data)) {// not initialize
        return srs_error_new(ERROR_QUEUE_POP, "queue init failed");
    }
    
    uint64_t current_pos, next_pos;
    do {
        current_pos = info->read_pos;
        if (current_pos == info->write_pos_r) {
            return srs_error_new(ERROR_QUEUE_POP, "queue is empty");
        }
        if (current_pos + 8 < info->max_nr) {
            size = *(uint64_t *)(data + current_pos);  // the size of current block
        } else {
            char temp_buffer[8] = {0};
            memcpy(temp_buffer, data + current_pos, info->max_nr - current_pos);
            memcpy(temp_buffer + info->max_nr - current_pos, data, 8 - (info->max_nr - current_pos));
            size = *(uint64_t *)temp_buffer;
        }
        
        next_pos = (current_pos + size + 8) % info->max_nr;
    } while (!__sync_bool_compare_and_swap(&info->read_pos, current_pos, next_pos));
    
    if (next_pos > current_pos) {
        memcpy(buffer, data + current_pos + 8, size);
    } else {
        if(current_pos + 8 < info->max_nr) {
            memcpy(buffer, data + current_pos + 8, info->max_nr - current_pos - 8);
            memcpy(buffer + info->max_nr - current_pos - 8, data, next_pos);
        } else {
           memcpy(buffer, data + 8 - (info->max_nr - current_pos), size);
        }
        
    }

    while (!__sync_bool_compare_and_swap(&info->read_pos_r, current_pos, next_pos)) {
        sched_yield();
    }
    __sync_sub_and_fetch(&info->curr_nr, 1);    // message num sub 1

    return err;
}

