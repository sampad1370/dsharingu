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
//==
//==
//==
//==
//==================================================================

//#define WIN32_LEAN_AND_MEAN
#include "drtysock.h"
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <gl/glew.h>

#include "psys.h"
#include "appbase3.h"
#include "socksupport.h"
#include "dsharingu_protocol.h"
#include "dsinstance.h"

//==================================================================
static bool				_do_quit_flag;
static DSChannel		*_basechannel;
static DSChannel		*_extrachannel;

#ifdef _DEBUG
#define USE_EXTRA_CHANNEL
#endif
//#define USE_EXTRA_CHANNEL

//==================================================================
bool main_start()
{
	try {
	// set float 2 int conversion to chop off !
	_control87( _RC_CHOP, _MCW_RC );
	ss_init_winsock();

	_basechannel = new DSChannel( "dsharingu.cfg" );
	_basechannel->Create( true );
#ifdef USE_EXTRA_CHANNEL
	_extrachannel = new DSChannel( "dsharingu2.cfg" );
	_extrachannel->Create( false );
#endif

//	FONT_Create();
	}
	catch (...)
	{
		return false;
	}
	
	return true;
}

//==================================================================
static void handleChannel( DSChannel *chanp )
{
	switch ( chanp->Idle() )
	{
	case DSChannel::STATE_IDLE:
		break;
	case DSChannel::STATE_CONNECTED:
		break;
	case DSChannel::STATE_DISCONNECTED:
		break;
	case DSChannel::STATE_QUIT:
		_do_quit_flag = 1;
		break;
	}
}

//==================================================================
void main_anim(void)
{
	handleChannel( _basechannel );

#ifdef USE_EXTRA_CHANNEL
	handleChannel( _extrachannel );
#endif

	Sleep( 20 );
}

//==================================================================
int main_requestquit(void)
{
	return _do_quit_flag;
}

//==================================================================
void main_quit(void)
{
}

