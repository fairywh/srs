//
// Copyright (c) 2013-2021 The SRS Authors
//
// SPDX-License-Identifier: MIT or MulanPSL-2.0
//


#include <srs_kernel_cirmem.hpp>

using namespace std;

SrsCirMem::SrsCirMem() : info(NULL), data(NULL)
{
    
}

SrsCirMem::~SrsCirMem()
{
    if(data) {
        free(data - sizeof(SrsCirmemHeader));
    }
}

int SrsCirMem::init(int shm_key, uint64_t mem_size)
{
    // max 1G
    if(data) {
        return 0;
    }
    if(mem_size > MAX_BUFFER_SIZE) {
        return -1;
    }

    void *data = NULL;
    uint64_t shm_size = mem_size + sizeof(SrsCirmemHeader);
    data = malloc(shm_size);   // or use mmap instead
    memset(data, 0, shm_size);

    info = (SrsCirmemHeader *)data; 
    data = (char *)data + sizeof(SrsCirmemHeader);
    info->max_nr = mem_size;

    return 0;
}

/*
    ret:
    -1  full
    0   ok
    
*/
int SrsCirMem::push(char *buffer, uint64_t size)
{
    if((!info) || (!data)) {// not initialize
        return -2;
    }
    
    uint64_t current_pos, next_pos;
    
    do {
        current_pos = info->write_pos;
        
        next_pos = (current_pos + size + 8) % info->max_nr;  // head is length
        
        //if (next_pos == info->read_pos_r)
        if (get_writable_size() < size + 8)
        {
            return -1;
        }
    } while(!__sync_bool_compare_and_swap(&info->write_pos, current_pos, next_pos));

    if(next_pos > current_pos) {
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

    while(!__sync_bool_compare_and_swap(&info->write_pos_r, current_pos, next_pos)) {
        sched_yield();
    }
    __sync_add_and_fetch(&info->curr_nr, 1);    // message num
    
    return 0;
}

/*
    ret:
    -1 no data
    0   ok
*/
int SrsCirMem::pop(char * buffer, uint64_t &size) {   
    if((!info) || (!data)) {// not initialize
        return -2;
    }
    
    uint64_t current_pos, next_pos;
    do {
        current_pos = info->read_pos;
        if (current_pos == info->write_pos_r) {
            return -1;
        }
        if(current_pos + 8 < info->max_nr) {
            size = *(uint64_t *)(data + current_pos);  // the size of current block
        } else {
            char temp_buffer[8] = {0};
            memcpy(temp_buffer, data + current_pos, info->max_nr - current_pos);
            memcpy(temp_buffer + info->max_nr - current_pos, data, 8 - (info->max_nr - current_pos));
            size = *(uint64_t *)temp_buffer;
        }
        
        next_pos = (current_pos + size + 8) % info->max_nr;
    } while(!__sync_bool_compare_and_swap(&info->read_pos, current_pos, next_pos));
    
    if(next_pos > current_pos) {
        memcpy(buffer, data + current_pos + 8, size);
    } else {
        if(current_pos + 8 < info->max_nr) {
            memcpy(buffer, data + current_pos + 8, info->max_nr - current_pos - 8);
            memcpy(buffer + info->max_nr - current_pos - 8, data, next_pos);
        } else {
           memcpy(buffer, data + 8 - (info->max_nr - current_pos), size);
        }
        
    }
    while(!__sync_bool_compare_and_swap(&info->read_pos_r, current_pos, next_pos)) {
        sched_yield();
    }
    __sync_sub_and_fetch(&info->curr_nr, 1);

    return 0;
}

