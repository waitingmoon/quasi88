/************************************************************************/
/*									*/
/* CRTC と DMAC の処理							*/
/*									*/
/************************************************************************/

#include "quasi88.h"
#include "crtcdmac.h"

#include "screen.h"
#include "suspend.h"


int		text_display = TEXT_ENABLE;	/* テキスト表示フラグ	*/

int		blink_cycle;		/* 点滅の周期	8/16/24/32	*/
int		blink_counter = 0;	/* 点滅制御カウンタ		*/

int		dma_wait_count = 0;	/* DMAで消費するサイクル数	*/


static	int	crtc_command;
static	int	crtc_param_num;

static	byte	crtc_status;
static	byte	crtc_light_pen[2];
static	byte	crtc_load_cursor_position;


	int	crtc_active;		/* CRTCの状態 0:CRTC作動 1:CRTC停止 */
	int	crtc_intr_mask;		/* CRTCの割込マスク ==3 で表示	    */
	int	crtc_cursor[2];		/* カーソル位置 非表示の時は(-1,-1) */
	byte	crtc_format[5];		/* CRTC 期化時のフォーマット	    */


	int	crtc_reverse_display;	/* 真…反転表示 / 偽…通常表示	*/

	int	crtc_skip_line;		/* 真…1行飛ばし表示 / 偽…通常 */
	int	crtc_cursor_style;	/* ブロック / アンダライン	*/
	int	crtc_cursor_blink;	/* 真…点滅する 偽…点滅しない	*/
	int	crtc_attr_non_separate;	/* 真…VRAM、ATTR が交互に並ぶ	*/
	int	crtc_attr_color;	/* 真…カラー 偽…白黒		*/
	int	crtc_attr_non_special;	/* 偽…行の終りに ATTR が並ぶ	*/

	int	CRTC_SZ_LINES	   =20;	/* 表示する桁数 (20/25)		*/
#define		CRTC_SZ_COLUMNS	   (80)	/* 表示する行数 (80固定)	*/

	int	crtc_sz_lines      =20;	/* 桁数 (20〜25)		*/
	int	crtc_sz_columns    =80;	/* 行数 (2〜80)			*/
	int	crtc_sz_attrs      =20;	/* 属性量 (1〜20)		*/
	int	crtc_byte_per_line=120;	/* 1行あたりのメモリ バイト数	*/
	int	crtc_font_height   =10;	/* フォントの高さ ドット数(8/10)*/



/******************************************************************************

			←─────── crtc_byte_per_line  ───────→
			←──   crtc_sz_columns  ──→ ←  crtc_sz_attrs →
			+-------------------------------+-------------------+
		      ↑|				|↑		    |
		      │|	+--+ ↑			|│		    |
		      │|	|  | crtc_font_height	|│		    |
			|	+--+ ↓			|		    |
	   CRTC_SZ_LINES|				|crtc_sz_lines	    |
			|				|		    |
		      │|				|│		    |
		      │|				|│		    |
		      ↓|				|↓		    |
			+-------------------------------+-------------------+
			←──   CRTC_SZ_COLUMNS  ──→ 

	crtc_sz_columns		桁数	2〜80
	crtc_sz_attrs		属性量	1〜20
	crtc_byte_per_line	1行あたりのメモリ量	columns + attrs*2
	crtc_sz_lines		行数	20〜25
	crtc_font_height	フォントの高さドット量	8/10
	CRTC_SZ_COLUMNS		表示する桁数	80
	CRTC_SZ_LINES		表示する行数	20/25

******************************************************************************/









/* 参考までに……… 						*/
/*	SORCERIAN          … 1行飛ばし指定			*/
/*	Marchen Veil       … アトリビュートなしモード		*/
/*	Xanadu II (E disk) …             〃			*/
/*	Wizardry V         … ノントランスペアレント白黒モード	*/


enum{
  CRTC_RESET		= 0,
  CRTC_STOP_DISPLAY	= 0,
  CRTC_START_DISPLAY,
  CRTC_SET_INTERRUPT_MASK,
  CRTC_READ_LIGHT_PEN,
  CRTC_LOAD_CURSOR_POSITION,
  CRTC_RESET_INTERRUPT,
  CRTC_RESET_COUNTERS,
  CRTC_READ_STATUS,
  EndofCRTC
};
#define CRTC_STATUS_VE	(0x10)		/* 画面表示有効		*/
#define CRTC_STATUS_U	(0x08)		/* DMAアンダーラン	*/
#define CRTC_STATUS_N	(0x04)		/* 特殊制御文字割込発生 */
#define CRTC_STATUS_E	(0x02)		/* 表示終了割込発生	*/
#define CRTC_STATUS_LP	(0x01)		/* ライトペン入力 	*/


/****************************************************************/
/* CRTCへ同期信号を送る (OUT 40H,A ... bit3)			*/
/*	特にエミュレートの必要なし。。。。。と思う。		*/
/****************************************************************/
#ifdef	SUPPORT_CRTC_SEND_SYNC_SIGNAL
void	crtc_send_sync_signal( int flag )
{
}
#endif




/****************************************************************/
/*    CRTC エミュレーション					*/
/****************************************************************/

/*-------- 初期化 --------*/

void	crtc_init( void )
{
  crtc_out_command( CRTC_RESET << 5 );
  crtc_out_parameter( 0xce );
  crtc_out_parameter( 0x98 );
  crtc_out_parameter( 0x6f );
  crtc_out_parameter( 0x58 );
  crtc_out_parameter( 0x53 );

  crtc_out_command( CRTC_LOAD_CURSOR_POSITION << 5 );
  crtc_out_parameter( 0 );
  crtc_out_parameter( 0 );
}

/*-------- コマンド入力時 --------*/

void	crtc_out_command( byte data )
{
  crtc_command = data >> 5;
  crtc_param_num = 0;

  switch( crtc_command ){

  case CRTC_RESET:					/* リセット */
    crtc_status &= ~( CRTC_STATUS_VE | CRTC_STATUS_N | CRTC_STATUS_E );
    crtc_active = FALSE;
    set_text_display();
    set_screen_update_force();
    break;

  case CRTC_START_DISPLAY:				/* 表示開始 */
    crtc_reverse_display = data & 0x01;
    crtc_status |= CRTC_STATUS_VE;
    crtc_status &= ~( CRTC_STATUS_U );
    crtc_active = TRUE;
    set_text_display();
    set_screen_update_palette();
    break;

  case CRTC_SET_INTERRUPT_MASK:
    crtc_intr_mask = data & 0x03;
    set_text_display();
    set_screen_update_force();
    break;

  case CRTC_READ_LIGHT_PEN:
    crtc_status &= ~( CRTC_STATUS_LP );
    break;

  case CRTC_LOAD_CURSOR_POSITION:			/* カーソル設定 */
    crtc_load_cursor_position = data & 0x01;
    crtc_cursor[ 0 ] = -1;
    crtc_cursor[ 1 ] = -1;
    break;

  case CRTC_RESET_INTERRUPT:
  case CRTC_RESET_COUNTERS:
    crtc_status &= ~( CRTC_STATUS_N | CRTC_STATUS_E );
    break;

  }
}

/*-------- パラメータ入力時 --------*/

void	crtc_out_parameter( byte data )
{
  switch( crtc_command ){
  case CRTC_RESET:
    if( crtc_param_num < 5 ){
      crtc_format[ crtc_param_num++ ] = data;
    }

    crtc_skip_line         = crtc_format[2] & 0x80;		/* bool  */

    crtc_attr_non_separate = crtc_format[4] & 0x80;		/* bool */
    crtc_attr_color        = crtc_format[4] & 0x40;		/* bool */
    crtc_attr_non_special  = crtc_format[4] & 0x20;		/* bool */

    crtc_cursor_style      =(crtc_format[2] & 0x40) ?ATTR_REVERSE :ATTR_LOWER;
    crtc_cursor_blink      = crtc_format[2] & 0x20;		/* bool */
    blink_cycle            =(crtc_format[1]>>6) * 8 +8;		/* 8,16,24,48*/

    crtc_sz_lines          =(crtc_format[1] & 0x3f) +1;		/* 1〜25 */
    if     ( crtc_sz_lines <= 20 ) crtc_sz_lines = 20;
    else if( crtc_sz_lines >= 25 ) crtc_sz_lines = 25;
    else                           crtc_sz_lines = 24;

    crtc_sz_columns        =(crtc_format[0] & 0x7f) +2;		/* 2〜80 */
    if( crtc_sz_columns > 80 ) crtc_sz_columns = 80;

    crtc_sz_attrs          =(crtc_format[4] & 0x1f) +1;		/* 1〜20 */
    if     ( crtc_attr_non_special ) crtc_sz_attrs = 0;
    else if( crtc_sz_attrs > 20 )    crtc_sz_attrs = 20;

    crtc_byte_per_line  = crtc_sz_columns + crtc_sz_attrs * 2;	/*column+attr*/

    crtc_font_height    = (crtc_sz_lines>20) ?  8 : 10;
    CRTC_SZ_LINES	= (crtc_sz_lines>20) ? 25 : 20;

    blink_ctrl_update();
    break;

  case CRTC_LOAD_CURSOR_POSITION:
    if( crtc_param_num < 2 ){
      if( crtc_load_cursor_position ){
	crtc_cursor[ crtc_param_num++ ] = data;
      }else{
	crtc_cursor[ crtc_param_num++ ] = -1;
      }
    }
    break;

  }
}

/*-------- ステータス出力時 --------*/

byte	crtc_in_status( void )
{
  return crtc_status;
}

/*-------- パラメータ出力時 --------*/

byte	crtc_in_parameter( void )
{
  byte data = 0xff;

  switch( crtc_command ){
  case CRTC_READ_LIGHT_PEN:
    if( crtc_param_num < 2 ){
      data = crtc_light_pen[ crtc_param_num++ ];
    }
    return data;
  }

  return 0xff;
}





/****************************************************************/
/*    DMAC エミュレーション					*/
/****************************************************************/

static	int	dmac_flipflop;
	pair	dmac_address[4];
	pair	dmac_counter[4];
	int	dmac_mode;


void	dmac_init( void )
{
  dmac_flipflop = 0;
  dmac_address[0].W = 0;
  dmac_address[1].W = 0;
  dmac_address[2].W = 0xf3c8;
  dmac_address[3].W = 0;
  dmac_counter[0].W = 0;
  dmac_counter[1].W = 0;
  dmac_counter[2].W = 0;
  dmac_counter[3].W = 0;
}


void	dmac_out_mode( byte data )
{
  dmac_flipflop = 0;
  dmac_mode = data;

  set_text_display();
  set_screen_update_force();
}
byte	dmac_in_status( void )
{
  return 0x1f;
}


void	dmac_out_address( byte addr, byte data )
{
  if( dmac_flipflop==0 ) dmac_address[ addr ].B.l=data;
  else                   dmac_address[ addr ].B.h=data;

  dmac_flipflop ^= 0x1;
  set_screen_update_force();	/* 本当は、addr==2の時のみ……… */
}
void	dmac_out_counter( byte addr, byte data )
{
  if( dmac_flipflop==0 ) dmac_counter[ addr ].B.l=data;
  else                   dmac_counter[ addr ].B.h=data;

  dmac_flipflop ^= 0x1;
}


byte	dmac_in_address( byte addr )
{
  byte data;

  if( dmac_flipflop==0 ) data = dmac_address[ addr ].B.l;
  else                   data = dmac_address[ addr ].B.h;

  dmac_flipflop ^= 0x1;
  return data;
}
byte	dmac_in_counter( byte addr )
{
  byte data;

  if( dmac_flipflop==0 ) data = dmac_counter[ addr ].B.l;
  else                   data = dmac_counter[ addr ].B.h;

  dmac_flipflop ^= 0x1;
  return data;
}


/***********************************************************************
 * ステートロード／ステートセーブ
 ************************************************************************/

#define	SID	"CRTC"

static	T_SUSPEND_W	suspend_crtcdmac_work[]=
{
  { TYPE_INT,	&text_display,		},
  { TYPE_INT,	&blink_cycle,		},
  { TYPE_INT,	&blink_counter,		},

  { TYPE_INT,	&dma_wait_count,	},

  { TYPE_INT,	&crtc_command,		},
  { TYPE_INT,	&crtc_param_num,	},
  { TYPE_BYTE,	&crtc_status,		},
  { TYPE_BYTE,	&crtc_light_pen[0],	},
  { TYPE_BYTE,	&crtc_light_pen[1],	},
  { TYPE_BYTE,	&crtc_load_cursor_position,	},
  { TYPE_INT,	&crtc_active,		},
  { TYPE_INT,	&crtc_intr_mask,	},
  { TYPE_INT,	&crtc_cursor[0],	},
  { TYPE_INT,	&crtc_cursor[1],	},
  { TYPE_BYTE,	&crtc_format[0],	},
  { TYPE_BYTE,	&crtc_format[1],	},
  { TYPE_BYTE,	&crtc_format[2],	},
  { TYPE_BYTE,	&crtc_format[3],	},
  { TYPE_BYTE,	&crtc_format[4],	},
  { TYPE_INT,	&crtc_reverse_display,	},
  { TYPE_INT,	&crtc_skip_line,	},
  { TYPE_INT,	&crtc_cursor_style,	},
  { TYPE_INT,	&crtc_cursor_blink,	},
  { TYPE_INT,	&crtc_attr_non_separate,},
  { TYPE_INT,	&crtc_attr_color,	},
  { TYPE_INT,	&crtc_attr_non_special,	},
  { TYPE_INT,	&CRTC_SZ_LINES,		},
  { TYPE_INT,	&crtc_sz_lines,		},
  { TYPE_INT,	&crtc_sz_columns,	},
  { TYPE_INT,	&crtc_sz_attrs,		},
  { TYPE_INT,	&crtc_byte_per_line,	},
  { TYPE_INT,	&crtc_font_height,	},

  { TYPE_INT,	&dmac_flipflop,		},
  { TYPE_PAIR,	&dmac_address[0],	},
  { TYPE_PAIR,	&dmac_address[1],	},
  { TYPE_PAIR,	&dmac_address[2],	},
  { TYPE_PAIR,	&dmac_address[3],	},
  { TYPE_PAIR,	&dmac_counter[0],	},
  { TYPE_PAIR,	&dmac_counter[1],	},
  { TYPE_PAIR,	&dmac_counter[2],	},
  { TYPE_PAIR,	&dmac_counter[3],	},
  { TYPE_INT,	&dmac_mode,		},

  { TYPE_END,	0			},
};


int	statesave_crtcdmac( void )
{
  if( statesave_table( SID, suspend_crtcdmac_work ) == STATE_OK ) return TRUE;
  else                                                            return FALSE;
}

int	stateload_crtcdmac( void )
{
  if( stateload_table( SID, suspend_crtcdmac_work ) == STATE_OK ) return TRUE;
  else                                                            return FALSE;
}
