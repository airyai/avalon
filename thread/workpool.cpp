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


#include "workpool.h"

#include <boost/foreach.hpp>
#include <boost/asio/deadline_timer.hpp>

BEGIN_AVALON_NS2(thread)

using boost::asio::deadline_timer;

WorkPoolBase::WorkPoolBase ( size_t max_queue )
 :  lock_(),
    jobs_(new JobSet), 
    max_queue_(max_queue)
{
}

WorkPoolBase::~WorkPoolBase()
{
    stop();
}

void WorkPoolBase::stop(bool interrupt)
{
    // gain a copy from old jobset and create a new jobset.
    // so that I can prevent from long-life lock (like mutex).
    
    boost::shared_ptr<JobSet> jobs;
    {
        Lock::scoped_lock locker(lock_);
        jobs = jobs_;
        jobs_.reset(new JobSet);
    }
    
    BOOST_FOREACH(const AsyncResultPtr& ar, *jobs) {
        ar->cancel();
    }
}


void WorkPoolBase::drop ( const avalon::thread::AsyncResultPtr& ar )
{
    // NOTE: I assume that hash set is fast enough on a find and a erase.
    Lock::scoped_lock locker(lock_);
    JobSet::iterator it = jobs_->find(ar);
    if (it != jobs_->end())
        jobs_->erase(it);
}

void WorkPoolBase::drop_all ()
{
    Lock::scoped_lock locker(lock_);
    jobs_.reset(new JobSet);
}

AsyncResultPtr WorkPoolBase::submit ( const avalon::thread::AsyncResult::Task& job, 
                                      const avalon::thread::AsyncResult::Callback& callback )
{
    AsyncResultPtr ar(new AsyncResult(job));
    ar->add_all(callback);
    ar->add_all(boost::bind(&WorkPoolBase::drop, this, ar));
    {
        Lock::scoped_lock locker(lock_);
        if (jobs_->size() >= max_queue_)
            throw AvalonWorkPoolFull();
        jobs_->insert(ar);
    }
    return ar;
}


WorkPool::WorkPool ( size_t worker_count, size_t max_queue )
 :  WorkPoolBase(max_queue),
    work_(service_),
    pool_()
{  
    for (size_t i=0; i<worker_count; i++) {
        pool_.create_thread(boost::bind(&boost::asio::io_service::run, &service_));
    }
}

WorkPool::~WorkPool()
{
}

void WorkPool::stop(bool interrupt)
{
    service_.stop();
    avalon::thread::WorkPoolBase::stop();
    
    if (interrupt) {
        pool_.interrupt_all();
    } else {
        pool_.join_all();
    }
}

AsyncResultPtr WorkPool::submit ( const avalon::thread::AsyncResult::Task& job, 
                                  const avalon::thread::AsyncResult::Callback& callback )
{
    AsyncResultPtr ar = avalon::thread::WorkPoolBase::submit ( job, callback );
    service_.post(boost::bind(&WorkPool::exec_, this, ar));
    return ar;
}

void WorkPool::exec_(const AsyncResultPtr& ar)
{
    ar->do_task();
}



END_AVALON_NS2

