#undef	FONT_H
#undef	FONT_W
#undef	FONT_8x8
#undef	FONT_8x16
#undef	FONT_16x16
#undef	FONT_LOGO8x16
#undef	WORK_DEFINE
#undef	GET_FONT
#undef	GET_CURSOR
#undef	PUT_FONT

#undef	MENU2PIXEL



#define	FONT_H	32
#define	FONT_W	16

#define	FONT_8x8()				\
	font_inc  = 1; 				\
	font_dup  = TRUE;			\
	font_skip = 1;

#define	FONT_8x16()				\
	font_inc  = 1;				\
	font_dup  = FALSE;			\
	font_skip = 1;

#define	FONT_16x16()				\
	font_inc  = 2;				\
	font_dup  = FALSE;			\
	font_skip = 1;

#define	FONT_LOGO8x16()				\
	font_inc  = Q8GR_LOGO_W;		\
	font_dup  = FALSE;			\
	font_skip = 1;

#define	WORK_DEFINE()

#define	GET_FONT()	*font_ptr

#define	GET_CURSOR()	*cur_ptr

#define	PUT_FONT()							\
	if( style & 0x80 ) dst[ 0]=dst[ 1]=fg;   else   dst[ 0]=dst[ 1]=bg;\
	if( style & 0x40 ) dst[ 2]=dst[ 3]=fg;   else   dst[ 2]=dst[ 3]=bg;\
	if( style & 0x20 ) dst[ 4]=dst[ 5]=fg;   else   dst[ 4]=dst[ 5]=bg;\
	if( style & 0x10 ) dst[ 6]=dst[ 7]=fg;   else   dst[ 6]=dst[ 7]=bg;\
	if( style & 0x08 ) dst[ 8]=dst[ 9]=fg;   else   dst[ 8]=dst[ 9]=bg;\
	if( style & 0x04 ) dst[10]=dst[11]=fg;   else   dst[10]=dst[11]=bg;\
	if( style & 0x02 ) dst[12]=dst[13]=fg;   else   dst[12]=dst[13]=bg;\
	if( style & 0x01 ) dst[14]=dst[15]=fg;   else   dst[14]=dst[15]=bg;\
	memcpy( dst+SCREEN_WIDTH, dst, sizeof(TYPE)*16 );		\
	dst += 2*SCREEN_WIDTH;


#include "screen-menu.h"
