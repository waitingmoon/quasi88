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
	    +--+	    +--+	バッファ 1ライン を描画する。
	 8  |  |	 8  |  |
	    +--+	    +--+	vram 処理ライン数は 8。
					フォントデータは vram 1ライン毎に必要

=============================================================================*/
#define	IF_LINE200_OR_EVEN_LINE()	    /* nothing */

#define	DST_DEFINE()		int	dst_w = SCREEN_WIDTH;		\
				TYPE	*dst  = (TYPE *)SCREEN_START;




#define	DST_NEXT_LINE()		dst  += (dst_w);




#define	DST_RESTORE_LINE()	dst  -= CHARA_LINES * dst_w;




#define	DST_NEXT_CHARA()	dst  += 4*COLUMN_SKIP;




#define	DST_NEXT_TOP_CHARA()	dst  += CHARA_LINES * dst_w - 320;




#elif	defined( LINE400 )
/*=============================================================================
  高解像度(400ライン)モードの場合 (以下は、テキスト25行の例)

	  vram		バッファ	vram     2ライン につき
	    +--+	    +--+	バッファ 1ライン を描画する。
	16  |  |	 8  |  |
	    |  |	    +--+	vram 処理ライン数は 16 だが、
	    +--+			  ライン毎にスキップし、8ラインだけ処理
					フォントデータは vram 2ライン毎に必要
=============================================================================*/
#define	IF_LINE200_OR_EVEN_LINE()	if( k&1 ) continue;

#define	DST_DEFINE()		int	dst_w = SCREEN_WIDTH;		\
				TYPE	*dst  = (TYPE *)SCREEN_START;


#define	DST_NEXT_LINE()		dst  += dst_w;


#define	DST_RESTORE_LINE()	dst  -= (CHARA_LINES/2) * dst_w;


#define	DST_NEXT_CHARA()	dst  += 4*COLUMN_SKIP;


#define	DST_NEXT_TOP_CHARA()	dst  += (CHARA_LINES/2) * dst_w - 320;


#endif
