/******************************************************************************
 *	VRAM / TEXT の、前回描画から更新された部分だけを、描画
 *****************************************************************************/

#define	VRAM2SCREEN_DIFF( vram2screen_diff_function )			      \
int	vram2screen_diff_function( void )				      \
{									      \
  int	x0 = COLUMNS-1, x1 = 0, y0 = ROWS-1, y1 = 0;			      \
  int	i, j, k;							      \
  int	changed_line;		/* 更新するラインをビット(0〜COLUMN)で表す */ \
									      \
  unsigned short text, *text_attr= &text_attr_buf[ text_attr_flipflop   ][0]; \
  unsigned short old,  *old_attr = &text_attr_buf[ text_attr_flipflop^1 ][0]; \
  T_GRYPH fnt;				/* フォントの字形 1文字分   */	      \
  int	fnt_idx;			/* フォントの字形 参照位置  */	      \
  bit8	style = 0;			/* フォントの字形 8ドット分 */	      \
  int	tpal;				/* フォントの色コード       */	      \
  int	tcol;				/* フォントの色             */	      \
									      \
  char	*up = &screen_update[0];	/* VRAM更新フラグへのポインタ */      \
  bit32	*src  = main_vram4;		/* VRAMへのポインタ           */      \
  DST_DEFINE()				/* 描画エリアへのポインタ     */      \
  WORK_DEFINE()				/* 処理に必要なワーク         */      \
									      \
									      \
  for( i=0; i<ROWS; i++ ){  /*=============================================*/ \
					/* 1文字単位で描画していくので、   */ \
					/* 文字の (行×列) 分、ループさせる*/ \
				/*-----------------------------------------*/ \
    for( j=0; j<COLUMNS; j+=COLUMN_SKIP ){	/* 40桁なら2文字毎に処理   */ \
									      \
      text = *text_attr;  text_attr += COLUMN_SKIP;	/* テキストコード */  \
      old  = *old_attr;   old_attr  += COLUMN_SKIP;	/* ・属性を取得   */  \
									      \
      if( text != old ){			/* テキスト新旧不一致 ?    */ \
	changed_line = ~(0);			    /* yes 1文字分強制更新 */ \
      }									      \
      else{					    /* no 更新個所チェック */ \
	changed_line = 0;						      \
	for( k=0; k<CHARA_LINES; k++ ){					      \
	  if( up[ k*80 ] || 						      \
	      up[ k*80 + (COLUMN_SKIP-1) ] ){ changed_line |= (1<<k); }	      \
	}								      \
      }									      \
									      \
      if( changed_line ){	    /* いずれかのラインを描画する場合      */ \
        if(i<y0) y0=i;  if(i>y1) y1=i;  if(j<x0) x0=j;  if(j>x1) x1=j;	      \
									      \
	get_font_gryph( text, &fnt, &tpal );	/* フォントの形を取得      */ \
	tcol = COLOR_PIXEL( tpal );					      \
	fnt_idx = 0;							      \
					/*-------------------------------*/   \
	for( k=0; k<CHARA_LINES; k++ ){	    /* 文字のライン数分ループする*/   \
									      \
	  IF_LINE200_OR_EVEN_LINE()	    /* 200lineか400lineで偶数line*/   \
	    style = fnt.b[ fnt_idx++ ];	    /* の時は,フォント8ドット取得*/   \
									      \
	  if( changed_line & (1<<k) ){	    /* このラインを描画する場合*/     \
									      \
	    if      ( style==0xff ){		/* TEXT部 のみを描画   */     \
	      MASK_DOT();			/*                     */     \
	    }else if( style==0x00 ){		/* VRAM部 のみを描画   */     \
	      TRANS_DOT();			/*                     */     \
	    }else{				/* TEXT/VRAM合成描画   */     \
	      STORE_DOT();			/*                     */     \
	    }					/*                     */     \
	    COPY_DOT();				/* ラインの 隙間埋め   */     \
	  }				    /*                         */     \
									      \
	  DST_NEXT_LINE();		    /* 次のライン位置に進む      */   \
	}				/*-------------------------------*/   \
									      \
	DST_RESTORE_LINE();		/* ライン先頭に戻す                */ \
      }				    /*                                     */ \
									      \
									      \
      up  += COLUMN_SKIP;						      \
      src += COLUMN_SKIP;						      \
      DST_NEXT_CHARA();			/* 次の文字位置に進む              */ \
    }				/*-----------------------------------------*/ \
									      \
    up  += (CHARA_LINES-1)* 80;						      \
    src += (CHARA_LINES-1)* 80;						      \
    DST_NEXT_TOP_CHARA();		/* 次の行の先頭文字位置に進む      */ \
  }                         /*=============================================*/ \
									      \
  if( x0 <= x1 ){							      \
    return ( ((x0            )<<24) | (((y0  )*(200/ROWS))<<16) |	      \
             ((x1+COLUMN_SKIP)<< 8) | (((y1+1)*(200/ROWS))    ) );	      \
  }else{								      \
    return -1;								      \
  }									      \
}



/******************************************************************************
 *	VRAM / TEXT の全画面分を、描画
 *****************************************************************************/

#define	VRAM2SCREEN_ALL( vram2screen_all_function )			      \
int	vram2screen_all_function( void )				      \
{									      \
									      \
  int	i, j, k;							      \
									      \
									      \
  unsigned short text, *text_attr= &text_attr_buf[ text_attr_flipflop   ][0]; \
									      \
  T_GRYPH fnt;				/* フォントの字形 1文字分   */	      \
  int	fnt_idx;			/* フォントの字形 参照位置  */	      \
  bit8	style = 0;			/* フォントの字形 8ドット分 */	      \
  int	tpal;				/* フォントの色コード       */	      \
  int	tcol;				/* フォントの色             */	      \
									      \
									      \
  bit32	*src  = main_vram4;		/* VRAMへのポインタ           */      \
  DST_DEFINE()				/* 描画エリアへのポインタ     */      \
  WORK_DEFINE()				/* 処理に必要なワーク         */      \
									      \
									      \
  for( i=0; i<ROWS; i++ ){  /*=============================================*/ \
					/* 1文字単位で描画していくので、   */ \
					/* 文字の (行×列) 分、ループさせる*/ \
				/*-----------------------------------------*/ \
    for( j=0; j<COLUMNS; j+=COLUMN_SKIP ){	/* 40桁なら2文字毎に処理   */ \
									      \
      text = *text_attr;  text_attr += COLUMN_SKIP;	/* テキストコード */  \
									      \
									      \
									      \
									      \
									      \
									      \
									      \
									      \
									      \
									      \
									      \
									      \
									      \
      {				    /* 1文字単位で描画していく             */ \
									      \
									      \
	get_font_gryph( text, &fnt, &tpal );	/* フォントの形を取得      */ \
	tcol = COLOR_PIXEL( tpal );					      \
	fnt_idx = 0;							      \
					/*-------------------------------*/   \
	for( k=0; k<CHARA_LINES; k++ ){	    /* 文字のライン数分ループする*/   \
									      \
	  IF_LINE200_OR_EVEN_LINE()	    /* 200lineか400lineで偶数line*/   \
	    style = fnt.b[ fnt_idx++ ];	    /* の時は,フォント8ドット取得*/   \
									      \
	  {				    /* ライン単位で描画        */     \
									      \
	    if      ( style==0xff ){		/* TEXT部 のみを描画   */     \
	      MASK_DOT();			/*                     */     \
	    }else if( style==0x00 ){		/* VRAM部 のみを描画   */     \
	      TRANS_DOT();			/*                     */     \
	    }else{				/* TEXT/VRAM合成描画   */     \
	      STORE_DOT();			/*                     */     \
	    }					/*                     */     \
	    COPY_DOT();				/* ラインの 隙間埋め   */     \
	  }				    /*                         */     \
									      \
	  DST_NEXT_LINE();		    /* 次のライン位置に進む      */   \
	}				/*-------------------------------*/   \
									      \
	DST_RESTORE_LINE();		/* ライン先頭に戻す                */ \
      }				    /*                                     */ \
									      \
									      \
									      \
      src += COLUMN_SKIP;						      \
      DST_NEXT_CHARA();			/* 次の文字位置に進む              */ \
    }				/*-----------------------------------------*/ \
									      \
									      \
    src += (CHARA_LINES-1)* 80;						      \
    DST_NEXT_TOP_CHARA();		/* 次の行の先頭文字位置に進む      */ \
  }                         /*=============================================*/ \
									      \
									      \
  return ( ((0)<<24) | ((0)<<16) | ((COLUMNS)<<8) | (200) );		      \
}
