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


#ifndef WORKPOOL_H
#define WORKPOOL_H

#include "../define.h"

#include <boost/unordered_set.hpp>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/asio/io_service.hpp>

#include "asyncresult.h"

BEGIN_AVALON_NS2(thread)

/// The base workpool.
/**
 * This class defines a general interface for all workpools.
 * 
 * The workpool should be run in background, and waiting for jobs, until 
 * it is stopped.
 */
class WorkPoolBase : private boost::noncopyable
{
public:
    /// create a new workpool.
    /**
     * @param max_queue The maximum queue size for jobs. Zero means no limit.
     */
    explicit WorkPoolBase(size_t max_queue = 0);
    
    /// destroy the workpool.
    virtual ~WorkPoolBase();
    
    /// Stop the workpool's loop.
    /**
     * All jobs that is not running will be cancelled.
     * 
     * @param interrupt Interrupt the running jobs. Children may implement 
     *      this function, but may not either.
     * 
     * @throw AvalonWorkPoolMethodNotSupport If the workpool doesn't support.
     */
    virtual void stop(bool interrupt = false);
    
    /// Add a job to the workpool's job queue.
    /**
     * Add a certain job to the job queue.
     * 
     * All data should be accessible during the job schedule. Also, a non-deep 
     * copy may be emitted on job and callback function object. So consider using 
     * boost::shared_ptr to keep the data available, and prevent from too much 
     * copying.
     * 
     * Extended classes should override this to provide thread-safe codes.
     * 
     * @param job Job function object.
     * @param callback The callback function object after the job finished.
     * @return AsyncResult instance. Job can be marked cancelled via AsyncResult.
     */
    virtual AsyncResultPtr submit(const AsyncResult::Task& job, const AsyncResult::Callback& callback);
    
protected:
    /// The spin lock type.
    typedef boost::detail::spinlock Lock;
    
    /// The spin lock.
    Lock lock_;
    
    /// The job list type.
    typedef boost::unordered_set<AsyncResultPtr> JobSet;
    
    /// The job list.
    /**
     * I use shared_ptr to prevent mutex (shared_ptr uses spinlock,
     * or even atomic counter) when I need to copy all the content 
     * from the old set and create a new one.
     */
    boost::shared_ptr<JobSet> jobs_;
    
    /// The maximum job queue size.
    size_t max_queue_;
    
    /// Remove a job from job list.
    /**
     * Extended classes should override this to provide thread-safe coes.
     */
    virtual void drop(const AsyncResultPtr &ar);
    
    /// Remove all jobs from job list.
    virtual void drop_all();
};

/// The io_service based workpool.
/**
 * This class uses boost::asio::io_service to manage threads, 
 * so that it can provide a multi-threaded queued work executor.
 */
class WorkPool : public WorkPoolBase
{
public:
    /// Create a new workpool instance.
    /**
     * @param worker_count The number of worker threads.
     */
    explicit WorkPool(size_t worker_count, size_t max_queue = 0);
    
    /// Dispose the workpool.
    virtual ~WorkPool();

    /// Stop the workpool's loop.
    virtual void stop(bool interrupt = false);
    
    /// Add a job to the workpool's job queue.
    virtual AsyncResultPtr submit(const AsyncResult::Task& job, const AsyncResult::Callback& callback);

protected:
    /// The io_service.
    boost::asio::io_service service_;
    
    /// Occupy the io_service to make it keep running.
    boost::asio::io_service::work work_;
    
    /// The thread_group.
    boost::thread_group pool_;
    
    /// The handler for io_service.
    void exec_(const AsyncResultPtr& ar);
};

END_AVALON_NS2

#endif // WORKPOOL_H
