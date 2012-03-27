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
#include <boost/thread/condition.hpp>
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
    typedef boost::function<void (AsyncResult&)> Task;
    
    /// The callback type.
    typedef boost::function<void (AsyncResult&)> Callback;
    
    /// The AsyncResult status type.
    enum Status
    {
        /// The task is waiting for execution.
        WAIT = 0,
        
        /// The task is being executed.
        RUNNING = 1,
        
        /// The task has succesfully executed.
        SUCCESS = 2,
        
        /// There's some error during execution.
        ERROR = 3,
        
        /// The task has been cancelled.
        CANCELLED = 4,
        
        /// The thread in which the task is executed has been interrupted.
        INTERRUPTED = 5
    };
    
    /// The base class for Result.
    /**
     * Provide a result wrapper, to keep the result object alive.
     */
    class ResultBase {
    public:
        /// Create a new result.
        ResultBase();
        
        /// Dispose the result.
        virtual ~ResultBase();
    };
    
    /// Type specified Result.
    template <typename T>
    class Result : public ResultBase {
    public:
        /// The data pointer type.
        typedef boost::shared_ptr<T> DataPtr;
        
        /// Create a Result object and manages the data object.
        /**
         * After all Result instance that share the data object, it 
         * will be disposed.
         */
        Result(T* data);
        
        /// Create a Result object that shares the data object.
        Result(const DataPtr& data);
        
        /// Copy construct a Result object.
        Result(const Result<T>& other);
        
        /// Copy a Result object.
        Result<T>& operator=(const Result<T>& other);
        
        /// Dispose the Result object.
        virtual ~Result();
        
        /// Get the result data.
        const DataPtr &data() const;
        
    protected:
        DataPtr result_;
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
    
    /// The result exception.
    const AvalonException* exception();
    
    /// Get the result data.
    template <typename T>
    typename Result<T>::DataPtr get_result();
    
    /// Create a result object that manages the T* data.
    /**
     * The content of the pointer is not copied, but managed 
     * by shared_ptr. So just create a pointer and pass it to 
     * this method.
     */
    template <typename T>
    void set_result(T* data);
    
    /// Create a result object that shares the T* data.
    template <typename T>
    void set_result(const typename Result<T>::DataPtr &data);
    
    /// Clear the result object.
    void clear_result();
    
    /// Set the status to cancelled, and execute callbacks.
    /**
     * If the job has already executed, or is running, then the method 
     * do nothing.
     * 
     * @return true if did cancel, otherwise false.
     */
    bool cancel();
    
    /// Add callback on success.
    /**
     * Add the callback to callback list if current status is 
     * WAIT or RUNNING, otherwise execute the callback immediately 
     * if the status is SUCCESS.
     */
    void add_success(const Callback& callback);
    
    /// Add callback on error.
    /**
     * Add the callback to callback list if current status is 
     * WAIT or RUNNING, otherwise execute the callback immediately 
     * if the status if ERROR.
     */
    void add_error(const Callback& callback);
    
    /// Add callback on cancel.
    /**
     * Add the callback to callback list if current status is 
     * WAIT or RUNNING, otherwise execute the callback immediately 
     * if the status is CANCELLED.
     */
    void add_cancel(const Callback& callback);
    
    /// Add callback on interrupt.
    /**
     * Add the callback to callback list if current status is 
     * WAIT or RUNNING, otherwise execute the callback immediately 
     * if the status is CANCELLED.
     */
    void add_interrupt(const Callback& callback);
    
    /// Add calback on all events.
    /**
     * Add the callback to callback list if current status is 
     * WAIT or RUNNING, otherwise execute the callback immediately.
     */
    void add_all(const Callback& callback);
    
    /// Call the function method.
    /**
     * Only if the AsyncResult's status is WAIT, then the task will be called.
     * And, according to the status, callbacks will be executed.
     * 
     * @return True if really executed, otherwise false.
     */
    bool execute();
    
    /// Wait for the job to be finished.
    /**
     * Wait only if the AsyncResult's status is RUNNING or WAIT. When timeout 
     * specified, the method will return if time exceeds.
     * 
     * @param timeout The maximum waiting milliseconds. Zero means no limit.
     * @return false if time exceeds, otherwise true (including no waiting).
     */
    bool wait(size_t timeout = 0);
    
protected:
    /// The lock type.
    /**
     * At first I attempted to use spinlock, just as shared_ptr or some 
     * other objects. But later I realized, that AsyncResult may be used 
     * in many * many threads.
     * 
     * According to a brief test,
     * http://www.searchtb.com/2011/01/pthreads-mutex-vs-pthread-spinlock.html,
     * the efficiency decreased dramatically even if there are only about 10 
     * threads. 
     * 
     * Even in single-thread environment, the speed of mutex is just ok for 
     * server applications. In test/speed_workpool.cpp, 10,000,000 mutex lock 
     * uses 0.6s on my laptop.
     * 
     * Another reason is, mutex supports condition_variable.
     * 
     * So I chose mutex later.
     */
    typedef boost::mutex Lock;
    
    /// The spin lock for status.
    Lock lock_;
    
    /// The condition variable to notify that the job has been done.
    boost::condition_variable cond_;
    
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
    
    /// The interrupt callback flag.
    static const unsigned int CALLBACK_INTERRUPT = 0x8;
    
    /// The full callback flag.
    static const unsigned int CALLBACK_ALL = CALLBACK_SUCCESS | CALLBACK_ERROR
                                                | CALLBACK_CANCEL | CALLBACK_INTERRUPT;
    
    /// The callback list item.
    typedef std::pair<unsigned int, Callback> CallbackListItem;
    
    /// The callback list type.
    typedef std::list<CallbackListItem> CallbackList;
    
    /// The callback list.
    CallbackList callbacks_;
    
    /// The error object.
    boost::shared_ptr<AvalonException> exception_;
    
    /// The result object.
    boost::shared_ptr<ResultBase> result_;
    
    /// Set the status as success.
    void set_success();
    
    /// Set the status to cancelled, and execute callbacks.
    bool set_cancel();
    
    /// Set the error as an unknown exception.
    void set_error();
    
    /// Set the error as an AvalonException.
    void set_error(const AvalonException* err);
    
    /// Set the status to interrupted, and execute callbacks.
    void set_interrupt();
    
    /// Add calback on all events.
    /**
     * Add the callback to callback list if current status is 
     * WAIT or RUNNING, otherwise execute the callback immediately.
     */
    void add_callback(const Callback& callback, unsigned int flag);
    
    /// Execute the callbacks match flag modifier.
    void call_callback(unsigned int flag);
};

/// The job's AsyncResult shared_ptr.
typedef boost::shared_ptr<AsyncResult> AsyncResultPtr;

END_AVALON_NS2

#include "asyncresult.tpl.h"

#endif // ASYNCRESULT_H
