/************************************************************************/
/*									*/
/* ステータス部の表示 (FDD表示、ほかメッセージ表示)			*/
/*									*/
/************************************************************************/

#include <stdio.h>
#include <string.h>

#include "quasi88.h"
#include "initval.h"
#include "status.h"

#include "pc88main.h"		/* boot_basic	*/
#include "graph.h"
#include "memory.h"		/* font_rom	*/
#include "screen.h"

#include "drive.h"
#include "snddrv.h"		/* xmame_get_sound_volume()	*/
#include "wait.h"		/* wait_rate			*/
#include "intr.h"		/* no_wait			*/
#include "menu.h"		/* menu_lang			*/
#include "event.h"		/* get_keysym_menu()		*/

#include "emu.h"
#include "q8tk.h"


/*---------------------------------------------------------------------------*/

T_STATUS_INFO	status_info[3];			/* ステータスイメージ */

/*---------------------------------------------------------------------------*/

/*
 * ローカルなワーク
 */
static	int	stat_draw[3];
static	int	stat_msg_exist[3];
static	int	stat_msg_timer[3];
static	int	stat_var[3] = { -1, -1, -1 };

#define		stat_mode	stat_var[0]
#define		stat_fdd	stat_var[2]

/*
 * 表示するステータスのイメージ用バッファ
 *	バッファは8ドットフォント48文字分あるが、実際の表示はもっと小さい
 */
#define	PIXMAP_WIDTH	(48*8)
#define	PIXMAP_HEIGHT	(16)

static	byte	pixmap[3][ PIXMAP_WIDTH * PIXMAP_HEIGHT ];



/*
 * ローカルなフォント
 */
enum {
  FNT_START = 0xe0-1,

  FNT_2__1,  FNT_2__2,  FNT_2__3,	/* ドライブ2 */
  FNT_2D_1,  FNT_2D_2,  FNT_2D_3,

  FNT_1__1,  FNT_1__2,  FNT_1__3,	/* ドライブ1 */
  FNT_1D_1,  FNT_1D_2,  FNT_1D_3,

  FNT_T__1,  FNT_T__2,  FNT_T__3,	/* テープ */
  FNT_TR_1,  FNT_TR_2,  FNT_TR_3,
  FNT_TW_1,  FNT_TW_2,  FNT_TW_3,

  FNT_CAP_1, FNT_CAP_2,			/* CAPS     */
  FNT_KAN_1, FNT_KAN_2,			/* カナ     */
  FNT_RMJ_1, FNT_RMJ_2,			/* ローマ字 */
  FNT_NUM_1, FNT_NUM_2,			/* NUMlock  */

  FNT_END
};



static	const	byte	status_font[ 0x20 ][ 8*16 ] =
{
#define X	STATUS_BG
#define F	STATUS_FG
#define W	STATUS_WHITE
#define B	STATUS_BLACK
#define R	STATUS_RED
#define G	STATUS_GREEN


  {				/* ドライブ2(左側) 消灯：左−− */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,W,W,W,
    X,X,X,X,W,W,W,W,
    X,X,X,W,W,W,B,B,
    X,X,X,W,W,B,B,B,
    X,X,X,W,W,B,B,B,
    X,X,X,W,W,B,B,B,
    X,X,X,W,W,B,B,B,
    X,X,X,W,W,B,B,B,
    X,X,X,W,W,B,B,B,
    X,X,X,W,W,W,B,B,
    X,X,X,X,W,W,W,W,
    X,X,X,X,X,W,W,W,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* ドライブ2(左側) 消灯：−中− */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    W,W,W,W,W,W,W,W,
    W,W,W,W,W,W,W,W,
    B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,
    W,W,W,W,W,W,W,W,
    W,W,W,W,W,W,W,W,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* ドライブ2(左側) 消灯：−−右 */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    W,W,W,W,W,W,W,X,
    W,W,W,W,W,W,W,W,
    B,B,B,B,B,B,W,W,
    B,B,B,B,B,B,B,W,
    B,B,B,B,B,B,B,W,
    B,B,B,B,B,B,B,W,
    B,B,B,B,B,B,B,W,
    B,B,B,B,B,B,B,W,
    B,B,B,B,B,B,B,W,
    B,B,B,B,B,B,W,W,
    W,W,W,W,W,W,W,W,
    W,W,W,W,W,W,W,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* ドライブ2(左側) 点灯：左−− */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,W,W,W,
    X,X,X,X,W,W,W,W,
    X,X,X,W,W,W,R,R,
    X,X,X,W,W,R,R,R,
    X,X,X,W,W,R,R,R,
    X,X,X,W,W,R,R,R,
    X,X,X,W,W,R,R,R,
    X,X,X,W,W,R,R,R,
    X,X,X,W,W,R,R,R,
    X,X,X,W,W,W,R,R,
    X,X,X,X,W,W,W,W,
    X,X,X,X,X,W,W,W,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* ドライブ2(左側) 点灯：−中− */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    W,W,W,W,W,W,W,W,
    W,W,W,W,W,W,W,W,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    W,W,W,W,W,W,W,W,
    W,W,W,W,W,W,W,W,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* ドライブ2(左側) 点灯：−−右 */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    W,W,W,W,W,W,W,X,
    W,W,W,W,W,W,W,W,
    R,R,R,R,R,R,W,W,
    R,R,R,R,R,R,R,W,
    R,R,R,R,R,R,R,W,
    R,R,R,R,R,R,R,W,
    R,R,R,R,R,R,R,W,
    R,R,R,R,R,R,R,W,
    R,R,R,R,R,R,R,W,
    R,R,R,R,R,R,W,W,
    W,W,W,W,W,W,W,W,
    W,W,W,W,W,W,W,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* ドライブ1(右側) 消灯：左−− */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,W,W,W,W,W,W,W,
    W,W,W,W,W,W,W,W,
    W,W,B,B,B,B,B,B,
    W,B,B,B,B,B,B,B,
    W,B,B,B,B,B,B,B,
    W,B,B,B,B,B,B,B,
    W,B,B,B,B,B,B,B,
    W,B,B,B,B,B,B,B,
    W,B,B,B,B,B,B,B,
    W,W,B,B,B,B,B,B,
    W,W,W,W,W,W,W,W,
    X,W,W,W,W,W,W,W,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* ドライブ1(右側) 消灯：−中− */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    W,W,W,W,W,W,W,W,
    W,W,W,W,W,W,W,W,
    B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,
    B,B,B,B,B,B,B,B,
    W,W,W,W,W,W,W,W,
    W,W,W,W,W,W,W,W,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* ドライブ1(右側) 消灯：−−右 */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    W,W,W,X,X,X,X,X,
    W,W,W,W,X,X,X,X,
    B,B,W,W,W,X,X,X,
    B,B,B,W,W,X,X,X,
    B,B,B,W,W,X,X,X,
    B,B,B,W,W,X,X,X,
    B,B,B,W,W,X,X,X,
    B,B,B,W,W,X,X,X,
    B,B,B,W,W,X,X,X,
    B,B,W,W,W,X,X,X,
    W,W,W,W,X,X,X,X,
    W,W,W,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* ドライブ1(右側) 点灯：左−− */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,W,W,W,W,W,W,W,
    W,W,W,W,W,W,W,W,
    W,W,R,R,R,R,R,R,
    W,R,R,R,R,R,R,R,
    W,R,R,R,R,R,R,R,
    W,R,R,R,R,R,R,R,
    W,R,R,R,R,R,R,R,
    W,R,R,R,R,R,R,R,
    W,R,R,R,R,R,R,R,
    W,W,R,R,R,R,R,R,
    W,W,W,W,W,W,W,W,
    X,W,W,W,W,W,W,W,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* ドライブ1(右側) 点灯：−中− */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    W,W,W,W,W,W,W,W,
    W,W,W,W,W,W,W,W,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    W,W,W,W,W,W,W,W,
    W,W,W,W,W,W,W,W,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* ドライブ1(右側) 点灯：−−右 */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    W,W,W,X,X,X,X,X,
    W,W,W,W,X,X,X,X,
    R,R,W,W,W,X,X,X,
    R,R,R,W,W,X,X,X,
    R,R,R,W,W,X,X,X,
    R,R,R,W,W,X,X,X,
    R,R,R,W,W,X,X,X,
    R,R,R,W,W,X,X,X,
    R,R,R,W,W,X,X,X,
    R,R,W,W,W,X,X,X,
    W,W,W,W,X,X,X,X,
    W,W,W,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* テープ あり：左−− */
    X,X,X,X,X,X,X,X,
    X,W,W,W,W,W,W,W,
    X,W,F,F,F,F,F,F,
    X,W,F,F,X,X,X,X,
    X,W,F,X,F,X,X,X,
    X,W,F,X,X,F,X,X,
    X,W,F,X,X,X,F,F,
    X,W,F,X,X,X,X,X,
    X,W,F,X,X,X,X,X,
    X,W,F,X,X,X,X,F,
    X,W,F,X,X,X,X,X,
    X,W,F,X,X,X,X,X,
    X,W,F,X,X,X,X,X,
    X,W,F,F,F,F,F,F,
    X,W,W,W,W,W,W,W,
    X,X,X,X,X,X,X,X,
  },
  {				/* テープ あり：−中− */
    X,X,X,X,X,X,X,X,
    W,W,W,W,W,W,W,W,
    F,F,F,F,F,F,F,F,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    F,F,F,F,F,F,F,F,
    X,X,X,X,X,X,X,X,
    F,X,X,X,X,X,X,F,
    F,F,X,X,X,X,F,F,
    F,X,X,X,X,X,X,F,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    F,F,F,F,F,F,F,F,
    W,W,W,W,W,W,W,W,
    X,X,X,X,X,X,X,X,
  },
  {				/* テープ あり：−−右 */
    X,X,X,X,X,X,X,X,
    W,W,W,W,W,W,W,X,
    F,F,F,F,F,F,W,X,
    X,X,X,X,F,F,W,X,
    X,X,X,F,X,F,W,X,
    X,X,F,X,X,F,W,X,
    F,F,X,X,X,F,W,X,
    X,X,X,X,X,F,W,X,
    X,X,X,X,X,F,W,X,
    F,X,X,X,X,F,W,X,
    X,X,X,X,X,F,W,X,
    X,X,X,X,X,F,W,X,
    X,X,X,X,X,F,W,X,
    F,F,F,F,F,F,W,X,
    W,W,W,W,W,W,W,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* テープ 再生：左−− */
    X,X,X,X,X,X,X,X,
    X,W,W,W,W,W,W,W,
    X,W,X,X,X,X,X,X,
    X,W,X,X,F,F,F,F,
    X,W,X,F,X,F,F,F,
    X,W,X,F,F,X,F,F,
    X,W,X,F,F,F,X,X,
    X,W,X,F,F,F,F,F,
    X,W,X,F,F,F,F,F,
    X,W,X,F,F,F,F,X,
    X,W,X,F,F,F,F,F,
    X,W,X,F,F,F,F,F,
    X,W,X,F,F,F,F,F,
    X,W,X,X,X,X,X,X,
    X,W,W,W,W,W,W,W,
    X,X,X,X,X,X,X,X,
  },
  {				/* テープ 再生：−中− */
    X,X,X,X,X,X,X,X,
    W,W,W,W,W,W,W,W,
    X,X,X,X,X,X,X,X,
    F,F,F,F,F,F,F,F,
    F,F,F,F,F,F,F,F,
    F,F,F,F,F,F,F,F,
    X,X,X,X,X,X,X,X,
    F,F,F,F,F,F,F,F,
    X,F,F,F,F,F,F,X,
    X,X,F,F,F,F,X,X,
    X,F,F,F,F,F,F,X,
    F,F,F,F,F,F,F,F,
    F,F,F,F,F,F,F,F,
    X,X,X,X,X,X,X,X,
    W,W,W,W,W,W,W,W,
    X,X,X,X,X,X,X,X,
  },
  {				/* テープ 再生：−−右 */
    X,X,X,X,X,X,X,X,
    W,W,W,W,W,W,W,X,
    X,X,X,X,X,X,W,X,
    F,F,F,F,X,X,W,X,
    F,F,F,X,F,X,W,X,
    F,F,X,F,F,X,W,X,
    X,X,F,F,F,X,W,X,
    F,F,F,F,F,X,W,X,
    F,F,F,F,F,X,W,X,
    X,F,F,F,F,X,W,X,
    F,F,F,F,F,X,W,X,
    F,F,F,F,F,X,W,X,
    F,F,F,F,F,X,W,X,
    X,X,X,X,X,X,W,X,
    W,W,W,W,W,W,W,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* テープ 録音：左−− */
    X,X,X,X,X,X,X,X,
    X,W,W,W,W,W,W,W,
    X,W,X,X,X,X,X,X,
    X,W,X,X,R,R,R,R,
    X,W,X,R,X,R,R,R,
    X,W,X,R,R,X,R,R,
    X,W,X,R,R,R,X,X,
    X,W,X,R,R,R,R,R,
    X,W,X,R,R,R,R,R,
    X,W,X,R,R,R,R,X,
    X,W,X,R,R,R,R,R,
    X,W,X,R,R,R,R,R,
    X,W,X,R,R,R,R,R,
    X,W,X,X,X,X,X,X,
    X,W,W,W,W,W,W,W,
    X,X,X,X,X,X,X,X,
  },
  {				/* テープ 録音：−中− */
    X,X,X,X,X,X,X,X,
    W,W,W,W,W,W,W,W,
    X,X,X,X,X,X,X,X,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    X,X,X,X,X,X,X,X,
    R,R,R,R,R,R,R,R,
    X,R,R,R,R,R,R,X,
    X,X,R,R,R,R,X,X,
    X,R,R,R,R,R,R,X,
    R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,
    X,X,X,X,X,X,X,X,
    W,W,W,W,W,W,W,W,
    X,X,X,X,X,X,X,X,
  },
  {				/* テープ 録音：−−右 */
    X,X,X,X,X,X,X,X,
    W,W,W,W,W,W,W,X,
    X,X,X,X,X,X,W,X,
    R,R,R,R,X,X,W,X,
    R,R,R,X,R,X,W,X,
    R,R,X,R,R,X,W,X,
    X,X,R,R,R,X,W,X,
    R,R,R,R,R,X,W,X,
    R,R,R,R,R,X,W,X,
    X,R,R,R,R,X,W,X,
    R,R,R,R,R,X,W,X,
    R,R,R,R,R,X,W,X,
    R,R,R,R,R,X,W,X,
    X,X,X,X,X,X,W,X,
    W,W,W,W,W,W,W,X,
    X,X,X,X,X,X,X,X,
  },



  {				/* CAPS：左− */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,F,F,F,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,F,
    X,X,X,X,F,X,X,F,
    X,X,X,X,F,X,X,F,
    X,X,X,X,F,X,F,X,
    X,X,X,X,F,X,F,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,X,F,F,F,
    X,X,X,X,X,X,X,X,
  },
  {				/* CAPS：−右  */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    F,F,F,F,F,F,F,X,
    X,X,X,X,X,X,X,F,
    X,F,F,X,X,X,X,F,
    F,X,X,F,X,X,X,F,
    F,X,X,F,X,X,X,F,
    X,X,X,X,F,X,X,F,
    X,X,X,X,F,X,X,F,
    F,F,F,F,F,X,X,F,
    X,X,X,X,X,F,X,F,
    X,X,X,X,X,F,X,F,
    X,X,X,X,X,X,X,F,
    F,F,F,F,F,F,F,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* カナ：左− */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,F,F,F,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,F,F,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,F,
    X,X,X,X,F,X,X,F,
    X,X,X,X,F,X,F,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,X,F,F,F,
    X,X,X,X,X,X,X,X,
  },
  {				/* カナ：−右  */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    F,F,F,F,F,F,F,X,
    X,X,X,X,X,X,X,F,
    F,X,X,X,X,X,X,F,
    F,F,F,F,F,X,X,F,
    F,X,X,X,F,X,X,F,
    F,X,X,X,F,X,X,F,
    F,X,X,X,F,X,X,F,
    X,X,X,X,F,X,X,F,
    X,X,X,X,F,X,X,F,
    X,X,X,F,X,X,X,F,
    X,X,X,X,X,X,X,F,
    F,F,F,F,F,F,F,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* ローマ字：左−  */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,F,F,F,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,F,F,
    X,X,X,X,F,X,F,X,
    X,X,X,X,F,X,F,X,
    X,X,X,X,F,X,F,X,
    X,X,X,X,F,X,F,X,
    X,X,X,X,F,X,F,F,
    X,X,X,X,F,X,F,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,X,F,F,F,
    X,X,X,X,X,X,X,X,
  },
  {				/* ローマ字：−右 */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    F,F,F,F,F,F,F,X,
    X,X,X,X,X,X,X,F,
    X,X,X,X,X,X,X,F,
    F,F,F,F,F,X,X,F,
    X,X,X,X,F,X,X,F,
    X,X,X,X,F,X,X,F,
    X,X,X,X,F,X,X,F,
    X,X,X,X,F,X,X,F,
    F,F,F,F,F,X,X,F,
    X,X,X,X,F,X,X,F,
    X,X,X,X,X,X,X,F,
    F,F,F,F,F,F,F,X,
    X,X,X,X,X,X,X,X,
  },
  {				/* 数字：左−  */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,F,F,F,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,F,X,X,X,
    X,X,X,X,X,F,F,F,
    X,X,X,X,X,X,X,X,
  },
  {				/* 数字：−右  */
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    X,X,X,X,X,X,X,X,
    F,F,F,F,F,F,F,X,
    X,X,X,X,X,X,X,F,
    X,X,F,X,X,X,X,F,
    X,F,F,X,X,X,X,F,
    X,X,F,X,X,X,X,F,
    X,X,F,X,X,X,X,F,
    X,X,F,X,X,X,X,F,
    X,X,F,X,X,X,X,F,
    X,X,F,X,X,X,X,F,
    X,F,F,F,X,X,X,F,
    X,X,X,X,X,X,X,F,
    F,F,F,F,F,F,F,X,
    X,X,X,X,X,X,X,X,
  },



#undef X
#undef F
#undef W
#undef B
#undef R
#undef G
};




/*
 * 文字列をステータスのイメージ用バッファに転送
 */
static	void	status_puts( int type, const unsigned char *str )
{
  int i, j, k, c, w, h16;
  const byte *p;
  byte mask;
  byte *dst = status_info[ type ].pixmap;
  
  if( str ){
    w = MIN( strlen((char *)str) * 8, PIXMAP_WIDTH );

    for( i=0; i<w; i+=8 ){
      c = *str ++;
      if( c=='\0' ) break;

      if( c < 0xe0 ){

	if( has_kanji_rom && 			/* 漢字ROMあり */
	    (( 0x20 <= c && c <= 0x7f ) ||	/* ASCII    */
	     ( 0xa0 <= c && c <= 0xdf ) )  ){	/* カタカナ */
	  p = &kanji_rom[0][ c*8 ][0];
	  h16 = TRUE;
	}else{
	  p = &font_mem[ c*8 ];
	  h16 = FALSE;
	}
	for( j=0; j<16; j++ ){
	  for( mask=0x80, k=0;  k<8;  k++, mask>>=1 ){
	    if( *p & mask ) dst[ j*w +i +k ] = STATUS_FG;
	    else            dst[ j*w +i +k ] = STATUS_BG;
	  }
	  if( h16 || j&1 ) p++;
	}

      }else{		/* 0xe0〜0xff は ローカルなフォントを使用 */

	p = &status_font[ (c-0xe0) ][0];
	for( j=0; j<16; j++ ){
	  for( k=0;  k<8;  k++ ){
	    dst[ j*w +i +k ] = *p;
	    p++;
	  }
	}

      }
    }
    status_info[ type ].w = i;
  }else{
    status_info[ type ].w = 0;
  }
}

/*
 * ピックスマップをステータスのイメージ用バッファに転送
 */
static	void	status_bitmap( int type, const byte bitmap[], int size )
{
  if( bitmap ){
    memcpy( status_info[ type ].pixmap, bitmap, size );
    status_info[ type ].w = size / PIXMAP_HEIGHT;
  }else{
    status_info[ type ].w = 0;
  }
}





/***************************************************************************
 * ステータス関係のワーク類を初期化
 ****************************************************************************/
void	status_init( void )
{
  int i;
  for( i=0; i<3; i++ ){
    stat_draw[i] = TRUE;
    stat_msg_exist[i] = FALSE;
    stat_var[i] = -1;

    status_info[i].pixmap = &pixmap[i][0];
    status_info[i].w      = 0;
    status_info[i].h      = PIXMAP_HEIGHT;
  }
}



/***************************************************************************
 * ステータス表示・非表示切替の際の、ワーク再初期化
 ****************************************************************************/

void	status_reset( int show )
{
  if( show ){						/* 表示するなら  */
    stat_draw[0] = stat_draw[1] = stat_draw[2] = TRUE;	/* 描画フラグON  */

  }else{						/* 非表示なら    */
    stat_var[0]  = stat_var[1]  = stat_var[2]  = -1;	/* メッセージOFF */
  }
}




/***************************************************************************
 * ステータスにメッセージ(文字列)表示
 *	kind	… ステータスの番号 0 〜 2
 *	frames	… 表示する時間(フレーム数) 60で約1秒
 *		   0 で無限に表示。 <0 で消去
 *	msg	… 表示する文字列。 NULL は "" とみなす。
 ****************************************************************************/
void	status_message( int kind, int frames, const char *msg )
{
  status_puts( kind, (const unsigned char *)msg );

  if( frames >= 0 ){ stat_msg_exist[ kind ] = TRUE;   }
  else             { stat_msg_exist[ kind ] = FALSE;  stat_var[ kind ] = -1; }

  stat_msg_timer[ kind ] = frames;
  stat_draw[ kind ] = TRUE;
}





/***************************************************************************
 * ステータス表示用のイメージを更新
 *	表示イメージのピックスマップを status_info に転送する。
 *	引数 force が 偽の場合は、前回から変化のあったステータスのみ更新
 *	     force が 真の場合は、前回の状態とは関わらずステータスを更新
 *	戻り値は、実際に更新したステータス番号が、ビット 0〜2 に
 *		  セットされる。(ビットが 1 ならそのステータスは更新)
 ****************************************************************************/

INLINE void status_mode( void )
{
  int mode = 0;		/* bit :  ....  num kana caps 8mhz basic basic */
  byte buf[16];
  static const char *mode_str[] = {
    "N   4MHz       ", "V1S 4MHz       ", "V1H 4MHz       ", "V2  4MHz       ",
    "N   8MHz       ", "V1S 8MHz       ", "V1H 8MHz       ", "V2  8MHz       ",
  };
  switch( boot_basic ){
  case BASIC_N:		mode = 0;	break;
  case BASIC_V1S:	mode = 1;	break;
  case BASIC_V1H:	mode = 2;	break;
  case BASIC_V2:	mode = 3;	break;
  }
  if( boot_clock_4mhz == FALSE ) mode += 4;

  if( (key_scan[0x0a] & 0x80) == 0 ) mode += 8;
  if( (key_scan[0x08] & 0x20) == 0 ) mode += 16;
  if( numlock_emu ) mode += 32;

  if( stat_mode != mode ){	/* モードが変更したら表示更新 */
    stat_mode = mode;

    strcpy( (char *)buf, mode_str[ mode & 0x7 ] );
    if( mode & 8 ){
      buf[ 9] = FNT_CAP_1;
      buf[10] = FNT_CAP_2;
    }
    if( mode & 16 ){
      if( romaji_input_mode ){
	buf[11] = FNT_RMJ_1;
	buf[12] = FNT_RMJ_2;
      }else{
	buf[11] = FNT_KAN_1;
	buf[12] = FNT_KAN_2;
      }
    }
    if( mode & 32 ){
	buf[13] = FNT_NUM_1;
	buf[14] = FNT_NUM_2;
    }

    status_puts( 0, (const unsigned char *)buf );
    stat_draw[ 0 ] = TRUE;
  }
}

INLINE void status_fdd( void )
{
  byte *p, buf[16];
  int fdd = 0;		/* bit :  ....  tape tape drv2 drv1 */

  if( ! get_drive_ready(0) ){ fdd |= 1<<0; }	/* FDDランプON */
  if( ! get_drive_ready(1) ){ fdd |= 1<<1; }
  /* drive_check_empty(n) でディスクの有無もわかるけど… */

  if     ( tape_writing() ) fdd |= 3<<2;
  else if( tape_reading() ) fdd |= 2<<2;
  else if( tape_exist() )   fdd |= 1<<2;

  if( stat_fdd != fdd ){
    stat_fdd = fdd;

    p = buf;

    switch( fdd >> 2 ){
    case 1:
      *p ++ = FNT_T__1;
      *p ++ = FNT_T__2;
      *p ++ = FNT_T__3;
      *p ++ = ' ';
      break;
    case 2:
      *p ++ = FNT_TR_1;
      *p ++ = FNT_TR_2;
      *p ++ = FNT_TR_3;
      *p ++ = ' ';
      break;
    case 3:
      *p ++ = FNT_TW_1;
      *p ++ = FNT_TW_2;
      *p ++ = FNT_TW_3;
      *p ++ = ' ';
      break;
    }

    if( fdd & (1<<1) ){
      *p ++ = FNT_2D_1;
      *p ++ = FNT_2D_2;
      *p ++ = FNT_2D_3;
    }else{
      *p ++ = FNT_2__1;
      *p ++ = FNT_2__2;
      *p ++ = FNT_2__3;
    }

    if( fdd & (1<<0) ){
      *p ++ = FNT_1D_1;
      *p ++ = FNT_1D_2;
      *p ++ = FNT_1D_3;
    }else{
      *p ++ = FNT_1__1;
      *p ++ = FNT_1__2;
      *p ++ = FNT_1__3;
    }

    *p = '\0';

    status_puts( 2, (const unsigned char *)buf );
    stat_draw[ 2 ] = TRUE;
  }
}



/*
 *
 */
int	status_update( int force )
{
  int i, ret = 0;

  for( i=0; i<3; i++ ){

    if( stat_msg_exist[i] ){		/* メッセージ指定あり */
      if( stat_msg_timer[i] ){			/* タイマー指定時は   */
	if( -- stat_msg_timer[i]==0 ){		/* タイムアウトで     */
	  status_message( i, -1, NULL );	/* メッセージ指定消す */
	  stat_var[i] = -1;
	}
      }
    }

    if( stat_msg_exist[i] == FALSE ){	/* メッセージ指定なし */
      if     ( i==0 ){ status_mode(); }		/* 0 はモード表示  */
      else if( i==2 ){ status_fdd();  }		/* 2 はFDD状態表示 */
    }

  }

  if( now_status ){
    for( i=0; i<3; i++ ){
      if( stat_draw[i] || force ){
	stat_draw[i] = FALSE;
	ret |= 1<<i;
      }
    }
  }
  return ret;
}










/************************************************************************/
/* タイトル・バージョンを表示する。					*/
/*		起動時にのみ、この関数を呼びだそう。			*/
/************************************************************************/
void	indicate_bootup_logo( void )
{
  char menu_msg[24];
  const char *keysym = get_keysym_menu();

  if( keysym ){
    sprintf( menu_msg, "<%.8s> key to MENU", keysym );

    status_message( 0, 60*4, Q_TITLE " " Q_VERSION );
    status_message( 1, 0,    menu_msg );
  }
}


void	indicate_stateload_logo( void )
{
  status_message( 0, 60*4, Q_TITLE " " Q_VERSION );
  status_message( 1, 60*4, "State Load Successful !" );
}
