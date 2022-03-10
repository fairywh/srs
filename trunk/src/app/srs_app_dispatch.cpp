//
// Copyright (c) 2022 The SRS Authors
//
// SPDX-License-Identifier: MIT or MulanPSL-2.0
//

#include <srs_app_dispatch.hpp>

#include <srs_kernel_log.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_app_config.hpp>
#include <algorithm>
#include <map>

#define PARSER_SHARE_KEY 0x123450

#define PARSER_QUEUE_MEM_SIZE (9000 * sizeof(SrsStWork))


SrsDispatch::SrsDispatch(int processor_num)
{
    _log_queue = new cir_mem();
}

SrsDispatch::~SrsDispatch()
{
    srs_freep(_log_queue);
}

int SrsDispatch::Init()
{
    int ret = 0;
    int key;

    if((ret = InitShm(_log_queue, PARSER_SHARE_KEY, PARSER_QUEUE_MEM_SIZE)) != 0) {
        srs_error("init parser shm failed ret=%d, key=0x%x, size=%d.", ret, PARSER_SHARE_KEY, PARSER_QUEUE_MEM_SIZE);
        return ret;
    }
	
    return ret;
}

int SrsDispatch::InitShm(cir_mem *queue, int shm_key, int shm_size)
{
    if(queue == NULL) {
        return -1;
    }
    int ret = queue->Init(shm_key, shm_size);
    if(ret != 0) {
        srs_error("init shm error,need create,ret=%d", ret);
        ret = queue->Create(shm_key, shm_size);
    }
    
    if(ret != 0) {
        srs_error("create shm error, ret=%d", ret);
        return -2;
    }

    ret = queue->Init(shm_key, shm_size);
    if(ret != 0) {
        srs_error("init shm error,ret=%d", ret);
        return -3;
    }
    return ret;
}

int SrsDispatch::DispatchWork(SrsStWork &work)
{
    work.push_time = srs_update_system_time();
    // to do
    int ret = _log_queue->push((char *)&work, sizeof(SrsStWork));
    if(ret != 0) {
        srs_error("queue is full,ret=%d", ret);
        return ret;
    }
    unsigned int work_num = _log_queue->get_curr_nr();
    srs_info("push parse work type=%d, fd=%d success. %d in total",
        work.type, work.fd, work_num);
    return ret;
}

int SrsDispatch::GetWork(SrsStWork &work) 
{
    if (_log_queue->get_curr_nr() == 0) {
        return -1;
    }
    
    char result[1024] = {0};
    uint64_t size = 0;
    int ret = _log_queue->pop(result, size);
    
    if(ret != 0) {
        if (ret == -1) {// no data
            srs_info("queue pop empty");
        } else {
            srs_error("queue pop error, ret=%d", ret);
        }
        return -2;
    }
    SrsStWork *new_work = (SrsStWork*)result;
    work.type = new_work->type;
    work.info = new_work->info;
    work.fd = new_work->fd;
    work.push_time = new_work->push_time;

    srs_trace("get parser work type=%d, fd=%d",
        work.type, work.fd);
    return ret;
}

