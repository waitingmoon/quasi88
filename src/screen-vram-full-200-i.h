
/*
 * 画面サイズ	標準
 * 解像度	200LINE   (COLOR/MONO/UNDISP)
 * 効果		INTERLACE (奇数ラインは、テキストのみ描画)
 */

#include "screen-vram-w-h.h"
#include "screen-vram-pixel.h"
#include "screen-vram-full.h"

#undef	MASK_DOT
#undef	MASK_8DOT
#undef	MASK_16DOT
#undef	TRANS_DOT
#undef	TRANS_8DOT
#undef	TRANS_16DOT
#undef	STORE_DOT
#undef	STORE_8DOT
#undef	STORE_16DOT
#undef	COPY_DOT
#undef	COPY_8DOT
#undef	COPY_16DOT

/*===========================================================================*/

/* 【TEXTのみ描画】							*/
/*	カラー・白黒・VRAM非表示関係なく、テキストの色で塗りつぶす	*/

#define	MASK_8DOT()	for( m=0; m< 8; m++ ) dst[m] = dst2[m] = tcol;
#define	MASK_16DOT()	for( m=0; m<16; m++ ) dst[m] = dst2[m] = tcol;


		/* --------------------------------------------------------- */
#if		defined( COLOR )

/* 【VRAMのみ描画】 … カラーの場合 */

#define	TRANS_8DOT()				\
	vram = *(src + k*80);			\
	vcol[0] = get_pixel_index( vram, 0 );	\
	vcol[1] = get_pixel_index( vram, 1 );	\
	vcol[2] = get_pixel_index( vram, 2 );	\
	vcol[3] = get_pixel_index( vram, 3 );	\
	dst[0] = C7;				\
	dst[1] = C6;				\
	dst[2] = C5;				\
	dst[3] = C4;				\
	dst[4] = C3;				\
	dst[5] = C2;				\
	dst[6] = C1;				\
	dst[7] = C0;				\
	memset( dst2, BLACK, sizeof(TYPE)*8 );

#define	TRANS_16DOT()				\
	vram = *(src + k*80);			\
	vcol[0] = get_pixel_index( vram, 0 );	\
	vcol[1] = get_pixel_index( vram, 1 );	\
	vcol[2] = get_pixel_index( vram, 2 );	\
	vcol[3] = get_pixel_index( vram, 3 );	\
	dst[0] = C7;				\
	dst[1] = C6;				\
	dst[2] = C5;				\
	dst[3] = C4;				\
	dst[4] = C3;				\
	dst[5] = C2;				\
	dst[6] = C1;				\
	dst[7] = C0;				\
	vram = *(src + k*80+1);			\
	vcol[0] = get_pixel_index( vram, 0 );	\
	vcol[1] = get_pixel_index( vram, 1 );	\
	vcol[2] = get_pixel_index( vram, 2 );	\
	vcol[3] = get_pixel_index( vram, 3 );	\
	dst[ 8] = C7;				\
	dst[ 9] = C6;				\
	dst[10] = C5;				\
	dst[11] = C4;				\
	dst[12] = C3;				\
	dst[13] = C2;				\
	dst[14] = C1;				\
	dst[15] = C0;				\
	memset( dst2, BLACK, sizeof(TYPE)*16 );

/* 【TEXT/VRAM重ね合わせ描画】 … カラーの場合 */

#define	STORE_8DOT()						\
	vram = *(src + k*80);					\
	vcol[0] = get_pixel_index( vram, 0 );			\
	vcol[1] = get_pixel_index( vram, 1 );			\
	vcol[2] = get_pixel_index( vram, 2 );			\
	vcol[3] = get_pixel_index( vram, 3 );			\
	if( style & 0x80 ){ dst[0] =      dst2[0] = tcol;  }	\
	else              { dst[0] = C7;  dst2[0] = BLACK; }	\
	if( style & 0x40 ){ dst[1] =      dst2[1] = tcol;  }	\
	else              { dst[1] = C6;  dst2[1] = BLACK; }	\
	if( style & 0x20 ){ dst[2] =      dst2[2] = tcol;  }	\
	else              { dst[2] = C5;  dst2[2] = BLACK; }	\
	if( style & 0x10 ){ dst[3] =      dst2[3] = tcol;  }	\
	else              { dst[3] = C4;  dst2[3] = BLACK; }	\
	if( style & 0x08 ){ dst[4] =      dst2[4] = tcol;  }	\
	else              { dst[4] = C3;  dst2[4] = BLACK; }	\
	if( style & 0x04 ){ dst[5] =      dst2[5] = tcol;  }	\
	else              { dst[5] = C2;  dst2[5] = BLACK; }	\
	if( style & 0x02 ){ dst[6] =      dst2[6] = tcol;  }	\
	else              { dst[6] = C1;  dst2[6] = BLACK; }	\
	if( style & 0x01 ){ dst[7] =      dst2[7] = tcol;  }	\
	else              { dst[7] = C0;  dst2[7] = BLACK; }

#define	STORE_16DOT()								       \
	vram = *(src + k*80);							       \
	vcol[0] = get_pixel_index( vram, 0 );					       \
	vcol[1] = get_pixel_index( vram, 1 );					       \
	vcol[2] = get_pixel_index( vram, 2 );					       \
	vcol[3] = get_pixel_index( vram, 3 );					       \
	if( style & 0x80 ){ dst[ 0] =     dst[ 1] =     dst2[ 0] = dst2[1] = tcol;  }  \
	else              { dst[ 0] = C7; dst[ 1] = C6; dst2[ 0] = dst2[1] = BLACK; }  \
	if( style & 0x40 ){ dst[ 2] =     dst[ 3] =     dst2[ 2] = dst2[3] = tcol;  }  \
	else              { dst[ 2] = C5; dst[ 3] = C4; dst2[ 2] = dst2[3] = BLACK; }  \
	if( style & 0x20 ){ dst[ 4] =     dst[ 5] =     dst2[ 4] = dst2[5] = tcol;  }  \
	else              { dst[ 4] = C3; dst[ 5] = C2; dst2[ 4] = dst2[5] = BLACK; }  \
	if( style & 0x10 ){ dst[ 6] =     dst[ 7] =     dst2[ 6] = dst2[7] = tcol;  }  \
	else              { dst[ 6] = C1; dst[ 7] = C0; dst2[ 6] = dst2[7] = BLACK; }  \
	vram = *(src + k*80+1);							       \
	vcol[0] = get_pixel_index( vram, 0 );					       \
	vcol[1] = get_pixel_index( vram, 1 );					       \
	vcol[2] = get_pixel_index( vram, 2 );					       \
	vcol[3] = get_pixel_index( vram, 3 );					       \
	if( style & 0x08 ){ dst[ 8] =     dst[ 9] =     dst2[ 8] = dst2[ 9] = tcol;  } \
	else              { dst[ 8] = C7; dst[ 9] = C6; dst2[ 8] = dst2[ 9] = BLACK; } \
	if( style & 0x04 ){ dst[10] =     dst[11] =     dst2[10] = dst2[11] = tcol;  } \
	else              { dst[10] = C5; dst[11] = C4; dst2[10] = dst2[11] = BLACK; } \
	if( style & 0x02 ){ dst[12] =     dst[13] =     dst2[12] = dst2[13] = tcol;  } \
	else              { dst[12] = C3; dst[13] = C2; dst2[12] = dst2[13] = BLACK; } \
	if( style & 0x01 ){ dst[14] =     dst[15] =     dst2[14] = dst2[15] = tcol;  } \
	else              { dst[14] = C1; dst[15] = C0; dst2[14] = dst2[15] = BLACK; }


		/* --------------------------------------------------------- */
#elif		defined( MONO )

/* 【VRAMのみ描画】 … 白黒の場合 */

#define	TRANS_8DOT()					\
	vram  = *(src + k*80);				\
	vram &= mask;					\
	for( l=0; l<8; l++, vram<<=1 ){			\
	  dst[l]  = get_pixel_mono( vram, tcol );	\
	  dst2[l] = BLACK;				\
	}

#define	TRANS_16DOT()					\
	vram = *(src + k*80);				\
	vram &= mask;					\
	for( l=0; l<8; l++, vram<<=1 ){			\
	  dst[l]  = get_pixel_mono( vram, tcol );	\
	  dst2[l] = BLACK;				\
	}						\
	vram = *(src + k*80+1);				\
	vram &= mask;					\
	for(    ; l<16; l++, vram<<=1 ){		\
	  dst[l]  = get_pixel_mono( vram, tcol );	\
	  dst2[l] = BLACK;				\
	}


/* 【TEXT/VRAM重ね合わせ描画】 … 白黒の場合 */

#define	STORE_8DOT()									\
	vram  = *(src + k*80);								\
	vram &= mask;									\
	for( m=0x80, l=0; l<8; l++, m>>=1, vram<<=1 ){					\
	  if( style & m ){ dst[l] =                               dst2[l] = tcol;  }	\
	  else           { dst[l] = get_pixel_mono( vram, tcol ); dst2[l] = BLACK; }	\
	}

#define	STORE_16DOT()								\
	vram = *(src + k*80);							\
	vram &= mask;								\
	for( m=0x80, l=0; l<8; l+=2, m>>=1, vram<<=2 ){				\
	  if( style & m ){ dst[l]   = dst[l+1] = dst2[l] = dst2[l+1] = tcol; }	\
	  else           { dst[l]   = get_pixel_mono( vram,    tcol );		\
			   dst[l+1] = get_pixel_mono( vram<<1, tcol );		\
			   dst2[l]  = dst2[l+1] = BLACK; }			\
	}									\
	vram = *(src + k*80+1);							\
	vram &= mask;								\
	for(            ; l<16; l+=2, m>>=1, vram<<=2 ){			\
	  if( style & m ){ dst[l]   = dst[l+1] = dst2[l] = dst2[l+1] = tcol; }	\
	  else           { dst[l]   = get_pixel_mono( vram,    tcol );		\
			   dst[l+1] = get_pixel_mono( vram<<1, tcol );		\
			   dst2[l]  = dst2[l+1] = BLACK; }			\
	}


		/* --------------------------------------------------------- */
#elif		defined( UNDISP )

/* 【VRAMのみ描画】 … VRAM非表示の場合 */

#define	TRANS_8DOT()					\
	for( m=0; m<8; m++ ) dst[m] = dst2[m] = BLACK;

#define	TRANS_16DOT()					\
	for( m=0; m<16; m++ ) dst[m] = dst2[m] = BLACK;


/* 【TEXT/VRAM重ね合わせ描画】 … VRAM非表示の場合 */

#define	STORE_8DOT()								   \
	if( style & 0x80 ) dst[0] = dst2[0] = tcol;  else dst[0] = dst2[0] = BLACK;\
	if( style & 0x40 ) dst[1] = dst2[1] = tcol;  else dst[1] = dst2[1] = BLACK;\
	if( style & 0x20 ) dst[2] = dst2[2] = tcol;  else dst[2] = dst2[2] = BLACK;\
	if( style & 0x10 ) dst[3] = dst2[3] = tcol;  else dst[3] = dst2[3] = BLACK;\
	if( style & 0x08 ) dst[4] = dst2[4] = tcol;  else dst[4] = dst2[4] = BLACK;\
	if( style & 0x04 ) dst[5] = dst2[5] = tcol;  else dst[5] = dst2[5] = BLACK;\
	if( style & 0x02 ) dst[6] = dst2[6] = tcol;  else dst[6] = dst2[6] = BLACK;\
	if( style & 0x01 ) dst[7] = dst2[7] = tcol;  else dst[7] = dst2[7] = BLACK;

#define	STORE_16DOT()								\
	if( style & 0x80 ){ dst[ 0] = dst[ 1] = dst2[ 0] = dst2[ 1] = tcol; }	\
	else              { dst[ 0] = dst[ 1] = dst2[ 0] = dst2[ 1] = BLACK;}	\
	if( style & 0x40 ){ dst[ 2] = dst[ 3] = dst2[ 2] = dst2[ 3] = tcol; }	\
	else              { dst[ 2] = dst[ 3] = dst2[ 2] = dst2[ 3] = BLACK;}	\
	if( style & 0x20 ){ dst[ 4] = dst[ 5] = dst2[ 4] = dst2[ 5] = tcol; }	\
	else              { dst[ 4] = dst[ 5] = dst2[ 4] = dst2[ 5] = BLACK;}	\
	if( style & 0x10 ){ dst[ 6] = dst[ 7] = dst2[ 6] = dst2[ 7] = tcol; }	\
	else              { dst[ 6] = dst[ 7] = dst2[ 6] = dst2[ 7] = BLACK;}	\
	if( style & 0x08 ){ dst[ 8] = dst[ 9] = dst2[ 8] = dst2[ 9] = tcol; }	\
	else              { dst[ 8] = dst[ 9] = dst2[ 8] = dst2[ 9] = BLACK;}	\
	if( style & 0x04 ){ dst[10] = dst[11] = dst2[10] = dst2[11] = tcol; }	\
	else              { dst[10] = dst[11] = dst2[10] = dst2[11] = BLACK;}	\
	if( style & 0x02 ){ dst[12] = dst[13] = dst2[12] = dst2[13] = tcol; }	\
	else              { dst[12] = dst[13] = dst2[12] = dst2[13] = BLACK;}	\
	if( style & 0x01 ){ dst[14] = dst[15] = dst2[14] = dst2[15] = tcol; }	\
	else              { dst[14] = dst[15] = dst2[14] = dst2[15] = BLACK;}

#endif
		/* --------------------------------------------------------- */


/* 【ラインの隙間埋め】					*/
/*	すでに上記の処理で、奇数ラインも処理が終わって	*/
/*	いるので、ここでは処理なし			*/

#define	COPY_8DOT()
#define	COPY_16DOT()

/*===========================================================================*/

#if	(TEXT_WIDTH == 80)

#define	MASK_DOT()	MASK_8DOT()
#define	TRANS_DOT()	TRANS_8DOT()
#define	STORE_DOT()	STORE_8DOT()
#define	COPY_DOT()	COPY_8DOT()

#elif	(TEXT_WIDTH == 40)

#define	MASK_DOT()	MASK_16DOT()
#define	TRANS_DOT()	TRANS_16DOT()
#define	STORE_DOT()	STORE_16DOT()
#define	COPY_DOT()	COPY_16DOT()

#endif

#include "screen-vram.h"
