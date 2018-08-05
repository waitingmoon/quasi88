#undef	get_pixel_index
#undef	make_mask_mono
#undef	get_pixel_mono
#undef	get_pixel_400_B
#undef	get_pixel_400_R
#undef	WORK_DEFINE

/*======================================================================
 * 88VRAMメモリパレット情報を、描画バッファの色情報に変換
 *======================================================================*/

#ifdef LSB_FIRST
#define	get_pixel_index(data,x)					\
	(((data) & ((bit32)0x00000088>>(x)) ) >> (  3-(x) ) |	\
	 ((data) & ((bit32)0x00008800>>(x)) ) >> ( 10-(x) ) |	\
	 ((data) & ((bit32)0x00880000>>(x)) ) >> ( 17-(x) ) )
#else
#define	get_pixel_index(data,x)					\
	(((data) & ((bit32)0x88000000>>(x)) ) >> ( 27-(x) ) |	\
	 ((data) & ((bit32)0x00880000>>(x)) ) >> ( 18-(x) ) |	\
	 ((data) & ((bit32)0x00008800>>(x)) ) >> (  9-(x) ) )
#endif

/*----------------------------------------------------------------------*/

#ifdef LSB_FIRST
#define	make_mask_mono( mask )						\
	do{								\
	  mask = 0xffffffff;						\
	  if( grph_pile & GRPH_PILE_BLUE  ) mask &= 0x00ffff00;		\
	  if( grph_pile & GRPH_PILE_RED   ) mask &= 0x00ff00ff;		\
	  if( grph_pile & GRPH_PILE_GREEN ) mask &= 0x0000ffff;		\
	}while(0)
#define	get_pixel_mono( data, col )					\
		((data)&0x00808080) ? (col) : COLOR_PIXEL(0)
#else
#define	make_mask_mono( mask )						\
	do{								\
	  mask = 0xffffffff;						\
	  if( grph_pile & GRPH_PILE_BLUE  ) mask &= 0x00ffff00;		\
	  if( grph_pile & GRPH_PILE_RED   ) mask &= 0xff00ff00;		\
	  if( grph_pile & GRPH_PILE_GREEN ) mask &= 0xffff0000;		\
	}while(0)
#define	get_pixel_mono( data, col )					\
		((data)&0x80808000) ? (col) : COLOR_PIXEL(0)
#endif

/*----------------------------------------------------------------------*/

#ifdef LSB_FIRST
#define	get_pixel_400_B( data, col )				\
		((data)&0x00000080) ? (col) : COLOR_PIXEL(0)
#define	get_pixel_400_R( data, col )				\
		((data)&0x00008000) ? (col) : COLOR_PIXEL(0)
#else
#define	get_pixel_400_B( data, col )				\
		((data)&0x80000000) ? (col) : COLOR_PIXEL(0)
#define	get_pixel_400_R( data, col )				\
		((data)&0x00800000) ? (col) : COLOR_PIXEL(0)
#endif



/*======================================================================
 * VRAM → pixel 変換に使用するワークの定義
 *======================================================================*/

#if	defined( COLOR )

#define	WORK_DEFINE()					\
	int	m;					\
	bit32	vram;  					\
	bit32	vcol[4];

/*----------------------------------------------------------------------*/
#elif	defined( MONO )

#define	WORK_DEFINE()					\
	int	m, l;					\
	bit32	vram;					\
	bit32	mask;					\
	make_mask_mono( mask );

/*----------------------------------------------------------------------*/
#elif	defined( UNDISP )

#define	WORK_DEFINE()					\
	int	m;

/*----------------------------------------------------------------------*/
#elif	defined( HIRESO )

#define	WORK_DEFINE()					\
	int	m, l;					\
	bit32	vram;

#else
#error
#endif
/*----------------------------------------------------------------------*/


#undef	C7
#undef	C6
#undef	C5
#undef	C4
#undef	C3
#undef	C2
#undef	C1
#undef	C0

#define	C7	COLOR_PIXEL( vcol[0] >>4 )
#define	C6	COLOR_PIXEL( vcol[1] >>4 )
#define	C5	COLOR_PIXEL( vcol[2] >>4 )
#define	C4	COLOR_PIXEL( vcol[3] >>4 )
#define	C3	COLOR_PIXEL( vcol[0] & 7 )
#define	C2	COLOR_PIXEL( vcol[1] & 7 )
#define	C1	COLOR_PIXEL( vcol[2] & 7 )
#define	C0	COLOR_PIXEL( vcol[3] & 7 )
