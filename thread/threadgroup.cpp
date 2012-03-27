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


#include "threadgroup.h"

#include <boost/foreach.hpp>

BEGIN_AVALON_NS2(thread)

using boost::shared_mutex;

ThreadGroup::ThreadGroup()
 :  lock_(),
    threads_()
{
}

ThreadGroup::~ThreadGroup()
{
    threads_.clear();
}

void ThreadGroup::add_thread(const avalon::thread::ThreadPtr& thread)
{
    boost::lock_guard<shared_mutex> locker(lock_);
    if (thread)
        threads_.insert(thread);
}

void ThreadGroup::remove_thread(const avalon::thread::ThreadPtr& thread)
{
    boost::lock_guard<shared_mutex> locker(lock_);
    Threads::iterator it = threads_.find(thread);
    if (it != threads_.end())
        threads_.erase(it);
}

size_t ThreadGroup::size()
{
    boost::shared_lock<shared_mutex> locker(lock_);
    return threads_.size();
}

std::list< ThreadPtr > ThreadGroup::all_threads()
{
    boost::shared_lock<shared_mutex> locker(lock_);
    std::list<ThreadPtr> ret;
    for (Threads::iterator it = threads_.begin(); it != threads_.end(); it++) {
        ret.push_back(*it);
    }
    return ret;
}

void ThreadGroup::interrupt_all()
{
    boost::shared_lock<shared_mutex> locker(lock_);
    for (Threads::iterator it = threads_.begin(); it != threads_.end(); it++) {
        (*it)->interrupt();
    }
}

void ThreadGroup::join_all()
{
    boost::shared_lock<shared_mutex> locker(lock_);
    for (Threads::iterator it = threads_.begin(); it != threads_.end(); it++) {
        (*it)->join();
    }
}

void ThreadGroup::join_and_interrupt_all(size_t timeout)
{
    boost::shared_lock<shared_mutex> locker(lock_);
    for (Threads::iterator it = threads_.begin(); it != threads_.end(); it++) {
        try {
            if (timeout) {
                (*it)->timed_join(boost::posix_time::milliseconds(timeout));
            }
            (*it)->interrupt();
        } catch (boost::thread_interrupted) {}
    }
}



END_AVALON_NS2

