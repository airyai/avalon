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

#ifndef THREAD_ASYNCRESULT_TPL_H
#define THREAD_ASYNCRESULT_TPL_H

#include "asyncresult.h"

BEGIN_AVALON_NS2(thread)

template <typename T>
AsyncResult::Result<T>::Result(const AsyncResult::Result<T>& other)
 :  result_(other.result_)
{
}

template <typename T>
AsyncResult::Result<T>& AsyncResult::Result<T>::operator=(const AsyncResult::Result<T>& other)
{
    result_ = other._result;
}

template <typename T>
AsyncResult::Result<T>::Result(T* data)
 :  result_(data)
{
}

template <typename T>
AsyncResult::Result<T>::Result(const AsyncResult::Result<T>::DataPtr& data)
 :  result_(data)
{
}

template <typename T>
AsyncResult::Result<T>::~Result()
{
}

template <typename T>
const typename AsyncResult::Result<T>::DataPtr &AsyncResult::Result<T>::data() const
{
    return result_;
}

template <typename T>
typename AsyncResult::Result<T>::DataPtr AsyncResult::get_result()
{
    Lock::scoped_lock locker(lock_);
    Result<T>* res = (Result<T>*)result_.get();
    if (!res) 
        return typename AsyncResult::Result<T>::DataPtr();
    return res->data();
}

template <typename T>
void AsyncResult::set_result(T* data)
{
    Lock::scoped_lock locker(lock_);
    result_.reset(new Result<T>(data));
}

template <typename T>
void AsyncResult::set_result(const typename Result<T>::DataPtr &data)
{
    Lock::scoped_lock locker(lock_);
    result_.reset(new Result<T>(data));
}

END_AVALON_NS2

#endif // THREAD_ASYNCRESULT_TPL_H
