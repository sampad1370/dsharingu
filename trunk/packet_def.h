/*
	$Id: packet_def.h,v 1.4 2003/05/29 12:41:33 duddie Exp $

	$Log: packet_def.h,v $
	Revision 1.4  2003/05/29 12:41:33  duddie
	fixed add/del friends and add/get channels list per user
	
	Revision 1.3  2003/05/26 01:20:35  duddie
	changed outgoing packet from get_channel_list to channel_list
	

*/

#ifndef _PACKET_DEF_H
#define _PACKET_DEF_H


typedef struct
{
	unsigned short	id;		// packet identifier
	unsigned short	pad;
	unsigned int	size;	// packet size not including size of packet_header
} packet_header_t;



#endif // _PACKET_DEF_H