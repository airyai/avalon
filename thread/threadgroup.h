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


#ifndef THREADGROUP_H
#define THREADGROUP_H

#include "../define.h"

#include <list>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_set.hpp>

BEGIN_AVALON_NS2(thread)

/// The shared_ptr of boost::thread.
typedef boost::shared_ptr<boost::thread> ThreadPtr;

/// Manages a number of threads.
class ThreadGroup
 :  public boost::thread_group
{
public:
    /// Create a new ThreadGroup instance.
    ThreadGroup();
    
    /// Dispose the ThreadGroup instance.
    virtual ~ThreadGroup();
    
    /// Create a thread and add it to the group.
    /**
     * @param threadfunc Thread main loop function.
     */
    template <typename F>
    ThreadPtr create_thread(F threadfunc);
    
    /// Add a thread to the group.
    /**
     * @param thread Shared pointer managed thread.
     */
    void add_thread(const ThreadPtr& thread);
    
    /// Remove a thread from the group.
    /**
     * @param thread Shared pointer managed thread.
     */
    void remove_thread(const ThreadPtr& thread);
    
    /// Get the thread list.
    std::list<ThreadPtr> all_threads();
    
    /// Get the active thread count.
    size_t size();
    
    /// Join all threads.
    void join_all();
    
    /// Join each thread in limited time, and interrupt it if timeout.
    /**
     * @param timeout Wait milliseconds for each thread. Zero means do not 
     *      wait and interrupt immediately.
     */
    void join_and_interrupt_all(size_t timeout);
    
    /// Interrupt all threads.
    void interrupt_all();
    
protected:
    /// The object lock.
    boost::shared_mutex lock_;
    
    /// The map type.
    typedef boost::unordered_set<ThreadPtr> Threads;
    
    /// The thread hashset.
    Threads threads_;
};

template <typename F>
ThreadPtr ThreadGroup::create_thread(F threadfunc)
{
    boost::lock_guard<boost::shared_mutex> locker(lock_);
    ThreadPtr thread(new boost::thread(threadfunc));
    threads_.insert(thread);
    return thread;
}


END_AVALON_NS2

#endif // THREADGROUP_H
