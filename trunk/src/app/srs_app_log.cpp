//
// Copyright (c) 2013-2022 The SRS Authors
//
// SPDX-License-Identifier: MIT or MulanPSL-2.0
//

#include <srs_app_log.hpp>

#include <stdarg.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <srs_app_config.hpp>
#include <srs_kernel_error.hpp>
#include <srs_app_utility.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_app_threads.hpp>

// the max size of a line of log.
int LOG_MAX_SIZE = 8192;

// the tail append to each log.
#define LOG_TAIL '\n'
// reserved for the end of log data, it must be strlen(LOG_TAIL)
#define LOG_TAIL_SIZE 1

// Thread local log cache.
__thread char* _srs_log_data = NULL;

SrsFileLog::SrsFileLog()
{
    level = SrsLogLevelTrace;
    log_to_file_tank = false;
    utc = false;

    writer_ = NULL;
}

SrsFileLog::~SrsFileLog()
{
}

srs_error_t SrsFileLog::initialize()
{
    srs_error_t err = srs_success;

    if (_srs_config) {
        log_to_file_tank = _srs_config->get_log_tank_file();
        filename_ = _srs_config->get_log_file();
        level = srs_get_log_level(_srs_config->get_log_level());
        utc = _srs_config->get_utc_time();
    }

    if (!log_to_file_tank) {
        return err;
    }

    if (filename_.empty()) {
        return srs_error_new(ERROR_SYSTEM_LOGFILE, "no log filename");
    }

    // We only use the log writer, which is managed by another thread.
    if ((err = _srs_async_log->writer(filename_, &writer_)) != srs_success) {
        return srs_error_wrap(err, "create async writer for %s", filename_.c_str());
    }
    
    return err;
}

void SrsFileLog::verbose(const char* tag, SrsContextId context_id, const char* fmt, ...)
{
    if (level > SrsLogLevelVerbose) {
        return;
    }

    int size = 0;
    if (!srs_log_header(_srs_log_data, LOG_MAX_SIZE, utc, false, tag, context_id, "Verb", &size)) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    size += vsnprintf(_srs_log_data + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);

    write_log(_srs_log_data, size, SrsLogLevelVerbose);
}

void SrsFileLog::info(const char* tag, SrsContextId context_id, const char* fmt, ...)
{
    if (level > SrsLogLevelInfo) {
        return;
    }

    int size = 0;
    if (!srs_log_header(_srs_log_data, LOG_MAX_SIZE, utc, false, tag, context_id, "Debug", &size)) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    size += vsnprintf(_srs_log_data + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);

    write_log(_srs_log_data, size, SrsLogLevelInfo);
}

void SrsFileLog::trace(const char* tag, SrsContextId context_id, const char* fmt, ...)
{
    if (level > SrsLogLevelTrace) {
        return;
    }

    int size = 0;
    if (!srs_log_header(_srs_log_data, LOG_MAX_SIZE, utc, false, tag, context_id, "Trace", &size)) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    size += vsnprintf(_srs_log_data + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);

    write_log(_srs_log_data, size, SrsLogLevelTrace);
}

void SrsFileLog::warn(const char* tag, SrsContextId context_id, const char* fmt, ...)
{
    if (level > SrsLogLevelWarn) {
        return;
    }

    int size = 0;
    if (!srs_log_header(_srs_log_data, LOG_MAX_SIZE, utc, true, tag, context_id, "Warn", &size)) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    size += vsnprintf(_srs_log_data + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);

    write_log(_srs_log_data, size, SrsLogLevelWarn);
}

void SrsFileLog::error(const char* tag, SrsContextId context_id, const char* fmt, ...)
{
    if (level > SrsLogLevelError) {
        return;
    }
    
    int size = 0;
    if (!srs_log_header(_srs_log_data, LOG_MAX_SIZE, utc, true, tag, context_id, "Error", &size)) {
        return;
    }
    
    va_list ap;
    va_start(ap, fmt);
    // we reserved 1 bytes for the new line.
    size += vsnprintf(_srs_log_data + size, LOG_MAX_SIZE - size, fmt, ap);
    va_end(ap);
    
    // add strerror() to error msg.
    // Check size to avoid security issue https://github.com/ossrs/srs/issues/1229
    if (errno != 0 && size < LOG_MAX_SIZE) {
        size += snprintf(_srs_log_data + size, LOG_MAX_SIZE - size, "(%s)", strerror(errno));
    }
    
    write_log(_srs_log_data, size, SrsLogLevelError);
}

void SrsFileLog::write_log(char *str_log, int size, int level)
{
    srs_error_t err = srs_success;

    // ensure the tail and EOF of string
    //      LOG_TAIL_SIZE for the TAIL char.
    //      1 for the last char(0).
    size = srs_min(LOG_MAX_SIZE - 1 - LOG_TAIL_SIZE, size);
    
    // add some to the end of char.
    str_log[size++] = LOG_TAIL;
    
    // if not to file, to console and return.
    if (!log_to_file_tank) {
        // if is error msg, then print color msg.
        // \033[31m : red text code in shell
        // \033[32m : green text code in shell
        // \033[33m : yellow text code in shell
        // \033[0m : normal text code
        if (level <= SrsLogLevelTrace) {
            printf("%.*s", size, str_log);
        } else if (level == SrsLogLevelWarn) {
            printf("\033[33m%.*s\033[0m", size, str_log);
        } else{
            printf("\033[31m%.*s\033[0m", size, str_log);
        }
        fflush(stdout);
        
        return;
    }

    // write log to file.
    if (writer_ && (err = writer_->write(str_log, size, NULL)) != srs_success) {
        srs_error_reset(err); // Ignore any error for log writing.
    }
}

