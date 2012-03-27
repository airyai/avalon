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


#include "asyncresult.h"
#include <boost/foreach.hpp>

BEGIN_AVALON_NS2(thread)

AsyncResult::ResultBase::ResultBase()
{
}

AsyncResult::ResultBase::~ResultBase()
{
}

AsyncResult::AsyncResult ( const avalon::thread::AsyncResult::Task& task )
 :  lock_(), 
    cond_(), 
    status_(WAIT),
    task_(task), 
    callbacks_(), 
    exception_(), 
    result_()
{
}

AsyncResult::Status AsyncResult::status()
{
    Lock::scoped_lock locker(lock_);
    return status_;
}

AsyncResult::~AsyncResult()
{
    cancel();
}

const avalon::AvalonException* AsyncResult::exception()
{
    Lock::scoped_lock locker(lock_);
    return exception_.get();
}

void AsyncResult::set_success()
{
    bool should_call = false;
    {
        Lock::scoped_lock locker(lock_);
        if (status_ == RUNNING) {
            should_call = true;
            status_ = SUCCESS;
        }
    }
    if (should_call) {
        call_callback(CALLBACK_SUCCESS);
    }
}

void AsyncResult::set_interrupt()
{
    bool should_call = false;
    {
        Lock::scoped_lock locker(lock_);
        if (status_ == RUNNING) {
            should_call = true;
            status_ = INTERRUPTED;
        }
    }
    if (should_call) {
        call_callback(CALLBACK_INTERRUPT);
    }
}

void AsyncResult::set_error()
{
    set_error(NULL);
}

void AsyncResult::set_error ( const avalon::AvalonException* err )
{
    bool should_call = false;
    {
        Lock::scoped_lock locker(lock_);
        if (status_ == RUNNING) {
            should_call = true;
            status_ = ERROR;
        }
        if (err) {
            exception_.reset(new AvalonException(*err));
        } else {
            exception_.reset();
        }
    }
    if (should_call) {
        call_callback(CALLBACK_ERROR);
    }
}

bool AsyncResult::set_cancel()
{
    cancel();
}

void AsyncResult::clear_result()
{
    Lock::scoped_lock locker(lock_);
    result_.reset();
}

void AsyncResult::call_callback(unsigned int flag)
{
    cond_.notify_all();
    
    CallbackList callbacks;
    {
        Lock::scoped_lock locker(lock_);
        callbacks = callbacks_;
        callbacks_.clear();
    }
    BOOST_FOREACH(CallbackListItem& it, callbacks)
    {
        if (it.first & flag) {
            try {
                it.second(*this);
            } catch (...) {
                // simply ignore the callback errors.
            }
        }
    }
}

void AsyncResult::add_callback(const Callback& callback, unsigned int flag) {
    bool should_call = false;
    {
        Lock::scoped_lock locker(lock_);
        if (status_ == WAIT || status_ == RUNNING) {
            callbacks_.push_back(std::make_pair(flag, callback));
        } else {
            should_call = true;
        }
    }
    if (should_call) callback(*this);
}

void AsyncResult::add_all ( const avalon::thread::AsyncResult::Callback& callback )
{
    add_callback(callback, CALLBACK_ALL);
}

void AsyncResult::add_success ( const avalon::thread::AsyncResult::Callback& callback )
{
    add_callback(callback, CALLBACK_SUCCESS);
}

void AsyncResult::add_error ( const avalon::thread::AsyncResult::Callback& callback )
{
    add_callback(callback, CALLBACK_ERROR);
}

void AsyncResult::add_cancel ( const avalon::thread::AsyncResult::Callback& callback )
{
    add_callback(callback, CALLBACK_CANCEL);
}

void AsyncResult::add_interrupt ( const avalon::thread::AsyncResult::Callback& callback )
{
    add_callback(callback, CALLBACK_INTERRUPT);
}

bool AsyncResult::cancel()
{
    bool should_call = false;
    {
        Lock::scoped_lock locker(lock_);
        if (status_ == WAIT) {
            should_call = true;
            status_ = CANCELLED;
        }
    }
    if (should_call) {
        call_callback(CALLBACK_CANCEL);
    }
    return should_call;
}

bool AsyncResult::execute()
{
    bool should_call = false;
    {
        Lock::scoped_lock locker(lock_);
        if (status_ == WAIT) {
            should_call = true;
            status_ = RUNNING;
        }
    }
    if (should_call) {
        try {
            task_(*this);
            set_success();
        } catch (AvalonException& err) {
            set_error(&err);
        } catch (boost::thread_interrupted) {
            set_interrupt();
            throw;
        } catch (...) {
            set_error();
        }
    }
    return should_call;
}

bool AsyncResult::wait(size_t timeout)
{
    Lock::scoped_lock locker(lock_);
    if (status_ != WAIT && status_ != RUNNING) {
        return true;
    }
    if (timeout)
        return cond_.timed_wait(locker, boost::posix_time::milliseconds(timeout));
    else {
        cond_.wait(locker);
        return true;
    }
}



END_AVALON_NS2

