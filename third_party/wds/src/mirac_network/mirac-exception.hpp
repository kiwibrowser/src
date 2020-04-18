/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2014 Intel Corporation.
 *
 * Contact: Jussi Laako <jussi.laako@linux.intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */


#ifndef MIRAC_EXCEPTION_HPP
#define MIRAC_EXCEPTION_HPP

#include <cstring>
#include <exception>

class MiracException : public std::exception
{
    public:
        MiracException () throw ()
            { }
        MiracException (int error_code, const char *function = NULL) throw ()
            {
                ec = error_code;
                msg = strerror(ec);
                add_func(function);
            }
        MiracException (const char *error_msg, const char *function = NULL) throw ()
            {
                msg = error_msg;
                add_func(function);
            }
        MiracException (int error_code, const char *error_msg, const char *function = NULL) throw ()
            {
                ec = error_code;
                msg = std::string(error_msg) + std::string(": ") +
                    std::string(strerror(ec));
                add_func(function);
            }
        virtual ~MiracException () throw ()
            { }

        virtual const char * what () const throw ()
            { return msg.c_str(); }
        virtual operator int () const throw ()
            { return ec; }
        virtual operator std::string () const throw ()
            { return msg; }

    protected:
        int ec;
        std::string msg;
        void add_func (const char *function) throw ()
            {
                if (function)
                    msg = std::string(function) + std::string("(): ") + msg;
            }
};


#endif  // MIRAC_EXCEPTION_HPP

