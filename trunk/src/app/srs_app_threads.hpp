//
// Copyright (c) 2013-2022 The SRS Authors
//
// SPDX-License-Identifier: MIT or MulanPSL-2.0
//

#ifndef SRS_APP_THREADS_HPP
#define SRS_APP_THREADS_HPP

#include <srs_core.hpp>

#include <srs_app_hourglass.hpp>
#include <srs_kernel_error.hpp>
#include <srs_kernel_utility.hpp>

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <string>

class SrsThreadPool;
class SrsProcSelfStat;
class SrsFileWriter;
class SrsSharedPtrMessage;

// Protect server in high load.
class SrsCircuitBreaker : public ISrsFastTimer
{
private:
    // The config for high/critical water level.
    bool enabled_;
    int high_threshold_;
    int high_pulse_;
    int critical_threshold_;
    int critical_pulse_;
    int dying_threshold_;
    int dying_pulse_;
private:
    // Reset the water-level when CPU is low for N times.
    // @note To avoid the CPU change rapidly.
    int hybrid_high_water_level_;
    int hybrid_critical_water_level_;
    int hybrid_dying_water_level_;
public:
    SrsCircuitBreaker();
    virtual ~SrsCircuitBreaker();
public:
    srs_error_t initialize();
public:
    // Whether hybrid server water-level is high.
    bool hybrid_high_water_level();
    bool hybrid_critical_water_level();
    bool hybrid_dying_water_level();
// interface ISrsFastTimer
private:
    srs_error_t on_timer(srs_utime_t interval);
};

extern SrsCircuitBreaker* _srs_circuit_breaker;

// Initialize global shared variables cross all threads.
extern srs_error_t srs_global_initialize();

// The thread mutex wrapper, without error.
class SrsThreadMutex
{
private:
    pthread_mutex_t lock_;
    pthread_mutexattr_t attr_;
public:
    SrsThreadMutex();
    virtual ~SrsThreadMutex();
public:
    void lock();
    void unlock();
};

// The thread mutex locker.
// TODO: FIXME: Rename _SRS to _srs
#define SrsThreadLocker(instance) \
    impl__SrsThreadLocker _SRS_free_##instance(instance)

class impl__SrsThreadLocker
{
private:
    SrsThreadMutex* lock;
public:
    impl__SrsThreadLocker(SrsThreadMutex* l) {
        lock = l;
        lock->lock();
    }
    virtual ~impl__SrsThreadLocker() {
        lock->unlock();
    }
};

// The information for a thread.
class SrsThreadEntry
{
public:
    SrsThreadPool* pool;
    std::string label;
    std::string name;
    srs_error_t (*start)(void* arg);
    void* arg;
    int num;
    // @see https://man7.org/linux/man-pages/man2/gettid.2.html
    pid_t tid;
public:
    // The thread object.
    pthread_t trd;
    // The exit error of thread.
    srs_error_t err;
public:
    SrsThreadEntry();
    virtual ~SrsThreadEntry();
};

// Allocate a(or almost) fixed thread poll to execute tasks,
// so that we can take the advantage of multiple CPUs.
class SrsThreadPool
{
private:
    SrsThreadEntry* entry_;
    srs_utime_t interval_;
private:
    SrsThreadMutex* lock_;
    std::vector<SrsThreadEntry*> threads_;
private:
    // The hybrid server entry, the cpu percent used for circuit breaker.
    SrsThreadEntry* hybrid_;
    std::vector<SrsThreadEntry*> hybrids_;
private:
    // The pid file fd, lock the file write when server is running.
    // @remark the init.d script should cleanup the pid file, when stop service,
    //       for the server never delete the file; when system startup, the pid in pid file
    //       maybe valid but the process is not SRS, the init.d script will never start server.
    int pid_fd;
public:
    SrsThreadPool();
    virtual ~SrsThreadPool();
public:
    // Setup the thread-local variables.
    static srs_error_t setup_thread_locals();
    // Initialize the thread pool.
    srs_error_t initialize();
private:
    // Require the PID file for the whole process.
    virtual srs_error_t acquire_pid_file();
public:
    // Execute start function with label in thread.
    srs_error_t execute(std::string label, srs_error_t (*start)(void* arg), void* arg);
    // Run in the primordial thread, util stop or quit.
    srs_error_t run();
    // Stop the thread pool and quit the primordial thread.
    void stop();
public:
    SrsThreadEntry* self();
    SrsThreadEntry* hybrid();
    std::vector<SrsThreadEntry*> hybrids();
private:
    static void* start(void* arg);
};

// It MUST be thread-safe, global and shared object.
extern SrsThreadPool* _srs_thread_pool;

// The default capacity of queue.
#define SRS_CONST_MAX_QUEUE_SIZE 1 * 1024 * 1024

// The metadata for circle queue.
#pragma pack(1)
struct SrsCircleQueueMetadata
{
    // The number of capacity of queue, the max of elements.
    uint32_t nn_capacity;
    // The number of elems in queue.
    volatile uint32_t nn_elems;

    // The position we're reading, like a token we got to read.
    volatile uint32_t reading_token;
    // The position already read, ok for writting.
    volatile uint32_t read_already;

    // The position we're writing, like a token we got to write.
    volatile uint32_t writing_token;
    // The position already written, ready for reading.
    volatile uint32_t written_already;

    SrsCircleQueueMetadata() : nn_capacity(0), nn_elems(0), reading_token(0), read_already(0), writing_token(0), written_already(0) {
    }
};
#pragma pack()

template <typename T>
class SrsCircleQueue
{
public:
    SrsCircleQueue(uint32_t capacity = 0) {
        info = new SrsCircleQueueMetadata();
        data = NULL;
        initialize(capacity);
        lock = new SrsThreadMutex();
    }

    ~SrsCircleQueue() {
        srs_freep(info);
        srs_freepa(data);
        srs_freep(lock);
    }

private:
    void initialize(uint32_t capacity) {
        uint32_t count = (capacity > 0) ? capacity : SRS_CONST_MAX_QUEUE_SIZE;
        count = srs_min(count, SRS_CONST_MAX_QUEUE_SIZE);

        // We increase an element for reserved.
        count += 1;

        data = new T[count];
        info->nn_capacity = count;
        info->nn_elems = 0;
    }

public:
    // Push the elem to the end of queue.
    srs_error_t push(const T& elem) {
        srs_error_t err = srs_success;

        if (!info || !data) {
            return srs_error_new(ERROR_QUEUE_PUSH, "queue init failed");
        }

        // Find out a position to write at current, using mutex to ensure we got a dedicated one.
        lock->lock();

        uint32_t current = info->writing_token;
        uint32_t next = (current + 1) % info->nn_capacity;

        if (next == info->read_already) {
	    lock->unlock();
            return srs_error_new(ERROR_QUEUE_PUSH, "queue is full");
        }

        info->writing_token = next;

        // Then, we write the elem at current position, note that it's not readable.
        data[current] = elem;

        info->written_already = next;

        lock->unlock();

        // Done, we have already written an elem, increase the size of queue.
        __sync_add_and_fetch(&info->nn_elems, 1);

        return err;
    }

    // Remove and return the message from the font of queue.
    // Error if empty. Please use size() to check before shift().
    srs_error_t shift(T& elem) {
        srs_error_t err = srs_success;

        if (!info || !data) {
            return srs_error_new(ERROR_QUEUE_POP, "queue init failed");
        }

        // Find out a position to read at current, using mutex to ensure we got a dedicated one.
        lock->lock();

        uint32_t current = info->reading_token;
        uint32_t next = (current + 1) % info->nn_capacity;

        if (current == info->written_already) {
	    lock->unlock();
            return srs_error_new(ERROR_QUEUE_POP, "queue is empty");
        }
        info->reading_token = next;

        // Then, we read the elem out at the current position, note that it's not writable.
        elem = data[current];
        info->read_already = next;

        lock->unlock();

        // Done, we have already read an elem, decrease the size of queue.
        __sync_sub_and_fetch(&info->nn_elems, 1);

        return err;
    }

    unsigned int size() const {
        return (!info || !data) ? 0 : info->nn_elems;
    }

    unsigned int capacity() const {
        return info->nn_capacity - 1;
    }

private:
    SrsCircleQueue(const SrsCircleQueue&);
    const SrsCircleQueue& operator=(const SrsCircleQueue&);

private:
    SrsCircleQueueMetadata* info;
    T* data;
    SrsThreadMutex* lock;
};

// Async file writer, it's thread safe.
class SrsAsyncFileWriter : public ISrsWriter
{
    friend class SrsAsyncLogManager;
private:
    std::string filename_;
    SrsFileWriter* writer_;
private:
    // The thread-queue, to flush to disk by dedicated thread.
    SrsCircleQueue<SrsSharedPtrMessage*>* chunks_;
private:
    SrsAsyncFileWriter(std::string p);
    virtual ~SrsAsyncFileWriter();
public:
    // Open file writer, in truncate mode.
    virtual srs_error_t open();
    // Open file writer, in append mode.
    virtual srs_error_t open_append();
    // Close current writer.
    virtual void close();
// Interface ISrsWriteSeeker
public:
    virtual srs_error_t write(void* buf, size_t count, ssize_t* pnwrite);
    virtual srs_error_t writev(const iovec* iov, int iovcnt, ssize_t* pnwrite);
public:
    // Flush thread-queue to disk, generally by dedicated thread.
    srs_error_t flush();
};

// The async log file writer manager, use a thread to flush multiple writers,
// and reopen all log files when got LOGROTATE signal.
class SrsAsyncLogManager
{
private:
    // The async flush interval.
    srs_utime_t interval_;
private:
    // The async reopen event.
    bool reopen_;
    // The global shared writer.
    SrsAsyncFileWriter* writer_;
public:
    SrsAsyncLogManager();
    virtual ~SrsAsyncLogManager();
public:
    // Initialize the async log manager.
    srs_error_t initialize();
    // Run the async log manager thread.
    static srs_error_t start(void* arg);
    // Create or get the global shared writer, which is thread safe.
    // Note that SRS always use one global _srs_log to create the writer.
    srs_error_t writer(std::string filename, SrsAsyncFileWriter** pwriter);
    // Reopen all log files, asynchronously.
    virtual void reopen();
    // Get the number of logs in queue.
    int size();
private:
    srs_error_t do_start();
};

// It MUST be thread-safe, global shared object.
extern SrsAsyncLogManager* _srs_async_log;

#endif

