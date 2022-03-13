//
// Copyright (c) 2022 The SRS Authors
//
// SPDX-License-Identifier: MIT or MulanPSL-2.0
//


#ifndef SRS_APP_DISPATCH_HPP
#define SRS_APP_DISPATCH_HPP

#include <srs_core.hpp>

#include <string>
#include <srs_kernel_cirmem.hpp>
#include <srs_app_st.hpp>

class SrsConfig;
using namespace std;
#define RECV_BUF_LEN (40 * 1024)
#define SEND_RECV_BUF_LEN (40 * 1024)

#pragma pack(1)

enum SrsWrkType
{
    WORK_LOG = 1
    // to do
};

// example
struct SrsStWork
{// to do
    SrsWrkType type;
    void* info;
    uint64_t push_time;
};

class SrsDispatch
{
    public:
        SrsDispatch();
        ~SrsDispatch();
        srs_error_t init();

        srs_error_t dispatch_work(SrsStWork &work);

        srs_error_t get_work(SrsStWork &work);

    private:
        srs_error_t init_shm(SrsCirMem *queue, int shm_key, int shm_size);
        SrsCirMem *log_queue;
        // to do, for other queues
};


#pragma pack()

#endif



