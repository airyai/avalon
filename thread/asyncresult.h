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


#ifndef ASYNCRESULT_H
#define ASYNCRESULT_H

#include "../define.h"

#include <list>
#include <boost/function.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include "../errors.h"

BEGIN_AVALON_NS2(thread)

/// The async result.
/**
 * This class manages an asynchronous function call, as well as 
 * its success, error and cancel callbacks.
 * 
 * It is the caller's responsibility to keep AsyncResult alive 
 * when the task is running.
 */
class AsyncResult
    : private boost::noncopyable
{
public:
    /// The function call type.
    typedef boost::function<void ()> Task;
    
    /// The callback type.
    typedef boost::function<void (AsyncResult&)> Callback;
    
    /// The AsyncResult status type.
    enum Status
    {
        WAIT = 0,
        RUNNING = 1,
        SUCCESS = 2,
        ERROR = 3,
        CANCELLING = 4,
        CANCELLED = 5
    };
    
    /// The scoped status monitor.
    /**
     * The moment ScopedStatus is created, it will make sure that 
     * the status of AsyncResult is ensure_status, otherwise it will mark the 
     * AsyncResult unchangable.
     * 
     * If the AsyncResult is changable, then the Monitor will immediately 
     * set the status hold_status. And when the monitor is to be disposed,
     * the status will be changed to leave_status.
     */
    class ScopedStatus
    {
    public:
        ScopedStatus(AsyncResult& ar, Status hold_status = RUNNING, 
                     Status leave_status = SUCCESS, Status ensure_status = WAIT);
        ~ScopedStatus();
        
        bool changable() const;
        Status leave_status() const;
        void set_leave_status(Status leave_status);
        
    protected:
        AsyncResult& ar_;
        bool changable_;
        Status leave_status_;
    };
    
    /// create a new AsyncResult.
    explicit AsyncResult(const Task& task);
    
    /// Dispose the AsyncResult.
    ~AsyncResult();
    
    /// The result status.
    /**
     * It is safe to take actions when the status is SUCCESS, ERROR or CANCELLED.
     */
    Status status();
    
    /// Set the status as success.
    void set_success();
    
    /// The result exception.
    const AvalonException* exception() const;
    
    /// Set the error as an unknown exception.
    void set_exception();
    
    /// Set the error as an AvalonException.
    void set_exception(const AvalonException& other);
    
    /// Execute the callbacks on success.
    void call_success();
    
    /// Execute the callbacks on error.
    void call_error();
    
    /// Execute the callbacks on cancel.
    void call_cancel();
    
    /// Add callback on success
    /**
     * Add callback is not thread safe, for the initialization methods is 
     * almost occurred when the object is being initialized.
     */
    void add_success(const Callback& callback);
    
    /// Add callback on error
    /**
     * Add callback is not thread safe, for the initialization methods is 
     * almost occurred when the object is being initialized.
     */
    void add_error(const Callback& callback);
    
    /// Add callback on cancel
    /**
     * Add callback is not thread safe, for the initialization methods is 
     * almost occurred when the object is being initialized.
     */
    void add_cancel(const Callback& callback);
    
    /// Add calback on all events.
    /**
     * Add callback is not thread safe, for the initialization methods is 
     * almost occurred when the object is being initialized.
     */
    void add_all(const Callback& callback);
    
    /// Set the status to cancelled, and execute callbacks.
    /**
     * If the job has already executed, or is running, then the method 
     * do nothing.
     * 
     * @return Successfully marked cancelled.
     */
    bool cancel();
    
    /// Call the function method.
    /**
     * Only if the AsyncResult's status is WAIT, then the task will be called.
     * And, according to the status, callbacks will be executed.
     */
    void do_task();
    
protected:
    /// The lock type.
    typedef boost::detail::spinlock Lock;
    
    /// The spin lock for status.
    Lock lock_;
    
    /// The status.
    Status status_;
    
    /// The function call.
    Task task_;
    
    /// The success callback flag.
    static const unsigned int CALLBACK_SUCCESS = 0x1;
    
    /// The error callback flag.
    static const unsigned int CALLBACK_ERROR = 0x2;
    
    /// The cancel callback flag.
    static const unsigned int CALLBACK_CANCEL = 0x4;
    
    /// The full callback flag.
    static const unsigned int CALLBACK_ALL = CALLBACK_SUCCESS | CALLBACK_ERROR | CALLBACK_CANCEL;
    
    /// The callback list item.
    typedef std::pair<unsigned int, Callback> CallbackListItem;
    
    /// The callback list type.
    typedef std::list<CallbackListItem> CallbackList;
    
    /// The callback list.
    CallbackList callbacks_;
    
    /// The error object.
    boost::shared_ptr<AvalonException> exception_;
};

/// The job's AsyncResult shared_ptr.
typedef boost::shared_ptr<AsyncResult> AsyncResultPtr;

END_AVALON_NS2

#endif // ASYNCRESULT_H
