/************************************************************************/
/*									*/
/* 画面の表示								*/
/*									*/
/************************************************************************/

#include <string.h>

#include "quasi88.h"
#include "initval.h"
#include "screen.h"
#include "screen-func.h"
#include "graph.h"

#include "crtcdmac.h"
#include "pc88main.h"
#include "memory.h"

#include "status.h"
#include "suspend.h"

#include "intr.h"



PC88_PALETTE_T	vram_bg_palette;	/* OUT[52/54-5B]		*/
PC88_PALETTE_T	vram_palette[8];	/*		各種パレット	*/

byte	sys_ctrl;			/* OUT[30] SystemCtrl		*/
byte	grph_ctrl;			/* OUT[31] GraphCtrl		*/
byte	grph_pile;			/* OUT[53] 重ね合わせ		*/



char	screen_update[ 0x4000*2 ];	/* 画面表示差分更新フラグ	*/
int	screen_update_force = TRUE;	/* 画面強制更新フラグ		*/
int	screen_update_palette = TRUE;	/* パレット更新フラグ		*/



int	frameskip_rate  = DEFAULT_FRAMESKIP;	/* 画面表示の更新間隔	*/
int	monitor_analog  = TRUE;			/* アナログモニター     */

int	use_auto_skip = TRUE;		/* オートフレームスキップを使用する */
int	do_skip_draw = FALSE;		/* スクリーンへの描画をスキップする */
int	already_skip_draw = FALSE;	/* スキップしたか		    */

static	int	frame_counter = 0;	/* フレームスキップ用のカウンタ	*/
static	int	blink_ctrl_cycle   = 1;	/* カーソル表示用のカウンタ	*/
static	int	blink_ctrl_counter = 0;	/*              〃		*/





int	have_mouse_cursor = TRUE;	/* マウスカーソルがあるかどうか	*/
int	hide_mouse = FALSE;		/* マウスを隠すかどうか		*/
int	grab_mouse = FALSE;		/* グラブするかどうか		*/


int	use_interlace = 0;		/* インターレース表示 (-1,0,1)	*/
int	use_half_interp = TRUE;		/* 画面サイズ半分時、色補間する */


const SCREEN_SIZE_TABLE screen_size_tbl[ SCREEN_SIZE_END ] =
{
  {  320,  200,   0,  20, },		/* SCREEN_SIZE_HALF	320x200	*/
  {  640,  400,   0,  40, },		/* SCREEN_SIZE_FULL	640x400	*/
#ifdef	SUPPORT_DOUBLE
  { 1280,  800,   0, 112, },		/* SCREEN_SIZE_DOUBLE	1280x800*/
#endif
};

int	screen_size = SCREEN_SIZE_FULL;	/* 画面サイズ指定		*/
int	screen_size_max = SCREEN_SIZE_END - 1;
int	use_fullscreen = FALSE;		/* 全画面表示指定		*/


int	WIDTH  = 0;			/* 描画バッファ横サイズ		*/
int	HEIGHT = 0;			/* 描画バッファ縦サイズ		*/
int	DEPTH  = 8;			/* 色ビット数	(8 or 16 or 32)	*/
int	SCREEN_W = 0;			/* 画面横サイズ (320/640/1280)	*/
int	SCREEN_H = 0;			/* 画面縦サイズ (200/400/800)	*/

char	*screen_buf;			/* 描画バッファ先頭		*/
char	*screen_start;			/* 画面先頭			*/


int	show_status = TRUE;		/* ステータス表示有無		*/

char	*status_buf;			/* ステータス全域 先頭		*/
char	*status_start[3];		/* ステータス描画 先頭		*/
int	status_sx[3];			/* ステータス描画サイズ		*/
int	status_sy[3];


Ulong	color_pixel[16];			/* 色コード		*/
Ulong	color_half_pixel[16][16];		/* 色補完時の色コード	*/
Ulong	black_pixel;				/* 黒の色コード		*/
Ulong	status_pixel[ STATUS_COLOR_END ];	/* ステータスの色コード	*/







/***********************************************************************
 * テキスト点滅 (カーソルおよび、文字属性の点滅) 処理のワーク設定
 *	CRTC の frameskip_rate, blink_cycle が変更されるたびに呼び出す
 ************************************************************************/
void	blink_ctrl_update( void )
{
  int	wk;

  wk = blink_cycle / frameskip_rate;

  if( wk==0 ||
     !( blink_cycle -wk*frameskip_rate < (wk+1)*frameskip_rate -blink_cycle ) )
    wk++;
  
  blink_ctrl_cycle = wk;
  blink_ctrl_counter = blink_ctrl_cycle;
}



/***********************************************************************
 * フレームカウンタ初期化
 *	次フレームは、必ず表示される。(スキップされない)
 ************************************************************************/
void	reset_frame_counter( void )
{
  frame_counter = 0;
}



/***********************************************************************
 * 描画の際に使用する、実際のパレット情報を引数 syspal にセットする
 ************************************************************************/
void	setup_palette( SYSTEM_PALETTE_T syspal[16] )
{
  int     i;

	/* VRAM の カラーパレット設定   syspal[0]〜[7] */

  if( grph_ctrl & GRPH_CTRL_COLOR ){		/* VRAM カラー */

    if( monitor_analog ){
      for( i=0; i<8; i++ ){
	syspal[i].red   = vram_palette[i].red   * 73 / 2;
	syspal[i].green = vram_palette[i].green * 73 / 2;
	syspal[i].blue  = vram_palette[i].blue  * 73 / 2;
      }
    }else{
      for( i=0; i<8; i++ ){
	syspal[i].red   = vram_palette[i].red   ? 0xff : 0;
	syspal[i].green = vram_palette[i].green ? 0xff : 0;
	syspal[i].blue  = vram_palette[i].blue  ? 0xff : 0;
      }
    }

  }else{					/* VRAM 白黒 */

    if( monitor_analog ){
      syspal[0].red   = vram_bg_palette.red   * 73 / 2;
      syspal[0].green = vram_bg_palette.green * 73 / 2;
      syspal[0].blue  = vram_bg_palette.blue  * 73 / 2;
    }else{
      syspal[0].red   = vram_bg_palette.red   ? 0xff : 0;
      syspal[0].green = vram_bg_palette.green ? 0xff : 0;
      syspal[0].blue  = vram_bg_palette.blue  ? 0xff : 0;
    }
    for( i=1; i<8; i++ ){
      syspal[i].red   = 0;
      syspal[i].green = 0;
      syspal[i].blue  = 0;
    }

  }


	/* TEXT の カラーパレット設定   syspal[8]〜[15] */

  if( grph_ctrl & GRPH_CTRL_COLOR ){		/* VRAM カラー */

    for( i=8; i<16; i++ ){				/* TEXT 白黒の場合は */
      syspal[i].red   = (i&0x02) ? 0xff : 0;		/* 黒=[8],白=[15] を */
      syspal[i].green = (i&0x04) ? 0xff : 0;		/* 使うので問題なし  */
      syspal[i].blue  = (i&0x01) ? 0xff : 0;
    }

  }else{					/* VRAM 白黒   */

    if( misc_ctrl & MISC_CTRL_ANALOG ){			/* アナログパレット時*/

      if( monitor_analog ){
	for( i=8; i<16; i++ ){
	  syspal[i].red   = vram_palette[i&0x7].red   * 73 / 2;
	  syspal[i].green = vram_palette[i&0x7].green * 73 / 2;
	  syspal[i].blue  = vram_palette[i&0x7].blue  * 73 / 2;
	}
      }else{
	for( i=8; i<16; i++ ){
	  syspal[i].red   = vram_palette[i&0x7].red   ? 0xff : 0;
	  syspal[i].green = vram_palette[i&0x7].green ? 0xff : 0;
	  syspal[i].blue  = vram_palette[i&0x7].blue  ? 0xff : 0;
	}
      }

    }else{						/* デジタルパレット時*/
      for( i=8; i<16; i++ ){
	syspal[i].red   = (i&0x02) ? 0xff : 0;
	syspal[i].green = (i&0x04) ? 0xff : 0;
	syspal[i].blue  = (i&0x01) ? 0xff : 0;
      }
    }

  }
}



/*======================================================================
 * テキストVRAMのアトリビュートを専用ワークに設定する
 *
 *	バッファは2個あり、交互に切替えて使用する。
 *	画面書き換えの際は、この2個のバッファを比較し、変化の
 *	あった部分だけを更新する。
 *
 *	ワークは、16bitで、上位8bitが文字コード、下位は属性。
 *		色、グラフィックモード、アンダーライン、
 *		アッパーライン、シークレット、リバース
 *		+---------------------+--+--+--+--+--+--+--+--+
 *		|    ASCII 8bit       |Ｇ|Ｒ|Ｂ|GR|LO|UP|SC|RV|
 *		+---------------------+--+--+--+--+--+--+--+--+
 *	BLINK属性は、点灯時は無視、消灯時はシークレット。
 *
 *	さらに、シークレット属性の場合は 文字コードを 0 に置換する。
 *	(文字コード==0は無条件で空白としているので)
 *		+---------------------+--+--+--+--+--+--+--+--+
 *	     →	|    ASCII == 0       |Ｇ|Ｒ|Ｂ|０|LO|UP|０|RV|
 *		+---------------------+--+--+--+--+--+--+--+--+
 *	        グラフィックモードとシークレット属性も消してもOKだが、
 *		アンダー、アッパーライン、リバースは有効なので残す。
 *
 *======================================================================*/

int	text_attr_flipflop = 0;
Ushort	text_attr_buf[2][2048];		/* アトリビュート情報	*/
			/* ↑ 80文字x25行=2000で足りるのだが、	*/
			/* 余分に使うので、多めに確保する。	*/
				   

static	void	make_text_attr( void )
{
  int		global_attr  = (ATTR_G|ATTR_R|ATTR_B);
  int		global_blink = FALSE;
  int		i, j, tmp;
  int		column, attr, attr_rest;
  word		char_start_addr, attr_start_addr;
  word		c_addr, a_addr;
  Ushort	*text_attr = &text_attr_buf[ text_attr_flipflop ][0];


	/* CRTC も DMAC も止まっている場合 */
	/*  (文字もアトリビュートも無効)   */

  if( text_display==TEXT_DISABLE ){		/* ASCII=0、白色、装飾なし */
    for( i=0; i<CRTC_SZ_LINES; i++ ){		/* で初期化する。	   */
      for( j=0; j<CRTC_SZ_COLUMNS; j++ ){
	*text_attr ++ =  (ATTR_G|ATTR_R|ATTR_B);
      }
    }
    return;			/* 全画面反転やカーソルもなし。すぐに戻る  */
  }



	/* ノン・トランスペアレント型の場合 */
	/* (1文字置きに、VRAM、ATTR がある) */

			/* ……… ？詳細不明 				*/
			/*	CRTCの設定パターンからして、さらに行の	*/
			/*	最後に属性がある場合もありえそうだが…?	*/

  if( crtc_attr_non_separate ){

    char_start_addr = text_dma_addr.W;
    attr_start_addr = text_dma_addr.W + 1;

    for( i=0; i<crtc_sz_lines; i++ ){

      c_addr	= char_start_addr;
      a_addr	= attr_start_addr;

      char_start_addr += crtc_byte_per_line;
      attr_start_addr += crtc_byte_per_line;

      for( j=0; j<CRTC_SZ_COLUMNS; j+=2 ){		/* 属性を内部コードに*/
	attr = main_ram[ a_addr ];			/* 変換し、属性ワーク*/
	a_addr += 2;					/* を全て埋める。    */
	global_attr =( global_attr & COLOR_MASK ) |
		     ((attr &  MONO_GRAPH) >> 3 ) |
		     ((attr & (MONO_UNDER|MONO_UPPER|MONO_REVERSE))>>2) |
		     ((attr &  MONO_SECRET) << 1 );

					/* BLINKのOFF時はSECRET扱い    */
	if( (attr & MONO_BLINK) && ((blink_counter&0x03)==0) ){
	  global_attr |= ATTR_SECRET;
	}

	*text_attr ++ = ((Ushort)main_ram[ c_addr ++ ] << 8 ) | global_attr;
	*text_attr ++ = ((Ushort)main_ram[ c_addr ++ ] << 8 ) | global_attr;

      }

      if( crtc_skip_line ){
	if( ++i < crtc_sz_lines ){
	  for( j=0; j<CRTC_SZ_COLUMNS; j++ ){
	    *text_attr ++ =  global_attr | ATTR_SECRET;
	  }
	}
      }

    }
    for( ; i<CRTC_SZ_LINES; i++ ){		/* 残りの行は、SECRET */
      for( j=0; j<CRTC_SZ_COLUMNS; j++ ){	/*  (24行設定対策)    */
	*text_attr ++ =  global_attr | ATTR_SECRET;
      }
    }

  }else{

	/* トランスペアレント型の場合 */
	/* (行の最後に、ATTRがある)   */

    char_start_addr = text_dma_addr.W;
    attr_start_addr = text_dma_addr.W + crtc_sz_columns;

    for( i=0; i<crtc_sz_lines; i++ ){			/* 行単位で属性作成 */

      c_addr	= char_start_addr;
      a_addr	= attr_start_addr;

      char_start_addr += crtc_byte_per_line;
      attr_start_addr += crtc_byte_per_line;


      attr_rest = 0;						/*属性初期化 */
      for( j=0; j<=CRTC_SZ_COLUMNS; j++ ) text_attr[j] = 0;	/* [0]〜[80] */


      for( j=0; j<crtc_sz_attrs; j++ ){			/* 属性を指定番目の */
	column = main_ram[ a_addr++ ];			/* 配列に格納       */
	attr   = main_ram[ a_addr++ ];

	if( j!=0 && column==0    ) column = 0x80;		/* 特殊処理?*/
	if( j==0 && column==0x80 ){column = 0;
/*				   global_attr = (ATTR_G|ATTR_R|ATTR_B);
				   global_blink= FALSE;  }*/}

	if( column==0x80  &&  !attr_rest ){			/* 8bit目は */
	  attr_rest = attr | 0x100;				/* 使用済の */
	}							/* フラグ   */
	else if( column <= CRTC_SZ_COLUMNS  &&  !text_attr[ column ] ){
	  text_attr[ column ] = attr | 0x100;
	}
      }


      if( !text_attr[0] && attr_rest ){			/* 指定桁-1まで属性が*/
	for( j=CRTC_SZ_COLUMNS; j; j-- ){		/* 有効、という場合の*/
	  if( text_attr[j] ){				/* 処理。(指定桁以降 */
	    tmp          = text_attr[j];		/* 属性が有効、という*/
	    text_attr[j] = attr_rest;			/* ふうに並べ替える) */
	    attr_rest    = tmp;
	  }
	}
	text_attr[0] = attr_rest;
      }


      for( j=0; j<CRTC_SZ_COLUMNS; j++ ){		/* 属性を内部コードに*/
							/* 変換し、属性ワーク*/
	if( ( attr = *text_attr ) ){			/* を全て埋める。    */
	  if( crtc_attr_color ){
	    if( attr & COLOR_SWITCH ){
	      global_attr =( global_attr & MONO_MASK ) |
			   ( attr & (COLOR_G|COLOR_R|COLOR_B|COLOR_GRAPH));
	    }else{
	      global_attr =( global_attr & (COLOR_MASK|ATTR_GRAPH) ) |
			   ((attr & (MONO_UNDER|MONO_UPPER|MONO_REVERSE))>>2) |
			   ((attr &  MONO_SECRET) << 1 );
	      global_blink= (attr & MONO_BLINK);
	    }
	  }else{
	    global_attr =( global_attr & COLOR_MASK ) |
			 ((attr &  MONO_GRAPH) >> 3 ) |
			 ((attr & (MONO_UNDER|MONO_UPPER|MONO_REVERSE))>>2) |
			 ((attr &  MONO_SECRET) << 1 );
	    global_blink= (attr & MONO_BLINK);
	  }
					/* BLINKのOFF時はSECRET扱い    */
	  if( global_blink && ((blink_counter&0x03)==0) ){
	    global_attr =  global_attr | ATTR_SECRET;
	  }
	}

	*text_attr ++ = ((Ushort)main_ram[ c_addr ++ ] << 8 ) | global_attr;

      }

      if( crtc_skip_line ){				/* 1行飛ばし指定時は*/
	if( ++i < crtc_sz_lines ){			/* 次の行をSECRETで */
	  for( j=0; j<CRTC_SZ_COLUMNS; j++ ){		/* 埋める。         */
	    *text_attr ++ =  global_attr | ATTR_SECRET;
	  }
	}
      }

    }

    for( ; i<CRTC_SZ_LINES; i++ ){		/* 残りの行は、SECRET */
      for( j=0; j<CRTC_SZ_COLUMNS; j++ ){	/*  (24行設定対策)    */
	*text_attr ++ =  global_attr | ATTR_SECRET;
      }
    }

  }



	/* CRTC や DMAC は動いているけど、 テキストが非表示 */
	/* でVRAM白黒の場合 (アトリビュートの色だけが有効)  */

  if( text_display==TEXT_ATTR_ONLY ){

    text_attr = &text_attr_buf[ text_attr_flipflop ][0];

    for( i=0; i<CRTC_SZ_LINES; i++ ){
      for( j=0; j<CRTC_SZ_COLUMNS; j++ ){
	*text_attr ++ &=  (ATTR_G|ATTR_R|ATTR_B);
      }
    }
    return;			/* 全画面反転やカーソルは不要。ここでに戻る  */
  }




		/* 全体反転処理 */

  if( crtc_reverse_display && (grph_ctrl & GRPH_CTRL_COLOR)){
    text_attr = &text_attr_buf[ text_attr_flipflop ][0];
    for( i=0; i<CRTC_SZ_LINES; i++ ){
      for( j=0; j<CRTC_SZ_COLUMNS; j++ ){
	*text_attr ++ ^= ATTR_REVERSE;
      }
    }
  }

		/* カーソル表示処理 */

  if( 0 <= crtc_cursor[0] && crtc_cursor[0] < crtc_sz_columns &&
      0 <= crtc_cursor[1] && crtc_cursor[1] < crtc_sz_lines   ){
    if( !crtc_cursor_blink || (blink_counter&0x01) ){
      text_attr_buf[ text_attr_flipflop ][ crtc_cursor[1]*80 + crtc_cursor[0] ]
							^= crtc_cursor_style;
    }
  }


	/* シークレット属性処理 (文字コード 0x00 に置換) */

  text_attr = &text_attr_buf[ text_attr_flipflop ][0];
  for( i=0; i<CRTC_SZ_LINES; i++ ){
    for( j=0; j<CRTC_SZ_COLUMNS; j++ ){
      if( *text_attr & ATTR_SECRET ){		/* SECRET 属性は、コード00に */
	*text_attr &= (COLOR_MASK|ATTR_UPPER|ATTR_LOWER|ATTR_REVERSE);
      }
      text_attr ++;
    }
  }

}






/***********************************************************************
 * 指定された文字コード(属性・文字)より、フォント字形データを生成する
 *
 *	int attr	… 文字コード。 text_attr_buf[]の値である。
 *	T_GRYPH *gryph	… gryph->b[0]〜[7] に フォントのビットマップが
 *			   格納される。(20行時は、b[0]〜[9]に格納)
 *	int *color	… フォントの色が格納される。値は、 8〜15
 *
 *	字形データは、char 8〜10個なのだが、姑息な高速化のために long で
 *	アクセスしている。そのため、 T_GRYPH という妙な型を使っている。
 *	(大丈夫・・・だよね？)
 *
 *	注)	アトリビュート情報が必要なので、
 *		予め make_text_attr_table( ) を呼んでおくこと
 ************************************************************************/

void	get_font_gryph( int attr, T_GRYPH *gryph, int *color )
{
  int	chara;
  bit32	*src;
  bit32	*dst = (bit32 *)gryph;

  *color = ((attr & COLOR_MASK) >> 5) | 8;


  if( ( attr & ~(COLOR_MASK|ATTR_REVERSE) )==0 ){

    if( ( attr & ATTR_REVERSE ) == 0 ){		/* 空白フォント時 */

      *dst++ = 0;
      *dst++ = 0;
      *dst   = 0;

    }else{					/* ベタフォント時 */

      *dst++ = 0xffffffff;
      *dst++ = 0xffffffff;
      *dst   = 0xffffffff;
    }

  }else{					/* 通常フォント時 */

    chara = attr >> 8;

    if( attr & ATTR_GRAPH )
      src = (bit32 *)&font_rom[ (chara | 0x100)*8 ];
    else
      src = (bit32 *)&font_rom[ (chara        )*8 ];

					/* フォントをまず内部ワークにコピー */
    *dst++ = *src++;
    *dst++ = *src;
    *dst   = 0;

					/* 属性により内部ワークフォントを加工*/
    if( attr & ATTR_UPPER ) gryph->b[ 0 ] |= 0xff;
    if( attr & ATTR_LOWER ) gryph->b[ crtc_font_height-1 ] |= 0xff;
    if( attr & ATTR_REVERSE ){
      dst -= 2;
      *dst++ ^= 0xffffffff;
      *dst++ ^= 0xffffffff;
      *dst   ^= 0xffffffff;
    }
  }
}





/***********************************************************************
 * GVRAM/TVRAMを画像バッファに転送する関数の、リストを作成
 *	この関数は、 bpp ・ サイズ ・ エフェクト の変更時に呼び出す。
 ************************************************************************/

static	int  ( *vram2screen_list[4][4][2] )( void );
static	void ( *screen_buf_init_p )( void );

static	int  ( *menu2screen_p )( void );

static	void ( *status2screen_p )( int kind, byte pixmap[], int w, int h );
static	void ( *status_buf_init_p )( void );
static	void ( *status_buf_clear_p )( void );


typedef	int		( *V2S_FUNC_TYPE )( void );
typedef	V2S_FUNC_TYPE	V2S_FUNC_LIST[4][4][2];

INLINE	void	set_vram2screen_list( void )
{
  V2S_FUNC_LIST *list = NULL;


  if( DEPTH <= 8 ){		/* ----------------------------------------- */

#ifdef	SUPPORT_8BPP
    switch( now_screen_size ){
    case SCREEN_SIZE_FULL:
      if     ( use_interlace == 0 ){ list = &vram2screen_list_F_N__8; }
      else if( use_interlace >  0 ){ list = &vram2screen_list_F_I__8; }
      else                         { list = &vram2screen_list_F_S__8; }
      menu2screen_p = menu2screen_F_N__8;
      break;
    case SCREEN_SIZE_HALF:
      if( now_half_interp )   { list = &vram2screen_list_H_P__8;
				menu2screen_p = menu2screen_H_P__8; }
      else                    { list = &vram2screen_list_H_N__8;
				menu2screen_p = menu2screen_H_N__8; }
      break;
#ifdef	SUPPORT_DOUBLE
    case SCREEN_SIZE_DOUBLE:
      if     ( use_interlace == 0 ){ list = &vram2screen_list_D_N__8; }
      else if( use_interlace >  0 ){ list = &vram2screen_list_D_I__8; }
      else                         { list = &vram2screen_list_D_S__8; }
      menu2screen_p = menu2screen_D_N__8;
      break;
#endif
    }
    screen_buf_init_p = screen_buf_init__8;
    status2screen_p   = status2screen__8;
    status_buf_init_p = status_buf_init__8;
    status_buf_clear_p= status_buf_clear__8;
#else
    fprintf( stderr, "Error! This version is not support 8bpp !\n" );
    exit(1);
#endif

  } else if( DEPTH <= 16 ){	/* ----------------------------------------- */

#ifdef	SUPPORT_16BPP
    switch( now_screen_size ){
    case SCREEN_SIZE_FULL:
      if     ( use_interlace == 0 ){ list = &vram2screen_list_F_N_16; }
      else if( use_interlace >  0 ){ list = &vram2screen_list_F_I_16; }
      else                         { list = &vram2screen_list_F_S_16; }
      menu2screen_p = menu2screen_F_N_16;
      break;
    case SCREEN_SIZE_HALF:
      if( now_half_interp )   { list = &vram2screen_list_H_P_16;
				menu2screen_p = menu2screen_H_P_16; }
      else                    { list = &vram2screen_list_H_N_16;
				menu2screen_p = menu2screen_H_N_16; }
      break;
#ifdef	SUPPORT_DOUBLE
    case SCREEN_SIZE_DOUBLE:
      if     ( use_interlace == 0 ){ list = &vram2screen_list_D_N_16; }
      else if( use_interlace >  0 ){ list = &vram2screen_list_D_I_16; }
      else                         { list = &vram2screen_list_D_S_16; }
      menu2screen_p = menu2screen_D_N_16;
      break;
#endif
    }
    screen_buf_init_p = screen_buf_init_16;
    status2screen_p   = status2screen_16;
    status_buf_init_p = status_buf_init_16;
    status_buf_clear_p= status_buf_clear_16;
#else
    fprintf( stderr, "Error! This version is not support 16bpp !\n" );
    exit(1);
#endif

  }else if( DEPTH <= 32 ){	/* ----------------------------------------- */

#ifdef	SUPPORT_32BPP
    switch( now_screen_size ){
    case SCREEN_SIZE_FULL:
      if     ( use_interlace == 0 ){ list = &vram2screen_list_F_N_32; }
      else if( use_interlace >  0 ){ list = &vram2screen_list_F_I_32; }
      else                         { list = &vram2screen_list_F_S_32; }
      menu2screen_p = menu2screen_F_N_32;
      break;
    case SCREEN_SIZE_HALF:
      if( now_half_interp )   { list = &vram2screen_list_H_P_32;
				menu2screen_p = menu2screen_H_P_32; }
      else                    { list = &vram2screen_list_H_N_32;
				menu2screen_p = menu2screen_H_N_32; }
      break;
#ifdef	SUPPORT_DOUBLE
    case SCREEN_SIZE_DOUBLE:
      if     ( use_interlace == 0 ){ list = &vram2screen_list_D_N_32; }
      else if( use_interlace >  0 ){ list = &vram2screen_list_D_I_32; }
      else                         { list = &vram2screen_list_D_S_32; }
      menu2screen_p = menu2screen_D_N_32;
      break;
#endif
    }
    screen_buf_init_p = screen_buf_init_32;
    status2screen_p   = status2screen_32;
    status_buf_init_p = status_buf_init_32;
    status_buf_clear_p= status_buf_clear_32;
#else
    fprintf( stderr, "Error! This version is not support 32bpp !\n" );
    exit(1);
#endif

  }

  memcpy( vram2screen_list, list, sizeof(vram2screen_list) );
}



/***********************************************************************
 * GVRAM/TVRAM を screen_buf に転送する
 *
 *	int method == V_DIF … screen_update に基づき、差分だけを転送
 *		   == V_ALL … 画面すべてを転送
 *
 *	戻り値     == -1    … 転送なし (画面に変化なし)
 *		   != -1    … 上位から 8ビットずつに、x0, y0, x1, y1 の
 *			       4個の unsigned 値がセットされる。ここで、
 *				    ( x0*8, y0*2 )-( x1*8, y1*2 )
 *			       で表される範囲が、転送した領域となる。
 *
 *	予め、 set_vram2screen_list で関数リストを生成しておくこと
 ************************************************************************/

static	int	vram2screen( int method )
{
  int vram_mode, text_mode;

  if( sys_ctrl & SYS_CTRL_80 ){			/* テキストの行・桁 */
    if( CRTC_SZ_LINES == 25 ){ text_mode = V_80x25; }
    else                     { text_mode = V_80x20; }
  }else{
    if( CRTC_SZ_LINES == 25 ){ text_mode = V_40x25; }
    else                     { text_mode = V_40x20; }
  }

  if( grph_ctrl & GRPH_CTRL_VDISP ){		/* VRAM 表示する */

    if( grph_ctrl & GRPH_CTRL_COLOR ){			/* カラー */
      vram_mode = V_COLOR;
    }else{
      if( grph_ctrl & GRPH_CTRL_200 ){			/* 白黒 */
	vram_mode = V_MONO;
      }else{						/* 400ライン */
	vram_mode = V_HIRESO;
      }
    }

  }else{					/* VRAM 表示しない */

    vram_mode = V_UNDISP;
  }


  return (vram2screen_list[ vram_mode ][ text_mode ][ method ])();
}



/***********************************************************************
 * screen_buf の初期化
 *	screen_buf を黒でクリア(ボーダー部分も含む)し、
 *	ステータス表示の際はそれも初期画像表示する。
 *
 *	この関数は、ウインドウ生成時や、表示ロジックが変更された時
 *	( use_interlace, use_half_interp などの変数変更時 ) に呼び出す。
 ************************************************************************/
void	screen_buf_init( void )
{
  int     i;
  SYSTEM_PALETTE_T	syspal[16];

  for( i=0; i<16; i++ ){		/* ダミーの色でともかく初期化 */
    syspal[i].red   = (i&2) ? 255 : 0;
    syspal[i].green = (i&4) ? 255 : 0;
    syspal[i].blue  = (i&1) ? 255 : 0;
  }
  trans_palette( syspal );

					/* 転送・表示関数のリストを設定 */
  set_vram2screen_list();

  (screen_buf_init_p)();		/* 画面全クリア(ボーダー含) */

  if( now_status ){			/* ステータスも全クリア */
    (status_buf_init_p)();
  }else{
    (status_buf_clear_p)();
  }

  put_image_all();
}



/***********************************************************************
 * status_buf の初期化
 *	status_buf に初期画像表示(表示時)、ないし 黒でクリア(非表示時)
 *
 *	この関数は、ステータス表示・非表示切替時に呼び出す。
 ************************************************************************/
void	status_buf_init( void )
{
  if( now_status ){
    (status_buf_init_p)();
  }else{
    (status_buf_clear_p)();
  }

  put_image( -1, -1, -1, -1, TRUE, TRUE, TRUE );
}



/***********************************************************************
 * イメージ転送 (表示)
 *
 *	この関数は、表示タイミング (約 1/60秒毎) に呼び出される。
 *		VRAM の表示は指定されたフレーム毎に行う。
 *		ステータスの表示は、常時行う。
 ************************************************************************/

/*
 *
 */
INLINE	int	vram2screen_core( void )
{
  int rect = -1;
  SYSTEM_PALETTE_T syspal[16];

  if( screen_update_palette ){		/* パレットをシステムに転送 */
    setup_palette( syspal );
    trans_palette( syspal );
    screen_update_palette = FALSE; 
  }

	/* VRAM更新フラグ screen_update の例外処理			*/
	/*	VRAM非表示の場合、更新フラグは意味無いのでクリアする	*/
	/*	400ラインの場合、更新フラグを画面下半分にも拡張する	*/

  if( screen_update_force == FALSE ){
    if( ! (grph_ctrl & GRPH_CTRL_VDISP) ){			/* 非表示    */
      memset( screen_update, 0, sizeof( screen_update )/2 );
    }
    if( ! (grph_ctrl & (GRPH_CTRL_COLOR|GRPH_CTRL_200)) ){	/* 400ライン */
      memcpy( &screen_update[80*200], screen_update, 80*200 );
    }
  }

  make_text_attr();			/* TVRAM の 属性一覧作成	  */
					/* VRAM/TEXT → screen_buf 転送	  */
  rect = vram2screen( screen_update_force ? V_ALL : V_DIF );

  text_attr_flipflop ^= 1;		/* VRAM更新フラグ等をクリア */
  memset( screen_update, 0, sizeof( screen_update ));
  screen_update_force = FALSE;

  return rect;
}

/*
 *
 */
INLINE	int	status2screen_core( int force_update )
{
  int i;
  int flag;

  flag = status_update( force_update );

  for( i=0; i<3; i++ ){
    if( flag & (1<<i) ){
      (status2screen_p)( i, status_info[i].pixmap,
			 status_info[i].w, status_info[i].h );
    }
  }
  return flag;
}



void	draw_screen( void )
{
  int rect = -1;
  int flag;

  flag = status2screen_core( FALSE );

  if( (frame_counter%frameskip_rate)==0 ){	/* 指定フレームおきに処理 */

	/* ウエイト無効の場合、オートスキップは無視 */

    if( no_wait || !use_auto_skip || !do_skip_draw ){

      rect = vram2screen_core();	/* VRAM→バッファ 転送処理	*/

    }else{

      already_skip_draw = TRUE;		/* 非転送時は、スキップフラグON	*/

    }

    if( --blink_ctrl_counter == 0 ){	/* カーソル点滅ワーク更新	*/
      blink_ctrl_counter = blink_ctrl_cycle;
      blink_counter ++;
    }
  }

  ++ frame_counter;

  if( rect != -1 ){			/* screen_buf 表示		*/
    put_image( ((rect>>24)     ) * 8, ((rect>>16)&0xff) * 2,
	       ((rect>> 8)&0xff) * 8, ((rect    )&0xff) * 2,
	       (flag & 1), (flag & 2), (flag & 4) );
  }
  else if( flag ){
    put_image( -1, -1, -1, -1,
	       (flag & 1), (flag & 2), (flag & 4) );
  }
}



/***********************************************************************
 * イメージ転送 (強制再描画)
 ************************************************************************/
void	draw_screen_force( void )
{
  int rect;
  int flag;

  flag = status2screen_core( TRUE );


  screen_update_force   = TRUE;
  screen_update_palette = TRUE;

  rect = vram2screen_core();


  if( rect != -1 ){
    put_image( ((rect>>24)     ) * 8, ((rect>>16)&0xff) * 2,
	       ((rect>> 8)&0xff) * 8, ((rect    )&0xff) * 2,
	       (flag & 1), (flag & 2), (flag & 4) );
  }
}




/***********************************************************************
 * イメージ転送 (ステータスのみ)
 *
 *	エミュレーション中以外で、ステータスのみを更新するときに呼び出す
 *	エミュレーション中は、 draw_screen() を呼び出せばよい。 
 ************************************************************************/

void	draw_status( void )
{
  int flag;

  flag = status2screen_core( FALSE );


  if( flag ){
    put_image( -1, -1, -1, -1,
	       (flag & 1), (flag & 2), (flag & 4) );
  }
}






/***********************************************************************
 * イメージ転送 (メニュー画面用)
 ************************************************************************/
void	draw_menu_screen( void )
{
  int rect = -1;

  rect = (menu2screen_p)();

  if( rect != -1 ){
/*
  int x0=rect>>24,y0=(rect>>16)&0xff,x1=(rect>>8)&0xff,y1 =rect&0xff;
  printf("%d %d (%d %d)\n",x0,y0,x1-x0,y1-y0);fflush(stdout);
*/
    put_image( ((rect>>24)     ) * 8, ((rect>>16)&0xff) * 16,
	       ((rect>> 8)&0xff) * 8, ((rect    )&0xff) * 16,  -1, -1, -1 );
  }
}



/***********************************************************************
 * イメージ転送 (メニュー画面用  強制再描画)
 ************************************************************************/
void	draw_menu_screen_force( void )
{
  int rect = -1;
  int flag;

  flag = status2screen_core( TRUE );


  rect = (menu2screen_p)();

  if( rect != -1 ){
    put_image( ((rect>>24)     ) * 8, ((rect>>16)&0xff) * 16,
	       ((rect>> 8)&0xff) * 8, ((rect    )&0xff) * 16,
	       (flag & 1), (flag & 2), (flag & 4) );
  }
}




/***********************************************************************
 * ステートロード／ステートセーブ
 ************************************************************************/

#define	SID	"SCRN"

static	T_SUSPEND_W	suspend_screen_work[]=
{
  { TYPE_CHAR,	&vram_bg_palette.blue,	},
  { TYPE_CHAR,	&vram_bg_palette.red,	},
  { TYPE_CHAR,	&vram_bg_palette.green,	},

  { TYPE_CHAR,	&vram_palette[0].blue,	},
  { TYPE_CHAR,	&vram_palette[0].red,	},
  { TYPE_CHAR,	&vram_palette[0].green,	},
  { TYPE_CHAR,	&vram_palette[1].blue,	},
  { TYPE_CHAR,	&vram_palette[1].red,	},
  { TYPE_CHAR,	&vram_palette[1].green,	},
  { TYPE_CHAR,	&vram_palette[2].blue,	},
  { TYPE_CHAR,	&vram_palette[2].red,	},
  { TYPE_CHAR,	&vram_palette[2].green,	},
  { TYPE_CHAR,	&vram_palette[3].blue,	},
  { TYPE_CHAR,	&vram_palette[3].red,	},
  { TYPE_CHAR,	&vram_palette[3].green,	},
  { TYPE_CHAR,	&vram_palette[4].blue,	},
  { TYPE_CHAR,	&vram_palette[4].red,	},
  { TYPE_CHAR,	&vram_palette[4].green,	},
  { TYPE_CHAR,	&vram_palette[5].blue,	},
  { TYPE_CHAR,	&vram_palette[5].red,	},
  { TYPE_CHAR,	&vram_palette[5].green,	},
  { TYPE_CHAR,	&vram_palette[6].blue,	},
  { TYPE_CHAR,	&vram_palette[6].red,	},
  { TYPE_CHAR,	&vram_palette[6].green,	},
  { TYPE_CHAR,	&vram_palette[7].blue,	},
  { TYPE_CHAR,	&vram_palette[7].red,	},
  { TYPE_CHAR,	&vram_palette[7].green,	},

  { TYPE_BYTE,	&sys_ctrl,		},
  { TYPE_BYTE,	&grph_ctrl,		},
  { TYPE_BYTE,	&grph_pile,		},

  { TYPE_INT,	&frameskip_rate,	},
  { TYPE_INT,	&monitor_analog,	},
  { TYPE_INT,	&use_auto_skip,		},
/*{ TYPE_INT,	&frame_counter,		},	初期値でも問題ないだろう */
/*{ TYPE_INT,	&blink_ctrl_cycle,	},	初期値でも問題ないだろう */
/*{ TYPE_INT,	&blink_ctrl_counter,	},	初期値でも問題ないだろう */

  { TYPE_INT,	&use_interlace,		},
  { TYPE_INT,	&use_half_interp,	},

  { TYPE_END,	0			},
};


int	statesave_screen( void )
{
  if( statesave_table( SID, suspend_screen_work ) == STATE_OK ) return TRUE;
  else                                                          return FALSE;
}

int	stateload_screen( void )
{
  if( stateload_table( SID, suspend_screen_work ) == STATE_OK ) return TRUE;
  else                                                          return FALSE;
}
























/* デバッグ用の関数 */
void attr_misc(int line)
{
int i;

  text_attr_flipflop ^= 1;    
  for(i=0;i<80;i++){
    printf("%02X[%02X] ",
    text_attr_buf[text_attr_flipflop][line*80+i]>>8,
    text_attr_buf[text_attr_flipflop][line*80+i]&0xff );
  }
return;
  for(i=0;i<80;i++){
    printf("%c[%02X] ",
    text_attr_buf[text_attr_flipflop][9*80+i]>>8,
    text_attr_buf[text_attr_flipflop][9*80+i]&0xff );
  }
  for(i=0;i<80;i++){
    printf("%c[%02X] ",
    text_attr_buf[text_attr_flipflop][10*80+i]>>8,
    text_attr_buf[text_attr_flipflop][10*80+i]&0xff );
  }
  for(i=0;i<80;i++){
    printf("%c[%02X] ",
    text_attr_buf[text_attr_flipflop][11*80+i]>>8,
    text_attr_buf[text_attr_flipflop][11*80+i]&0xff );
  }
  printf("\n");
  for(i=0;i<80;i++){
    printf("%c[%02X] ",
    text_attr_buf[text_attr_flipflop][12*80+i]>>8,
    text_attr_buf[text_attr_flipflop][12*80+i]&0xff );
  }
  printf("\n");
  for(i=0;i<80;i++){
    printf("%c[%02X] ",
    text_attr_buf[text_attr_flipflop][13*80+i]>>8,
    text_attr_buf[text_attr_flipflop][13*80+i]&0xff );
  }
  printf("\n");
  for(i=0;i<80;i++){
    printf("%c[%02X] ",
    text_attr_buf[text_attr_flipflop][14*80+i]>>8,
    text_attr_buf[text_attr_flipflop][14*80+i]&0xff );
  }
#if 0
  for(i=0;i<80;i++){
    printf("%c[%02X] ",
    text_attr_buf[0][15*80+i]>>8,
    text_attr_buf[0][15*80+i]&0xff );
  }
  printf("\n");
  for(i=0;i<80;i++){
    printf("%c[%02X] ",
    text_attr_buf[0][16*80+i]>>8,
    text_attr_buf[0][16*80+i]&0xff );
  }
  printf("\n");
  for(i=0;i<80;i++){
    printf("%c[%02X] ",
    text_attr_buf[0][17*80+i]>>8,
    text_attr_buf[0][17*80+i]&0xff );
  }
  printf("\n");
  for(i=0;i<80;i++){
    printf("%c[%02X] ",
    text_attr_buf[0][18*80+i]>>8,
    text_attr_buf[0][18*80+i]&0xff );
  }
  printf("\n");
  for(i=0;i<80;i++){
    printf("%c[%02X] ",
    text_attr_buf[0][19*80+i]>>8,
    text_attr_buf[0][19*80+i]&0xff );
  }
  printf("\n");
#endif
}
