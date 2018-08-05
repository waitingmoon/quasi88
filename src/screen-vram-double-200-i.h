
/*
 * 画面サイズ	倍
 * 解像度	200LINE   (COLOR/MONO/UNDISP)
 * 効果		INTERLACE (奇数ラインは、テキストのみ描画)
 */

#include "screen-vram-w-h.h"
#include "screen-vram-pixel.h"
#include "screen-vram-double.h"

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

#define	MASK_8DOT()	for( m=0; m<16; m++ ) dst[m] = dst3[m] = tcol;
#define	MASK_16DOT()	for( m=0; m<32; m++ ) dst[m] = dst3[m] = tcol;


		/* --------------------------------------------------------- */
#if		defined( COLOR )

/* 【VRAMのみ描画】 … カラーの場合 */

#define	TRANS_8DOT()				\
	vram = *(src + k*80);			\
	vcol[0] = get_pixel_index( vram, 0 );	\
	vcol[1] = get_pixel_index( vram, 1 );	\
	vcol[2] = get_pixel_index( vram, 2 );	\
	vcol[3] = get_pixel_index( vram, 3 );	\
	dst[ 0]=dst[ 1] = C7;			\
	dst[ 2]=dst[ 3] = C6;			\
	dst[ 4]=dst[ 5] = C5;			\
	dst[ 6]=dst[ 7] = C4;			\
	dst[ 8]=dst[ 9] = C3;			\
	dst[10]=dst[11] = C2;			\
	dst[12]=dst[13] = C1;			\
	dst[14]=dst[15] = C0;			\
	memset( dst3, BLACK, sizeof(TYPE)*16 );

#define	TRANS_16DOT()				\
	vram = *(src + k*80);			\
	vcol[0] = get_pixel_index( vram, 0 );	\
	vcol[1] = get_pixel_index( vram, 1 );	\
	vcol[2] = get_pixel_index( vram, 2 );	\
	vcol[3] = get_pixel_index( vram, 3 );	\
	dst[ 0]=dst[ 1] = C7;			\
	dst[ 2]=dst[ 3] = C6;			\
	dst[ 4]=dst[ 5] = C5;			\
	dst[ 6]=dst[ 7] = C4;			\
	dst[ 8]=dst[ 9] = C3;			\
	dst[10]=dst[11] = C2;			\
	dst[12]=dst[13] = C1;			\
	dst[14]=dst[15] = C0;			\
	vram = *(src + k*80+1);			\
	vcol[0] = get_pixel_index( vram, 0 );	\
	vcol[1] = get_pixel_index( vram, 1 );	\
	vcol[2] = get_pixel_index( vram, 2 );	\
	vcol[3] = get_pixel_index( vram, 3 );	\
	dst[16]=dst[17] = C7;			\
	dst[18]=dst[19] = C6;			\
	dst[20]=dst[21] = C5;			\
	dst[22]=dst[23] = C4;			\
	dst[24]=dst[25] = C3;			\
	dst[26]=dst[27] = C2;			\
	dst[28]=dst[29] = C1;			\
	dst[30]=dst[31] = C0;			\
	memset( dst3, BLACK, sizeof(TYPE)*32 );

/* 【TEXT/VRAM重ね合わせ描画】 … カラーの場合 */

#define	STORE_8DOT()								\
	vram = *(src + k*80);							\
	vcol[0] = get_pixel_index( vram, 0 );					\
	vcol[1] = get_pixel_index( vram, 1 );					\
	vcol[2] = get_pixel_index( vram, 2 );					\
	vcol[3] = get_pixel_index( vram, 3 );					\
	if( style & 0x80 ){ dst[ 0]=dst[ 1] =     dst3[ 0]=dst3[ 1]= tcol;  }	\
	else              { dst[ 0]=dst[ 1] = C7; dst3[ 0]=dst3[ 1]= BLACK; }	\
	if( style & 0x40 ){ dst[ 2]=dst[ 3] =     dst3[ 2]=dst3[ 3]= tcol;  }	\
	else              { dst[ 2]=dst[ 3] = C6; dst3[ 2]=dst3[ 3]= BLACK; }	\
	if( style & 0x20 ){ dst[ 4]=dst[ 5] =     dst3[ 4]=dst3[ 5]= tcol;  }	\
	else              { dst[ 4]=dst[ 5] = C5; dst3[ 4]=dst3[ 5]= BLACK; }	\
	if( style & 0x10 ){ dst[ 6]=dst[ 7] =     dst3[ 6]=dst3[ 7]= tcol;  }	\
	else              { dst[ 6]=dst[ 7] = C4; dst3[ 6]=dst3[ 7]= BLACK; }	\
	if( style & 0x08 ){ dst[ 8]=dst[ 9] =     dst3[ 8]=dst3[ 9]= tcol;  }	\
	else              { dst[ 8]=dst[ 9] = C3; dst3[ 8]=dst3[ 9]= BLACK; }	\
	if( style & 0x04 ){ dst[10]=dst[11] =     dst3[10]=dst3[11]= tcol;  }	\
	else              { dst[10]=dst[11] = C2; dst3[10]=dst3[11]= BLACK; }	\
	if( style & 0x02 ){ dst[12]=dst[13] =     dst3[12]=dst3[13]= tcol;  }	\
	else              { dst[12]=dst[13] = C1; dst3[12]=dst3[13]= BLACK; }	\
	if( style & 0x01 ){ dst[14]=dst[15] =     dst3[14]=dst3[15]= tcol;  }	\
	else              { dst[14]=dst[15] = C0; dst3[14]=dst3[15]= BLACK; }

#define	STORE_16DOT()								\
	vram = *(src + k*80);							\
	vcol[0] = get_pixel_index( vram, 0 );					\
	vcol[1] = get_pixel_index( vram, 1 );					\
	vcol[2] = get_pixel_index( vram, 2 );					\
	vcol[3] = get_pixel_index( vram, 3 );					\
	if( style & 0x80 ){  dst[ 0]= dst[ 1] =     dst[ 2]= dst[ 3] = tcol;	\
			    dst3[ 0]=dst3[ 1] =    dst3[ 2]=dst3[ 3] = tcol;  }	\
	else              {  dst[ 0]= dst[ 1] =C7;  dst[ 2]= dst[ 3] = C6;	\
			    dst3[ 0]=dst3[ 1] =    dst3[ 2]=dst3[ 3] = BLACK; }	\
	if( style & 0x40 ){  dst[ 4]= dst[ 5] =     dst[ 6]= dst[ 7] = tcol;	\
			    dst3[ 4]=dst3[ 5] =    dst3[ 6]=dst3[ 7] = tcol;  }	\
	else              {  dst[ 4]= dst[ 5] =C5;  dst[ 6]= dst[ 7] = C4;	\
			    dst3[ 4]=dst3[ 5] =    dst3[ 6]=dst3[ 7] = BLACK; }	\
	if( style & 0x20 ){  dst[ 8]= dst[ 9] =     dst[10]= dst[11] = tcol;	\
			    dst3[ 8]=dst3[ 9] =    dst3[10]=dst3[11] = tcol;  }	\
	else              {  dst[ 8]= dst[ 9] =C3;  dst[10]= dst[11] = C2;	\
			    dst3[ 8]=dst3[ 9] =    dst3[10]=dst3[11] = BLACK; }	\
	if( style & 0x10 ){  dst[12]= dst[13] =     dst[14]= dst[15] = tcol;	\
			    dst3[12]=dst3[13] =    dst3[14]=dst3[15] = tcol;  }	\
	else              {  dst[12]= dst[13] =C1;  dst[14]= dst[15] = C0;	\
			    dst3[12]=dst3[13] =    dst3[14]=dst3[15] = BLACK; }	\
	vram = *(src + k*80 +1);						\
	vcol[0] = get_pixel_index( vram, 0 );					\
	vcol[1] = get_pixel_index( vram, 1 );					\
	vcol[2] = get_pixel_index( vram, 2 );					\
	vcol[3] = get_pixel_index( vram, 3 );					\
	if( style & 0x08 ){  dst[16]= dst[17] =     dst[18]= dst[19] = tcol;	\
			    dst3[16]=dst3[17] =    dst3[18]=dst3[19] = tcol;  }	\
	else              {  dst[16]= dst[17] =C7;  dst[18]= dst[19] = C6;	\
			    dst3[16]=dst3[17] =    dst3[18]=dst3[19] = BLACK; }	\
	if( style & 0x04 ){  dst[20]= dst[21] =     dst[22]= dst[23] = tcol;	\
			    dst3[20]=dst3[21] =    dst3[22]=dst3[23] = tcol;  }	\
	else              {  dst[20]= dst[21] =C5;  dst[22]= dst[23] = C4;	\
			    dst3[20]=dst3[21] =    dst3[22]=dst3[23] = BLACK; }	\
	if( style & 0x02 ){  dst[24]= dst[25] =     dst[26]= dst[27] = tcol;	\
			    dst3[24]=dst3[25] =    dst3[26]=dst3[27] = tcol;  }	\
	else              {  dst[24]= dst[25] =C3;  dst[26]= dst[27] = C2;	\
			    dst3[24]=dst3[25] =    dst3[26]=dst3[27] = BLACK; }	\
	if( style & 0x01 ){  dst[28]= dst[29] =     dst[30]= dst[31] = tcol;	\
			    dst3[28]=dst3[29] =    dst3[30]=dst3[31] = tcol;  }	\
	else              {  dst[28]= dst[29] =C1;  dst[30]= dst[31] = C0;	\
			    dst3[28]=dst3[29] =    dst3[30]=dst3[31] = BLACK; }

		/* --------------------------------------------------------- */
#elif		defined( MONO )

/* 【VRAMのみ描画】 … 白黒の場合 */

#define	TRANS_8DOT()						\
	vram  = *(src + k*80);					\
	vram &= mask;						\
	for( l=0; l<16; l+=2, vram<<=1 ){			\
	  dst[l]  = dst[l+1]  = get_pixel_mono( vram, tcol );	\
	  dst3[l] = dst3[l+1] = BLACK;				\
	}

#define	TRANS_16DOT()						\
	vram = *(src + k*80);					\
	vram &= mask;						\
	for( l=0; l<16; l+=2, vram<<=1 ){			\
	  dst[l]  = dst[l+1]  = get_pixel_mono( vram, tcol );	\
	  dst3[l] = dst3[l+1] = BLACK;				\
	}							\
	vram = *(src + k*80+1);					\
	vram &= mask;						\
	for(    ; l<32; l+=2, vram<<=1 ){			\
	  dst[l]  = dst[l+1]  = get_pixel_mono( vram, tcol );	\
	  dst3[l] = dst3[l+1] = BLACK;				\
	}

/* 【TEXT/VRAM重ね合わせ描画】 … 白黒の場合 */

#define	STORE_8DOT()								\
	vram  = *(src + k*80);							\
	vram &= mask;								\
	for( m=0x80, l=0; l<16; l+=2, m>>=1, vram<<=1 ){			\
	  if( style & m ){ dst[l] =dst[l+1]  = dst3[l]=dst3[l+1]=tcol; }	\
	  else           { dst[l] =dst[l+1]  = get_pixel_mono( vram, tcol );	\
			   dst3[l]=dst3[l+1] = BLACK; }				\
	}

#define	STORE_16DOT()								\
	vram = *(src + k*80);							\
	vram &= mask;								\
	for( m=0x80, l=0; l<16; l+=4, m>>=1, vram<<=2 ){			\
	  if( style&m ){dst[l]  = dst[l+1] =  dst[l+2]= dst[l+3] = tcol;	\
		        dst3[l] =dst3[l+1] = dst3[l+2]=dst3[l+3] = tcol; }	\
	  else         {dst[l]  = dst[l+1] = get_pixel_mono(vram,   tcol);	\
		        dst[l+2]= dst[l+3] = get_pixel_mono(vram<<1,tcol);	\
		        dst3[l] =dst3[l+1] = dst3[l+2]=dst3[l+3] = BLACK; }	\
	}									\
	vram = *(src + k*80+1);							\
	vram &= mask;								\
	for(            ; l<32; l+=4, m>>=1, vram<<=2 ){			\
	  if( style&m ){dst[l]  = dst[l+1] =  dst[l+2]= dst[l+3] = tcol;	\
		        dst3[l] =dst3[l+1] = dst3[l+2]=dst3[l+3] = tcol; }	\
	  else         {dst[l]  = dst[l+1] = get_pixel_mono(vram,   tcol);	\
		        dst[l+2]= dst[l+3] = get_pixel_mono(vram<<1,tcol);	\
		        dst3[l] =dst3[l+1] = dst3[l+2]=dst3[l+3] = BLACK; }	\
	}

		/* --------------------------------------------------------- */
#elif		defined( UNDISP )

/* 【VRAMのみ描画】 … VRAM非表示の場合 */

#define	TRANS_8DOT()						\
	for( m=0; m<16; m++ ) dst[m] = dst3[m] = BLACK;

#define	TRANS_16DOT()						\
	for( m=0; m<32; m++ ) dst[m] = dst3[m] = BLACK;


/* 【TEXT/VRAM重ね合わせ描画】 … VRAM非表示の場合 */

#define	STORE_8DOT()											     \
	if(style&0x80) dst[ 0]=dst[ 1]=dst3[ 0]=dst3[ 1]=tcol;  else dst[ 0]=dst[ 1]=dst3[ 0]=dst3[ 1]=BLACK;\
	if(style&0x40) dst[ 2]=dst[ 3]=dst3[ 2]=dst3[ 3]=tcol;  else dst[ 2]=dst[ 3]=dst3[ 2]=dst3[ 3]=BLACK;\
	if(style&0x20) dst[ 4]=dst[ 5]=dst3[ 4]=dst3[ 5]=tcol;  else dst[ 4]=dst[ 5]=dst3[ 4]=dst3[ 5]=BLACK;\
	if(style&0x10) dst[ 6]=dst[ 7]=dst3[ 6]=dst3[ 7]=tcol;  else dst[ 6]=dst[ 7]=dst3[ 6]=dst3[ 7]=BLACK;\
	if(style&0x08) dst[ 8]=dst[ 9]=dst3[ 8]=dst3[ 9]=tcol;  else dst[ 8]=dst[ 9]=dst3[ 8]=dst3[ 9]=BLACK;\
	if(style&0x04) dst[10]=dst[11]=dst3[10]=dst3[11]=tcol;  else dst[10]=dst[11]=dst3[10]=dst3[11]=BLACK;\
	if(style&0x02) dst[12]=dst[13]=dst3[12]=dst3[13]=tcol;  else dst[12]=dst[13]=dst3[12]=dst3[13]=BLACK;\
	if(style&0x01) dst[14]=dst[15]=dst3[14]=dst3[15]=tcol;  else dst[14]=dst[15]=dst3[14]=dst3[15]=BLACK;

#define	STORE_16DOT()										      \
	if( style & 0x80 ) dst[ 0]=dst[ 1]=dst[ 2]=dst[ 3]=dst3[ 0]=dst3[ 1]=dst3[ 2]=dst3[ 3]= tcol; \
	else               dst[ 0]=dst[ 1]=dst[ 2]=dst[ 3]=dst3[ 0]=dst3[ 1]=dst3[ 2]=dst3[ 3]= BLACK;\
	if( style & 0x40 ) dst[ 4]=dst[ 5]=dst[ 6]=dst[ 7]=dst3[ 4]=dst3[ 5]=dst3[ 6]=dst3[ 7]= tcol; \
	else               dst[ 4]=dst[ 5]=dst[ 6]=dst[ 7]=dst3[ 4]=dst3[ 5]=dst3[ 6]=dst3[ 7]= BLACK;\
	if( style & 0x20 ) dst[ 8]=dst[ 9]=dst[10]=dst[11]=dst3[ 8]=dst3[ 9]=dst3[10]=dst3[11]= tcol; \
	else               dst[ 8]=dst[ 9]=dst[10]=dst[11]=dst3[ 8]=dst3[ 9]=dst3[10]=dst3[11]= BLACK;\
	if( style & 0x10 ) dst[12]=dst[13]=dst[14]=dst[15]=dst3[12]=dst3[13]=dst3[14]=dst3[15]= tcol; \
	else               dst[12]=dst[13]=dst[14]=dst[15]=dst3[12]=dst3[13]=dst3[14]=dst3[15]= BLACK;\
	if( style & 0x08 ) dst[16]=dst[17]=dst[18]=dst[19]=dst3[16]=dst3[17]=dst3[18]=dst3[19]= tcol; \
	else               dst[16]=dst[17]=dst[18]=dst[19]=dst3[16]=dst3[17]=dst3[18]=dst3[19]= BLACK;\
	if( style & 0x04 ) dst[20]=dst[21]=dst[22]=dst[23]=dst3[20]=dst3[21]=dst3[22]=dst3[23]= tcol; \
	else               dst[20]=dst[21]=dst[22]=dst[23]=dst3[20]=dst3[21]=dst3[22]=dst3[23]= BLACK;\
	if( style & 0x02 ) dst[24]=dst[25]=dst[26]=dst[27]=dst3[24]=dst3[25]=dst3[26]=dst3[27]= tcol; \
	else               dst[24]=dst[25]=dst[26]=dst[27]=dst3[24]=dst3[25]=dst3[26]=dst3[27]= BLACK;\
	if( style & 0x01 ) dst[28]=dst[29]=dst[30]=dst[31]=dst3[28]=dst3[29]=dst3[30]=dst3[31]= tcol; \
	else               dst[28]=dst[29]=dst[30]=dst[31]=dst3[28]=dst3[29]=dst3[30]=dst3[31]= BLACK;


#endif

		/* --------------------------------------------------------- */

/* 【ラインの隙間埋め】					*/

#define	COPY_8DOT()	memcpy( dst2, dst,   sizeof(TYPE)*16 );	\
			memcpy( dst4, dst3,  sizeof(TYPE)*16 );

#define	COPY_16DOT()	memcpy( dst2, dst,   sizeof(TYPE)*32 );	\
			memcpy( dst4, dst3,  sizeof(TYPE)*32 );

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
