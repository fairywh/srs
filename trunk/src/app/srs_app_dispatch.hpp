//
// Copyright (c) 2022 The SRS Authors
//
// SPDX-License-Identifier: MIT or MulanPSL-2.0
//


#ifndef SRS_APP_DISPATCH_HPP
#define SRS_APP_DISPATCH_HPP

#include <srs_core.hpp>

#include <string>
#include <sys/epoll.h>
#include <pthread.h>
#include <vector>
#include <srs_kernel_cirmem.hpp>
#include <srs_app_st.hpp>
#include <map>

class SrsConfig;
using namespace std;
#define RECV_BUF_LEN (40 * 1024)
#define SEND_RECV_BUF_LEN (40 * 1024)

#pragma pack(1)

enum WorkType
{
    WORK_LOG = 1
    // to do
};

// example
struct SrsStWork
{// to do
    WorkType type;
    void* info;
    int fd;
    uint64_t push_time;
};

class SrsDispatch
{
    public:
        SrsDispatch(int processor_num);
        ~SrsDispatch();
        int Init();

        int DispatchWork(SrsStWork &work);

        int GetWork(SrsStWork &work);

    private:
        int InitShm(cir_mem *queue, int shm_key, int shm_size);
        cir_mem *_log_queue;
        std::vector<string> streams;
};


#pragma pack()

extern SrsDispatch * _srs_dispatch;
#endif



