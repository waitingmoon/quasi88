
/*
 * 画面サイズ	標準
 * 解像度	400LINE  (HIRESO)
 * 効果		NORMAL
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
#if	!defined(HIRESO)
#error
#endif

/*===========================================================================*/

/* 【TEXTのみ描画】							*/
/*	カラー・白黒・VRAM非表示関係なく、テキストの色で塗りつぶす	*/

#define	MASK_8DOT()	for( m=0; m< 8; m++ ) dst[m] = tcol;
#define	MASK_16DOT()	for( m=0; m<16; m++ ) dst[m] = tcol;


/* 【VRAMのみ描画】 */

#undef		UPPER_SCREEN
#if	( ROWS == 20 )
#define		UPPER_SCREEN	(i<10)
#elif	( ROWS == 25 )
#define		UPPER_SCREEN	((i<12) || (i==12 && k<8) )
#endif

#define	TRANS_8DOT()							\
	if( UPPER_SCREEN ){						\
	  vram  = *(src + k*80);					\
	  for( l=0; l<8; l++, vram<<=1 ){				\
	    dst[l] = get_pixel_400_B( vram, tcol );			\
	  }								\
	}else{								\
	  vram  = *(src + k*80 -80*200);				\
	  for( l=0; l<8; l++, vram<<=1 ){				\
	    dst[l] = get_pixel_400_R( vram, tcol );			\
	  }								\
	}

#define	TRANS_16DOT()							\
	if( UPPER_SCREEN ){						\
	  vram = *(src + k*80);						\
	  for( l=0; l<8; l++, vram<<=1 ){				\
	    dst[l] = get_pixel_400_B( vram, tcol );			\
	  }								\
	  vram = *(src + k*80+1);					\
	  for(    ; l<16; l++, vram<<=1 ){				\
	    dst[l] = get_pixel_400_B( vram, tcol );			\
	  }								\
	}else{								\
	  vram = *(src + k*80 -80*200);					\
	  for( l=0; l<8; l++, vram<<=1 ){				\
	    dst[l] = get_pixel_400_R( vram, tcol );			\
	  }								\
	  vram = *(src + k*80+1 -80*200);				\
	  for(    ; l<16; l++, vram<<=1 ){				\
	    dst[l] = get_pixel_400_R( vram, tcol );			\
	  }								\
	}

/* 【TEXT/VRAM重ね合わせ描画】 */

#define	STORE_8DOT()							\
	if( UPPER_SCREEN ){						\
	  vram  = *(src + k*80);					\
	  for( m=0x80, l=0; l<8; l++, m>>=1, vram<<=1 ){		\
	    if( style & m ) dst[l] = tcol;				\
	    else            dst[l] = get_pixel_400_B( vram, tcol );	\
	  }								\
	}else{								\
	  vram  = *(src + k*80 -80*200);				\
	  for( m=0x80, l=0; l<8; l++, m>>=1, vram<<=1 ){		\
	    if( style & m ) dst[l] = tcol;				\
	    else            dst[l] = get_pixel_400_R( vram, tcol );	\
	  }								\
	}

#define	STORE_16DOT()							\
	if( UPPER_SCREEN ){						\
	  vram = *(src + k*80);						\
	  for( m=0x80, l=0; l<8; l+=2, m>>=1, vram<<=2 ){		\
	    if( style & m ){dst[l]  =dst[l+1] = tcol; }			\
	    else           {dst[l]  =get_pixel_400_B( vram,    tcol );	\
			    dst[l+1]=get_pixel_400_B( vram<<1, tcol );}	\
	  }								\
	  vram = *(src + k*80+1);					\
	  for(            ; l<16; l+=2, m>>=1, vram<<=2 ){		\
	    if( style & m ){dst[l]  =dst[l+1] = tcol; }			\
	    else           {dst[l]  =get_pixel_400_B( vram,    tcol );	\
			    dst[l+1]=get_pixel_400_B( vram<<1, tcol );}	\
	  }								\
	}else{								\
	  vram = *(src + k*80 -80*200);					\
	  for( m=0x80, l=0; l<8; l+=2, m>>=1, vram<<=2 ){		\
	    if( style & m ){dst[l]  =dst[l+1] = tcol; }			\
	    else           {dst[l]  =get_pixel_400_R( vram,    tcol );	\
			    dst[l+1]=get_pixel_400_R( vram<<1, tcol );}	\
	  }								\
	  vram = *(src + k*80+1 -80*200);				\
	  for(            ; l<16; l+=2, m>>=1, vram<<=2 ){		\
	    if( style & m ){dst[l]  =dst[l+1] = tcol; }			\
	    else           {dst[l]  =get_pixel_400_R( vram,    tcol );	\
			    dst[l+1]=get_pixel_400_R( vram<<1, tcol );}	\
	 }								\
	}


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
