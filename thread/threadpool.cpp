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


#include "threadpool.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/scope_exit.hpp>
#include <boost/foreach.hpp>

#include "errors.h"


BEGIN_AVALON_NS2(thread)

using boost::shared_mutex;

ThreadPool::ThreadPool(size_t workers, size_t max_queue)
 :  running_(false),
    workers_(workers),
    max_queue_(max_queue),
    lock_(),
    cond_(),
    service_(),
    work_(),
    threads_(),
    next_job_id_(0),
    jobs_(new Jobs())
{
}

ThreadPool::~ThreadPool()
{
    stop();
    cancel_all();
}

void ThreadPool::add_workers(size_t n)
{
    boost::unique_lock<Lock> locker(lock_);
    workers_ += n;
    if (!running_)
        return;
    
    for (size_t i=0; i<n; i++) {
        threads_->create_thread(boost::bind(&ThreadPool::run_thread, this));
    }
}

void ThreadPool::reduce_workers(size_t n)
{
    boost::unique_lock<Lock> locker(lock_);
    if (n >= workers_)
        AVALON_THROW_INFO( AvalonInvalidArgument, error_argument("n") );
    workers_ -= n;
    if (!running_)
        return;
    
    const ThreadGroup* current_thread_group = threads_.get();
    for (size_t i=0; i<n; i++) {
        service_->post(boost::bind(&ThreadPool::reduce_worker_handler, this, current_thread_group));
    }
}

size_t ThreadPool::worker_count()
{
    boost::unique_lock<Lock> locker(lock_);
    return workers_;
}

void ThreadPool::run_thread()
{
    // We do not really remove the thread from ThreadGroup.
    // // Remove from thread list on exit.
    // BOOST_SCOPE_EXIT( (&thread)(&threads_) ) {
    //     if (threads_) threads_->remove_thread(thread);
    // } BOOST_SCOPE_EXIT_END
    
    // Enter thread.
    try {
        // Run the io_service loop.
        service_->run();
        
    } catch (InterruptWorker) {
        // A managed style to reduce worker.
    }
}

void ThreadPool::reduce_worker_handler(const ThreadGroup* thread_group)
{
    if (threads_.get() == thread_group)
        throw InterruptWorker();
}

void ThreadPool::run()
{
    boost::unique_lock<Lock> locker(lock_);
    if (running_) return;
    
    // check jobs
    if (!jobs_) jobs_.reset(new Jobs);
    
    // init thread group
    threads_.reset(new ThreadGroup());
    
    // init main loop
    service_.reset(new boost::asio::io_service());
    work_.reset(new boost::asio::io_service::work(*service_));
    
    for (size_t i=0; i<workers_; i++) {
        threads_->create_thread(boost::bind(&ThreadPool::run_thread, this));
    }
    
    // put unfinished jobs in loop.
    for (Jobs::iterator it=jobs_->begin(); it!=jobs_->end(); it++) {
        if (it->second)
            service_->post(boost::bind(&AsyncResult::execute, it->second));
    }
}

void ThreadPool::stop(size_t timeout)
{
    boost::shared_ptr<ThreadGroup> threads;
    {
        boost::unique_lock<Lock> locker(lock_);
        if (!running_) return;
        
        // stop the main loop
        service_->stop();
        work_.reset();
        
        // swap the pointer
        // if we do not do this, mutex may be blocked in join_all.
        threads = threads_;
        threads_.reset();
    }
    
    // kill threads
    threads->join_and_interrupt_all(timeout);
}

void ThreadPool::cancel_all()
{
    boost::shared_ptr<Jobs> jobs;
    {
        boost::unique_lock<Lock> locker(lock_);
        jobs = jobs_;
        jobs_.reset();
    }
    
    for (Jobs::iterator it=jobs->begin(); it != jobs->end(); it++) {
        if (it->second) it->second->cancel();
    }
}

void ThreadPool::task_finish_handler(AsyncResult& ar, unsigned int job_id)
{
    boost::unique_lock<Lock> locker(lock_);
    if (!jobs_) return;
    Jobs::iterator it = jobs_->find(job_id);
    if (it != jobs_->end())
        jobs_->erase(it);
}

bool ThreadPool::wait(size_t timeout)
{
    boost::unique_lock<Lock> locker(lock_);
    if (timeout)
        return cond_.timed_wait(locker, boost::posix_time::milliseconds(timeout));
    else {
        cond_.wait(locker);
        return true;
    }
}

AsyncResultPtr ThreadPool::submit(const avalon::thread::AsyncResult::Task& job, 
                                  const avalon::thread::AsyncResult::Callback& callback)
{
    boost::unique_lock<Lock> locker(lock_);
    
    // check limit
    if (!jobs_ || jobs_->size() >= max_queue_)
        AVALON_THROW(AvalonThreadPoolIsFull);
    
    // Create AsyncResult
    unsigned int job_id = next_job_id_++;
    AsyncResultPtr p(new AsyncResult(job));
    p->add_all(callback);
    p->add_all(boost::bind(&ThreadPool::task_finish_handler, this, _1, job_id));
    
    // Add to internal work list.
    jobs_->insert(std::make_pair(job_id, p));
    
    // Submit to io_service
    if (running_) service_->post(boost::bind(&AsyncResult::execute, p));
}

END_AVALON_NS2
