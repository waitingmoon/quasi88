
/*
 * 画面サイズ	半分
 * 解像度	200LINE  (COLOR/MONO/UNDISP)
 * 効果		INTERPOLATE (2ドット毎に色補完)
 */

#include "screen-vram-w-h.h"
#include "screen-vram-pixel.h"
#include "screen-vram-half.h"

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

#define	MASK_8DOT()	for( m=0; m<4; m++ ) dst[m] = tcol;
#define	MASK_16DOT()	for( m=0; m<8; m++ ) dst[m] = tcol;


		/* --------------------------------------------------------- */
#if		defined( COLOR )

/* 【VRAMのみ描画】 … カラーの場合 */

#define	TRANS_8DOT()						\
	vram = *(src + k*80);					\
	vcol[0] = get_pixel_index( vram, 0 );			\
	vcol[1] = get_pixel_index( vram, 1 );			\
	vcol[2] = get_pixel_index( vram, 2 );			\
	vcol[3] = get_pixel_index( vram, 3 );			\
	dst[0] = MIXED_PIXEL( vcol[0] >>4 , vcol[1] >>4 );	\
	dst[1] = MIXED_PIXEL( vcol[2] >>4 , vcol[3] >>4 );	\
	dst[2] = MIXED_PIXEL( vcol[0] & 7 , vcol[1] & 7 );	\
	dst[3] = MIXED_PIXEL( vcol[2] & 7 , vcol[3] & 7 );

#define	TRANS_16DOT()						\
	vram = *(src + k*80);					\
	vcol[0] = get_pixel_index( vram, 0 );			\
	vcol[1] = get_pixel_index( vram, 1 );			\
	vcol[2] = get_pixel_index( vram, 2 );			\
	vcol[3] = get_pixel_index( vram, 3 );			\
	dst[0] = MIXED_PIXEL( vcol[0] >>4 , vcol[1] >>4 );	\
	dst[1] = MIXED_PIXEL( vcol[2] >>4 , vcol[3] >>4 );	\
	dst[2] = MIXED_PIXEL( vcol[0] & 7 , vcol[1] & 7 );	\
	dst[3] = MIXED_PIXEL( vcol[2] & 7 , vcol[3] & 7 );	\
	vram = *(src + k*80+1);					\
	vcol[0] = get_pixel_index( vram, 0 );			\
	vcol[1] = get_pixel_index( vram, 1 );			\
	vcol[2] = get_pixel_index( vram, 2 );			\
	vcol[3] = get_pixel_index( vram, 3 );			\
	dst[4] = MIXED_PIXEL( vcol[0] >>4 , vcol[1] >>4 );	\
	dst[5] = MIXED_PIXEL( vcol[2] >>4 , vcol[3] >>4 );	\
	dst[6] = MIXED_PIXEL( vcol[0] & 7 , vcol[1] & 7 );	\
	dst[7] = MIXED_PIXEL( vcol[2] & 7 , vcol[3] & 7 );


/* 【TEXT/VRAM重ね合わせ描画】 … カラーの場合 */

#define	STORE_8DOT()						\
	vram = *(src + k*80);					\
	vcol[0] = get_pixel_index( vram, 0 );			\
	vcol[1] = get_pixel_index( vram, 1 );			\
	vcol[2] = get_pixel_index( vram, 2 );			\
	vcol[3] = get_pixel_index( vram, 3 );			\
	{							\
	  int h,l;						\
	  if( style&0x80 ) h = tpal;  else h = vcol[0] >>4;	\
	  if( style&0x40 ) l = tpal;  else l = vcol[1] >>4;	\
	  dst[0] = MIXED_PIXEL( h , l );			\
	  if( style&0x20 ) h = tpal;  else h = vcol[2] >>4;	\
	  if( style&0x10 ) l = tpal;  else l = vcol[3] >>4;	\
	  dst[1] = MIXED_PIXEL( h , l );			\
	  if( style&0x08 ) h = tpal;  else h = vcol[0] & 7;	\
	  if( style&0x04 ) l = tpal;  else l = vcol[1] & 7;	\
	  dst[2] = MIXED_PIXEL( h , l );			\
	  if( style&0x02 ) h = tpal;  else h = vcol[2] & 7;	\
	  if( style&0x01 ) l = tpal;  else l = vcol[3] & 7;	\
	  dst[3] = MIXED_PIXEL( h , l );			\
	}

#define	STORE_16DOT()								\
	vram = *(src + k*80);							\
	vcol[0] = get_pixel_index( vram, 0 );					\
	vcol[1] = get_pixel_index( vram, 1 );					\
	vcol[2] = get_pixel_index( vram, 2 );					\
	vcol[3] = get_pixel_index( vram, 3 );					\
	if( style & 0x80 ) dst[0] = tcol;					\
	else               dst[0] = MIXED_PIXEL( vcol[0] >>4 , vcol[1] >>4 );	\
	if( style & 0x40 ) dst[1] = tcol;					\
	else               dst[1] = MIXED_PIXEL( vcol[2] >>4 , vcol[3] >>4 );	\
	if( style & 0x20 ) dst[2] = tcol;					\
	else               dst[2] = MIXED_PIXEL( vcol[0] & 7 , vcol[1] & 7 );	\
	if( style & 0x10 ) dst[3] = tcol;					\
	else               dst[3] = MIXED_PIXEL( vcol[2] & 7 , vcol[3] & 7 );	\
	vram = *(src + k*80+1);							\
	vcol[0] = get_pixel_index( vram, 0 );					\
	vcol[1] = get_pixel_index( vram, 1 );					\
	vcol[2] = get_pixel_index( vram, 2 );					\
	vcol[3] = get_pixel_index( vram, 3 );					\
	if( style & 0x08 ) dst[4] = tcol;					\
	else               dst[4] = MIXED_PIXEL( vcol[0] >>4 , vcol[1] >>4 );	\
	if( style & 0x04 ) dst[5] = tcol;					\
	else               dst[5] = MIXED_PIXEL( vcol[2] >>4 , vcol[3] >>4 );	\
	if( style & 0x02 ) dst[6] = tcol;					\
	else               dst[6] = MIXED_PIXEL( vcol[0] & 7 , vcol[1] & 7 );	\
	if( style & 0x01 ) dst[7] = tcol;					\
	else               dst[7] = MIXED_PIXEL( vcol[2] & 7 , vcol[3] & 7 );

		/* --------------------------------------------------------- */
#elif		defined( MONO )

/* 【VRAMのみ描画】 … 白黒の場合 */

#define	TRANS_8DOT()					\
	vram  = *(src + k*80);				\
	vram &= mask;					\
	for( l=0; l<4; l++, vram<<=2 ){			\
	  dst[l] = get_pixel_mono( vram, tcol );	\
	}

#define	TRANS_16DOT()					\
	vram = *(src + k*80);				\
	vram &= mask;					\
	for( l=0; l<4; l++, vram<<=2 ){			\
	  dst[l] = get_pixel_mono( vram, tcol );	\
	}						\
	vram = *(src + k*80+1);				\
	vram &= mask;					\
	for(    ; l<8; l++, vram<<=2 ){			\
	  dst[l] = get_pixel_mono( vram, tcol );	\
	}

/* 【TEXT/VRAM重ね合わせ描画】 … 白黒の場合 */

#define	STORE_8DOT()							\
	vram  = *(src + k*80);						\
	vram &= mask;							\
	for( m=0xc0, l=0; l<4; l++, m>>=2, vram<<=2 ){			\
	  if( style & m ) dst[l] = tcol;				\
	  else            dst[l] = get_pixel_mono( vram, tcol );	\
	}

#define	STORE_16DOT()							\
	vram = *(src + k*80);						\
	vram &= mask;							\
	for( m=0x80, l=0; l<4; l++, m>>=1, vram<<=2 ){			\
	  if( style & m ) dst[l] = tcol;				\
	  else            dst[l] = get_pixel_mono( vram, tcol );	\
	}								\
	vram = *(src + k*80+1);						\
	vram &= mask;							\
	for(            ; l<8; l++, m>>=1, vram<<=2 ){			\
	  if( style & m ) dst[l] = tcol;				\
	  else            dst[l] = get_pixel_mono( vram, tcol );	\
	}

		/* --------------------------------------------------------- */
#elif		defined( UNDISP )

/* 【VRAMのみ描画】 … VRAM非表示の場合 */

#define	TRANS_8DOT()				\
	for( m=0; m<4; m++ ) dst[m] = BLACK;

#define	TRANS_16DOT()				\
	for( m=0; m<8; m++ ) dst[m] = BLACK;

/* 【TEXT/VRAM重ね合わせ描画】 … VRAM非表示の場合 */

#define	STORE_8DOT()					\
	{						\
	  int bpal = (grph_ctrl&GRPH_CTRL_COLOR) ?8 :7;	\
	  int h,l;					\
	  if( style&0x80 ) h = tpal;  else h = bpal;	\
	  if( style&0x40 ) l = tpal;  else l = bpal;	\
	  dst[0] = MIXED_PIXEL( h , l );		\
	  if( style&0x20 ) h = tpal;  else h = bpal;	\
	  if( style&0x10 ) l = tpal;  else l = bpal;	\
	  dst[1] = MIXED_PIXEL( h , l );		\
	  if( style&0x08 ) h = tpal;  else h = bpal;	\
	  if( style&0x04 ) l = tpal;  else l = bpal;	\
	  dst[2] = MIXED_PIXEL( h , l );		\
	  if( style&0x02 ) h = tpal;  else h = bpal;	\
	  if( style&0x01 ) l = tpal;  else l = bpal;	\
	  dst[3] = MIXED_PIXEL( h , l );		\
	}

#define	STORE_16DOT()						\
	if( style & 0x80 ) dst[0] = tcol;  else dst[0] = BLACK;	\
	if( style & 0x40 ) dst[1] = tcol;  else dst[1] = BLACK;	\
	if( style & 0x20 ) dst[2] = tcol;  else dst[2] = BLACK;	\
	if( style & 0x10 ) dst[3] = tcol;  else dst[3] = BLACK;	\
	if( style & 0x08 ) dst[4] = tcol;  else dst[4] = BLACK;	\
	if( style & 0x04 ) dst[5] = tcol;  else dst[5] = BLACK;	\
	if( style & 0x02 ) dst[6] = tcol;  else dst[6] = BLACK;	\
	if( style & 0x01 ) dst[7] = tcol;  else dst[7] = BLACK;

#endif

		/* --------------------------------------------------------- */

/* 【ラインの隙間埋め】					*/
/*	隙間なぞ、無い！				*/

#define		COPY_8DOT()
#define		COPY_16DOT()

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
