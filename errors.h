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


#ifndef ERRORS_H
#define ERRORS_H

#include "define.h"

#include <string>
#include <boost/exception/all.hpp>

BEGIN_AVALON_NS

/// The exception class name.
typedef boost::error_info<struct tag_error_class, std::string> error_class;

/// The error number.
typedef boost::error_info<struct tag_error_number, int> error_number;

/// The error message.
typedef boost::error_info<struct tag_error_message, std::string> error_message;

/// The argument name.
typedef boost::error_info<struct tag_error_argument, std::string> error_argument;

/// The base class for all Avalon exceptions.
/**
 * Any class extending this should not add custom fields, 
 * for the exception sometimes need to be copied. (e.g. AsyncResult).
 * 
 * However, you can use boost::error_info to provide your custom fields.
 */
class AvalonException : public boost::exception, public std::exception {};

/// The operation is not permitted.
class AvalonOperationForbid : public AvalonException {};

/// The argument is invalid.
class AvalonInvalidArgument : public AvalonException {};

/// The workpool is full.
class AvalonWorkPoolFull : public AvalonException {};

/// The workpool cannot perform such method.
class AvalonWorkPoolMethodNotSupport : public AvalonException {};

/// Create an instance of exception class and set the class name.
#define AVALON_THROW(class) BOOST_THROW_EXCEPTION(class() << ::avalon::error_class(#class))

/// Create an instance of exception class, insert error info, and set class name.
/**
 * Usage: AVALON_THROW_INFO(className, error_info1 [ << error_info2 [ ... ] ])
 */
#define AVALON_THROW_INFO(class, error_info) \
    BOOST_THROW_EXCEPTION(class() << ::avalon::error_class(#class) << error_info)

END_AVALON_NS

#endif // ERRORS_H
