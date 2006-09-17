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
///
///
///
///
//==================================================================

#include <windows.h>
#include <stdio.h>
#include "SHA1.h"
#include "wingui_utils.h"

//==================================================================
void DlgEnableItem( HWND hwnd, int id, BOOL onoff )
{
	HWND iw = GetDlgItem( hwnd, id );
	EnableWindow( iw, onoff );
}

//==================================================================
void SetDlgItemInt( HWND hwnd, int id, int val )
{
	char	buff[64];
	sprintf( buff, "%i", val );
	SetDlgItemText( hwnd, id, buff );
}

//==================================================================
int GetDlgItemInt( HWND hwnd, int id )
{
	char	buff[64];
	GetDlgItemText( hwnd, id, buff, sizeof(buff)-1 );
	return atoi( buff );
}

//==================================================================
bool IsDlgButtonON( HWND hwnd, u_int item_id )
{
	return IsDlgButtonChecked( hwnd, item_id ) ? true : false;
}

//===============================================================
bool GetDlgItemSHA1PW( HWND hDlg, int nIDDlgItem, sha1_t *sha1hashp )
{
	char	tmp[64];
	GetDlgItemText( hDlg, nIDDlgItem, tmp, sizeof(tmp)-1 );

	CSHA1	sha1;

	sha1.Reset();
	sha1.Update( (UINT_8 *)tmp, strlen(tmp) );
	sha1.Final();
	sha1.GetHash( sha1hashp->_data );

	return true;
}

//==================================================================
void SetDlgEditForReview( HWND hwnd, u_int item_id )
{
	SetFocus( GetDlgItem( hwnd, item_id ) );
	SendDlgItemMessage( hwnd, item_id, EM_SETSEL, 0, -1 );
}


//==================================================================
//==================================================================
static const char _keep_pass_string[] = "\01\02\03\04\05\06\07";

//==================================================================
bool IsDlgEditPasswordChanged( HWND hwnd, u_int item_id )
{
	char	rem_buff[128];

	GetDlgItemText( hwnd, item_id, rem_buff, sizeof(rem_buff) );

	// if the password has any character between 0 and 7, then it's unchanged
	// because the user couldn't possibly have filled in those values while we
	// did it at setup time
	for (int i=0; rem_buff[i] != 0; ++i)
		if ( rem_buff[i] < 8 )
		{
			return false;
		}

	return true;
}

//==================================================================
void SetDlgItemUnchangedPassword( HWND hwnd, u_int item_id )
{
	SetDlgItemText( hwnd, item_id, _keep_pass_string );
}

//===============================================================
WGUTCheckPWMsg GetDlgEditPasswordState( HWND hwnd, u_int item_id, const char *prompt_titlep )
{
	if NOT( IsDlgEditPasswordChanged( hwnd, item_id ) )
		return CHECKPW_MSG_UNCHANGED;

	char	rem_buff[128];
	GetDlgItemText( hwnd, item_id, rem_buff, sizeof(rem_buff) );

	int len = strlen(rem_buff);


	if ( len == 0 )
	{
		if ( prompt_titlep )
		{
			MessageBox( hwnd, "No password provided !\nUse provide a password (4 characters minimum).", prompt_titlep, MB_OK | MB_ICONSTOP );
			SetDlgEditForReview( hwnd, item_id );
		}
		return CHECKPW_MSG_EMPTY;
	}


	if ( len > 32 )
	{
		if ( prompt_titlep )
		{
			MessageBox( hwnd, "Password too long !\nUse 32 characters maximum.", prompt_titlep, MB_OK | MB_ICONSTOP );
			SetDlgEditForReview( hwnd, item_id );
		}
		return CHECKPW_MSG_BAD;
	}

	if ( len < 4 )
	{
		if ( prompt_titlep )
		{
			MessageBox( hwnd, "Password too short !\nUse 4 characters minimum.", prompt_titlep, MB_OK | MB_ICONSTOP );
			SetDlgEditForReview( hwnd, item_id );
		}
		return CHECKPW_MSG_BAD;
	}

	return CHECKPW_MSG_GOOD;
}
