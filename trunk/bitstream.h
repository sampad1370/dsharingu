//============================================================================
//							
//----------------------------------------------------------------------------
// ・
//----------------------------------------------------------------------------
// Date			Author		Comments
//----------------------------------------------------------------------------
// 2005/08/26	ダビデ	新規作成
//============================================================================

#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

#include "ksys.h"

//============================================================================
struct BitStream
{
	int		bits_per_value;
	u_int	max_value;
	int		cur_idx;
	u_char	*datap;
	int		data_max_size;
};

//============================================================================
void	BitStream_Init( BitStream *T, u_int max_value, void *datap, int data_max_size );
void	BitStream_Init( BitStream *T, u_int max_value, const void *datap, int data_max_size );

void	BitStream_WriteValue( BitStream *T, u_int value );
u_int	BitStream_ReadValue( BitStream *T );

void	BitStream_WriteEnd( BitStream *T );
u_char	*BitStream_GetEndPtr( BitStream *T );

#endif
