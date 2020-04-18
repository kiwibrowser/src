/*-
 * Copyright (c) 1980, 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * -
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * -
 * --Copyright--
 */

/*
 *      @(#)netdb.h	8.1 (Berkeley) 6/2/93
 *      From: Id: netdb.h,v 8.9 1996/11/19 08:39:29 vixie Exp $
 * $FreeBSD: /repoman/r/ncvs/src/include/netdb.h,v 1.41 2006/04/15 16:20:26 ume Exp $
 */

#ifndef _NETDB_H_
#define _NETDB_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifndef _PATH_HEQUIV
# define	_PATH_HEQUIV	"/system/etc/hosts.equiv"
#endif
#define	_PATH_HOSTS	"/system/etc/hosts"
#define	_PATH_NETWORKS	"/system/etc/networks"
#define	_PATH_PROTOCOLS	"/system/etc/protocols"
#define	_PATH_SERVICES	"/system/etc/services"

/*
 * Structures returned by network data base library.  All addresses are
 * supplied in host order, and returned in network order (suitable for
 * use in system calls).
 */
struct hostent {
	char	*h_name;	/* official name of host */
	char	**h_aliases;	/* alias list */
	int	h_addrtype;	/* host address type */
	int	h_length;	/* length of address */
	char	**h_addr_list;	/* list of addresses from name server */
#define	h_addr	h_addr_list[0]	/* address, for backward compatibility */
};

struct netent {
	char		*n_name;	/* official name of net */
	char		**n_aliases;	/* alias list */
	int		n_addrtype;	/* net address type */
	uint32_t	n_net;		/* network # */
};

struct servent {
	char	*s_name;	/* official service name */
	char	**s_aliases;	/* alias list */
	int	s_port;		/* port # */
	char	*s_proto;	/* protocol to use */
};

struct protoent {
	char	*p_name;	/* official protocol name */
	char	**p_aliases;	/* alias list */
	int	p_proto;	/* protocol # */
};

struct addrinfo {
	int	ai_flags;	/* AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST */
	int	ai_family;	/* PF_xxx */
	int	ai_socktype;	/* SOCK_xxx */
	int	ai_protocol;	/* 0 or IPPROTO_xxx for IPv4 and IPv6 */
	socklen_t ai_addrlen;	/* length of ai_addr */
	char	*ai_canonname;	/* canonical name for hostname */
	struct	sockaddr *ai_addr;	/* binary address */
	struct	addrinfo *ai_next;	/* next structure in linked list */
};

/*
 * Error return codes from gethostbyname() and gethostbyaddr()
 * (left in h_errno).
 */

#define	NETDB_INTERNAL	-1	/* see errno */
#define	NETDB_SUCCESS	0	/* no problem */
#define	HOST_NOT_FOUND	1 /* Authoritative Answer Host not found */
#define	TRY_AGAIN	2 /* Non-Authoritative Host not found, or SERVERFAIL */
#define	NO_RECOVERY	3 /* Non recoverable errors, FORMERR, REFUSED, NOTIMP */
#define	NO_DATA		4 /* Valid name, no data record of requested type */
#define	NO_ADDRESS	NO_DATA		/* no address, look for MX record */

/*
 * Error return codes from getaddrinfo()
 */
#define	EAI_ADDRFAMILY	 1	/* address family for hostname not supported */
#define	EAI_AGAIN	 2	/* temporary failure in name resolution */
#define	EAI_BADFLAGS	 3	/* invalid value for ai_flags */
#define	EAI_FAIL	 4	/* non-recoverable failure in name resolution */
#define	EAI_FAMILY	 5	/* ai_family not supported */
#define	EAI_MEMORY	 6	/* memory allocation failure */
#define	EAI_NODATA	 7	/* no address associated with hostname */
#define	EAI_NONAME	 8	/* hostname nor servname provided, or not known */
#define	EAI_SERVICE	 9	/* servname not supported for ai_socktype */
#define	EAI_SOCKTYPE	10	/* ai_socktype not supported */
#define	EAI_SYSTEM	11	/* system error returned in errno */
#define	EAI_BADHINTS	12	/* invalid value for hints */
#define	EAI_PROTOCOL	13	/* resolved protocol is unknown */
#define	EAI_OVERFLOW	14	/* argument buffer overflow */
#define	EAI_MAX		15

/*
 * Flag values for getaddrinfo()
 */
#define	AI_PASSIVE	0x00000001 /* get address to use bind() */
#define	AI_CANONNAME	0x00000002 /* fill ai_canonname */
#define	AI_NUMERICHOST	0x00000004 /* prevent host name resolution */
#define	AI_NUMERICSERV	0x00000008 /* prevent service name resolution */
/* valid flags for addrinfo (not a standard def, apps should not use it) */
#define AI_MASK \
    (AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST | AI_NUMERICSERV | \
    AI_ADDRCONFIG)

#define	AI_ALL		0x00000100 /* IPv6 and IPv4-mapped (with AI_V4MAPPED) */
#define	AI_V4MAPPED_CFG	0x00000200 /* accept IPv4-mapped if kernel supports */
#define	AI_ADDRCONFIG	0x00000400 /* only if any address is assigned */
#define	AI_V4MAPPED	0x00000800 /* accept IPv4-mapped IPv6 address */
/* special recommended flags for getipnodebyname */
#define	AI_DEFAULT	(AI_V4MAPPED_CFG | AI_ADDRCONFIG)

/*
 * Constants for getnameinfo()
 */
#define	NI_MAXHOST	1025
#define	NI_MAXSERV	32

/*
 * Flag values for getnameinfo()
 */
#define	NI_NOFQDN	0x00000001
#define	NI_NUMERICHOST	0x00000002
#define	NI_NAMEREQD	0x00000004
#define	NI_NUMERICSERV	0x00000008
#define	NI_DGRAM	0x00000010
#if 0 /* obsolete */
#define NI_WITHSCOPEID	0x00000020
#endif

/*
 * Scope delimit character
 */
#define	SCOPE_DELIMITER	'%'

#define IPPORT_RESERVED 1024

__BEGIN_DECLS

int getaddrinfo(const char* __node, const char* __service, const struct addrinfo* __hints, struct addrinfo** __result);
void freeaddrinfo(struct addrinfo* __ptr);

/* Android ABI error: POSIX getnameinfo(3) uses socklen_t rather than size_t. */
int getnameinfo(const struct sockaddr* __sa, socklen_t __sa_length, char* __host, size_t __host_length, char* __service, size_t __service_length, int __flags);
const char* gai_strerror(int __error);

/* These functions are obsolete. Use getaddrinfo/getnameinfo instead. */
#define h_errno (*__get_h_errno())
int* __get_h_errno(void);
void herror(const char* __s);
const char* hstrerror(int __error);
struct hostent* gethostbyaddr(const void* __addr, socklen_t __length, int __type);

#if __ANDROID_API__ >= 23
int gethostbyaddr_r(const void* __addr, socklen_t __length, int __type, struct hostent* __ret, char* __buf, size_t __buf_size, struct hostent** __result, int* __h_errno_ptr) __INTRODUCED_IN(23);
#endif /* __ANDROID_API__ >= 23 */

struct hostent* gethostbyname(const char* __name);
int gethostbyname_r(const char* __name, struct hostent* __ret, char* __buf, size_t __buf_size, struct hostent** __result, int* __h_errno_ptr);
struct hostent* gethostbyname2(const char* __name, int __af);

#if __ANDROID_API__ >= 23
int gethostbyname2_r(const char* __name, int __af, struct hostent* __ret, char* __buf, size_t __buf_size, struct hostent** __result, int* __h_errno_ptr) __INTRODUCED_IN(23);
#endif /* __ANDROID_API__ >= 23 */


#if __ANDROID_API__ >= __ANDROID_API_FUTURE__
void endhostent(void) __INTRODUCED_IN_FUTURE;
#endif /* __ANDROID_API__ >= __ANDROID_API_FUTURE__ */

struct hostent* gethostent(void);

#if __ANDROID_API__ >= __ANDROID_API_FUTURE__
void sethostent(int __stay_open) __INTRODUCED_IN_FUTURE;

/* These functions are obsolete. None of these functions return anything but nullptr. */
void endnetent(void) __INTRODUCED_IN_FUTURE;
#endif /* __ANDROID_API__ >= __ANDROID_API_FUTURE__ */

struct netent* getnetbyaddr(uint32_t __net, int __type);
struct netent* getnetbyname(const char* __name);

#if __ANDROID_API__ >= __ANDROID_API_FUTURE__
struct netent* getnetent(void) __INTRODUCED_IN_FUTURE;
void setnetent(int __stay_open) __INTRODUCED_IN_FUTURE;

/* None of these functions return anything but nullptr. */
void endprotoent(void) __INTRODUCED_IN_FUTURE;
#endif /* __ANDROID_API__ >= __ANDROID_API_FUTURE__ */

struct protoent* getprotobyname(const char* __name);
struct protoent* getprotobynumber(int __proto);

#if __ANDROID_API__ >= __ANDROID_API_FUTURE__
struct protoent* getprotoent(void) __INTRODUCED_IN_FUTURE;
void setprotoent(int __stay_open) __INTRODUCED_IN_FUTURE;
#endif /* __ANDROID_API__ >= __ANDROID_API_FUTURE__ */


/* These functions return entries from a built-in database. */
void endservent(void);
struct servent* getservbyname(const char* __name, const char* __proto);
struct servent* getservbyport(int __port_in_network_order, const char* __proto);
struct servent* getservent(void);
void setservent(int __stay_open);

__END_DECLS

#endif
