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
//= Creation: Davide Pasca 1999
//=
//=
//=
//=
//==================================================================

#include "drtysock.h"
#include "socksupport.h"

//==================================================================
static void (*_read_callback)(int);

//==================================================================
void ss_set_read_callback( void (*readcb)(int) )
{
	_read_callback = readcb;
}

//==================================================================
char *ss_getsockerr(void)
{
	return ss_getsockerr_n( LAST_SOCK_ERR );
}

//==================================================================
char *ss_getsockerr_n( int err )
{
	switch ( err )
	{
	case NOTINITIALISED:	return "NOTINITIALISED:	A successful WSAStartup must occur before using this API.";
	case ENETDOWN:			return "ENETDOWN:	The network subsystem has failed.";
	case HOST_NOT_FOUND:	return "HOST_NOT_FOUND:	Authoritative Answer Host not found.";
	case TRY_AGAIN:			return "TRY_AGAIN:	Non-Authoritative Host not found, or server failed.";
	case NO_RECOVERY:		return "NO_RECOVERY:	Non-recoverable error occurred.";
	case NO_DATA:			return "NO_DATA:	Valid name, no data record of requested type.";
	case EINPROGRESS:		return "EINPROGRESS:	A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
	case EAFNOSUPPORT:		return "EAFNOSUPPORT:	The type specified is not supported by the Windows Sockets implementation.";
	case EFAULT:			return "EFAULT:	The addr argument is not a valid part of the user address space, or the len argument is too small.";
	case EINTR:				return "EINTR:	The (blocking) call was canceled via WSACancelBlockingCall";

	case EINVAL:	return "EINVAL: The socket has not been bound with bind.";
	case EMFILE:	return "EMFILE: No more socket descriptors are available.";
	case ENOBUFS:
		return "ENOBUFS: No buffer space is available.";
	case ENOTSOCK:	return "ENOTSOCK: The descriptor is not a socket.";
	case EOPNOTSUPP:	return "EOPNOTSUPP: The referenced socket is not of a type that supports the listen operation.";

	case EWOULDBLOCK      : return "EWOULDBLOCK";
	case EALREADY         : return "EALREADY";
	case EDESTADDRREQ     : return "EDESTADDRREQ";
	case EMSGSIZE         : return "EMSGSIZE";
	case EPROTOTYPE       : return "EPROTOTYPE";
	case ENOPROTOOPT      : return "ENOPROTOOPT";
	case EPROTONOSUPPORT  : return "EPROTONOSUPPORT";
	case ESOCKTNOSUPPORT  : return "ESOCKTNOSUPPORT";
	case EPFNOSUPPORT     : return "EPFNOSUPPORT";
	case EADDRINUSE       : return "EADDRINUSE";
	case EADDRNOTAVAIL    : return "EADDRNOTAVAIL";
	case ENETUNREACH      : return "ENETUNREACH";
	case ENETRESET        : return "ENETRESET";
	case ECONNABORTED     : return "ECONNABORTED";
	case ECONNRESET       : return "ECONNRESET";
	case EISCONN          : return "EISCONN";
	case ENOTCONN         : return "ENOTCONN";
	case ESHUTDOWN        : return "ESHUTDOWN";
	case ETOOMANYREFS     : return "ETOOMANYREFS";
	case ETIMEDOUT        : return "ETIMEDOUT";
	case ECONNREFUSED     : return "ECONNREFUSED";
	case ELOOP            : return "ELOOP";
	case ENAMETOOLONG     : return "ENAMETOOLONG";
	case EHOSTDOWN        : return "EHOSTDOWN";
	case EHOSTUNREACH     : return "EHOSTUNREACH";
	case ENOTEMPTY        : return "ENOTEMPTY";
	case EPROCLIM         : return "EPROCLIM";
	case EUSERS           : return "EUSERS";
	case EDQUOT           : return "EDQUOT";
	case ESTALE           : return "ESTALE";
	case EREMOTE          : return "EREMOTE";
	}

	return "";//"Unknown Error";
}

//==================================================================
static void ss_quit(void)
{
	WSACleanup();
}

//==================================================================
int ss_init_winsock(void)
{
static int	done_that_been_there;
static int	and_was_good;
WORD		wVersionRequested;
WSADATA		wsaData;

	if ( done_that_been_there )
		return and_was_good;

	wVersionRequested = MAKEWORD( 1, 1 );
	if ( WSAStartup( wVersionRequested, &wsaData ) )
	{
		MessageBox( NULL, "Error", "Need winsock.dll 1.1 or greater !!", MB_OK | MB_ICONERROR );
		exit(-1);
		and_was_good = 0;
	}
	else
	{
		and_was_good = 1;
		atexit( ss_quit );
	}

	done_that_been_there = 1;
	return and_was_good;
}
