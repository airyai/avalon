/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2012  <copyright holder> <email>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "../define.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/unordered_map.hpp>

#include "asyncresult.h"
#include "threadgroup.h"

BEGIN_AVALON_NS2(thread)

/// The ThreadPool.
/**
 * This class uses boost::asio::io_service to manage threads, 
 * so that it can provide a multi-threaded queued work executor.
 */
class ThreadPool
{
public:
    /// create a new threadpool.
    /**
     * @param workers The initial thread count.
     * @param max_queue The maximum queue size for tasks. Zero means no limit.
     */
    ThreadPool(size_t workers, size_t max_queue);
    
    /// destroy the threadpool
    virtual ~ThreadPool();
    
    /// Add a number of threads to the pool.
    void add_workers(size_t n);
    
    /// Reduce a number of threads in the pool.
    void reduce_workers(size_t n);
    
    /// Get worker count.
    size_t worker_count();
    
    /// Run the threadpool.
    /**
     * Start the threadpool task loop. This method blocks the caller's 
     * thread, executes all the tasks existing, and waits for more tasks. 
     * Only when stop is called, the loop will end.
     * 
     * @throw AvalonThreadPoolIsRunning.
     */
    void run();
    
    /// Wait for all jobs to be finished.
    /**
     * If more jobs are added, then it will wait until all the jobs are done.
     * 
     * @param timeout The maximum waiting milliseconds. Zero means no limit.
     * @return false if time exceeds, otherwise true (including no waiting).
     */
    bool wait(size_t timeout = 0);
    
    /// Run the threadpool.
    /**
     * Stop the threadpool task loop. Existing tasks will remain in queue, 
     * waiting for next task loop.
     * 
     * @param timeout Milliseconds to wait before tell the threads to interrupt.
     *      Zero means do not wait and interrupt immediately.
     * @throw AvalonThreadPoolIsNotRunning.
     */
    void stop(size_t timeout = 0);
    
    /// Cancel all jobs in the queue.
    void cancel_all();
    
    /// Add a job to the workpool's job queue.
    /**
     * Add a certain job to the job queue.
     * 
     * All input data should be alive during the job schedule. Also, a non-deep 
     * copy may be emitted on job and callback function object. So consider using 
     * boost::shared_ptr to keep the data available, and prevent from too much 
     * copying.
     * 
     * @param job Job function object.
     * @param callback The callback function object after the job finished.
     * @return AsyncResult instance. Job can be marked cancelled via AsyncResult.
     */
    AsyncResultPtr submit(const AsyncResult::Task& job, const AsyncResult::Callback& callback);
    
protected:
    /// Notify a thread to give up executing io_service loop.
    class InterruptWorker {};
    
    /// Indicating the threads are running.
    bool running_;
    
    /// The thread count.
    size_t workers_;
    
    /// The maximum queued tasks.
    size_t max_queue_;
    
    /// The mutex type.
    typedef boost::mutex Lock;
    
    /// The mutex.
    Lock lock_;
    
    /// The condition variable to notify all jobs done.
    boost::condition_variable cond_;
    
    /// The io_service.
    boost::shared_ptr<boost::asio::io_service> service_;
    
    /// Occupy the io_service.
    boost::shared_ptr<boost::asio::io_service::work> work_;
    
    /// The thread_group.
    boost::shared_ptr<ThreadGroup> threads_;
    
    /// The next available job id.
    unsigned int next_job_id_;
    
    /// The task set type.
    typedef boost::unordered_map<unsigned int, AsyncResultPtr> Jobs;
    
    /// The task set.
    boost::shared_ptr<Jobs> jobs_;
    
    /// Run the io_service loop in a thread.
    void run_thread();
    
    /// The handler to reduce worker.
    /**
     * @param thread_group Current active ThreadGroup instance. 
     */
    void reduce_worker_handler(const ThreadGroup *thread_group);
    
    /// The handler for task finished.
    void task_finish_handler(AsyncResult& ar, unsigned int job_id);
};

END_AVALON_NS2

#endif // THREADPOOL_H
