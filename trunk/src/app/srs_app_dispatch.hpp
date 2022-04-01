//
// Copyright (c) 2022 The SRS Authors
//
// SPDX-License-Identifier: MIT or MulanPSL-2.0
//

#ifndef SRS_APP_DISPATCH_HPP
#define SRS_APP_DISPATCH_HPP

#include <srs_core.hpp>

#include <string>

class SrsCircleQueue;

enum SrsWorkType
{
    SrsWorkTypeLog = 1
};

struct SrsStWork
{
    SrsWorkType type;
    void* info;
    srs_utime_t push_time;
};

class SrsDispatch
{
public:
    SrsDispatch();
    ~SrsDispatch();
public:
    srs_error_t init();
    srs_error_t dispatch_log(SrsStWork& work);
    srs_error_t fetch_log(SrsStWork& work);
    bool empty();
private:
    srs_error_t do_init(SrsCircleQueue* queue, int shm_key, int shm_size);
    SrsCircleQueue* log_queue_;
};

#endif

