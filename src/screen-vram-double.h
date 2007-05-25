#undef	IF_LINE200_OR_EVEN_LINE
#undef	DST_DEFINE
#undef	DST_NEXT_LINE
#undef	DST_RESTORE_LINE
#undef	DST_NEXT_CHARA
#undef	DST_NEXT_TOP_CHARA

#if	defined( LINE200 )
/*=============================================================================
  低解像度(200ライン)モードの場合 (以下は、テキスト25行の例)

	  vram		バッファ	vram     1ライン につき
	    +--+	    +--+	バッファ 4ライン を描画する。
	 8  |  |	32  |  |
	    +--+	    |  |	vram 処理ライン数は 8。
			    |  |	フォントデータは vram 1ライン毎に必要
			    +--+
=============================================================================*/
#define	IF_LINE200_OR_EVEN_LINE()	    /* nothing */

#define	DST_DEFINE()		int	dst_w = SCREEN_WIDTH;		\
				TYPE	*dst  = (TYPE *)SCREEN_START;	\
				TYPE	*dst2 = dst  + dst_w;		\
				TYPE	*dst3 = dst2 + dst_w;		\
				TYPE	*dst4 = dst3 + dst_w;

#define	DST_NEXT_LINE()		dst  += (4*dst_w);			\
				dst2 += (4*dst_w);			\
				dst3 += (4*dst_w);			\
				dst4 += (4*dst_w);

#define	DST_RESTORE_LINE()	dst  -= CHARA_LINES *4 *dst_w;		\
				dst2 -= CHARA_LINES *4 *dst_w;		\
				dst3 -= CHARA_LINES *4 *dst_w;		\
				dst4 -= CHARA_LINES *4 *dst_w;

#define	DST_NEXT_CHARA()	dst  += 16*COLUMN_SKIP;			\
				dst2 += 16*COLUMN_SKIP;			\
				dst3 += 16*COLUMN_SKIP;			\
				dst4 += 16*COLUMN_SKIP;

#define	DST_NEXT_TOP_CHARA()	dst  += CHARA_LINES *4 *dst_w -1280;	\
				dst2 += CHARA_LINES *4 *dst_w -1280;	\
				dst3 += CHARA_LINES *4 *dst_w -1280;	\
				dst4 += CHARA_LINES *4 *dst_w -1280;

#elif	defined( LINE400 )
/*=============================================================================
  高解像度(400ライン)モードの場合 (以下は、テキスト25行の例)

	  vram		バッファ	vram     1ライン につき
	    +--+	    +--+	バッファ 2ライン を描画する。
	16  |  |	32  |  |
	    |  |	    |  |	vram 処理ライン数は 16。
	    +--+	    |  |	フォントデータは vram 2ライン毎に必要
			    +--+	
=============================================================================*/
#define	IF_LINE200_OR_EVEN_LINE()	if( (k&1)==0 )

#define	DST_DEFINE()		int	dst_w = SCREEN_WIDTH;		\
				TYPE	*dst  = (TYPE *)SCREEN_START;	\
				TYPE	*dst2 = dst  + dst_w;

#define	DST_NEXT_LINE()		dst  += (2*dst_w);			\
				dst2 += (2*dst_w);

#define	DST_RESTORE_LINE()	dst  -= CHARA_LINES *2 *dst_w;		\
				dst2 -= CHARA_LINES *2 *dst_w;

#define	DST_NEXT_CHARA()	dst  += 16*COLUMN_SKIP;			\
				dst2 += 16*COLUMN_SKIP;

#define	DST_NEXT_TOP_CHARA()	dst  += CHARA_LINES *2 *dst_w - 1280;	\
				dst2 += CHARA_LINES *2 *dst_w - 1280;

#endif
