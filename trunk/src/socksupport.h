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

#ifndef SOCKSUPPORT_H
#define SOCKSUPPORT_H

#define SS_READ_FLG		1
#define SS_WRITE_FLG	2

#ifdef  __cplusplus
extern "C" {
#endif

char *ss_getsockerr(void);
char *ss_getsockerr_n( int err );
int ss_init_winsock(void);

#ifdef  __cplusplus
};
#endif

#endif