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

AsyncResult::ScopedStatus::ScopedStatus ( avalon::thread::AsyncResult& ar, 
                                          avalon::thread::AsyncResult::Status hold_status,
                                          avalon::thread::AsyncResult::Status leave_status,
                                          avalon::thread::AsyncResult::Status ensure_status)
 :  ar_(ar), 
    changable_(false), 
    leave_status_(leave_status)
{
    AsyncResult::Lock::scoped_lock locker(ar.lock_);
    if (ar.status_ == ensure_status) {
        changable_ = true;
        ar.status_ = hold_status;
    }
}

AsyncResult::ScopedStatus::~ScopedStatus()
{
    if (changable_) {
        AsyncResult::Lock::scoped_lock locker(ar_.lock_);
        ar_.status_ = leave_status_;
    }
}

bool AsyncResult::ScopedStatus::changable() const
{
    return changable_;
}

AsyncResult::Status AsyncResult::ScopedStatus::leave_status() const
{
    return leave_status_;
}

void AsyncResult::ScopedStatus::set_leave_status ( AsyncResult::Status leave_status )
{
    leave_status_ = leave_status;
}

AsyncResult::AsyncResult ( const avalon::thread::AsyncResult::Task& task )
    :   lock_(), status_(WAIT), task_(task), callbacks_()
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

void AsyncResult::set_success()
{
    bool should_call = false;
    {
        ScopedStatus status_locker(*this, RUNNING, SUCCESS, RUNNING);
        should_call = status_locker.changable();
    }
    if (should_call) {
        call_success();
    }
}


const avalon::AvalonException* AsyncResult::exception() const
{
    return exception_.get();
}

void AsyncResult::set_error()
{
    bool should_call = false;
    {
        ScopedStatus status_locker(*this, RUNNING, ERROR, RUNNING);
        should_call = status_locker.changable();
    }
    if (should_call) {
        call_error();
    }
}

void AsyncResult::set_error ( const avalon::AvalonException& other )
{
    bool should_call = false;
    {
        ScopedStatus status_locker(*this, RUNNING, ERROR, RUNNING);
        should_call = status_locker.changable();
    }
    if (should_call) {
        exception_.reset(new AvalonException(other));
        call_error();
    }
}

void AsyncResult::call_success ()
{
    BOOST_FOREACH(CallbackListItem& it, callbacks_)
    {
        if (it.first & CALLBACK_SUCCESS) {
            try {
                it.second(*this);
            } catch (...) {
                // simply ignore the callback errors.
            }
        }
    }
}

void AsyncResult::call_error ()
{
    BOOST_FOREACH(CallbackListItem& it, callbacks_)
    {
        if (it.first & CALLBACK_ERROR) {
            try {
                it.second(*this);
            } catch (...) {
                // simply ignore the callback errors.
            }
        }
    }
}

void AsyncResult::call_cancel()
{
    BOOST_FOREACH(CallbackListItem& it, callbacks_)
    {
        if (it.first & CALLBACK_CANCEL) {
            try {
                it.second(*this);
            } catch (...) {
                // simply ignore the callback errors.
            }
        }
    }
}

void AsyncResult::add_all ( const avalon::thread::AsyncResult::Callback& callback )
{
    callbacks_.push_back(std::make_pair(CALLBACK_ALL, callback));
}

void AsyncResult::add_success ( const avalon::thread::AsyncResult::Callback& callback )
{
    callbacks_.push_back(std::make_pair(CALLBACK_SUCCESS, callback));
}

void AsyncResult::add_error ( const avalon::thread::AsyncResult::Callback& callback )
{
    callbacks_.push_back(std::make_pair(CALLBACK_ERROR, callback));
}

void AsyncResult::add_cancel ( const avalon::thread::AsyncResult::Callback& callback )
{
    callbacks_.push_back(std::make_pair(CALLBACK_CANCEL, callback));
}

bool AsyncResult::set_cancel()
{
    cancel();
}

bool AsyncResult::cancel()
{
    bool should_call_cancel = false;
    {
        ScopedStatus status_locker(*this, CANCELLING, CANCELLED);
        if (status_locker.changable()) {
            should_call_cancel = true;
        }
    }
    if (should_call_cancel) {
        call_cancel();
    }
    return should_call_cancel;
}

bool AsyncResult::execute()
{
    bool should_call = false;
    {
        ScopedStatus status_locker(*this, RUNNING, RUNNING);
        should_call = status_locker.changable();
    }
    if (should_call) {
        try {
            task_();
            set_success();
        } catch (AvalonException& err) {
            set_error(err);
        } catch (...) {
            set_error();
        }
    }
}




END_AVALON_NS2

