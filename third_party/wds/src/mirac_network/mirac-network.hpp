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


#ifndef MIRAC_NETWORK_HPP
#define MIRAC_NETWORK_HPP

#include <cstring>
#include <string>

#include "mirac-exception.hpp"


class MiracConnectionLostException : public MiracException
{
    public:
        MiracConnectionLostException (const char *function = NULL) throw ()
            {
                msg = std::string("Connection lost");
                add_func(function);
            }
        virtual ~MiracConnectionLostException () throw ()
            { }
};



class MiracNetwork
{
    public:
        MiracNetwork ();
        MiracNetwork (int conn_handle);
        virtual ~MiracNetwork ();
        void Bind (const char *address, const char *service);
        MiracNetwork * Accept ();
        bool Connect (const char *address, const char *service);
        int GetHandle () const
            { return handle; }
        std::string GetPeerAddress ();
        unsigned short GetHostPort ();
        bool Receive (std::string &message);
        bool Receive (std::string &message, size_t length);
        bool Send (const std::string &message = std::string());

    protected:
        int handle;
        size_t page_size;
        std::string recv_buf;
        std::string send_buf;

        void Init ();
        void Close ();

    private:
        void *conn_ares;
        void *conn_aptr;
};


#endif  /* MIRAC_NETWORK_HPP */

