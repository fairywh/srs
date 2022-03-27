//
// Copyright (c) 2022 The SRS Authors
//
// SPDX-License-Identifier: MIT or MulanPSL-2.0
//

#ifndef SRS_APP_DISPATCH_HPP
#define SRS_APP_DISPATCH_HPP

#include <srs_core.hpp>

#include <string>
#include <srs_kernel_cirqueue.hpp>
#include <srs_app_st.hpp>

class SrsConfig;
using namespace std;
#define RECV_BUF_LEN (40 * 1024)
#define SEND_RECV_BUF_LEN (40 * 1024)

enum SrsWorkType
{
    SrsWorkTypeLog = 1
    // to do
};

// example
struct SrsStWork
{// to do
    SrsWorkType type;
    void* info;
    srs_utime_t push_time;
};

class SrsDispatch
{
public:
    SrsDispatch();
    ~SrsDispatch();

    srs_error_t init();

    srs_error_t dispatch_log(SrsStWork &work);

    srs_error_t fetch_log(SrsStWork &work);

    bool empty();

private:
    srs_error_t init_shm(SrsCircleQueue *queue, int shm_key, int shm_size);
    SrsCircleQueue *log_queue_;
    // to do, for other queues
};

#endif


