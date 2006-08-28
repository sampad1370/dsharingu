//==================================================================
//	Copyright (C) 2006  Davide Pasca
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//==================================================================
//= Creation: Davide Pasca
//= Modify:	 
//= Copyright (c) Davide Pasca 1999
//=========================================================

#ifndef _DRTYSOCK_H_
#define _DRTYSOCK_H_

#ifdef _WINDOWS
#include <winsock2.h>

#define NOTINITIALISED		WSANOTINITIALISED	
#define ENETDOWN			WSAENETDOWN	
#define HOST_NOT_FOUND		WSAHOST_NOT_FOUND	
#define TRY_AGAIN			WSATRY_AGAIN	
#define NO_RECOVERY			WSANO_RECOVERY	
#define NO_DATA				WSANO_DATA	

#define EINPROGRESS			WSAEINPROGRESS	
#define EAFNOSUPPORT		WSAEAFNOSUPPORT	
#define EFAULT				WSAEFAULT	
#define EINTR				WSAEINTR

#define EINVAL	WSAEINVAL
#define EMFILE	WSAEMFILE
#define ENOBUFS			WSAENOBUFS
#define ENOTSOCK		WSAENOTSOCK
#define EOPNOTSUPP		WSAEOPNOTSUPP

#define EWOULDBLOCK             WSAEWOULDBLOCK
#define EALREADY                WSAEALREADY
#define ENOTSOCK                WSAENOTSOCK
#define EDESTADDRREQ            WSAEDESTADDRREQ
#define EMSGSIZE                WSAEMSGSIZE
#define EPROTOTYPE              WSAEPROTOTYPE
#define ENOPROTOOPT             WSAENOPROTOOPT
#define EPROTONOSUPPORT         WSAEPROTONOSUPPORT
#define ESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
#define EOPNOTSUPP              WSAEOPNOTSUPP
#define EPFNOSUPPORT            WSAEPFNOSUPPORT
#define EADDRINUSE              WSAEADDRINUSE
#define EADDRNOTAVAIL           WSAEADDRNOTAVAIL
#define ENETUNREACH             WSAENETUNREACH
#define ENETRESET               WSAENETRESET
#define ECONNABORTED            WSAECONNABORTED
#define ECONNRESET              WSAECONNRESET
#define ENOBUFS                 WSAENOBUFS
#define EISCONN                 WSAEISCONN
#define ENOTCONN                WSAENOTCONN
#define ESHUTDOWN               WSAESHUTDOWN
#define ETOOMANYREFS            WSAETOOMANYREFS
#define ETIMEDOUT               WSAETIMEDOUT
#define ECONNREFUSED            WSAECONNREFUSED
#define ELOOP                   WSAELOOP
#define ENAMETOOLONG            WSAENAMETOOLONG
#define EHOSTDOWN               WSAEHOSTDOWN
#define EHOSTUNREACH            WSAEHOSTUNREACH
#define ENOTEMPTY               WSAENOTEMPTY
#define EPROCLIM                WSAEPROCLIM
#define EUSERS                  WSAEUSERS
#define EDQUOT                  WSAEDQUOT
#define ESTALE                  WSAESTALE
#define EREMOTE                 WSAEREMOTE

#define	LAST_SOCK_ERR		WSAGetLastError()
#define ioctl				ioctlsocket

#else	// _WINDOWS

#define	LAST_SOCK_ERR		errno

#endif	// _WINDOWS

#endif
