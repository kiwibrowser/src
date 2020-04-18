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


#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <algorithm>
#include <memory>

#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "mirac-network.hpp"


#define MIRAC_MAX_NAMELEN       255


MiracNetwork::MiracNetwork ()
{
    Init();
}


MiracNetwork::MiracNetwork (int conn_handle)
{
    Init();
    handle = conn_handle;
}



MiracNetwork::~MiracNetwork ()
{
    Close();
}


void MiracNetwork::Init ()
{
    long ps;

    handle = -1;

    ps = sysconf(_SC_PAGESIZE);
    page_size = (ps <= 0) ? 4096 : static_cast<size_t> (ps);

    conn_ares = NULL;
}


void MiracNetwork::Close ()
{
    if (handle >= 0)
        close(handle);
    handle = -1;

    if (conn_ares)
    {
        freeaddrinfo(reinterpret_cast<struct addrinfo *> (conn_ares));
        conn_ares = NULL;
    }
}


void MiracNetwork::Bind (const char *address, const char *service)
{
    int ec;
    int reuse = 1;
    struct addrinfo *addr_res = NULL;
    struct addrinfo *bind_addr;
    struct addrinfo addr_hint;

    Close();

    memset(&addr_hint, 0x00, sizeof(addr_hint));
    addr_hint.ai_flags = AI_PASSIVE;
    ec = getaddrinfo(address, service, &addr_hint, &addr_res);
    if (ec)
        throw MiracException(gai_strerror(ec), __FUNCTION__);
    bind_addr = addr_res;
    while (bind_addr)
    {
        /* note, the SOCK_NONBLOCK is specific to Linux 2.6.27+,
         * on other platforms use either fcntl() or ioctl(h, FIONBIO, 1) */
        handle = socket(addr_res->ai_family,
            addr_res->ai_socktype | SOCK_NONBLOCK, addr_res->ai_protocol);
        if (handle < 0)
            throw MiracException(errno, "socket()", __FUNCTION__);
        if (setsockopt(handle, SOL_SOCKET, SO_REUSEADDR,
            &reuse, sizeof(reuse)))
            throw MiracException(errno, "setsockopt()", __FUNCTION__);
	if (bind(handle, bind_addr->ai_addr, bind_addr->ai_addrlen) == 0)
           break;
        else if (!bind_addr->ai_next)
            throw MiracException(errno, "bind()", __FUNCTION__);
        close(handle);
	bind_addr = bind_addr->ai_next;
    }
    freeaddrinfo(addr_res);

    if (listen(handle, 1))
        throw MiracException(errno, "listen()", __FUNCTION__);
}


MiracNetwork * MiracNetwork::Accept ()
{
    int ch;
    int nonblock = 1;

    ch = accept(handle, NULL, NULL);
    if (ch < 0)
        throw MiracException(errno, "accept()", __FUNCTION__);
    if (ioctl(ch, FIONBIO, &nonblock))
        throw MiracException(errno, "ioctl(FIONBIO)", __FUNCTION__);
    return new MiracNetwork(ch);
}


bool MiracNetwork::Connect (const char *address, const char *service)
{
    int ec = 0;

    if (address && service)
    {
        Close();

        struct addrinfo addr_hint;
        memset(&addr_hint, 0x00, sizeof(addr_hint));
        addr_hint.ai_socktype = SOCK_STREAM;
        ec = getaddrinfo(address, service, &addr_hint,
            reinterpret_cast<struct addrinfo **> (&conn_ares));
        if (ec)
            throw MiracException(gai_strerror(ec), __FUNCTION__);
        conn_aptr = conn_ares;
    }
    else
    {
        socklen_t optlen = sizeof(ec);
        if (getsockopt(handle, SOL_SOCKET, SO_ERROR, &ec, &optlen))
            throw MiracException(errno, "getsockopt()", __FUNCTION__);
        if (!ec)
            return true;
        conn_aptr = reinterpret_cast<struct addrinfo *> (conn_aptr)->ai_next;
    }

    struct addrinfo *addr = reinterpret_cast<struct addrinfo *> (conn_aptr);
    if (!addr)
        throw MiracException("peer unavailable", __FUNCTION__);
    /* note, the SOCK_NONBLOCK is specific to Linux 2.6.27+,
     * on other platforms use either fcntl() or ioctl(h, FIONBIO, 1) */
    handle = socket(addr->ai_family,
        addr->ai_socktype | SOCK_NONBLOCK, addr->ai_protocol);
    if (handle < 0)
        throw MiracException(errno, "socket()", __FUNCTION__);
    if (connect(handle, addr->ai_addr, addr->ai_addrlen))
    {
        if (errno == EINPROGRESS)
            return false;
        throw MiracException(errno, "connect()", __FUNCTION__);
    }
    return true;
}


std::string MiracNetwork::GetPeerAddress ()
{
    int ec;
    socklen_t addrsize = std::max(sizeof(sockaddr_in), sizeof(sockaddr_in6));
    std::unique_ptr<uint8_t []> addrbuf(new uint8_t[addrsize]);
    std::unique_ptr<char []> namebuf(new char [MIRAC_MAX_NAMELEN + 1]);
    std::unique_ptr<char []> servbuf(new char [MIRAC_MAX_NAMELEN + 1]);

    if (getpeername(handle,
        reinterpret_cast<struct sockaddr *> (addrbuf.get()), &addrsize))
        throw MiracException(errno, "getpeername()", __FUNCTION__);
    ec = getnameinfo(
        reinterpret_cast<struct sockaddr *> (addrbuf.get()), addrsize,
        namebuf.get(), MIRAC_MAX_NAMELEN,
        servbuf.get(), MIRAC_MAX_NAMELEN,
        NI_NOFQDN|NI_NUMERICHOST|NI_NUMERICSERV);
    if (ec)
        throw MiracException(gai_strerror(ec), __FUNCTION__);
    return std::string(namebuf.get());
}


unsigned short MiracNetwork::GetHostPort ()
{
    socklen_t addrsize = std::max(sizeof(sockaddr_in), sizeof(sockaddr_in6));
    std::unique_ptr<uint8_t []> addrbuf(new uint8_t[addrsize]);

    if (getsockname(handle,
        reinterpret_cast<struct sockaddr *> (addrbuf.get()), &addrsize))
        throw MiracException(errno, "getsockname()", __FUNCTION__);

    struct sockaddr *saddr =
        reinterpret_cast<struct sockaddr *> (addrbuf.get());
    if (saddr->sa_family == AF_INET)
    {
        struct sockaddr_in *ip4addr =
            reinterpret_cast<struct sockaddr_in *> (saddr);
        return ntohs(ip4addr->sin_port);
    }
    else if (saddr->sa_family == AF_INET6)
    {
        struct sockaddr_in6 *ip6addr =
            reinterpret_cast<struct sockaddr_in6 *> (saddr);
        return ntohs(ip6addr->sin6_port);
    }
    return 0;
}


bool MiracNetwork::Receive (std::string &message)
{
    int ec;
    char nb[page_size];

    do {
        ec = recv(handle, nb, page_size, 0);
        if (ec > 0)
            message.append(nb, ec);
        else if (ec < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            if (errno == ECONNRESET)
                throw MiracConnectionLostException( __FUNCTION__);
            throw MiracException(errno, "recv()", __FUNCTION__);
        }
        else  // ec == 0
            throw MiracConnectionLostException( __FUNCTION__);
    } while (ec > 0);

    return true;
}


bool MiracNetwork::Receive (std::string &message, size_t length)
{
    int ec;
    char nb[page_size];

    do {
        ec = recv(handle, nb, page_size, 0);
        if (ec > 0)
            recv_buf.append(nb, ec);
        else if (ec < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            if (errno == ECONNRESET)
                throw MiracConnectionLostException( __FUNCTION__);
            throw MiracException(errno, "recv()", __FUNCTION__);
        }
        else  // ec == 0
            throw MiracConnectionLostException( __FUNCTION__);
    } while (ec > 0);

    if (recv_buf.size() < length)
        return false;

    message = recv_buf.substr(0, length);
    recv_buf.erase(0, length);
    return true;
}


bool MiracNetwork::Send (const std::string &message)
{
    int ec;

    if (!message.empty())
        send_buf.append(message);
    do {
        ec = send(handle, send_buf.c_str(), send_buf.size(), MSG_NOSIGNAL);
        if (ec > 0)
            send_buf.erase(0, ec);
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return false;
            if (errno == EPIPE || errno == ENOTCONN)
                throw MiracConnectionLostException(__FUNCTION__);
            throw MiracException(errno, "send()", __FUNCTION__);
        }
    } while (send_buf.size() > 0);

    return true;
}

