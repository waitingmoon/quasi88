/***********************************************************************
 * イベント処理 (システム依存)
 *
 *	詳細は、 event.h 参照
 ************************************************************************/

/* ----------------------------------------------------------------------
 *		ヘッダファイル部
 * ---------------------------------------------------------------------- */

#include <SDL.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "quasi88.h"
#include "keyboard.h"

#include "graph.h"	/* now_screen_size		*/

#include "drive.h"

#include "emu.h"
#include "device.h"
#include "screen.h"
#include "event.h"



int	use_cmdkey = 1;			/* Commandキーでメニューへ遷移     */

int	keyboard_type = 1;		/* キーボードの種類                */
char	*file_keyboard = NULL;		/* キー設定ファイル名		   */

int	use_joydevice = TRUE;		/* ジョイスティックデバイスを開く? */

	int	use_unicode = FALSE;	/* UNICODEを使えば106キーボードでも */
static	int	now_unicode = FALSE;	/* 正確にキーが拾えるようだが、キー */
					/* リリースの検知が面倒なので保留   */

static	SDL_Joystick *joy = NULL;	/* NULLならジョイスティック使用不可 */


/*==========================================================================
 * キー配列について
 *
 *  一般キー(文字キー) は、
 *	106 キーボードの場合、PC-8801 と同じなので問題なし。
 *	101 キーボードの場合、一部配置が異なるし、キーが足りない。
 *	とりあえず、以下のように配置してみる。
 *		` → \、 = → ^、 [ ] はそのまま、 \ → @、' → :、右CTRL → _
 *
 *  特殊キー(機能キー) は、
 *	ホスト側のキー刻印と似た雰囲気の機能を、PC-8801のキーに割り当てよう。
 *	Pause は STOP、 PrintScreen は COPY など。個人的な主観で決める。
 *
 *  テンキーは、
 *	PC-8801と106キーでキー刻印が若干異なるが、そのままのキー刻印を使う。
 *	となると、 = と , が無い。 mac ならあるが。
 *
 *  最下段のキーの配置は、適当に割り振る。 (カッコのキーには割り振らない)
 *
 *	PC-8801        かな GRPH 決定  スペース 変換  PC    全角
 *	101キー   Ctrl      Alt        スペース             Alt          Ctrl
 *	104キー   Ctrl Win  Alt        スペース             Alt  Win App Ctrl
 *	109キー   Ctrl Win  Alt 無変換 スペース 変換 (ひら) Alt  Win App Ctrl
 *	mac ?     Ctrl      Opt (Cmd)  スペース      (cmd) (Opt)        (Ctrl)
 *
 * SDLのキー入力についての推測 (Windows & 106キーの場合)
 *	○環境変数 SDL_VIDEODRIVER が windib と directx とで挙動が異なる。
 *	○「SHIFT」を押しながら 「1」 を押しても、keysym は 「1」 のまま。
 *	  つまり、 「!」 は入力されないみたい。
 *	  大文字 「A」 も入力されない。 keysym は 「a」 となる。
 *	○キー配列は 101 がベースになっている。
 *	  「^」 を押しても keysym は 「=」 になる。
 *	○いくつかのキーで、 keycode が重複している
 *	  windib  だと、カーソルキーとテンキーなど、たくさん。
 *	  directx なら、重複は無い ?
 *	○いくつかのキーで、 keysym が重複している
 *	  windib  だと、￥ と ]  (ともに ￥ になる)
 *	  directx なら、重複は無い ?
 *	○いくつかのキーで、キーシンボルが未定義
 *	  無変換、変換、カタカナひらがな が未定義
 *	  windib  だと、＼ が未定義
 *	  directx だと、＾￥＠：、半角/全角 が未定義
 *	○いくつかのキーで、キーを離した時の検知が不安定(?)
 *	  windib  だと 半角/全角、カタカナひらがな、PrintScreen
 *	  directx だと ALT
 *	○キーロックに難あり(?)
 *	  NumLockはロックできる。
 *	  windib  だと SHIFT + CapsLock がロック可。
 *	  NumLock だと CapsLock、カタカナひらがな、半角/全角がロック可。
 *
 *	○メニューモードでは、UNICODE を有効にする。
 *	  こうすれば、「SHIFT」+「1」 を 「!」 と認識できるし、「SHIFT」+「2」
 *	  は 「"」になる。しかし、  directx だと、入力できない文字があるぞ。
 *
 *	○ところで、日本語Windowsでの101キーボードと、英語Windowsでの
 *	  101キーボードって、同じ挙動なんだろうか・・・
 *	  directx の時のキーコード割り当てが明らかに不自然なのだが。
 *===========================================================================*/

/*----------------------------------------------------------------------
 * SDL の keysym を QUASI88 の キーコードに変換するテーブル
 *
 *	キーシンボル SDLK_xxx が押されたら、 
 *	keysym2key88[ SDLK_xxx ] が押されたとする。
 *----------------------------------------------------------------------*/
static int keysym2key88[ SDLK_LAST ];



/*----------------------------------------------------------------------
 * SDL の scancode を QUASI88 の キーコードに変換するテーブル
 *
 *	スキャンコード code が押されたら、
 *	keycode2key88[ code ] が押されたとする。
 *	(keysym2key88[]に優先する。スキャンコードの値は 0〜配列maxまで)
 *----------------------------------------------------------------------*/
static int scancode2key88[ 256 ];

 

/*----------------------------------------------------------------------
 * ソフトウェア NumLock オン/オフ 時のマッピング変更テーブル
 *
 *	binding[].code が押されたら、
 *	binding[].new_key88 が押されたことにする。
 *	(変更できるキーの個数は、配列maxまで)
 *----------------------------------------------------------------------*/
typedef struct{
  int type;		/* 0:終端 / 1:keysym / 2:scancode	*/
  int code;		/* キーシンボル、ないし、スキャンコード	*/
  int org_key88;	/* NumLock OFF時の QUASI88キーコード	*/
  int new_key88;	/* NumLock ON 時の QUASI88キーコード	*/
} T_BINDING;

static T_BINDING binding[ 64 ];





/*----------------------------------------------------------------------
 * SDLK_xxx → KEY88_xxx 変換テーブル (デフォルト)
 *----------------------------------------------------------------------*/

static const int keysym2key88_default[ SDLK_LAST ] =
{
  0,				/*	SDLK_UNKNOWN		= 0,	*/
  0, 0, 0, 0, 0, 0, 0,
  KEY88_INS_DEL,		/*	SDLK_BACKSPACE		= 8,	*/
  KEY88_TAB,			/*	SDLK_TAB		= 9,	*/
  0, 0,
  KEY88_HOME,			/*	SDLK_CLEAR		= 12,	*/
  KEY88_RETURNL,		/*	SDLK_RETURN		= 13,	*/
  0, 0, 0, 0, 0,
  KEY88_STOP,			/*	SDLK_PAUSE		= 19,	*/
  0, 0, 0, 0, 0, 0, 0,
  KEY88_ESC,			/*	SDLK_ESCAPE		= 27,	*/
  0, 0, 0, 0,

  KEY88_SPACE,			/*	SDLK_SPACE		= 32,	*/
  KEY88_EXCLAM,			/*	SDLK_EXCLAIM		= 33,	*/
  KEY88_QUOTEDBL,		/*	SDLK_QUOTEDBL		= 34,	*/
  KEY88_NUMBERSIGN,		/*	SDLK_HASH		= 35,	*/
  KEY88_DOLLAR,			/*	SDLK_DOLLAR		= 36,	*/
  KEY88_PERCENT,		/*					*/
  KEY88_AMPERSAND,		/*	SDLK_AMPERSAND		= 38,	*/
  KEY88_APOSTROPHE,		/*	SDLK_QUOTE		= 39,	*/
  KEY88_PARENLEFT,		/*	SDLK_LEFTPAREN		= 40,	*/
  KEY88_PARENRIGHT,		/*	SDLK_RIGHTPAREN		= 41,	*/
  KEY88_ASTERISK,		/*	SDLK_ASTERISK		= 42,	*/
  KEY88_PLUS,			/*	SDLK_PLUS		= 43,	*/
  KEY88_COMMA,			/*	SDLK_COMMA		= 44,	*/
  KEY88_MINUS,			/*	SDLK_MINUS		= 45,	*/
  KEY88_PERIOD,			/*	SDLK_PERIOD		= 46,	*/
  KEY88_SLASH,			/*	SDLK_SLASH		= 47,	*/
  KEY88_0,			/*	SDLK_0			= 48,	*/
  KEY88_1,			/*	SDLK_1			= 49,	*/
  KEY88_2,			/*	SDLK_2			= 50,	*/
  KEY88_3,			/*	SDLK_3			= 51,	*/
  KEY88_4,			/*	SDLK_4			= 52,	*/
  KEY88_5,			/*	SDLK_5			= 53,	*/
  KEY88_6,			/*	SDLK_6			= 54,	*/
  KEY88_7,			/*	SDLK_7			= 55,	*/
  KEY88_8,			/*	SDLK_8			= 56,	*/
  KEY88_9,			/*	SDLK_9			= 57,	*/
  KEY88_COLON,			/*	SDLK_COLON		= 58,	*/
  KEY88_SEMICOLON,		/*	SDLK_SEMICOLON		= 59,	*/
  KEY88_LESS,			/*	SDLK_LESS		= 60,	*/
  KEY88_EQUAL,			/*	SDLK_EQUALS		= 61,	*/
  KEY88_GREATER,		/*	SDLK_GREATER		= 62,	*/
  KEY88_QUESTION,		/*	SDLK_QUESTION		= 63,	*/
  KEY88_AT,			/*	SDLK_AT			= 64,	*/
  KEY88_A,			/*					*/
  KEY88_B,			/*					*/
  KEY88_C,			/*					*/
  KEY88_D,			/*					*/
  KEY88_E,			/*					*/
  KEY88_F,			/*					*/
  KEY88_G,			/*					*/
  KEY88_H,			/*					*/
  KEY88_I,			/*					*/
  KEY88_J,			/*					*/
  KEY88_K,			/*					*/
  KEY88_L,			/*					*/
  KEY88_M,			/*					*/
  KEY88_N,			/*					*/
  KEY88_O,			/*					*/
  KEY88_P,			/*					*/
  KEY88_Q,			/*					*/
  KEY88_R,			/*					*/
  KEY88_S,			/*					*/
  KEY88_T,			/*					*/
  KEY88_U,			/*					*/
  KEY88_V,			/*					*/
  KEY88_W,			/*					*/
  KEY88_X,			/*					*/
  KEY88_Y,			/*					*/
  KEY88_Z,			/*					*/
  KEY88_BRACKETLEFT,		/*	SDLK_LEFTBRACKET	= 91,	*/
  KEY88_YEN,			/*	SDLK_BACKSLASH		= 92,	*/
  KEY88_BRACKETRIGHT,		/*	SDLK_RIGHTBRACKET	= 93,	*/
  KEY88_CARET,			/*	SDLK_CARET		= 94,	*/
  KEY88_UNDERSCORE,		/*	SDLK_UNDERSCORE		= 95,	*/
  KEY88_BACKQUOTE,		/*	SDLK_BACKQUOTE		= 96,	*/
  KEY88_a,			/*	SDLK_a			= 97,	*/
  KEY88_b,			/*	SDLK_b			= 98,	*/
  KEY88_c,			/*	SDLK_c			= 99,	*/
  KEY88_d,			/*	SDLK_d			= 100,	*/
  KEY88_e,			/*	SDLK_e			= 101,	*/
  KEY88_f,			/*	SDLK_f			= 102,	*/
  KEY88_g,			/*	SDLK_g			= 103,	*/
  KEY88_h,			/*	SDLK_h			= 104,	*/
  KEY88_i,			/*	SDLK_i			= 105,	*/
  KEY88_j,			/*	SDLK_j			= 106,	*/
  KEY88_k,			/*	SDLK_k			= 107,	*/
  KEY88_l,			/*	SDLK_l			= 108,	*/
  KEY88_m,			/*	SDLK_m			= 109,	*/
  KEY88_n,			/*	SDLK_n			= 110,	*/
  KEY88_o,			/*	SDLK_o			= 111,	*/
  KEY88_p,			/*	SDLK_p			= 112,	*/
  KEY88_q,			/*	SDLK_q			= 113,	*/
  KEY88_r,			/*	SDLK_r			= 114,	*/
  KEY88_s,			/*	SDLK_s			= 115,	*/
  KEY88_t,			/*	SDLK_t			= 116,	*/
  KEY88_u,			/*	SDLK_u			= 117,	*/
  KEY88_v,			/*	SDLK_v			= 118,	*/
  KEY88_w,			/*	SDLK_w			= 119,	*/
  KEY88_x,			/*	SDLK_x			= 120,	*/
  KEY88_y,			/*	SDLK_y			= 121,	*/
  KEY88_z,			/*	SDLK_z			= 122,	*/
  KEY88_BRACELEFT,		/*					*/
  KEY88_BAR,			/*					*/
  KEY88_BRACERIGHT,		/*					*/
  KEY88_TILDE,			/*					*/
  KEY88_DEL,			/*	SDLK_DELETE		= 127,	*/

  0, 0, 0, 0, 0, 0, 0, 0,	/*	SDLK_WORLD_0		= 160,	*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*		:			*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*		:			*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*		:			*/
				/*		:			*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*		:			*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*		:			*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*		:			*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*		:			*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*		:			*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*		:			*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*		:			*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*		:			*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*		:			*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*		:			*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*		:			*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*	SDLK_WORLD_95		= 255,	*/

  KEY88_KP_0,			/*	SDLK_KP0		= 256,	*/
  KEY88_KP_1,			/*	SDLK_KP1		= 257,	*/
  KEY88_KP_2,			/*	SDLK_KP2		= 258,	*/
  KEY88_KP_3,			/*	SDLK_KP3		= 259,	*/
  KEY88_KP_4,			/*	SDLK_KP4		= 260,	*/
  KEY88_KP_5,			/*	SDLK_KP5		= 261,	*/
  KEY88_KP_6,			/*	SDLK_KP6		= 262,	*/
  KEY88_KP_7,			/*	SDLK_KP7		= 263,	*/
  KEY88_KP_8,			/*	SDLK_KP8		= 264,	*/
  KEY88_KP_9,			/*	SDLK_KP9		= 265,	*/
  KEY88_KP_PERIOD,		/*	SDLK_KP_PERIOD		= 266,	*/
  KEY88_KP_DIVIDE,		/*	SDLK_KP_DIVIDE		= 267,	*/
  KEY88_KP_MULTIPLY,		/*	SDLK_KP_MULTIPLY	= 268,	*/
  KEY88_KP_SUB,			/*	SDLK_KP_MINUS		= 269,	*/
  KEY88_KP_ADD,			/*	SDLK_KP_PLUS		= 270,	*/
  KEY88_RETURNR,		/*	SDLK_KP_ENTER		= 271,	*/
  KEY88_KP_EQUAL,		/*	SDLK_KP_EQUALS		= 272,	*/
  KEY88_UP,			/*	SDLK_UP			= 273,	*/
  KEY88_DOWN,			/*	SDLK_DOWN		= 274,	*/
  KEY88_RIGHT,			/*	SDLK_RIGHT		= 275,	*/
  KEY88_LEFT,			/*	SDLK_LEFT		= 276,	*/
  KEY88_INS,			/*	SDLK_INSERT		= 277,	*/
  KEY88_HOME,			/*	SDLK_HOME		= 278,	*/
  KEY88_HELP,			/*	SDLK_END		= 279,	*/
  KEY88_ROLLDOWN,		/*	SDLK_PAGEUP		= 280,	*/
  KEY88_ROLLUP,			/*	SDLK_PAGEDOWN		= 281,	*/
  KEY88_F1,			/*	SDLK_F1			= 282,	*/
  KEY88_F2,			/*	SDLK_F2			= 283,	*/
  KEY88_F3,			/*	SDLK_F3			= 284,	*/
  KEY88_F4,			/*	SDLK_F4			= 285,	*/
  KEY88_F5,			/*	SDLK_F5			= 286,	*/
  KEY88_F6,			/*	SDLK_F6			= 287,	*/
  KEY88_F7,			/*	SDLK_F7			= 288,	*/
  KEY88_F8,			/*	SDLK_F8			= 289,	*/
  KEY88_F9,			/*	SDLK_F9			= 290,	*/
  KEY88_F10,			/*	SDLK_F10		= 291,	*/
  KEY88_F11,			/*	SDLK_F11		= 292,	*/
  KEY88_F12,			/*	SDLK_F12		= 293,	*/
  KEY88_F13,			/*	SDLK_F13		= 294,	*/
  KEY88_F14,			/*	SDLK_F14		= 295,	*/
  KEY88_F15,			/*	SDLK_F15		= 296,	*/
  0, 0, 0,
  0,				/*	SDLK_NUMLOCK		= 300,	*/
  KEY88_CAPS,			/*	SDLK_CAPSLOCK		= 301,	*/
  KEY88_KANA,			/*	SDLK_SCROLLOCK		= 302,	*/
  KEY88_SHIFTR,			/*	SDLK_RSHIFT		= 303,	*/
  KEY88_SHIFTL,			/*	SDLK_LSHIFT		= 304,	*/
  KEY88_CTRL,			/*	SDLK_RCTRL		= 305,	*/
  KEY88_CTRL,			/*	SDLK_LCTRL		= 306,	*/
  KEY88_GRAPH,			/*	SDLK_RALT		= 307,	*/
  KEY88_GRAPH,			/*	SDLK_LALT		= 308,	*/
  KEY88_GRAPH,			/*	SDLK_RMETA		= 309,	*/
  KEY88_GRAPH,			/*	SDLK_LMETA		= 310,	*/
  0,				/*	SDLK_LSUPER		= 311,	*/
  0,				/*	SDLK_RSUPER		= 312,	*/
  0,				/*	SDLK_MODE		= 313,	*/
  0,				/*	SDLK_COMPOSE		= 314,	*/
  KEY88_HELP,			/*	SDLK_HELP		= 315,	*/
  KEY88_COPY,			/*	SDLK_PRINT		= 316,	*/
  0,				/*	SDLK_SYSREQ		= 317,	*/
  KEY88_STOP,			/*	SDLK_BREAK		= 318,	*/
  0,				/*	SDLK_MENU		= 319,	*/
  0,				/*	SDLK_POWER		= 320,	*/
  0,				/*	SDLK_EURO		= 321,	*/
  0,				/*	SDLK_UNDO		= 322,	*/
};



/*----------------------------------------------------------------------
 * keysym2key88[] と scancode2key88[] の一部を、変更するテーブル
 *----------------------------------------------------------------------*/

typedef struct{
  int type;		/* 0:終端 / 1:keysym / 2:scancode	*/
  int code;		/* キーシンボル、ないし、スキャンコード	*/
  int key88;
} T_REMAPPING;


const T_REMAPPING remapping_x11_106[] =
{
  {	1,  SDLK_LSUPER,	KEY88_KANA,		},
  {	1,  SDLK_RALT,		KEY88_ZENKAKU,		},
/*{	1,  SDLK_RCTRL,		KEY88_UNDERSCORE,	},*/
  {	1,  SDLK_MENU,		KEY88_SYS_MENU, 	},
  {	2,  49,			KEY88_ZENKAKU,		},	/* 半角全角 */
  {	2,  133,		KEY88_YEN,		},	/* \ |      */
  {	2,  123,		KEY88_UNDERSCORE,	},	/* \ _ ロ   */
  {	2,  131,		KEY88_KETTEI,		},
  {	2,  129,		KEY88_HENKAN,		},
  {	2,  120,		KEY88_KANA,		},	/* カタひら */
  {	0,  0,			0,			},
};

const T_REMAPPING remapping_x11_101[] =
{
  {	1,  SDLK_BACKQUOTE,	KEY88_YEN,		},
  {	1,  SDLK_EQUALS,	KEY88_CARET,		},
  {	1,  SDLK_BACKSLASH,	KEY88_AT,		},
  {	1,  SDLK_QUOTE,		KEY88_COLON,		},
  {	1,  SDLK_LSUPER,	KEY88_KANA,		},
  {	1,  SDLK_RALT,		KEY88_ZENKAKU,		},
  {	1,  SDLK_RCTRL,		KEY88_UNDERSCORE,	},
  {	1,  SDLK_MENU,		KEY88_SYS_MENU, 	},
  {	0,  0,			0,			},
};

const T_REMAPPING remapping_windib_106[] =
{
  {	1,  SDLK_BACKQUOTE,	0,			},	/* 半角全角 */
  {	1,  SDLK_EQUALS,	KEY88_CARET,		},	/* ^        */
  {	1,  SDLK_LEFTBRACKET,	KEY88_AT,		},	/* @        */
  {	1,  SDLK_RIGHTBRACKET,	KEY88_BRACKETLEFT,	},	/* [        */
  {	1,  SDLK_QUOTE,		KEY88_COLON,		},	/* :        */
  {	1,  SDLK_LSUPER,	KEY88_KANA,		},	/* 左Window */
  {	1,  SDLK_RALT,		KEY88_ZENKAKU,		},	/* 右Alt    */
/*{	1,  SDLK_RCTRL,		KEY88_UNDERSCORE,	},*/	/* 右Ctrl   */
  {	1,  SDLK_MENU,		KEY88_SYS_MENU, 	},	/* Menu     */
  {	2,  125,		KEY88_YEN,		},	/* \ |      */
  {	2,   43,		KEY88_BRACKETRIGHT,	},	/* ] }      */
  {	2,  115,		KEY88_UNDERSCORE,	},	/* \ _ ロ   */
  {	2,  123,		KEY88_KETTEI,		},	/* 無変換   */
  {	2,  121,		KEY88_HENKAN,		},	/* 変換     */
/*{	2,  112,		0,			},*/	/* カタひら */
  {	0,  0,			0,			},
};

const T_REMAPPING remapping_windib_101[] =
{
  {	1,  SDLK_BACKQUOTE,	KEY88_YEN,		}, 	/* `        */
  {	1,  SDLK_EQUALS,	KEY88_CARET,		}, 	/* =        */
  {	1,  SDLK_BACKSLASH,	KEY88_AT,		},	/* \        */
  {	1,  SDLK_QUOTE,		KEY88_COLON,		}, 	/* '        */
  {	1,  SDLK_LSUPER,	KEY88_KANA,		},	/* 左Window */
  {	1,  SDLK_RALT,		KEY88_ZENKAKU,		},	/* 右Alt    */
  {	1,  SDLK_RCTRL,		KEY88_UNDERSCORE,	},	/* 右Ctrl   */
  {	1,  SDLK_MENU,		KEY88_SYS_MENU, 	},	/* Menu     */
  {	0,  0,			0,			},
};

const T_REMAPPING remapping_directx_106[] =
{
  {	1,  SDLK_BACKSLASH,	KEY88_UNDERSCORE,	},	/* \ _ ロ   */
  {	1,  SDLK_LMETA,		KEY88_KANA,		},	/* 左Window */
  {	1,  SDLK_RALT,		KEY88_ZENKAKU,		},	/* 右Alt    */
/*{	1,  SDLK_RCTRL,		KEY88_UNDERSCORE,	},*/	/* 右Ctrl   */
  {	1,  SDLK_MENU,		KEY88_SYS_MENU, 	},	/* Menu     */
/*{	2,  148,		0,			},*/	/* 半角全角 */
  {	2,  144,		KEY88_CARET,		},	/* ^        */
  {	2,  125,		KEY88_YEN,		},	/* \        */
  {	2,  145,		KEY88_AT,		},	/* @        */
  {	2,  146,		KEY88_COLON,		},	/* :        */
  {	2,  123,		KEY88_KETTEI,		},	/* 無変換   */
  {	2,  121,		KEY88_HENKAN,		},	/* 変換     */
  {	2,  112,		KEY88_KANA,		},	/* カタひら */
  {	0,  0,			0,			},
};

const T_REMAPPING remapping_directx_101[] =
{
  {	1,  SDLK_BACKQUOTE,	KEY88_YEN,		}, 	/* `        */
  {	1,  SDLK_EQUALS,	KEY88_CARET,		}, 	/* =        */
  {	1,  SDLK_BACKSLASH,	KEY88_AT,		},	/* \        */
  {	1,  SDLK_QUOTE,		KEY88_COLON,		}, 	/* '        */
  {	1,  SDLK_LMETA,		KEY88_KANA,		},	/* 左Window */
  {	1,  SDLK_RALT,		KEY88_ZENKAKU,		},	/* 右Alt    */
  {	1,  SDLK_RCTRL,		KEY88_UNDERSCORE,	},	/* 右Ctrl   */
  {	1,  SDLK_MENU,		KEY88_SYS_MENU, 	},	/* Menu     */
  {	2,  148,		KEY88_YEN,		},
  {	2,  144,		KEY88_CARET,		},
  {	2,  145,		KEY88_AT,		},
  {	2,  146,		KEY88_COLON,		},
  {	2,  125,		KEY88_YEN,		},
  {	0,  0,			0,			},
};

const T_REMAPPING remapping_toolbox_106[] =
{
  {	1,  SDLK_LMETA,		KEY88_SYS_MENU,		},
  {	1,  SDLK_RMETA,		KEY88_SYS_MENU,		},
  {	2,  95,			KEY88_KP_COMMA,		},
/*{	2,  102,		0,			},*/	 /* 英数    */
/*{	2,  104,		0,			},*/	 /* カナ    */
  {	0,  0,			0,			},
};

const T_REMAPPING remapping_toolbox_101[] =
{
  {	1,  SDLK_LMETA,		KEY88_SYS_MENU,		},
  {	1,  SDLK_RMETA,		KEY88_SYS_MENU,		},
  {	1,  SDLK_BACKQUOTE,	KEY88_YEN,		},
  {	1,  SDLK_EQUALS,	KEY88_CARET,		},
  {	1,  SDLK_BACKSLASH,	KEY88_AT,		},
  {	1,  SDLK_QUOTE,		KEY88_COLON,		},
  {	0,  0,			0,			},
};

const T_REMAPPING remapping_dummy[] =
{
  {	0,  0,			0,			},
};



/*----------------------------------------------------------------------
 * ソフトウェア NumLock オン/オフ 時のマッピング変更テーブル
 *----------------------------------------------------------------------*/

static const T_BINDING binding_106[] =
{
  {	1,	SDLK_5,		0,	KEY88_HOME,		},
  {	1,	SDLK_6,		0,	KEY88_HELP,		},
  {	1,	SDLK_7,		0,	KEY88_KP_7,		},
  {	1,	SDLK_8,		0,	KEY88_KP_8,		},
  {	1,	SDLK_9,		0,	KEY88_KP_9,		},
  {	1,	SDLK_0,		0,	KEY88_KP_MULTIPLY,	},
  {	1,	SDLK_MINUS,	0,	KEY88_KP_SUB,		},
  {	1,	SDLK_CARET,	0,	KEY88_KP_DIVIDE,	},
  {	1,	SDLK_u,		0,	KEY88_KP_4,		},
  {	1,	SDLK_i,		0,	KEY88_KP_5,		},
  {	1,	SDLK_o,		0,	KEY88_KP_6,		},
  {	1,	SDLK_p,		0,	KEY88_KP_ADD,		},
  {	1,	SDLK_j,		0,	KEY88_KP_1,		},
  {	1,	SDLK_k,		0,	KEY88_KP_2,		},
  {	1,	SDLK_l,		0,	KEY88_KP_3,		},
  {	1,	SDLK_SEMICOLON,	0,	KEY88_KP_EQUAL,		},
  {	1,	SDLK_m,		0,	KEY88_KP_0,		},
  {	1,	SDLK_COMMA,	0,	KEY88_KP_COMMA,		},
  {	1,	SDLK_PERIOD,	0,	KEY88_KP_PERIOD,	},
  {	1,	SDLK_SLASH,	0,	KEY88_RETURNR,		},
  {	0,	0,		0,	0,			},
};

static const T_BINDING binding_101[] =
{
  {	1,	SDLK_5,		0,	KEY88_HOME,		},
  {	1,	SDLK_6,		0,	KEY88_HELP,		},
  {	1,	SDLK_7,		0,	KEY88_KP_7,		},
  {	1,	SDLK_8,		0,	KEY88_KP_8,		},
  {	1,	SDLK_9,		0,	KEY88_KP_9,		},
  {	1,	SDLK_0,		0,	KEY88_KP_MULTIPLY,	},
  {	1,	SDLK_MINUS,	0,	KEY88_KP_SUB,		},
  {	1,	SDLK_EQUALS,	0,	KEY88_KP_DIVIDE,	},
  {	1,	SDLK_u,		0,	KEY88_KP_4,		},
  {	1,	SDLK_i,		0,	KEY88_KP_5,		},
  {	1,	SDLK_o,		0,	KEY88_KP_6,		},
  {	1,	SDLK_p,		0,	KEY88_KP_ADD,		},
  {	1,	SDLK_j,		0,	KEY88_KP_1,		},
  {	1,	SDLK_k,		0,	KEY88_KP_2,		},
  {	1,	SDLK_l,		0,	KEY88_KP_3,		},
  {	1,	SDLK_SEMICOLON,	0,	KEY88_KP_EQUAL,		},
  {	1,	SDLK_m,		0,	KEY88_KP_0,		},
  {	1,	SDLK_COMMA,	0,	KEY88_KP_COMMA,		},
  {	1,	SDLK_PERIOD,	0,	KEY88_KP_PERIOD,	},
  {	1,	SDLK_SLASH,	0,	KEY88_RETURNR,		},
  {	0,	0,		0,	0,			},
};

static const T_BINDING binding_directx[] =
{
  {	1,	SDLK_5,		0,	KEY88_HOME,		},
  {	1,	SDLK_6,		0,	KEY88_HELP,		},
  {	1,	SDLK_7,		0,	KEY88_KP_7,		},
  {	1,	SDLK_8,		0,	KEY88_KP_8,		},
  {	1,	SDLK_9,		0,	KEY88_KP_9,		},
  {	1,	SDLK_0,		0,	KEY88_KP_MULTIPLY,	},
  {	1,	SDLK_MINUS,	0,	KEY88_KP_SUB,		},
  {	1,	SDLK_EQUALS,	0,	KEY88_KP_DIVIDE,	},
  {	2,	144,		0,	KEY88_KP_DIVIDE,	},
  {	1,	SDLK_u,		0,	KEY88_KP_4,		},
  {	1,	SDLK_i,		0,	KEY88_KP_5,		},
  {	1,	SDLK_o,		0,	KEY88_KP_6,		},
  {	1,	SDLK_p,		0,	KEY88_KP_ADD,		},
  {	1,	SDLK_j,		0,	KEY88_KP_1,		},
  {	1,	SDLK_k,		0,	KEY88_KP_2,		},
  {	1,	SDLK_l,		0,	KEY88_KP_3,		},
  {	1,	SDLK_SEMICOLON,	0,	KEY88_KP_EQUAL,		},
  {	1,	SDLK_m,		0,	KEY88_KP_0,		},
  {	1,	SDLK_COMMA,	0,	KEY88_KP_COMMA,		},
  {	1,	SDLK_PERIOD,	0,	KEY88_KP_PERIOD,	},
  {	1,	SDLK_SLASH,	0,	KEY88_RETURNR,		},
  {	0,	0,		0,	0,			},
};



/******************************************************************************
 * メニューに入る際のキーの名称を返す
 *	とりあえず、起動時のステータスの表示にて使用。
 *	本当はメニューモードの中でも反映させる必要があるのだが ………
 *		面倒なので、実装は今後の課題にしよう
 *****************************************************************************/
const	char	*get_keysym_menu( void )
{
#if	defined( QUASI88_FMAC )
  if( use_cmdkey ){
    return	"Command";
  }
#endif

  return	"F12";
}


/******************************************************************************
 * イベントハンドリング
 *
 *	1/60毎に呼び出される。
 *****************************************************************************/
static	int	analyze_keyconf_file( const char *video_driver );

/*
 * これは 起動時に1回だけ呼ばれる
 */
void	event_handle_init( void )
{
  const T_REMAPPING *map;
  const T_BINDING   *bin;
  int i;
  char video_driver[32];


  if( SDL_VideoDriverName( video_driver, sizeof(video_driver) ) == NULL ){
    memset( video_driver, 0, sizeof(video_driver) );
  }

  memset( keysym2key88, 0, sizeof(keysym2key88) );
  for( i=0; i<COUNTOF(scancode2key88); i++ ){
    scancode2key88[ i ] = -1;
  }
  memset( binding, 0, sizeof(binding) );

  function_f[ 11 ] = FN_STATUS;
  function_f[ 12 ] = FN_MENU;



  switch( keyboard_type ){

  case 0:					/* デフォルトキーボード */
    if( analyze_keyconf_file( video_driver ) ){
      ;
    }else{
      memcpy( keysym2key88, keysym2key88_default,sizeof(keysym2key88_default));
      memcpy( binding, binding_106, sizeof(binding_106) );
    }
    break;


  case 1:					/* 106日本語キーボード */
  case 2:					/* 101英語キーボード ? */
    memcpy( keysym2key88, keysym2key88_default, sizeof(keysym2key88_default) );

#if	defined( QUASI88_FUNIX )

    if( keyboard_type == 1 ){ map = remapping_x11_106;	bin = binding_106; }
    else                    { map = remapping_x11_101;	bin = binding_101; }

#elif	defined( QUASI88_FWIN )

    if( strcmp( video_driver, "directx" ) == 0 ){
      if( keyboard_type == 1 ) map = remapping_directx_106;
      else                     map = remapping_directx_101;
      bin = binding_directx;
    }else{
      if( keyboard_type == 1 ) map = remapping_windib_106;
      else                     map = remapping_windib_101;
      bin = binding_101;
    }

#elif	defined( QUASI88_FMAC )

    if( keyboard_type == 1 ){ map = remapping_toolbox_106; bin = binding_106; }
    else                    { map = remapping_toolbox_101; bin = binding_101; }

    if( use_cmdkey == FALSE ){
      map += 2;
    }

#else
    map = remapping_dummy;
    bin = binding_106;
#endif

    for( ;  map->type;  map ++ ){

      if      ( map->type == 1 ){

	keysym2key88[ map->code ] = map->key88;

      }else if( map->type == 2 ){

	scancode2key88[ map->code ] = map->key88;

      }
    }

    for( i=0; i<COUNTOF(binding); i++ ){
      if( bin->type == 0 ) break;

      binding[ i ].type      = bin->type;
      binding[ i ].code      = bin->code;
      binding[ i ].org_key88 = bin->org_key88;
      binding[ i ].new_key88 = bin->new_key88;
      bin ++;
    }
    break;
  }



  /* ソフトウェアNumLock 時のキー差し替えの準備 */

  for( i=0; i<COUNTOF(binding); i++ ){

    if      ( binding[i].type == 1 ){

      binding[i].org_key88 = keysym2key88[ binding[i].code ];

    }else if( binding[i].type == 2 ){

      binding[i].org_key88 = scancode2key88[ binding[i].code ];

    }else{
      break;
    }
  }

}



/*
 * 約 1/60 毎に呼ばれる
 */
void	event_handle( void )
{
  static enum {
    AXIS_U = 0x01,
    AXIS_D = 0x02,
    AXIS_L = 0x04,
    AXIS_R = 0x08
  } pad_axis = 0x00;


  SDL_Event E;
  SDLKey keysym;
  int    key88, x, y;


  SDL_PumpEvents();		/* イベントを汲み上げる */

  while( SDL_PeepEvents(&E, 1, SDL_GETEVENT,
			SDL_EVENTMASK(SDL_KEYDOWN)        | 
			SDL_EVENTMASK(SDL_KEYUP)          |
			SDL_EVENTMASK(SDL_MOUSEMOTION)    |
			SDL_EVENTMASK(SDL_MOUSEBUTTONDOWN)|
			SDL_EVENTMASK(SDL_MOUSEBUTTONUP)  |
			SDL_EVENTMASK(SDL_JOYAXISMOTION)  |
			SDL_EVENTMASK(SDL_JOYBUTTONDOWN)  |
			SDL_EVENTMASK(SDL_JOYBUTTONUP)    |
			SDL_EVENTMASK(SDL_VIDEOEXPOSE)    |
			SDL_EVENTMASK(SDL_ACTIVEEVENT)    |
    			SDL_EVENTMASK(SDL_QUIT)) ) {

    switch( E.type ){

    case SDL_KEYDOWN:		/*------------------------------------------*/
    case SDL_KEYUP:

      keysym  = E.key.keysym.sym;

      if( now_unicode ){		/* キーUNICODE時 (メニューなど) */

	if( E.key.keysym.unicode <= 0xff &&
	    isprint( E.key.keysym.unicode ) ){
	  keysym = E.key.keysym.unicode;
	}
	if( SDLK_SPACE <= keysym && keysym < SDLK_DELETE ) key88 = keysym;
	else                                 key88 = keysym2key88[ keysym ];

      }else{				/* キーASCII時 */

	key88 = keysym2key88[ keysym ];

	if( E.key.keysym.scancode < COUNTOF(scancode2key88) &&
	    scancode2key88[ E.key.keysym.scancode ] >= 0    ){

	  key88 = scancode2key88[ E.key.keysym.scancode ];
	}

/*printf("%d %d %d\n",key88,keysym,E.key.keysym.scancode );*/
      }
      pc88_key( key88, (E.type==SDL_KEYDOWN) );

      break;

    case SDL_MOUSEMOTION:	/*------------------------------------------*/
      if( mouse_rel_move ){		/* マウスがウインドウの端に届いても */
					/* 相対的な動きを検出できる場合     */
	x = E.motion.xrel;
	y = E.motion.yrel;
#if 0
	if( get_emu_mode()==EXEC ){	/* マウスの速度調整が可能だけども */
	  x /= 10;			/* マウスを表示している場合、     */
	  y /= 10;			/* 表示位置と座標がずれるので注意 */
	}
#endif
	pc88_mouse_moved_rel( x, y );
      

      }else{

	x = E.motion.x;
	y = E.motion.y;

	x -= SCREEN_DX;
	y -= SCREEN_DY;
	if     ( now_screen_size == SCREEN_SIZE_HALF )  { x *= 2; y *= 2; }
#ifdef	SUPPORT_DOUBLE
	else if( now_screen_size == SCREEN_SIZE_DOUBLE ){ x /= 2; y /= 2; }
#endif
	pc88_mouse_moved_abs( x, y );
      }
      break;


    case SDL_MOUSEBUTTONDOWN:	/*------------------------------------------*/
    case SDL_MOUSEBUTTONUP:

      /* マウス移動イベントも同時に処理する必要があるなら、
	 pc88_mouse_moved_abs/rel 関数をここで呼び出しておく */

      if     ( E.button.button==SDL_BUTTON_LEFT      ) key88 = KEY88_MOUSE_L;
      else if( E.button.button==SDL_BUTTON_MIDDLE    ) key88 = KEY88_MOUSE_M;
      else if( E.button.button==SDL_BUTTON_RIGHT     ) key88 = KEY88_MOUSE_R;
      else if( E.button.button==SDL_BUTTON_WHEELUP   ) key88 = KEY88_MOUSE_WUP;
      else if( E.button.button==SDL_BUTTON_WHEELDOWN ) key88 = KEY88_MOUSE_WDN;
      else break;

      pc88_mouse( key88, (E.type==SDL_MOUSEBUTTONDOWN) );
      break;


    case SDL_JOYAXISMOTION:	/*------------------------------------------*/
/*printf("%d %d %d\n",E.jaxis.which,E.jaxis.axis,E.jaxis.value);*/
      if( E.jbutton.which == 0 ){
	int now, chg;

	if( E.jaxis.axis == 0 ){	/* 左右方向 */

	  now = pad_axis & ~(AXIS_L|AXIS_R);

	  if     ( E.jaxis.value < -0x4000 ) now |= AXIS_L;
	  else if( E.jaxis.value >  0x4000 ) now |= AXIS_R;

	  chg = pad_axis ^ now;
	  if( chg & AXIS_L ) pc88_pad( KEY88_PAD_LEFT,  (now & AXIS_L) );
	  if( chg & AXIS_R ) pc88_pad( KEY88_PAD_RIGHT, (now & AXIS_R) );

	}else{				/* 上下方向 */

	  now = pad_axis & ~(AXIS_U|AXIS_D);

	  if     ( E.jaxis.value < -0x4000 ) now |= AXIS_U;
	  else if( E.jaxis.value >  0x4000 ) now |= AXIS_D;

	  chg = pad_axis ^ now;
	  if( chg & AXIS_U ) pc88_pad( KEY88_PAD_UP,    (now & AXIS_U) );
	  if( chg & AXIS_D ) pc88_pad( KEY88_PAD_DOWN,  (now & AXIS_D) );
	}
	pad_axis = now;
      }
      break;


    case SDL_JOYBUTTONDOWN:	/*------------------------------------------*/
    case SDL_JOYBUTTONUP:
/*printf("%d %d\n",E.jbutton.which,E.jbutton.button);*/
      if( E.jbutton.which == 0 ){
	if( E.jbutton.button < 8 ){
	  key88 = KEY88_PAD_A + E.jbutton.button;
	  pc88_pad( key88, ( E.type==SDL_JOYBUTTONDOWN ) );
	}
      }
      break;


    case SDL_QUIT:		/*------------------------------------------*/
      if( verbose_proc ) printf( "Window Closed.....\n" );
      pc88_quit();
      break;


    case SDL_ACTIVEEVENT:	/*------------------------------------------*/
      /* -focus オプションを機能させたいなら、 
	 pc88_focus_in / pc88_focus_out を適宜呼び出す必要がある。 */

      if( E.active.state & SDL_APPINPUTFOCUS ){
	if( E.active.gain ){
	  pc88_focus_in();
	}else{
	  pc88_focus_out();
	}
      }
      break;


    case SDL_VIDEOEXPOSE:	/*------------------------------------------*/
      put_image_all();			/* EXPOSE 時は 勝手に再描画しておく */
      break;
    }
  }
}



/***********************************************************************
 * 現在のマウス座標取得関数
 *
 *	現在のマウスの絶対座標を *x, *y にセット
 ************************************************************************/

void	init_mouse_position( int *x, int *y )
{
  int win_x, win_y;

  SDL_PumpEvents();
  SDL_GetMouseState( &win_x, &win_y );

  win_x -= SCREEN_DX;
  win_y -= SCREEN_DY;

  if     ( now_screen_size == SCREEN_SIZE_HALF )  { win_x *= 2;  win_y *= 2; }
#ifdef	SUPPORT_DOUBLE
  else if( now_screen_size == SCREEN_SIZE_DOUBLE ){ win_x /= 2;  win_y /= 2; }
#endif
  *x = win_x;
  *y = win_y;
}




/******************************************************************************
 * ソフトウェア NumLock 有効／無効
 *
 *****************************************************************************/

INLINE	void	numlock_setup( int enable )
{
  int i;

  for( i=0; i<COUNTOF(binding); i++ ){

    if      ( binding[i].type == 1 ){

      if( enable ){
	keysym2key88[ binding[i].code ] = binding[i].new_key88;
      }else{
	keysym2key88[ binding[i].code ] = binding[i].org_key88;
      }

    }else if( binding[i].type == 2 ){

      if( enable ){
	scancode2key88[ binding[i].code ] = binding[i].new_key88;
      }else{
	scancode2key88[ binding[i].code ] = binding[i].org_key88;
      }

    }else{
      break;
    }
  }
}

int	numlock_on ( void ){ numlock_setup( TRUE );  return TRUE; }
void	numlock_off( void ){ numlock_setup( FALSE ); }



/******************************************************************************
 * エミュレート／メニュー／ポーズ／モニターモード の 開始時の処理
 *
 *****************************************************************************/

void	event_init( void )
{
  /* 既存のイベントをすべて破棄 */
  /* なんてことは、しない ? */

  if( get_emu_mode() == EXEC ){

    if( use_unicode ){

      /* キー押下を UNICODE に変換可能とする (処理が重いらしいので指定時のみ)*/
      SDL_EnableUNICODE( 1 );
      now_unicode = TRUE;

    }else{

      /* キー押下を ASCII コードに変換可能とする */
      SDL_EnableUNICODE( 0 );
      now_unicode = FALSE;
    }

  }else{

    SDL_EnableUNICODE( 1 );
    now_unicode = TRUE;

  }

  /* マウス表示、グラブの設定 (ついでにキーリピートも) */
  set_mouse_state();
}



/******************************************************************************
 * ジョイスティック
 *
 * 一度だけの初期化時点（ウインドウとか？）にて、オープンする。
 *****************************************************************************/

int	joy_init( void )
{
  joy = NULL;

  if( ! SDL_WasInit( SDL_INIT_JOYSTICK ) ){
    if( SDL_InitSubSystem( SDL_INIT_JOYSTICK ) ){
      return FALSE;
    }
  }

  if( SDL_NumJoysticks() >= 1 ){	/* ジョイスティックがあるかチェック */

    joy = SDL_JoystickOpen(0);		/* ジョイスティックをオープン */

    if( joy )
      SDL_JoystickEventState( SDL_ENABLE );	/* イベント処理を有効にする */
  }

  if( joy ) return TRUE;
  else      return FALSE;
}



int	joystick_available( void )
{
  if( joy ) return TRUE;
  else      return FALSE;
}






/****************************************************************************
 * キー設定ファイルを読み込んで、設定する。
 *	設定ファイルが無ければ偽、あれば処理して真を返す
 *****************************************************************************/

/* キー設定ファイル1行あたりの最大文字数 */
#define	MAX_KEYFILE_LINE	(256)

static	int	convert_str2keysym( const char *str );
static	int	analyze_keyconf_file( const char *video_driver )
{
  FILE *fp = NULL;
  int	enable = FALSE;
  int	initialized = FALSE;

  int  line_cnt = 0;
  int  binding_cnt = 0;
  int  code, key88, chg = -1;
  char   line[ MAX_KEYFILE_LINE ];
  char buffer[ MAX_KEYFILE_LINE ];
  char *parm1, *parm2, *parm3, *parm4;


	/* キー設定ファイルを開く */

  if( file_keyboard == NULL ){			/* キー設定ファイル名取得 */
    file_keyboard = alloc_keyboard_cfgname();
  }

  if( file_keyboard ){
    fp = fopen( file_keyboard, "r" );		/* キー設定ファイルを開く */
    if( verbose_proc ){
      if( fp ){
	printf( "\"%s\" read and initialize\n", file_keyboard );
      }else{
	printf( "can't open keyboard configuration file \"%s\"\n",
		file_keyboard );
	printf( "\n" );
      }
      fflush(stdout); fflush(stderr);
    }
    free( file_keyboard );
  }

  if( fp == NULL ) return 0;			/* 開けなかったら偽を返す */



	/* キー設定ファイルを1行づつ解析 */

  while( fgets( line, MAX_KEYFILE_LINE, fp ) ){

    line_cnt ++;
    parm1 = parm2 = parm3 = parm4 = NULL;

	/* パラメータを parm1〜parm4 にセット */

    { char *str = line;
      char *b; {             b = &buffer[0];    str = my_strtok( b, str ); }
      if( str ){ parm1 = b;  b += strlen(b)+1;  str = my_strtok( b, str ); }
      if( str ){ parm2 = b;  b += strlen(b)+1;  str = my_strtok( b, str ); }
      if( str ){ parm3 = b;  b += strlen(b)+1;  str = my_strtok( b, str ); }
      if( str ){ parm4 = b;  }
    }


	/* パラメータがなければ次の行へ、あれば解析処理 */

    if      ( parm1 == NULL ){			/* パラメータなし */
      ;

    }else if( parm2 == NULL ){			/* パラメータ 1個 */

      if( parm1[0] == '[' ){

	if( my_strcmp( parm1, "[SDL]" ) == 0 &&
	    initialized == FALSE ){

	  enable      = TRUE;
	  initialized = TRUE;
	  if( verbose_proc ) printf( "(read start in line %d)\n", line_cnt );

	}else{
	  if( enable ){
	    if(verbose_proc) printf( "(read end   in line %d)\n", line_cnt-1);
	  }
	  enable = FALSE;
	}

      }else if( enable ){
	fprintf( stderr, "warning: error in line %d (ignored)\n", line_cnt );
      }

    }else if( parm3 == NULL ||			/* パラメータ 2個 */
	      parm4 == NULL ){			/* パラメータ 3個 */

      if( parm3 == NULL   &&
	  parm1[0] == '[' ){

	if( my_strcmp( parm1, "[SDL]" )      == 0 &&
	    my_strcmp( parm2, video_driver ) == 0 ){

	  enable      = TRUE;
	  initialized = TRUE;
	  if( verbose_proc ) printf( "(read start in line %d)\n", line_cnt );

	}else{
	  if( enable ){
	    if(verbose_proc) printf( "(read end   in line %d)\n", line_cnt-1);
	  }
	  enable = FALSE;
	}

      }else if( enable ){

	code  = convert_str2keysym( parm1 );
	key88 = convert_str2key88( parm2 );

	if( parm3 ) chg = convert_str2key88( parm3 );


	if( code  < 0 ||
	    key88 < 0 ||
	    (parm3 && chg < 0) ){
	  fprintf( stderr, "warning: error in line %d (ignored)\n", line_cnt );

	}else{

	  if( parm1[0] == '<' ){
	    scancode2key88[ code ] = key88;
	  }else{
	    keysym2key88[ code ]   = key88;
	  }

	  if( parm3 ){
	    if( binding_cnt >= COUNTOF(binding) ){
	      fprintf( stderr,
		"warning: too many NumLock-code in %d (ignored)\n", line_cnt );

	    }else{
	      binding[ binding_cnt ].type      = ( parm1[0] == '<' ) ? 2 : 1;
	      binding[ binding_cnt ].code      = code;
	      binding[ binding_cnt ].new_key88 = chg;
	      binding_cnt ++;
	    }
	  }
	}
      }

    }else{					/* パラメータ いっぱい */

      if( enable ){
	fprintf( stderr, "warning: too many argument in line %d\n", line_cnt );
      }

    }

  }
  fclose(fp);


  if( enable ){
    if(verbose_proc) printf( "(read end   in line %d)\n", line_cnt-1);
  }

  if( initialized == FALSE ){
    fprintf( stderr, "warning: not configured (use initial config)\n" );
  }

  if( verbose_proc ){
    printf( "\n" );
  }
  fflush(stdout); fflush(stderr);

  return (initialized) ? 1 : 0;
}


/*---------------------------------------------------------------------------
 *---------------------------------------------------------------------------*/
static	int	convert_str2keysym( const char *str )
{
  const struct{
    char *name;    int val;
  }
  list[] = {
    {	"SDLK_BACKSPACE",	SDLK_BACKSPACE		}, /*	= 8,	*/
    {	"SDLK_TAB",		SDLK_TAB		}, /*	= 9,	*/
    {	"SDLK_CLEAR",		SDLK_CLEAR		}, /*	= 12,	*/
    {	"SDLK_RETURN",		SDLK_RETURN		}, /*	= 13,	*/
    {	"SDLK_PAUSE",		SDLK_PAUSE		}, /*	= 19,	*/
    {	"SDLK_ESCAPE",		SDLK_ESCAPE		}, /*	= 27,	*/
    {	"SDLK_SPACE",		SDLK_SPACE		}, /*	= 32,	*/
    {	"SDLK_EXCLAIM",		SDLK_EXCLAIM		}, /*	= 33,	*/
    {	"SDLK_QUOTEDBL",	SDLK_QUOTEDBL		}, /*	= 34,	*/
    {	"SDLK_HASH",		SDLK_HASH		}, /*	= 35,	*/
    {	"SDLK_DOLLAR",		SDLK_DOLLAR		}, /*	= 36,	*/
    {	"SDLK_AMPERSAND",	SDLK_AMPERSAND		}, /*	= 38,	*/
    {	"SDLK_QUOTE",		SDLK_QUOTE		}, /*	= 39,	*/
    {	"SDLK_LEFTPAREN",	SDLK_LEFTPAREN		}, /*	= 40,	*/
    {	"SDLK_RIGHTPAREN",	SDLK_RIGHTPAREN		}, /*	= 41,	*/
    {	"SDLK_ASTERISK",	SDLK_ASTERISK		}, /*	= 42,	*/
    {	"SDLK_PLUS",		SDLK_PLUS		}, /*	= 43,	*/
    {	"SDLK_COMMA",		SDLK_COMMA		}, /*	= 44,	*/
    {	"SDLK_MINUS",		SDLK_MINUS		}, /*	= 45,	*/
    {	"SDLK_PERIOD",		SDLK_PERIOD		}, /*	= 46,	*/
    {	"SDLK_SLASH",		SDLK_SLASH		}, /*	= 47,	*/
    {	"SDLK_0",		SDLK_0			}, /*	= 48,	*/
    {	"SDLK_1",		SDLK_1			}, /*	= 49,	*/
    {	"SDLK_2",		SDLK_2			}, /*	= 50,	*/
    {	"SDLK_3",		SDLK_3			}, /*	= 51,	*/
    {	"SDLK_4",		SDLK_4			}, /*	= 52,	*/
    {	"SDLK_5",		SDLK_5			}, /*	= 53,	*/
    {	"SDLK_6",		SDLK_6			}, /*	= 54,	*/
    {	"SDLK_7",		SDLK_7			}, /*	= 55,	*/
    {	"SDLK_8",		SDLK_8			}, /*	= 56,	*/
    {	"SDLK_9",		SDLK_9			}, /*	= 57,	*/
    {	"SDLK_COLON",		SDLK_COLON		}, /*	= 58,	*/
    {	"SDLK_SEMICOLON",	SDLK_SEMICOLON		}, /*	= 59,	*/
    {	"SDLK_LESS",		SDLK_LESS		}, /*	= 60,	*/
    {	"SDLK_EQUALS",		SDLK_EQUALS		}, /*	= 61,	*/
    {	"SDLK_GREATER",		SDLK_GREATER		}, /*	= 62,	*/
    {	"SDLK_QUESTION",	SDLK_QUESTION		}, /*	= 63,	*/
    {	"SDLK_AT",		SDLK_AT			}, /*	= 64,	*/
    {	"SDLK_LEFTBRACKET",	SDLK_LEFTBRACKET	}, /*	= 91,	*/
    {	"SDLK_BACKSLASH",	SDLK_BACKSLASH		}, /*	= 92,	*/
    {	"SDLK_RIGHTBRACKET",	SDLK_RIGHTBRACKET	}, /*	= 93,	*/
    {	"SDLK_CARET",		SDLK_CARET		}, /*	= 94,	*/
    {	"SDLK_UNDERSCORE",	SDLK_UNDERSCORE		}, /*	= 95,	*/
    {	"SDLK_BACKQUOTE",	SDLK_BACKQUOTE		}, /*	= 96,	*/
    {	"SDLK_a",		SDLK_a			}, /*	= 97,	*/
    {	"SDLK_b",		SDLK_b			}, /*	= 98,	*/
    {	"SDLK_c",		SDLK_c			}, /*	= 99,	*/
    {	"SDLK_d",		SDLK_d			}, /*	= 100,	*/
    {	"SDLK_e",		SDLK_e			}, /*	= 101,	*/
    {	"SDLK_f",		SDLK_f			}, /*	= 102,	*/
    {	"SDLK_g",		SDLK_g			}, /*	= 103,	*/
    {	"SDLK_h",		SDLK_h			}, /*	= 104,	*/
    {	"SDLK_i",		SDLK_i			}, /*	= 105,	*/
    {	"SDLK_j",		SDLK_j			}, /*	= 106,	*/
    {	"SDLK_k",		SDLK_k			}, /*	= 107,	*/
    {	"SDLK_l",		SDLK_l			}, /*	= 108,	*/
    {	"SDLK_m",		SDLK_m			}, /*	= 109,	*/
    {	"SDLK_n",		SDLK_n			}, /*	= 110,	*/
    {	"SDLK_o",		SDLK_o			}, /*	= 111,	*/
    {	"SDLK_p",		SDLK_p			}, /*	= 112,	*/
    {	"SDLK_q",		SDLK_q			}, /*	= 113,	*/
    {	"SDLK_r",		SDLK_r			}, /*	= 114,	*/
    {	"SDLK_s",		SDLK_s			}, /*	= 115,	*/
    {	"SDLK_t",		SDLK_t			}, /*	= 116,	*/
    {	"SDLK_u",		SDLK_u			}, /*	= 117,	*/
    {	"SDLK_v",		SDLK_v			}, /*	= 118,	*/
    {	"SDLK_w",		SDLK_w			}, /*	= 119,	*/
    {	"SDLK_x",		SDLK_x			}, /*	= 120,	*/
    {	"SDLK_y",		SDLK_y			}, /*	= 121,	*/
    {	"SDLK_z",		SDLK_z			}, /*	= 122,	*/
    {	"SDLK_DELETE",		SDLK_DELETE		}, /*	= 127,	*/
    {	"SDLK_WORLD_0",		SDLK_WORLD_0		}, /*	= 160,	*/
    {	"SDLK_WORLD_1",		SDLK_WORLD_1		}, /*	= 161,	*/
    {	"SDLK_WORLD_2",		SDLK_WORLD_2		}, /*	= 162,	*/
    {	"SDLK_WORLD_3",		SDLK_WORLD_3		}, /*	= 163,	*/
    {	"SDLK_WORLD_4",		SDLK_WORLD_4		}, /*	= 164,	*/
    {	"SDLK_WORLD_5",		SDLK_WORLD_5		}, /*	= 165,	*/
    {	"SDLK_WORLD_6",		SDLK_WORLD_6		}, /*	= 166,	*/
    {	"SDLK_WORLD_7",		SDLK_WORLD_7		}, /*	= 167,	*/
    {	"SDLK_WORLD_8",		SDLK_WORLD_8		}, /*	= 168,	*/
    {	"SDLK_WORLD_9",		SDLK_WORLD_9		}, /*	= 169,	*/
    {	"SDLK_WORLD_10",	SDLK_WORLD_10		}, /*	= 170,	*/
    {	"SDLK_WORLD_11",	SDLK_WORLD_11		}, /*	= 171,	*/
    {	"SDLK_WORLD_12",	SDLK_WORLD_12		}, /*	= 172,	*/
    {	"SDLK_WORLD_13",	SDLK_WORLD_13		}, /*	= 173,	*/
    {	"SDLK_WORLD_14",	SDLK_WORLD_14		}, /*	= 174,	*/
    {	"SDLK_WORLD_15",	SDLK_WORLD_15		}, /*	= 175,	*/
    {	"SDLK_WORLD_16",	SDLK_WORLD_16		}, /*	= 176,	*/
    {	"SDLK_WORLD_17",	SDLK_WORLD_17		}, /*	= 177,	*/
    {	"SDLK_WORLD_18",	SDLK_WORLD_18		}, /*	= 178,	*/
    {	"SDLK_WORLD_19",	SDLK_WORLD_19		}, /*	= 179,	*/
    {	"SDLK_WORLD_20",	SDLK_WORLD_20		}, /*	= 180,	*/
    {	"SDLK_WORLD_21",	SDLK_WORLD_21		}, /*	= 181,	*/
    {	"SDLK_WORLD_22",	SDLK_WORLD_22		}, /*	= 182,	*/
    {	"SDLK_WORLD_23",	SDLK_WORLD_23		}, /*	= 183,	*/
    {	"SDLK_WORLD_24",	SDLK_WORLD_24		}, /*	= 184,	*/
    {	"SDLK_WORLD_25",	SDLK_WORLD_25		}, /*	= 185,	*/
    {	"SDLK_WORLD_26",	SDLK_WORLD_26		}, /*	= 186,	*/
    {	"SDLK_WORLD_27",	SDLK_WORLD_27		}, /*	= 187,	*/
    {	"SDLK_WORLD_28",	SDLK_WORLD_28		}, /*	= 188,	*/
    {	"SDLK_WORLD_29",	SDLK_WORLD_29		}, /*	= 189,	*/
    {	"SDLK_WORLD_30",	SDLK_WORLD_30		}, /*	= 190,	*/
    {	"SDLK_WORLD_31",	SDLK_WORLD_31		}, /*	= 191,	*/
    {	"SDLK_WORLD_32",	SDLK_WORLD_32		}, /*	= 192,	*/
    {	"SDLK_WORLD_33",	SDLK_WORLD_33		}, /*	= 193,	*/
    {	"SDLK_WORLD_34",	SDLK_WORLD_34		}, /*	= 194,	*/
    {	"SDLK_WORLD_35",	SDLK_WORLD_35		}, /*	= 195,	*/
    {	"SDLK_WORLD_36",	SDLK_WORLD_36		}, /*	= 196,	*/
    {	"SDLK_WORLD_37",	SDLK_WORLD_37		}, /*	= 197,	*/
    {	"SDLK_WORLD_38",	SDLK_WORLD_38		}, /*	= 198,	*/
    {	"SDLK_WORLD_39",	SDLK_WORLD_39		}, /*	= 199,	*/
    {	"SDLK_WORLD_40",	SDLK_WORLD_40		}, /*	= 200,	*/
    {	"SDLK_WORLD_41",	SDLK_WORLD_41		}, /*	= 201,	*/
    {	"SDLK_WORLD_42",	SDLK_WORLD_42		}, /*	= 202,	*/
    {	"SDLK_WORLD_43",	SDLK_WORLD_43		}, /*	= 203,	*/
    {	"SDLK_WORLD_44",	SDLK_WORLD_44		}, /*	= 204,	*/
    {	"SDLK_WORLD_45",	SDLK_WORLD_45		}, /*	= 205,	*/
    {	"SDLK_WORLD_46",	SDLK_WORLD_46		}, /*	= 206,	*/
    {	"SDLK_WORLD_47",	SDLK_WORLD_47		}, /*	= 207,	*/
    {	"SDLK_WORLD_48",	SDLK_WORLD_48		}, /*	= 208,	*/
    {	"SDLK_WORLD_49",	SDLK_WORLD_49		}, /*	= 209,	*/
    {	"SDLK_WORLD_50",	SDLK_WORLD_50		}, /*	= 210,	*/
    {	"SDLK_WORLD_51",	SDLK_WORLD_51		}, /*	= 211,	*/
    {	"SDLK_WORLD_52",	SDLK_WORLD_52		}, /*	= 212,	*/
    {	"SDLK_WORLD_53",	SDLK_WORLD_53		}, /*	= 213,	*/
    {	"SDLK_WORLD_54",	SDLK_WORLD_54		}, /*	= 214,	*/
    {	"SDLK_WORLD_55",	SDLK_WORLD_55		}, /*	= 215,	*/
    {	"SDLK_WORLD_56",	SDLK_WORLD_56		}, /*	= 216,	*/
    {	"SDLK_WORLD_57",	SDLK_WORLD_57		}, /*	= 217,	*/
    {	"SDLK_WORLD_58",	SDLK_WORLD_58		}, /*	= 218,	*/
    {	"SDLK_WORLD_59",	SDLK_WORLD_59		}, /*	= 219,	*/
    {	"SDLK_WORLD_60",	SDLK_WORLD_60		}, /*	= 220,	*/
    {	"SDLK_WORLD_61",	SDLK_WORLD_61		}, /*	= 221,	*/
    {	"SDLK_WORLD_62",	SDLK_WORLD_62		}, /*	= 222,	*/
    {	"SDLK_WORLD_63",	SDLK_WORLD_63		}, /*	= 223,	*/
    {	"SDLK_WORLD_64",	SDLK_WORLD_64		}, /*	= 224,	*/
    {	"SDLK_WORLD_65",	SDLK_WORLD_65		}, /*	= 225,	*/
    {	"SDLK_WORLD_66",	SDLK_WORLD_66		}, /*	= 226,	*/
    {	"SDLK_WORLD_67",	SDLK_WORLD_67		}, /*	= 227,	*/
    {	"SDLK_WORLD_68",	SDLK_WORLD_68		}, /*	= 228,	*/
    {	"SDLK_WORLD_69",	SDLK_WORLD_69		}, /*	= 229,	*/
    {	"SDLK_WORLD_70",	SDLK_WORLD_70		}, /*	= 230,	*/
    {	"SDLK_WORLD_71",	SDLK_WORLD_71		}, /*	= 231,	*/
    {	"SDLK_WORLD_72",	SDLK_WORLD_72		}, /*	= 232,	*/
    {	"SDLK_WORLD_73",	SDLK_WORLD_73		}, /*	= 233,	*/
    {	"SDLK_WORLD_74",	SDLK_WORLD_74		}, /*	= 234,	*/
    {	"SDLK_WORLD_75",	SDLK_WORLD_75		}, /*	= 235,	*/
    {	"SDLK_WORLD_76",	SDLK_WORLD_76		}, /*	= 236,	*/
    {	"SDLK_WORLD_77",	SDLK_WORLD_77		}, /*	= 237,	*/
    {	"SDLK_WORLD_78",	SDLK_WORLD_78		}, /*	= 238,	*/
    {	"SDLK_WORLD_79",	SDLK_WORLD_79		}, /*	= 239,	*/
    {	"SDLK_WORLD_80",	SDLK_WORLD_80		}, /*	= 240,	*/
    {	"SDLK_WORLD_81",	SDLK_WORLD_81		}, /*	= 241,	*/
    {	"SDLK_WORLD_82",	SDLK_WORLD_82		}, /*	= 242,	*/
    {	"SDLK_WORLD_83",	SDLK_WORLD_83		}, /*	= 243,	*/
    {	"SDLK_WORLD_84",	SDLK_WORLD_84		}, /*	= 244,	*/
    {	"SDLK_WORLD_85",	SDLK_WORLD_85		}, /*	= 245,	*/
    {	"SDLK_WORLD_86",	SDLK_WORLD_86		}, /*	= 246,	*/
    {	"SDLK_WORLD_87",	SDLK_WORLD_87		}, /*	= 247,	*/
    {	"SDLK_WORLD_88",	SDLK_WORLD_88		}, /*	= 248,	*/
    {	"SDLK_WORLD_89",	SDLK_WORLD_89		}, /*	= 249,	*/
    {	"SDLK_WORLD_90",	SDLK_WORLD_90		}, /*	= 250,	*/
    {	"SDLK_WORLD_91",	SDLK_WORLD_91		}, /*	= 251,	*/
    {	"SDLK_WORLD_92",	SDLK_WORLD_92		}, /*	= 252,	*/
    {	"SDLK_WORLD_93",	SDLK_WORLD_93		}, /*	= 253,	*/
    {	"SDLK_WORLD_94",	SDLK_WORLD_94		}, /*	= 254,	*/
    {	"SDLK_WORLD_95",	SDLK_WORLD_95		}, /*	= 255,	*/
    {	"SDLK_KP0",		SDLK_KP0		}, /*	= 256,	*/
    {	"SDLK_KP1",		SDLK_KP1		}, /*	= 257,	*/
    {	"SDLK_KP2",		SDLK_KP2		}, /*	= 258,	*/
    {	"SDLK_KP3",		SDLK_KP3		}, /*	= 259,	*/
    {	"SDLK_KP4",		SDLK_KP4		}, /*	= 260,	*/
    {	"SDLK_KP5",		SDLK_KP5		}, /*	= 261,	*/
    {	"SDLK_KP6",		SDLK_KP6		}, /*	= 262,	*/
    {	"SDLK_KP7",		SDLK_KP7		}, /*	= 263,	*/
    {	"SDLK_KP8",		SDLK_KP8		}, /*	= 264,	*/
    {	"SDLK_KP9",		SDLK_KP9		}, /*	= 265,	*/
    {	"SDLK_KP_PERIOD",	SDLK_KP_PERIOD		}, /*	= 266,	*/
    {	"SDLK_KP_DIVIDE",	SDLK_KP_DIVIDE		}, /*	= 267,	*/
    {	"SDLK_KP_MULTIPLY",	SDLK_KP_MULTIPLY	}, /*	= 268,	*/
    {	"SDLK_KP_MINUS",	SDLK_KP_MINUS		}, /*	= 269,	*/
    {	"SDLK_KP_PLUS",		SDLK_KP_PLUS		}, /*	= 270,	*/
    {	"SDLK_KP_ENTER",	SDLK_KP_ENTER		}, /*	= 271,	*/
    {	"SDLK_KP_EQUALS",	SDLK_KP_EQUALS		}, /*	= 272,	*/
    {	"SDLK_UP",		SDLK_UP			}, /*	= 273,	*/
    {	"SDLK_DOWN",		SDLK_DOWN		}, /*	= 274,	*/
    {	"SDLK_RIGHT",		SDLK_RIGHT		}, /*	= 275,	*/
    {	"SDLK_LEFT",		SDLK_LEFT		}, /*	= 276,	*/
    {	"SDLK_INSERT",		SDLK_INSERT		}, /*	= 277,	*/
    {	"SDLK_HOME",		SDLK_HOME		}, /*	= 278,	*/
    {	"SDLK_END",		SDLK_END		}, /*	= 279,	*/
    {	"SDLK_PAGEUP",		SDLK_PAGEUP		}, /*	= 280,	*/
    {	"SDLK_PAGEDOWN",	SDLK_PAGEDOWN		}, /*	= 281,	*/
    {	"SDLK_F1",		SDLK_F1			}, /*	= 282,	*/
    {	"SDLK_F2",		SDLK_F2			}, /*	= 283,	*/
    {	"SDLK_F3",		SDLK_F3			}, /*	= 284,	*/
    {	"SDLK_F4",		SDLK_F4			}, /*	= 285,	*/
    {	"SDLK_F5",		SDLK_F5			}, /*	= 286,	*/
    {	"SDLK_F6",		SDLK_F6			}, /*	= 287,	*/
    {	"SDLK_F7",		SDLK_F7			}, /*	= 288,	*/
    {	"SDLK_F8",		SDLK_F8			}, /*	= 289,	*/
    {	"SDLK_F9",		SDLK_F9			}, /*	= 290,	*/
    {	"SDLK_F10",		SDLK_F10		}, /*	= 291,	*/
    {	"SDLK_F11",		SDLK_F11		}, /*	= 292,	*/
    {	"SDLK_F12",		SDLK_F12		}, /*	= 293,	*/
    {	"SDLK_F13",		SDLK_F13		}, /*	= 294,	*/
    {	"SDLK_F14",		SDLK_F14		}, /*	= 295,	*/
    {	"SDLK_F15",		SDLK_F15		}, /*	= 296,	*/
    {	"SDLK_NUMLOCK",		SDLK_NUMLOCK		}, /*	= 300,	*/
    {	"SDLK_CAPSLOCK",	SDLK_CAPSLOCK		}, /*	= 301,	*/
    {	"SDLK_SCROLLOCK",	SDLK_SCROLLOCK		}, /*	= 302,	*/
    {	"SDLK_RSHIFT",		SDLK_RSHIFT		}, /*	= 303,	*/
    {	"SDLK_LSHIFT",		SDLK_LSHIFT		}, /*	= 304,	*/
    {	"SDLK_RCTRL",		SDLK_RCTRL		}, /*	= 305,	*/
    {	"SDLK_LCTRL",		SDLK_LCTRL		}, /*	= 306,	*/
    {	"SDLK_RALT",		SDLK_RALT		}, /*	= 307,	*/
    {	"SDLK_LALT",		SDLK_LALT		}, /*	= 308,	*/
    {	"SDLK_RMETA",		SDLK_RMETA		}, /*	= 309,	*/
    {	"SDLK_LMETA",		SDLK_LMETA		}, /*	= 310,	*/
    {	"SDLK_LSUPER",		SDLK_LSUPER		}, /*	= 311,	*/
    {	"SDLK_RSUPER",		SDLK_RSUPER		}, /*	= 312,	*/
    {	"SDLK_MODE",		SDLK_MODE		}, /*	= 313,	*/
    {	"SDLK_COMPOSE",		SDLK_COMPOSE		}, /*	= 314,	*/
    {	"SDLK_HELP",		SDLK_HELP		}, /*	= 315,	*/
    {	"SDLK_PRINT",		SDLK_PRINT		}, /*	= 316,	*/
    {	"SDLK_SYSREQ",		SDLK_SYSREQ		}, /*	= 317,	*/
    {	"SDLK_BREAK",		SDLK_BREAK		}, /*	= 318,	*/
    {	"SDLK_MENU",		SDLK_MENU		}, /*	= 319,	*/
    {	"SDLK_POWER",		SDLK_POWER		}, /*	= 320,	*/
    {	"SDLK_EURO",		SDLK_EURO		}, /*	= 321,	*/
    {	"SDLK_UNDO",		SDLK_UNDO		}, /*	= 322,	*/
  };

  char *conv_end;
  unsigned long i;

  if(  str==NULL ) return -1;
  if( *str=='\0' ) return -1;

  if( str[0] == '<' ){				/* <数字> の場合 */
    i = strtoul( &str[1], &conv_end, 0 );
    if( *conv_end == '>' &&
	i < COUNTOF(scancode2key88) ){
      return i;
    }
    return -1;
  }
					/* 定義文字列に合致するのを探す */
  for( i=0; i<COUNTOF(list); i++ ){
    if( strcmp( list[i].name, str ) == 0 ){
      return list[i].val;
    }
  }

  return -1;
}
