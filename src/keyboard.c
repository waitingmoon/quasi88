#include <stdlib.h>
#include <string.h>

#include "quasi88.h"
#include "keyboard.h"
#include "romaji.h"
#include "event.h"

#include "soundbd.h"	/* sound_reg[]			*/
#include "graph.h"	/* set_key_and_mouse()		*/
#include "pc88cpu.h"	/* z80main_cpu			*/
#include "intr.h"	/* state_of_cpu			*/

#include "drive.h"

#include "emu.h"
#include "status.h"
#include "pause.h"
#include "menu.h"
#include "screen.h"
#include "snapshot.h"

#include "suspend.h"

/******************************************************************************
 *
 *****************************************************************************/

/*
 *	内部状態 (ステートセーブ不要?)
 */

int		mouse_x;		/* 現在の マウス x座標		*/
int		mouse_y;		/* 現在の マウス y座標		*/

static	int	mouse_dx;		/* マウス x方向移動量		*/
static	int	mouse_dy;		/* マウス y方向移動量		*/


unsigned char	key_scan[ 0x10 ];	/* IN(00h)〜(0Eh) キースキャン	*/
					/* key_scan[0]〜[14]はキー用	*/
					/* key_scan[15]は パッドに使用	*/

static	int	key_func[ KEY88_END ];	/* キーの代替コード,機能割当	*/


/*
 *	PC88状態 / 内部状態
 */

static	int	jop1_step;	/* 汎用I/Oポートのリードステップ	*/
static	int	jop1_dx;	/* 汎用I/Oポートの値 (マウス x方向変位)	*/
static	int	jop1_dy;	/* 汎用I/Oポートの値 (マウス y方向変位)	*/
static	int	jop1_time;	/* 汎用I/Oポートのストローブ処理した時	*/

	int	romaji_input_mode = FALSE;	/* 真:ローマ字入力中	*/



/*
  mouse_x, mouse_dx, jop1_dx の関係
	マウスが移動した時
		mouse_x にその絶対座標をセットする。
		前回座標との変位 mouse_dx にセットする。
	汎用I/Oポートからマウス座標をリードした時、
		mouse_dx をクリッピングした値を、 jop1_dx にセットする。
*/



/*
 *	設定
 */

int	mouse_mode	= 0;		/* マウス・ジョイステック処理	*/


int	mouse_key_mode	= 0;		/* マウス入力をキーに反映	*/
int	mouse_key_assign[6];		/*     0:なし 1:テンキー 2:任意 */
static const int mouse_key_assign_tenkey[6] =
{
  KEY88_KP_8, KEY88_KP_2, KEY88_KP_4, KEY88_KP_6,
  KEY88_x,    KEY88_z,
};


int	joy_key_mode	= 0;		/* ジョイ入力をキーに反映	*/
int	joy_key_assign[12];		/*     0:なし 1:テンキー 2:任意 */
static const int joy_key_assign_tenkey[12] =
{
  KEY88_KP_8, KEY88_KP_2, KEY88_KP_4, KEY88_KP_6,
  KEY88_x,    KEY88_z,    0, 0, 0, 0, 0, 0,
};
int	joy_swap_button   = FALSE;	/* ボタンのABを入れ替える  	*/


int	cursor_key_mode = 0;		/* カーソルキーを別キーに反映	*/
int	cursor_key_assign[4];		/*     0:なし 1:テンキー 2:任意 */
static const int cursor_key_assign_tenkey[4] =
{
  KEY88_KP_8, KEY88_KP_2, KEY88_KP_4, KEY88_KP_6,
};		/* Cursor KEY -> 10 KEY , original by funa. (thanks!) */
		/* Cursor Key -> 任意のキー , original by floi. (thanks!) */



int	tenkey_emu      = FALSE;	/* 真:数字キーをテンキーに	*/
int	numlock_emu     = FALSE;	/* 真:ソフトウェアNumLockを行う	*/




int	function_f[ 1 + 20 ] =		/* ファンクションキーの機能     */
{
  FN_FUNC,    /* [0] はダミー */
  FN_FUNC,    FN_FUNC,    FN_FUNC,    FN_FUNC,    FN_FUNC,	/* f1 〜f5  */
  FN_FUNC,    FN_FUNC,    FN_FUNC,    FN_FUNC,    FN_FUNC,	/* f6 〜f10 */
  FN_STATUS,  FN_MENU,    FN_FUNC,    FN_FUNC,    FN_FUNC,	/* f11〜f15 */
  FN_FUNC,    FN_FUNC,    FN_FUNC,    FN_FUNC,    FN_FUNC,	/* f16〜f20 */
};


int	fn_max_speed = 1600;
double	fn_max_clock = CONST_4MHZ_CLOCK*16;
int	fn_max_boost = 16;


int	romaji_type = 0;		/* ローマ字変換のタイプ		*/





/*
 *	キー操作 記録・再生
 */

char	*file_rec	= NULL;		/* キー入力記録のファイル名 */
char	*file_pb	= NULL;		/* キー入力再生のファイル名 */

static	OSD_FILE *fp_rec;
static	OSD_FILE *fp_pb;

static struct {				/* キー入力記録構造体		*/
  Uchar	key[16];			/*	I/O 00H〜0FH 		*/
   char	dx_h;				/*	マウス dx 上位		*/
  Uchar	dx_l;				/*	マウス dx 下位		*/
   char	dy_h;				/*	マウス dy 上位		*/
  Uchar	dy_l;				/*	マウス dy 下位		*/
   char	image[2];			/*	イメージNo -1空,0同,1〜	*/
   char resv[2];
} key_record;				/* 24 bytes			*/





/*---------------------------------------------------------------------------
 *	キーコード と I/O ポートの対応
 *---------------------------------------------------------------------------*/

#define	Port0	0x00
#define	Port1	0x01
#define	Port2	0x02
#define	Port3	0x03
#define	Port4	0x04
#define	Port5	0x05
#define	Port6	0x06
#define	Port7	0x07
#define	Port8	0x08
#define	Port9	0x09
#define	PortA	0x0a
#define	PortB	0x0b
#define	PortC	0x0c
#define	PortD	0x0d
#define	PortE	0x0e
#define	PortX	0x0f

#define	Bit0	0x01
#define	Bit1	0x02
#define	Bit2	0x04
#define	Bit3	0x08
#define	Bit4	0x10
#define	Bit5	0x20
#define	Bit6	0x40
#define	Bit7	0x80

#define	PadA	Bit4
#define	PadB	Bit5

#define	PadU	Bit0
#define	PadD	Bit1
#define	PadL	Bit2
#define	PadR	Bit3


enum {
  /* 後期型キーの別名定義 */

  KEY88_EXT_F6		= KEY88_END + 0,
  KEY88_EXT_F7		= KEY88_END + 1,
  KEY88_EXT_F8		= KEY88_END + 2,
  KEY88_EXT_F9		= KEY88_END + 3,
  KEY88_EXT_F10		= KEY88_END + 4,
  KEY88_EXT_BS		= KEY88_END + 5,
  KEY88_EXT_INS		= KEY88_END + 6,
  KEY88_EXT_DEL		= KEY88_END + 7,
  KEY88_EXT_HENKAN	= KEY88_END + 8,
  KEY88_EXT_KETTEI	= KEY88_END + 9,
  KEY88_EXT_PC		= KEY88_END + 10,
  KEY88_EXT_ZENKAKU	= KEY88_END + 11,
  KEY88_EXT_RETURNL	= KEY88_END + 12,
  KEY88_EXT_RETURNR	= KEY88_END + 13,
  KEY88_EXT_SHIFTL	= KEY88_END + 14,
  KEY88_EXT_SHIFTR	= KEY88_END + 15,

  KEY88_EXT_END		= KEY88_END + 16
};


typedef struct {
  unsigned char port;
  unsigned char mask;
} T_KEYPORT;

static const T_KEYPORT keyport[ KEY88_EXT_END ] =
{
  { 0,0 },			/*	  KEY88_INVALID		= 0,	*/

  { 0,0 },{ 0,0 },{ 0,0 },
  { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
  { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
  { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
  { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
  { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
  { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
  { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },

  { Port9, Bit6 },		/*	  KEY88_SPACE		= 32,	*/
  { Port6, Bit1 },		/*	  KEY88_EXCLAM		= 33,	*/
  { Port6, Bit2 },		/*	  KEY88_QUOTEDBL	= 34,	*/
  { Port6, Bit3 },		/*	  KEY88_NUMBERSIGN	= 35,	*/
  { Port6, Bit4 },		/*	  KEY88_DOLLAR		= 36,	*/
  { Port6, Bit5 },		/*	  KEY88_PERCENT		= 37,	*/
  { Port6, Bit6 },		/*	  KEY88_AMPERSAND	= 38,	*/
  { Port6, Bit7 },		/*	  KEY88_APOSTROPHE	= 39,	*/
  { Port7, Bit0 },		/*	  KEY88_PARENLEFT	= 40,	*/
  { Port7, Bit1 },		/*	  KEY88_PARENRIGHT	= 41,	*/
  { Port7, Bit2 },		/*	  KEY88_ASTERISK	= 42,	*/
  { Port7, Bit3 },		/*	  KEY88_PLUS		= 43,	*/
  { Port7, Bit4 },		/*	  KEY88_COMMA		= 44,	*/
  { Port5, Bit7 },		/*	  KEY88_MINUS		= 45,	*/
  { Port7, Bit5 },		/*	  KEY88_PERIOD		= 46,	*/
  { Port7, Bit6 },		/*	  KEY88_SLASH		= 47,	*/
  { Port6, Bit0 },		/*	  KEY88_0		= 48,	*/
  { Port6, Bit1 },		/*	  KEY88_1		= 49,	*/
  { Port6, Bit2 },		/*	  KEY88_2		= 50,	*/
  { Port6, Bit3 },		/*	  KEY88_3		= 51,	*/
  { Port6, Bit4 },		/*	  KEY88_4		= 52,	*/
  { Port6, Bit5 },		/*	  KEY88_5		= 53,	*/
  { Port6, Bit6 },		/*	  KEY88_6		= 54,	*/
  { Port6, Bit7 },		/*	  KEY88_7		= 55,	*/
  { Port7, Bit0 },		/*	  KEY88_8		= 56,	*/
  { Port7, Bit1 },		/*	  KEY88_9		= 57,	*/
  { Port7, Bit2 },		/*	  KEY88_COLON		= 58,	*/
  { Port7, Bit3 },		/*	  KEY88_SEMICOLON	= 59,	*/
  { Port7, Bit4 },		/*	  KEY88_LESS		= 60,	*/
  { Port5, Bit7 },		/*	  KEY88_EQUAL		= 61,	*/
  { Port7, Bit5 },		/*	  KEY88_GREATER		= 62,	*/
  { Port7, Bit6 },		/*	  KEY88_QUESTION	= 63,	*/
  { Port2, Bit0 },		/*	  KEY88_AT		= 64,	*/
  { Port2, Bit1 },		/*	  KEY88_A		= 65,	*/
  { Port2, Bit2 },		/*	  KEY88_B		= 66,	*/
  { Port2, Bit3 },		/*	  KEY88_C		= 67,	*/
  { Port2, Bit4 },		/*	  KEY88_D		= 68,	*/
  { Port2, Bit5 },		/*	  KEY88_E		= 69,	*/
  { Port2, Bit6 },		/*	  KEY88_F		= 70,	*/
  { Port2, Bit7 },		/*	  KEY88_G		= 71,	*/
  { Port3, Bit0 },		/*	  KEY88_H		= 72,	*/
  { Port3, Bit1 },		/*	  KEY88_I		= 73,	*/
  { Port3, Bit2 },		/*	  KEY88_J		= 74,	*/
  { Port3, Bit3 },		/*	  KEY88_K		= 75,	*/
  { Port3, Bit4 },		/*	  KEY88_L		= 76,	*/
  { Port3, Bit5 },		/*	  KEY88_M		= 77,	*/
  { Port3, Bit6 },		/*	  KEY88_N		= 78,	*/
  { Port3, Bit7 },		/*	  KEY88_O		= 79,	*/
  { Port4, Bit0 },		/*	  KEY88_P		= 80,	*/
  { Port4, Bit1 },		/*	  KEY88_Q		= 81,	*/
  { Port4, Bit2 },		/*	  KEY88_R		= 82,	*/
  { Port4, Bit3 },		/*	  KEY88_S		= 83,	*/
  { Port4, Bit4 },		/*	  KEY88_T		= 84,	*/
  { Port4, Bit5 },		/*	  KEY88_U		= 85,	*/
  { Port4, Bit6 },		/*	  KEY88_V		= 86,	*/
  { Port4, Bit7 },		/*	  KEY88_W		= 87,	*/
  { Port5, Bit0 },		/*	  KEY88_X		= 88,	*/
  { Port5, Bit1 },		/*	  KEY88_Y		= 89,	*/
  { Port5, Bit2 },		/*	  KEY88_Z		= 90,	*/
  { Port5, Bit3 },		/*	  KEY88_BRACKETLEFT	= 91,	*/
  { Port5, Bit4 },		/*	  KEY88_YEN		= 92,	*/
  { Port5, Bit5 },		/*	  KEY88_BRACKETRIGHT	= 93,	*/
  { Port5, Bit6 },		/*	  KEY88_CARET		= 94,	*/
  { Port7, Bit7 },		/*	  KEY88_UNDERSCORE	= 95,	*/
  { Port2, Bit0 },		/*	  KEY88_BACKQUOTE	= 96,	*/
  { Port2, Bit1 },		/*	  KEY88_a		= 97,	*/
  { Port2, Bit2 },		/*	  KEY88_b		= 98,	*/
  { Port2, Bit3 },		/*	  KEY88_c		= 99,	*/
  { Port2, Bit4 },		/*	  KEY88_d		= 100,	*/
  { Port2, Bit5 },		/*	  KEY88_e		= 101,	*/
  { Port2, Bit6 },		/*	  KEY88_f		= 102,	*/
  { Port2, Bit7 },		/*	  KEY88_g		= 103,	*/
  { Port3, Bit0 },		/*	  KEY88_h		= 104,	*/
  { Port3, Bit1 },		/*	  KEY88_i		= 105,	*/
  { Port3, Bit2 },		/*	  KEY88_j		= 106,	*/
  { Port3, Bit3 },		/*	  KEY88_k		= 107,	*/
  { Port3, Bit4 },		/*	  KEY88_l		= 108,	*/
  { Port3, Bit5 },		/*	  KEY88_m		= 109,	*/
  { Port3, Bit6 },		/*	  KEY88_n		= 110,	*/
  { Port3, Bit7 },		/*	  KEY88_o               = 111,	*/
  { Port4, Bit0 },		/*	  KEY88_p               = 112,	*/
  { Port4, Bit1 },		/*	  KEY88_q               = 113,  */
  { Port4, Bit2 },		/*	  KEY88_r               = 114,  */
  { Port4, Bit3 },		/*	  KEY88_s               = 115,  */
  { Port4, Bit4 },		/*	  KEY88_t               = 116,  */
  { Port4, Bit5 },		/*	  KEY88_u               = 117,  */
  { Port4, Bit6 },		/*	  KEY88_v               = 118,  */
  { Port4, Bit7 },		/*	  KEY88_w               = 119,  */
  { Port5, Bit0 },		/*	  KEY88_x		= 120,	*/
  { Port5, Bit1 },		/*	  KEY88_y		= 121,	*/
  { Port5, Bit2 },		/*	  KEY88_z		= 122,	*/
  { Port5, Bit3 },		/*	  KEY88_BRACELEFT	= 123,	*/
  { Port5, Bit4 },		/*	  KEY88_BAR		= 124,	*/
  { Port5, Bit5 },		/*	  KEY88_BRACERIGHT	= 125,	*/
  { Port5, Bit6 },		/*	  KEY88_TILDE		= 126,	*/
  {     0,    0 },
  { Port0, Bit0 },		/*	  KEY88_KP_0		= 128,	*/
  { Port0, Bit1 },		/*	  KEY88_KP_1		= 129,	*/
  { Port0, Bit2 },		/*	  KEY88_KP_2		= 130,	*/
  { Port0, Bit3 },		/*	  KEY88_KP_3		= 131,	*/
  { Port0, Bit4 },		/*	  KEY88_KP_4		= 132,	*/
  { Port0, Bit5 },		/*	  KEY88_KP_5		= 133,	*/
  { Port0, Bit6 },		/*	  KEY88_KP_6		= 134,	*/
  { Port0, Bit7 },		/*	  KEY88_KP_7		= 135,	*/
  { Port1, Bit0 },		/*	  KEY88_KP_8		= 136,	*/
  { Port1, Bit1 },		/*	  KEY88_KP_9		= 137,	*/
  { Port1, Bit2 },		/*	  KEY88_KP_MULTIPLY	= 138,	*/
  { Port1, Bit3 },		/*	  KEY88_KP_ADD		= 139,	*/
  { Port1, Bit4 },		/*	  KEY88_KP_EQUAL	= 140,	*/
  { Port1, Bit5 },		/*	  KEY88_KP_COMMA	= 141,	*/
  { Port1, Bit6 },		/*	  KEY88_KP_PERIOD	= 142,	*/
  { PortA, Bit5 },		/*	  KEY88_KP_SUB		= 143,	*/
  { PortA, Bit6 },		/*	  KEY88_KP_DIVIDE	= 144,	*/

  { Port1, Bit7 },		/*	  KEY88_RETURN		= 145,	*/
  { Port8, Bit0 },		/*	  KEY88_HOME		= 146,	*/
  { Port8, Bit1 },		/*	  KEY88_UP		= 147,	*/
  { Port8, Bit2 },		/*	  KEY88_RIGHT		= 148,	*/
  { Port8, Bit3 },		/*	  KEY88_INS_DEL		= 149,	*/
  { Port8, Bit4 },		/*	  KEY88_GRAPH		= 150,	*/
  { Port8, Bit5 },		/*	  KEY88_KANA		= 151,	*/
  { Port8, Bit6 },		/*	  KEY88_SHIFT		= 152,	*/
  { Port8, Bit7 },		/*	  KEY88_CTRL		= 153,	*/
  { Port9, Bit0 },		/*	  KEY88_STOP		= 154,	*/
  { Port9, Bit6 },		/*	  KEY88_SPACE		= 155,	*/
  { Port9, Bit7 },		/*	  KEY88_ESC		= 156,	*/
  { PortA, Bit0 },		/*	  KEY88_TAB		= 157,	*/
  { PortA, Bit1 },		/*	  KEY88_DOWN		= 158,	*/
  { PortA, Bit2 },		/*	  KEY88_LEFT		= 159,	*/
  { PortA, Bit3 },		/*	  KEY88_HELP		= 160,	*/
  { PortA, Bit4 },		/*	  KEY88_COPY		= 161,	*/
  { PortA, Bit7 },		/*	  KEY88_CAPS		= 162,	*/
  { PortB, Bit0 },		/*	  KEY88_ROLLUP		= 163,	*/
  { PortB, Bit1 },		/*	  KEY88_ROLLDOWN	= 164,	*/

  { Port9, Bit1 },		/*	  KEY88_F1		= 165,	*/
  { Port9, Bit2 },		/*	  KEY88_F2		= 166,	*/
  { Port9, Bit3 },		/*	  KEY88_F3		= 167,	*/
  { Port9, Bit4 },		/*	  KEY88_F4		= 168,	*/
  { Port9, Bit5 },		/*	  KEY88_F5		= 169,	*/

  { 0,0 },{ 0,0 },
  { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
  { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },

  { Port9, Bit1 },	/*f-1*/	/*	  KEY88_F6		= 180,	*/
  { Port9, Bit2 },	/*f-2*/	/*	  KEY88_F7		= 181,	*/
  { Port9, Bit3 },	/*f-3*/	/*	  KEY88_F8		= 182,	*/
  { Port9, Bit4 },	/*f-4*/	/*	  KEY88_F9		= 183,	*/
  { Port9, Bit5 },	/*f-5*/	/*	  KEY88_F10		= 184,	*/
  { Port8, Bit3 },	/*del*/	/*	  KEY88_BS		= 185,	*/
  { Port8, Bit3 },	/*del*/	/*	  KEY88_INS		= 186,	*/
  { Port8, Bit3 },	/*del*/	/*	  KEY88_DEL		= 187,	*/
  { Port9, Bit6 },	/*spc*/	/*	  KEY88_HENKAN		= 188,	*/
  { Port9, Bit6 },	/*spc*/	/*	  KEY88_KETTEI		= 189,	*/
  {     0,    0 },		/*	  KEY88_PC		= 190,	*/
  {     0,    0 },		/*	  KEY88_ZENKAKU		= 191,	*/
  { Port1, Bit7 },	/*ret*/	/*	  KEY88_RETURNL		= 192,	*/
  { Port1, Bit7 },	/*ret*/	/*	  KEY88_RETURNR		= 193,	*/
  { Port8, Bit6 },	/*sft*/	/*	  KEY88_SHIFTL		= 194,	*/
  { Port8, Bit6 },	/*sft*/	/*	  KEY88_SHIFTR		= 195,	*/

  { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
  { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
  { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },

  {     0,    0 },		/*	  KEY88_MOUSE_UP        = 208,	*/
  {     0,    0 },		/*	  KEY88_MOUSE_DOWN      = 209,	*/
  {     0,    0 },		/*	  KEY88_MOUSE_LEFT      = 210,	*/
  {     0,    0 },		/*	  KEY88_MOUSE_RIGHT     = 211,	*/
  {     0,    0 },		/*	  KEY88_MOUSE_L         = 212,	*/
  {     0,    0 },		/*	  KEY88_MOUSE_M         = 213,	*/
  {     0,    0 },		/*	  KEY88_MOUSE_R         = 214,	*/
  {     0,    0 },		/*	  KEY88_MOUSE_WUP       = 215,	*/
  {     0,    0 },		/*	  KEY88_MOUSE_WDN       = 216,	*/

  { 0,0 },{ 0,0 },{ 0,0 },
  { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },

  { PortX, PadU }, 		/*	  KEY88_PAD_UP          = 224,	*/
  { PortX, PadD }, 		/*	  KEY88_PAD_DOWN        = 225,	*/
  { PortX, PadL }, 		/*	  KEY88_PAD_LEFT        = 226,	*/
  { PortX, PadR }, 		/*	  KEY88_PAD_RIGHT       = 227,	*/
  { PortX, PadA }, 		/*	  KEY88_PAD_A           = 228,	*/
  { PortX, PadB }, 		/*	  KEY88_PAD_B           = 229,	*/
  {     0,    0 },		/*	  KEY88_PAD_C           = 230,	*/
  {     0,    0 },		/*	  KEY88_PAD_D           = 232,	*/
  {     0,    0 },		/*	  KEY88_PAD_E           = 233,	*/
  {     0,    0 },		/*	  KEY88_PAD_F           = 234,	*/
  {     0,    0 },		/*	  KEY88_PAD_G           = 235,	*/
  {     0,    0 },		/*	  KEY88_PAD_H           = 236,	*/

  { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
  { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
  { 0,0 },{ 0,0 },{ 0,0 },

  { 0,0 },{ 0,0 },{ 0,0 },{ 0,0 },
  { 0,0 },{ 0,0 },
  { 0,0 },			/*	  KEY88_SYS_STATUS      = 254,	*/
  { 0,0 },			/*	  KEY88_SYS_MENU        = 255,	*/

  { 0,0 },			/*	  KEY88_END             = 256,	*/


  { PortC, Bit0 },		/*	  KEY88_EXT_F6		= 256,	*/
  { PortC, Bit1 },		/*	  KEY88_EXT_F7		= 257,	*/
  { PortC, Bit2 },		/*	  KEY88_EXT_F8		= 258,	*/
  { PortC, Bit3 },		/*	  KEY88_EXT_F9		= 259,	*/
  { PortC, Bit4 },		/*	  KEY88_EXT_F10		= 260,	*/
  { PortC, Bit5 },		/*	  KEY88_EXT_BS		= 261,	*/
  { PortC, Bit6 },		/*	  KEY88_EXT_INS		= 262,	*/
  { PortC, Bit7 },		/*	  KEY88_EXT_DEL		= 263,	*/
  { PortD, Bit0 },		/*	  KEY88_EXT_HENKAN	= 264,	*/
  { PortD, Bit1 },		/*	  KEY88_EXT_KETTEI	= 265,	*/
  { PortD, Bit2 },		/*	  KEY88_EXT_PC		= 266,	*/
  { PortD, Bit3 },		/*	  KEY88_EXT_ZENKAKU	= 267,	*/
  { PortE, Bit0 },		/*	  KEY88_EXT_RETURNL	= 268,	*/
  { PortE, Bit1 },		/*	  KEY88_EXT_RETURNR	= 269,	*/
  { PortE, Bit2 },		/*	  KEY88_EXT_SHIFTL	= 270,	*/
  { PortE, Bit3 },		/*	  KEY88_EXT_SHIFTR	= 271,	*/
};


/*---------------------------------------------------------------------------
 *
 *---------------------------------------------------------------------------*/

#define	PAD_PRESS(pad)		key_scan[ PortX ] &= ~(pad)
#define	PAD_RELEASE(pad)	key_scan[ PortX ] |=  (pad)
#define	PAD_TOGGLE(pad)		key_scan[ PortX ] ^=  (pad)
#define	IS_PAD_STATUS()		key_scan[ PortX ]


#define	KEY88_PRESS(code)	\
	key_scan[ keyport[(code)].port ] &= ~keyport[(code)].mask

#define	KEY88_RELEASE(code)	\
	key_scan[ keyport[(code)].port ] |=  keyport[(code)].mask

#define	KEY88_TOGGLE(code)	\
	key_scan[ keyport[(code)].port ] ^=  keyport[(code)].mask

#define	IS_KEY88_PRESS(code)	\
	(~key_scan[ keyport[(code)].port ] & keyport[(code)].mask)

#define	IS_KEY88_RELEASE(code)	\
	( key_scan[ keyport[(code)].port ] & keyport[(code)].mask)


#define	IS_KEY88_PRINTTABLE(c)	(32 <= (c) && (c) <= 144)
#define	IS_KEY88_FUNCTION(c)	(KEY88_F1 <= (c) && (c) <= KEY88_F20)
#define	IS_KEY88_LATTERTYPE(c)	(KEY88_F6 <= (c) && (c) <= KEY88_SHIFTR)



static	void	key_record_playback( void );


/*---------------------------------------------------------------------------
 * キーのバインディング変更 (キーコード、機能)
 *---------------------------------------------------------------------------*/
INLINE	void	clr_key_function( void )
{
  int i;
  for( i=0; i<COUNTOF(key_func); i++ ){ key_func[i] = 0; }

  /* この2個のキーは、機能固定 */
  key_func[ KEY88_SYS_STATUS ] = FN_STATUS;
  key_func[ KEY88_SYS_MENU   ] = FN_MENU;
}


INLINE	void	set_key_function( int keycode, int func_no )
{
  key_func[ keycode ] = func_no;

  /* この2個のキーは、機能固定 */
  key_func[ KEY88_SYS_STATUS ] = FN_STATUS;
  key_func[ KEY88_SYS_MENU   ] = FN_MENU;
}




/****************************************************************************
 * キーのバインディング変更 / 復帰
 *	オプションや、メニューでの設定に基づき、キーバインドを変更する/戻す。
 *	keyboard_start() は エミュレートモードの開始時に呼ばれる。
 *	keyboard_stop()  は エミュレートモード以外の開始時に呼ばれる。
 *****************************************************************************/

void	keyboard_start( void )
{
  const int *p;

  clr_key_function();

  if( numlock_emu ){
    numlock_on();			/* システムの NUM LOCK を設定 */
  }

  if( tenkey_emu ){			/* 数字キーをテンキーに設定 */
    set_key_function( KEY88_1, KEY88_KP_1 );
    set_key_function( KEY88_2, KEY88_KP_2 );
    set_key_function( KEY88_3, KEY88_KP_3 );
    set_key_function( KEY88_4, KEY88_KP_4 );
    set_key_function( KEY88_5, KEY88_KP_5 );
    set_key_function( KEY88_6, KEY88_KP_6 );
    set_key_function( KEY88_7, KEY88_KP_7 );
    set_key_function( KEY88_8, KEY88_KP_8 );
    set_key_function( KEY88_9, KEY88_KP_9 );
    set_key_function( KEY88_0, KEY88_KP_0 );
  }

  if( cursor_key_mode ){		/* カーソルの入力割り当て変更 */
    if( cursor_key_mode == 1 ) p = cursor_key_assign_tenkey;
    else                       p = cursor_key_assign;

    set_key_function( KEY88_UP,    *p );	p ++;
    set_key_function( KEY88_DOWN,  *p );	p ++;
    set_key_function( KEY88_LEFT,  *p );	p ++;
    set_key_function( KEY88_RIGHT, *p );
  }

  switch( mouse_mode ){			/* マウスの入力割り当て変更 */
  case MOUSE_NONE:
  case MOUSE_JOYSTICK:
    if( mouse_key_mode ){
      if( mouse_key_mode == 1 ) p = mouse_key_assign_tenkey;
      else                      p = mouse_key_assign;

      set_key_function( KEY88_MOUSE_UP,    *p );	p ++;
      set_key_function( KEY88_MOUSE_DOWN,  *p );	p ++;
      set_key_function( KEY88_MOUSE_LEFT,  *p );	p ++;
      set_key_function( KEY88_MOUSE_RIGHT, *p );	p ++;
      set_key_function( KEY88_MOUSE_L,     *p );	p ++;
      set_key_function( KEY88_MOUSE_R,     *p );
    }
    break;

  case MOUSE_JOYMOUSE:
    set_key_function( KEY88_MOUSE_UP,    KEY88_PAD_UP    );
    set_key_function( KEY88_MOUSE_DOWN,  KEY88_PAD_DOWN  );
    set_key_function( KEY88_MOUSE_LEFT,  KEY88_PAD_LEFT  );
    set_key_function( KEY88_MOUSE_RIGHT, KEY88_PAD_RIGHT );
    set_key_function( KEY88_MOUSE_L,     KEY88_PAD_A     );
    set_key_function( KEY88_MOUSE_R,     KEY88_PAD_B     );
    break;

  case MOUSE_MOUSE:
    set_key_function( KEY88_MOUSE_L,     KEY88_PAD_A     );
    set_key_function( KEY88_MOUSE_R,     KEY88_PAD_B     );
    break;
  }

  switch( mouse_mode ){			/* ジョイスティック入力割り当て変更 */
  case MOUSE_NONE:
  case MOUSE_MOUSE:
  case MOUSE_JOYMOUSE:
    if( joy_key_mode ){
      if( joy_key_mode == 1 ) p = joy_key_assign_tenkey;
      else                    p = joy_key_assign;

      set_key_function( KEY88_PAD_UP,    *p );		p ++;
      set_key_function( KEY88_PAD_DOWN,  *p );		p ++;
      set_key_function( KEY88_PAD_LEFT,  *p );		p ++;
      set_key_function( KEY88_PAD_RIGHT, *p );		p ++;
      set_key_function( KEY88_PAD_A,     *p );		p ++;
      set_key_function( KEY88_PAD_B,     *p );		p ++;
      set_key_function( KEY88_PAD_C,     *p );		p ++;
      set_key_function( KEY88_PAD_D,     *p );		p ++;
      set_key_function( KEY88_PAD_E,     *p );		p ++;
      set_key_function( KEY88_PAD_F,     *p );		p ++;
      set_key_function( KEY88_PAD_G,     *p );		p ++;
      set_key_function( KEY88_PAD_H,     *p );		p ++;
    }
    break;

  case MOUSE_JOYSTICK:
    break;
  }

					/* ファンクションキーの機能割り当て */
  set_key_function( KEY88_F1,  function_f[  1 ] );
  set_key_function( KEY88_F2,  function_f[  2 ] );
  set_key_function( KEY88_F3,  function_f[  3 ] );
  set_key_function( KEY88_F4,  function_f[  4 ] );
  set_key_function( KEY88_F5,  function_f[  5 ] );
  set_key_function( KEY88_F6,  function_f[  6 ] );
  set_key_function( KEY88_F7,  function_f[  7 ] );
  set_key_function( KEY88_F8,  function_f[  8 ] );
  set_key_function( KEY88_F9,  function_f[  9 ] );
  set_key_function( KEY88_F10, function_f[ 10 ] );
  set_key_function( KEY88_F11, function_f[ 11 ] );
  set_key_function( KEY88_F12, function_f[ 12 ] );
  set_key_function( KEY88_F13, function_f[ 13 ] );
  set_key_function( KEY88_F14, function_f[ 14 ] );
  set_key_function( KEY88_F15, function_f[ 15 ] );
  set_key_function( KEY88_F16, function_f[ 16 ] );
  set_key_function( KEY88_F17, function_f[ 17 ] );
  set_key_function( KEY88_F18, function_f[ 18 ] );
  set_key_function( KEY88_F19, function_f[ 19 ] );
  set_key_function( KEY88_F20, function_f[ 20 ] );


				/* マウス入力に備えて、マウス位置を再設定 */
  init_mouse_position( &mouse_x, &mouse_y );
  mouse_dx = 0;
  mouse_dy = 0;
}





void	keyboard_stop( void )
{
  romaji_clear();		/* ローマ字変換ワークを初期化 */

  numlock_off();		/* システムの NUM LOCK を解除 */

				/* メニューに備えて、マウス位置を再設定 */
  init_mouse_position( &mouse_x, &mouse_y );
}





/****************************************************************************
 * 【機種依存部より、キー押下時に呼び出される】
 *
 *	code は、キーコードで、 KEY88_SPACE <= code <= KEY88_SHIFTR
 *	on   は、キー押下なら真、キー開放なら偽
 *****************************************************************************/
static	void	do_lattertype( int code, int on );
static	int	do_func( int func, int on );

void	pc88_key( int code, int on )
{
  if( get_emu_mode() == EXEC ){		/*===================================*/

    if( key_func[ code ] ){			/* 特殊処理割り当て済の場合 */
      code = do_func( key_func[ code ], on );	/*	特殊機能処理を実行  */
      if( code==0 ) return;			/*	戻値は 新キーコード */
    }

    if( romaji_input_mode && on ){		/* ローマ字入力モードの場合 */
      if( romaji_input( code )==0 ){		/*	変換処理を実行      */
	return;
      }
    }

    if( IS_KEY88_LATTERTYPE( code ) ){		/* 後期型キーボードの処理   */
      do_lattertype( code, on );
    }
						/* キー押下をIOポートに反映 */
    if( on ) KEY88_PRESS( code );
    else     KEY88_RELEASE( code );

/*
if(code==KEY88_RETURNL){
  if( on ) printf("+%d\n",code);
  else     printf("-%d\n",code);
}
*/

  }else
  if( get_emu_mode() == MENU ){		/*===================================*/
/*printf("%d\n",code);*/
						/* メニュー用の読み替え… */
    switch( code ){
    case KEY88_KP_0:		code = KEY88_0;		break;
    case KEY88_KP_1:		code = KEY88_1;		break;
    case KEY88_KP_2:		code = KEY88_2;		break;
    case KEY88_KP_3:		code = KEY88_3;		break;
    case KEY88_KP_4:		code = KEY88_4;		break;
    case KEY88_KP_5:		code = KEY88_5;		break;
    case KEY88_KP_6:		code = KEY88_6;		break;
    case KEY88_KP_7:		code = KEY88_7;		break;
    case KEY88_KP_8:		code = KEY88_8;		break;
    case KEY88_KP_9:		code = KEY88_9;		break;
    case KEY88_KP_MULTIPLY:	code = KEY88_ASTERISK;	break;
    case KEY88_KP_ADD:		code = KEY88_PLUS;	break;
    case KEY88_KP_EQUAL:	code = KEY88_EQUAL;	break;
    case KEY88_KP_COMMA:	code = KEY88_COMMA;	break;
    case KEY88_KP_PERIOD:	code = KEY88_PERIOD;	break;
    case KEY88_KP_SUB:		code = KEY88_MINUS;	break;
    case KEY88_KP_DIVIDE:	code = KEY88_SLASH;	break;

    case KEY88_INS_DEL:		code = KEY88_BS;	break;
  /*case KEY88_DEL:		code = KEY88_BS;	break;	DELのまま */
    case KEY88_KETTEI:		code = KEY88_SPACE;	break;
    case KEY88_HENKAN:		code = KEY88_SPACE;	break;
    case KEY88_RETURNL:		code = KEY88_RETURN;	break;
    case KEY88_RETURNR:		code = KEY88_RETURN;	break;
    case KEY88_SHIFTL:		code = KEY88_SHIFT;	break;
    case KEY88_SHIFTR:		code = KEY88_SHIFT;	break;
    }
    if( on ) q8tk_event_key_on( code );
    else     q8tk_event_key_off( code );

  }else
  if( get_emu_mode() == PAUSE ){	/*===================================*/

    if( on ){
      if( key_func[ code ] ){
	if( key_func[ code ] == FN_MENU ){

	  pause_event_key_on_menu();

	}else if( key_func[ code ] == FN_PAUSE ){

	  pause_event_key_on_esc();

	}

      }else if( code == KEY88_ESC ){

	pause_event_key_on_esc();

      }

    }
  }
}





/*----------------------------------------------------------------------
 * 後期型キーボードのキー押下/開放時の処理
 *		シフトキーや、同一機能キーのポートを同時処理する
 *----------------------------------------------------------------------*/
static	void	do_lattertype( int code, int on )
{
				  /* KEY88_XXX を KEY88_EXT_XXX に変換 */
  int code2 = code - KEY88_F6 + KEY88_END;

  switch( code ){
  case KEY88_F6:
  case KEY88_F7:
  case KEY88_F8:
  case KEY88_F9:
  case KEY88_F10:
  case KEY88_INS:		/* KEY88_SHIFTR, KEY88_SHIFTL は ? */
    if( on ){ KEY88_PRESS  ( KEY88_SHIFT );  KEY88_PRESS  ( code2 ); }
    else    { KEY88_RELEASE( KEY88_SHIFT );  KEY88_RELEASE( code2 ); }
    break;

  case KEY88_DEL:
  case KEY88_BS:
  case KEY88_HENKAN:
  case KEY88_KETTEI:
  case KEY88_ZENKAKU:
  case KEY88_PC:
  case KEY88_RETURNR:		/* KEY88_RETURNL は ? */
  case KEY88_RETURNL:		/* KEY88_RETURNR は ? */
  case KEY88_SHIFTR:		/* KEY88_SHIFTL  は ? */
  case KEY88_SHIFTL:		/* KEY88_SHIFTR  は ? */
    if( on ){ KEY88_PRESS  ( code2 ); }
    else    { KEY88_RELEASE( code2 ); }
    break;

  default:
    return;
  }
}

/*----------------------------------------------------------------------
 * ファンクションキーに割り当てた機能の処理
 *		戻り値は、新たなキーコード (0ならキー押下なし扱い)
 *----------------------------------------------------------------------*/
static int do_func( int func, int on )
{
  switch( func ){
  case FN_FUNC:					/* 機能なし */
    return 0;

  case FN_FRATE_UP:				/* フレーム */
    if( on ) quasi88_framerate_up();
    return 0;
  case FN_FRATE_DOWN:				/* フレーム */
    if( on ) quasi88_framerate_down();
    return 0;

  case FN_VOLUME_UP:				/* 音量 */
    if( on ) quasi88_volume_up();
    return 0;
  case FN_VOLUME_DOWN:				/* 音量 */
    if( on ) quasi88_volume_down();
    return 0;

  case FN_PAUSE:				/* 一時停止 */
    if( on ) quasi88_pause();
    return 0;

  case FN_RESIZE:				/* リサイズ */
    if( on ){
      if( ++ screen_size > screen_size_max ) screen_size = 0;
      quasi88_change_screen();
    }
    return 0;

  case FN_NOWAIT:				/* ウエイト */
    if( on ) quasi88_wait_none();
    return 0;
  case FN_SPEED_UP:
    if( on ) quasi88_wait_up();
    return 0;
  case FN_SPEED_DOWN:
    if( on ) quasi88_wait_down();
    return 0;

  case FN_FULLSCREEN:				/* 全画面切り替え */
    if( on ){
      if( enable_fullscreen > 0 ){
	use_fullscreen = (now_fullscreen) ? FALSE : TRUE;
	quasi88_change_screen();
      }
    }
    return 0;

  case FN_IMAGE_NEXT1:				/* DRIVE1: イメージ変更 */
    if( on ) quasi88_drv1_image_empty();
    else     quasi88_drv1_image_next();
    return 0;
  case FN_IMAGE_PREV1:
    if( on ) quasi88_drv1_image_empty();
    else     quasi88_drv1_image_prev();
    return 0;
  case FN_IMAGE_NEXT2:				/* DRIVE2: イメージ変更 */
    if( on ) quasi88_drv2_image_empty();
    else     quasi88_drv2_image_next();
    return 0;
  case FN_IMAGE_PREV2:
    if( on ) quasi88_drv2_image_empty();
    else     quasi88_drv2_image_prev();
    return 0;

  case FN_NUMLOCK:				/* NUM lock */
    if( on ){
      if( numlock_emu ) numlock_off();
      numlock_emu ^= 1;
      keyboard_start();
    }
    return 0;

  case FN_RESET:				/* リセット */
    if( on ) quasi88_reset();
    return 0;

  case FN_KANA:					/* カナ */
    if( on ){
      KEY88_TOGGLE( KEY88_KANA );
      romaji_input_mode = FALSE;
    }
    return 0;

  case FN_ROMAJI:				/* カナ(ローマ字) */
    if( on ){
      KEY88_TOGGLE( KEY88_KANA );
      if( IS_KEY88_PRESS( KEY88_KANA ) ){
	status_message( 1, 60, "Romaji ON" );
	romaji_input_mode = TRUE;
	romaji_clear();
      }else{
	status_message( 1, 60, "Romaji OFF" );
	romaji_input_mode = FALSE;
      }
    }
    return 0;

  case FN_CAPS:					/* CAPS */
    if( on ){
      KEY88_TOGGLE( KEY88_CAPS );
    }
    return 0;

  case FN_SNAPSHOT:				/* スクリーンスナップショット*/
    if( on ) quasi88_snapshot();
    return 0;

  case FN_MAX_SPEED:
    if( on ) quasi88_max_speed( fn_max_speed );
    return 0;
  case FN_MAX_CLOCK:
    if( on ) quasi88_max_clock( fn_max_clock );
    return 0;
  case FN_MAX_BOOST:
    if( on ) quasi88_max_boost( fn_max_boost );
    return 0;

  case FN_STATUS:				/* FDDステータス表示 */
    if( on ) quasi88_status();
    return 0;
  case FN_MENU:					/* メニューモード */
    if( on ) quasi88_menu();
    return 0;

  }
  return func;
}










/******************************************************************************
 * 【機種依存部より、マウスのボタン押下時に呼び出される】
 *
 *	code は、キーコードで、 KEY88_MOUSE_L <= code <= KEY88_MOUSE_DOWN
 *	on   は、キー押下なら真、キー開放なら偽
 *****************************************************************************/
void	pc88_mouse( int code, int on )
{
  switch( get_emu_mode() ){

  case EXEC:				/*===================================*/
/*
if( on ) printf("+%d\n",code);
else     printf("-%d\n",code);
*/

    if( key_func[ code ] ){			/* キーコードの読み替え      */
      code = do_func( key_func[ code ], on );	/*	特殊機能があれば、   */
      if( code==0 ) return;			/*	処理されるので注意   */
    }						/*  ローマ字入力や、         */
						/*  後期型キーは処理しない   */

    if( on ) KEY88_PRESS( code );		/* I/Oポートへ反映           */
    else     KEY88_RELEASE( code );
    break;

  case MENU:				/*===================================*/

    if( on ) q8tk_event_mouse_on( code );
    else     q8tk_event_mouse_off( code );
    break;
  }
}




/******************************************************************************
 * 【機種依存部より、ジョイスティック入力時に呼び出される】
 *
 *	code は、キーコードで、 KEY88_PAD_UP <= code <= KEY88_PAD_H
 *	on   は、キー押下なら真、キー開放なら偽
 *****************************************************************************/
void	pc88_pad( int code, int on )
{
  if( get_emu_mode() == EXEC ){		/*===================================*/

    if( joy_swap_button ){			/* A/B ボタン入れ替え        */
      if     ( code == KEY88_PAD_A ) code = KEY88_PAD_B;
      else if( code == KEY88_PAD_B ) code = KEY88_PAD_A;
    }

    if( key_func[ code ] ){			/* キーコードの読み替え      */
      code = do_func( key_func[ code ], on );	/*	特殊機能があれば、   */
      if( code==0 ) return;			/*	処理されるので注意   */
    }						/*  ローマ字入力や、         */
						/*  後期型キーは処理しない   */

    if( on ) KEY88_PRESS( code );		/* I/Oポートへ反映           */
    else     KEY88_RELEASE( code );

  }
}






/****************************************************************************
 * 【機種依存部より、マウス移動時に呼び出される。】
 *
 *	abs_coord が、真なら、x,y はマウスの移動先の座標を示す。
 *		座標は、画面領域を (0,0)-(640,400) とした時の座標とし、
 *		画面サイズやボーダーによる座標のずれは呼び出し元で補正しておく
 *		ただしクリッピングは不要で、範囲外の値でも可。
 *
 *	abs_coord が 偽なら、x,y はマウスの移動量を示す。
 *		画面サイズやマウス速度による補正は、呼び出し元でしておく。
 *****************************************************************************/
void	pc88_mouse_move( int x, int y, int abs_coord )
{
  switch( get_emu_mode() ){

  case EXEC:				/*===================================*/
    if( abs_coord ){
      mouse_dx += x - mouse_x;
      mouse_dy += y - mouse_y;
      mouse_x = x;
      mouse_y = y;

    }else{
      mouse_dx += x;
      mouse_dy += y;
    }
    break;

  case MENU:				/*===================================*/
    if( abs_coord ){
      mouse_x = x;
      mouse_y = y;

    }else{
      mouse_x += x;
      if( mouse_x <   0 ) mouse_x = 0;
      if( mouse_x > 640 ) mouse_x = 640;

      mouse_y += y;
      if( mouse_y <   0 ) mouse_y = 0;
      if( mouse_y > 400 ) mouse_y = 400;
    }

    q8tk_event_mouse_moved( mouse_x, mouse_y );
  }
}



/****************************************************************************
 * 【機種依存部より、フォーカスを得た時に呼び出される】
 *****************************************************************************/
void	pc88_focus_in( void )
{
  if( get_emu_mode() == PAUSE ){	/*===================================*/

    pause_event_focus_in_when_pause();

  }
}

void	pc88_focus_out( void )
{
  if( get_emu_mode() == EXEC ){		/*===================================*/

    pause_event_focus_out_when_exec();

  }
}



/****************************************************************************
 * 【機種依存部より、強制終了した時に呼び出される】
 *****************************************************************************/
void	pc88_quit( void )
{
  set_emu_mode( QUIT );

  switch( get_emu_mode() ){
  case EXEC:				/*===================================*/
    break;

  case MENU:				/*===================================*/
    q8tk_event_quit();
    break;

  case PAUSE:				/*===================================*/
    break;
  }
}






/****************************************************************************
 *
 *	キースキャン処理
 *
 *****************************************************************************/

void	keyboard_init( void )
{
  size_t i;
  for( i=0; i<sizeof(key_scan); i++ )  key_scan[i] = 0xff;

  romaji_init();
}



void	scan_keyboard( void )
{
	/* ローマ字入力モード時は、ローマ字変換後のカナを key_scan[] に反映 */

  if( romaji_input_mode ) romaji_output();


	/* マウス入力を key_scan[] に反映 */

  switch( mouse_mode ){
    int status;

  case MOUSE_NONE:
  case MOUSE_JOYSTICK:
    if( mouse_key_mode == 0 ){	/* マウスキー割り当て無しなら */
      mouse_dx = 0;		/* マウス移動量は不要         */
      mouse_dy = 0;
      break;
    }
    /* FALLTHROUGH */		/* マウスキー割り当て有りか、 */
				/* ジョイスティックモードなら */
  case MOUSE_JOYMOUSE:		/* マウス移動量をポートに反映 */
    if( mouse_dx==0 ){
      if     ( mouse_dy ==0 ) status = 0;		/* ---- */
      else if( mouse_dy > 0 ) status = ( PadD );	/* ↓   */
      else                    status = ( PadU );	/* ↑   */
    }else if( mouse_dx > 0 ){
      int a = mouse_dy*100/mouse_dx;
      if     ( a >  241 ) status = ( PadD );		/* ↓   */
      else if( a >   41 ) status = ( PadD | PadR );	/* ↓→ */
      else if( a >  -41 ) status = (        PadR );	/*   → */
      else if( a > -241 ) status = ( PadU | PadR );	/* ↑→ */
      else                status = ( PadU );		/* ↑   */
    }else{
      int a = -mouse_dy*100/mouse_dx;
      if     ( a >  241 ) status = ( PadD );		/* ↓   */
      else if( a >   41 ) status = ( PadD | PadL );	/* ↓← */
      else if( a >  -41 ) status = (        PadL );	/*   ← */
      else if( a > -241 ) status = ( PadU | PadL );	/* ↑← */
      else                status = ( PadU );		/* ↑   */
    }

    pc88_mouse( KEY88_MOUSE_UP,    (status & PadU) );
    pc88_mouse( KEY88_MOUSE_DOWN,  (status & PadD) );
    pc88_mouse( KEY88_MOUSE_LEFT,  (status & PadL) );
    pc88_mouse( KEY88_MOUSE_RIGHT, (status & PadR) );

    mouse_dx = 0;		/* ポート反映後は移動量クリア */
    mouse_dy = 0;
    break;

  case MOUSE_MOUSE:		/* マウス装着時なら、         */
    break;			/* マウス移動量は保持しておく */
  }


	/* キー操作(scan_key[], mouse_dx, mouse_dy) を記録／再生 */

  key_record_playback();


	/* key_scan[0x0f] (ジョイスティック) をサウンド汎用入力ポートに反映 */

  switch( mouse_mode ){
  case	MOUSE_NONE:
    sound_reg[ 0x0e ] = 0xff;
    sound_reg[ 0x0f ] = 0xff;
    break;

  case	MOUSE_MOUSE:
    sound_reg[ 0x0f ] = ( IS_PAD_STATUS() >> 4 ) | 0xfc;
    break;

  case	MOUSE_JOYMOUSE:
  case	MOUSE_JOYSTICK:
    sound_reg[ 0x0e ] = ( IS_PAD_STATUS()      ) | 0xf0;
    sound_reg[ 0x0f ] = ( IS_PAD_STATUS() >> 4 ) | 0xfc;
/*printf("%02x\n",sound_reg[ 0x0e ]&0xff);*/
    break;
  }

}





/****************************************************************************
 *
 *	キー入力 記録・再生
 *
 *****************************************************************************/

void	key_record_playback_init( void )
{
  int i;

  for( i=0; i<16; i++ ) key_record.key[i]     = 0xff;
  key_record.dx_h = 0;
  key_record.dx_l = 0;
  key_record.dy_h = 0;
  key_record.dy_l = 0;
  key_record.image[0] = -1;
  key_record.image[1] = -1;


  fp_pb  = NULL;
  fp_rec = NULL;

  if( file_pb && file_pb[0] ){			/* 再生用ファイルをオープン */

    fp_pb = osd_fopen( FTYPE_KEY_PB, file_pb, "rb" );

    if( fp_pb ){
      if( verbose_proc )
	printf( "Key-Input Playback file <%s> ... OK\n", file_pb );
    }else{
      printf( "Can't open <%s>\nKey-Input PlayBack is invalid\n", file_pb );
    }
  }

  if( file_rec && file_rec[0] ){		/* 記録用ファイルをオープン */

    fp_rec = osd_fopen( FTYPE_KEY_REC, file_rec, "wb" );

    if( fp_rec ){
      if( verbose_proc )
	printf( "Key-Input Record file <%s> ... OK\n", file_rec );
    }else{
      printf( "Can't open <%s>\nKey-Input Record is invalid\n", file_rec );
    }
  }
}


void	key_record_playback_term( void )
{
  if( fp_pb ){
    osd_fclose( fp_pb );
    fp_pb = NULL;
    if( file_pb ) file_pb[0] = '\0';
  }
  if( fp_rec ){
    osd_fclose( fp_rec );
    fp_rec = NULL;
    if( file_rec ) file_rec[0] = '\0';
  }
}


static	void	key_record_playback( void )
{
  int i, img;

  if( get_emu_mode() != EXEC ) return;

  if( fp_rec ){
    for( i=0; i<0x10; i++ )
      key_record.key[i] = key_scan[i];

    key_record.dx_h = (mouse_dx>>8) & 0xff;
    key_record.dx_l =  mouse_dx     & 0xff;
    key_record.dy_h = (mouse_dy>>8) & 0xff;
    key_record.dy_l =  mouse_dy     & 0xff;

    for( i=0; i<2; i++ ){
      if( disk_image_exist( i ) &&
	  drive_check_empty( i ) == FALSE )
	img = disk_image_selected(i) + 1;
      else
	img = -1;
      if( key_record.image[i] != img ) key_record.image[i] = img;
      else                             key_record.image[i] = 0;
    }

    if( osd_fwrite( &key_record, sizeof(char), sizeof(key_record), fp_rec )
							== sizeof(key_record)){
      ;
    }else{
      printf( "Can't write Record file <%s>\n", file_rec );
      osd_fclose( fp_rec );
      fp_rec = NULL;
    }
  }


  if( fp_pb ){

    if( osd_fread( &key_record, sizeof(char), sizeof(key_record), fp_pb )
							== sizeof(key_record)){
      for( i=0; i<0x10; i++ )
	key_scan[i] = key_record.key[i];

      mouse_dx  = (int)key_record.dx_h << 8;
      mouse_dx |=      key_record.dx_l;
      mouse_dy  = (int)key_record.dy_h << 8;
      mouse_dy |=      key_record.dy_l;

      for( i=0; i<2; i++ ){
	if( key_record.image[i]==-1 ){
	  drive_set_empty( i );
	}else if( disk_image_exist( i ) &&
		  key_record.image[i] > 0 &&
		  key_record.image[i] <= disk_image_num( i ) ){
	  drive_unset_empty( i );
	  disk_change_image( i, key_record.image[i]-1 );
	}
      }

    }else{
      printf(" (( %s : Playback file EOF ))\n", file_pb );
      status_message( 1, 60*4, "Playback  [EOF]" );
      osd_fclose( fp_pb );
      fp_pb = NULL;
    }
  }
}




/****************************************************************************
 *
 *	エミュでマウスをいじる時に呼び出される部分
 *
 *****************************************************************************/

/*
 * 汎用 I/O ポート初期化
 *		汎用 I/O ポートが、出力→入力 に切り替わった時の処理
 */

void	jop1_init( void )
{
#if 0	/* - - - - - - - - - - - - - - - - - マウス関連のワークを初期化する  */

#if 0						/* マウス座標も初期化したほう*/
  init_mouse_position( &mouse_x, &mouse_y );	/* がいいかもしれないけど、  */
#endif						/* そこまでしなくてもいっか  */
  mouse_dx = 0;
  mouse_dy = 0;

#endif	/* - - - - - - - - でも、実際にはこれをしなくても実害はでないと思う。*/
	/*			実害があったので、ver 0.6.2 以降では削除     */

  jop1_step = 0;
  jop1_dx = 0;
  jop1_dy = 0;
}



/*
 * 汎用 I/O ポート ストローブ ON/OFF
 */

/* ストローブ処理は 720state 以内に完了させる。   (×1.25はマージン)	*/
/*  (8MHzの場合は 1440state なんだけどまあいっか)			*/
#define	JOP1_STROBE_LIMIT		((int)(720 * 1.25))

void	jop1_strobe( void )
{
  if( mouse_mode==MOUSE_MOUSE       &&		/* マウス 有効 */
      (sound_reg[ 0x07 ] & 0x80)==0 ){		/* 汎用I/O 入力設定時 */

    {
      int now = state_of_cpu + z80main_cpu.state0;

/*	int interval = now - jop1_time;
	if( interval < 0 ) interval += state_of_vsync;
	printf("JOP %d (%d)\n",jop1_step,interval);*/

      if( jop1_step == 2 ){
	int interval = now - jop1_time;
	if( interval < 0 ) interval += state_of_vsync;
	if( interval > JOP1_STROBE_LIMIT ) jop1_init();
      }

      jop1_time = now;
    }

    switch( jop1_step ){

    case 0:		/* 最初のストローブ(ON)で、マウス移動量の値を確定し  */
			/* 2回目以降のストローブで、この確定した値を転送する */
		{
		  int dx = mouse_dx;			/* x 方向 変位 */
		  int dy = mouse_dy;			/* y 方向 変位 */

#if 1			/* 変位を±127の範囲内にクリッピングする */
		  int f = 0;

			/* x、yのうち 変位が ±127を超えている方を探す。     */
		  	/* ともに超えてたら、変位の大きいほうが超えたとする。*/
		  if( dx < -127 || 127 < dx ) f |= 0x01;
		  if( dy < -127 || 127 < dy ) f |= 0x02;
		  if( f==0x03 ){
		    if( ABS(dx) > ABS(dy) ) f = 0x01;
		    else                    f = 0x02;
		  }
		  if( f==0x01 ){		/* x変位が ±127を超えた場合 */
		    				/* x変位をmax値にしてyを補正 */
		    dy = 127 * SGN(dx) * dy / dx;
		    dx = 127 * SGN(dx);
		  }
		  else if( f==0x02 ){		/* y変位が ±127を超えた場合 */
						/* y変位をmax値にしてxを補正 */
		    dx = 127 * SGN(dy) * dx / dy;
		    dy = 127 * SGN(dy);
		  }
#endif
		  mouse_dx -= dx;
		  mouse_dy -= dy;

		  jop1_dx = dx;
		  jop1_dy = dy;
		  /*printf("%d,%d\n",jop1_dx,jop1_dy);*/
		}
		sound_reg[ 0x0e ] = ((-jop1_dx)>>4) & 0x0f;	break;
    case 1:	sound_reg[ 0x0e ] =  (-jop1_dx)     & 0x0f;	break;
    case 2:	sound_reg[ 0x0e ] = ((-jop1_dy)>>4) & 0x0f;	break;
    case 3:	sound_reg[ 0x0e ] =  (-jop1_dy)     & 0x0f;	break;
    }

  }else{
    		sound_reg[ 0x0e ] = 0xff;
  }

  jop1_step = (jop1_step + 1) & 0x03;
}



/****************************************************************************
 *
 *	メニューでソフトウェアキーボードをいじる時に呼び出される部分
 *
 *****************************************************************************/

int	is_key_pressed( int code )
{
  if( IS_KEY88_LATTERTYPE( code ) ){
    code = code - KEY88_F6 + KEY88_END;
  }
  return((key_scan[ keyport[(code)].port ] & keyport[(code)].mask) == 0);
}
void	do_key_press( int code )
{
  if( IS_KEY88_LATTERTYPE( code ) ){
    do_lattertype( code, TRUE );
  }
  KEY88_PRESS(code);
}
void	do_key_release( int code )
{
  if( IS_KEY88_LATTERTYPE( code ) ){
    do_lattertype( code, FALSE );
  }
  KEY88_RELEASE(code);
}
void	do_key_all_release( void )
{
  int i;
  for( i=0; i<0x10; i++ ) key_scan[i] = 0xff;
}
void	do_key_bug( void )
{
  int	 my_port, your_port;
  byte my_val,  your_val,  save_val;

  save_val = key_scan[8] & 0xf0;	/* port 8 の 上位 4bit は対象外 */
  key_scan[8] |= 0xf0;

  for( my_port=0; my_port<12; my_port++ ){
    for( your_port=0; your_port<12; your_port++ ){

      if( my_port==your_port ) continue;

      my_val   = key_scan[ my_port ];
      your_val = key_scan[ your_port ];

      if( ( my_val | your_val ) != 0xff ){
	key_scan[ my_port ]   =
	  key_scan[ your_port ] = my_val & your_val;
      }

    }
  }

  key_scan[8] &= ~0xf0;
  key_scan[8] |= save_val;
}







/****************************************************************************
 *
 *	ユーティリティ
 *
 *****************************************************************************/

/***********************************************************************
 * 与えられた文字列を、QUASI88 キーコードに変換する
 *	キー定義ファイルの解析などに使おう
 *
 *   例)
 *	"KEY88_SPACE" -> KEY88_SPACE	定義そのままの文字列は、直に変換
 *	"key88_SPACE" -> KEY88_SPACE	小文字混在でもよい
 *	"KEY88_Z"     -> KEY88_Z	これも、定義そのままの例
 *	"KEY88_z"     -> KEY88_z	これも、定義そのままの例
 *	"key88_z"     -> KEY88_Z	小文字混在の場合は大文字になるよ
 *	"0x20"        -> KEY88_SPACE	0x20〜0xf7 は直にキーコードに変換
 *	"0x01"        -> KEY88_INVALID	上記の範囲外なら無効(0)を返す
 *	"32"          -> KEY88_SPACE	10進数や8進数でも同様
 *	"KP1"         -> KEY88_KP_1	KP と 1文字 で、テンキーとする
 *	"KP+"         -> KEY88_KP_ADD	記号でもよい
 *	"Kp9"         -> KEY88_KP_9	小文字混在でもよい
 *	"Err"         -> -1		どれにも合致しなかったら、負を返す
 ************************************************************************/
int	convert_str2key88( const char *str )
{
  const struct{
    char *name;    int val;
  }
  tenkey_list[] = {	/* テンキーに限り、例外的な表記が可能 */

    { "KP0"			,	KEY88_KP_0              },
    { "KP1"			,	KEY88_KP_1              },
    { "KP2"			,	KEY88_KP_2              },
    { "KP3"			,	KEY88_KP_3              },
    { "KP4"			,	KEY88_KP_4              },
    { "KP5"			,	KEY88_KP_5              },
    { "KP6"			,	KEY88_KP_6              },
    { "KP7"			,	KEY88_KP_7              },
    { "KP8"			,	KEY88_KP_8              },
    { "KP9"			,	KEY88_KP_9              },
    { "KP*"			,	KEY88_KP_MULTIPLY       },
    { "KP+"			,	KEY88_KP_ADD            },
    { "KP="			,	KEY88_KP_EQUAL          },
    { "KP,"			,	KEY88_KP_COMMA          },
    { "KP."			,	KEY88_KP_PERIOD         },
    { "KP-"			,	KEY88_KP_SUB            },
    { "KP/"			,	KEY88_KP_DIVIDE         },
  },

  list[] = {		/* 以下は、定義リテラルそのもの */

    { "KEY88_INVALID"		,	KEY88_INVALID		},
    { "KEY88_SPACE" 		,	KEY88_SPACE         	},
    { "KEY88_EXCLAM"		,	KEY88_EXCLAM        	},
    { "KEY88_QUOTEDBL"		,	KEY88_QUOTEDBL      	},
    { "KEY88_NUMBERSIGN"	,	KEY88_NUMBERSIGN    	},
    { "KEY88_DOLLAR"   		,	KEY88_DOLLAR        	},
    { "KEY88_PERCENT"		,	KEY88_PERCENT       	},
    { "KEY88_AMPERSAND"		,	KEY88_AMPERSAND     	},
    { "KEY88_APOSTROPHE"	,	KEY88_APOSTROPHE    	},
    { "KEY88_PARENLEFT"		,	KEY88_PARENLEFT     	},
    { "KEY88_PARENRIGHT"	,	KEY88_PARENRIGHT    	},
    { "KEY88_ASTERISK" 		,	KEY88_ASTERISK      	},
    { "KEY88_PLUS"   		,	KEY88_PLUS          	},
    { "KEY88_COMMA"  		,	KEY88_COMMA         	},
    { "KEY88_MINUS"		,	KEY88_MINUS         	},
    { "KEY88_PERIOD"		,	KEY88_PERIOD        	},
    { "KEY88_SLASH"		,	KEY88_SLASH         	},
    { "KEY88_0"  		,	KEY88_0             	},
    { "KEY88_1"			,	KEY88_1             	},
    { "KEY88_2"			,	KEY88_2             	},
    { "KEY88_3"			,	KEY88_3             	},
    { "KEY88_4"			,	KEY88_4             	},
    { "KEY88_5"			,	KEY88_5             	},
    { "KEY88_6"			,	KEY88_6             	},
    { "KEY88_7"			,	KEY88_7             	},
    { "KEY88_8"			,	KEY88_8             	},
    { "KEY88_9"			,	KEY88_9             	},
    { "KEY88_COLON"		,	KEY88_COLON         	},
    { "KEY88_SEMICOLON"		,	KEY88_SEMICOLON     	},
    { "KEY88_LESS"    		,	KEY88_LESS          	},
    { "KEY88_EQUAL"		,	KEY88_EQUAL         	},
    { "KEY88_GREATER"		,	KEY88_GREATER       	},
    { "KEY88_QUESTION"		,	KEY88_QUESTION      	},
    { "KEY88_AT" 	   	,	KEY88_AT            	},
    { "KEY88_A"			,	KEY88_A             	},
    { "KEY88_B"			,	KEY88_B             	},
    { "KEY88_C"			,	KEY88_C             	},
    { "KEY88_D"			,	KEY88_D             	},
    { "KEY88_E"			,	KEY88_E             	},
    { "KEY88_F"			,	KEY88_F             	},
    { "KEY88_G"			,	KEY88_G             	},
    { "KEY88_H"			,	KEY88_H             	},
    { "KEY88_I"			,	KEY88_I             	},
    { "KEY88_J"			,	KEY88_J             	},
    { "KEY88_K"			,	KEY88_K             	},
    { "KEY88_L"			,	KEY88_L             	},
    { "KEY88_M"			,	KEY88_M             	},
    { "KEY88_N"			,	KEY88_N             	},
    { "KEY88_O"			,	KEY88_O             	},
    { "KEY88_P"			,	KEY88_P             	},
    { "KEY88_Q"			,	KEY88_Q             	},
    { "KEY88_R"			,	KEY88_R             	},
    { "KEY88_S"			,	KEY88_S             	},
    { "KEY88_T"			,	KEY88_T             	},
    { "KEY88_U"			,	KEY88_U             	},
    { "KEY88_V"			,	KEY88_V             	},
    { "KEY88_W"			,	KEY88_W             	},
    { "KEY88_X"			,	KEY88_X             	},
    { "KEY88_Y"			,	KEY88_Y             	},
    { "KEY88_Z"			,	KEY88_Z             	},
    { "KEY88_BRACKETLEFT"	,	KEY88_BRACKETLEFT   	},
    { "KEY88_YEN"	 	,	KEY88_YEN	     	},
    { "KEY88_BRACKETRIGHT"	,	KEY88_BRACKETRIGHT  	},
    { "KEY88_CARET"      	,	KEY88_CARET         	},
    { "KEY88_UNDERSCORE"	,	KEY88_UNDERSCORE    	},
    { "KEY88_BACKQUOTE"		,	KEY88_BACKQUOTE     	},
    { "KEY88_a"  	    	,	KEY88_a             	},
    { "KEY88_b"  	    	,	KEY88_b             	},
    { "KEY88_c"			,	KEY88_c             	},
    { "KEY88_d"			,	KEY88_d             	},
    { "KEY88_e"			,	KEY88_e             	},
    { "KEY88_f"			,	KEY88_f             	},
    { "KEY88_g"			,	KEY88_g             	},
    { "KEY88_h"			,	KEY88_h             	},
    { "KEY88_i"			,	KEY88_i             	},
    { "KEY88_j"			,	KEY88_j             	},
    { "KEY88_k"			,	KEY88_k             	},
    { "KEY88_l"			,	KEY88_l             	},
    { "KEY88_m"			,	KEY88_m             	},
    { "KEY88_n"			,	KEY88_n             	},
    { "KEY88_o"			,	KEY88_o             	},
    { "KEY88_p"			,	KEY88_p             	},
    { "KEY88_q"			,	KEY88_q             	},
    { "KEY88_r"			,	KEY88_r             	},
    { "KEY88_s"			,	KEY88_s             	},
    { "KEY88_t"			,	KEY88_t             	},
    { "KEY88_u"			,	KEY88_u             	},
    { "KEY88_v"			,	KEY88_v             	},
    { "KEY88_w"			,	KEY88_w             	},
    { "KEY88_x"			,	KEY88_x             	},
    { "KEY88_y"			,	KEY88_y             	},
    { "KEY88_z"			,	KEY88_z             	},
    { "KEY88_BRACELEFT"		,	KEY88_BRACELEFT     	},
    { "KEY88_BAR"	    	,	KEY88_BAR           	},
    { "KEY88_BRACERIGHT"	,	KEY88_BRACERIGHT    	},
    { "KEY88_TILDE"    		,	KEY88_TILDE         	},
    { "KEY88_KP_0"		,	KEY88_KP_0          	},
    { "KEY88_KP_1"		,	KEY88_KP_1          	},
    { "KEY88_KP_2"		,	KEY88_KP_2          	},
    { "KEY88_KP_3"		,	KEY88_KP_3          	},
    { "KEY88_KP_4"		,	KEY88_KP_4          	},
    { "KEY88_KP_5"		,	KEY88_KP_5          	},
    { "KEY88_KP_6"		,	KEY88_KP_6          	},
    { "KEY88_KP_7"		,	KEY88_KP_7          	},
    { "KEY88_KP_8"		,	KEY88_KP_8          	},
    { "KEY88_KP_9"		,	KEY88_KP_9          	},
    { "KEY88_KP_MULTIPLY"	,	KEY88_KP_MULTIPLY   	},
    { "KEY88_KP_ADD"    	,	KEY88_KP_ADD        	},
    { "KEY88_KP_EQUAL"		,	KEY88_KP_EQUAL     	},
    { "KEY88_KP_COMMA"		,	KEY88_KP_COMMA      	},
    { "KEY88_KP_PERIOD"		,	KEY88_KP_PERIOD     	},
    { "KEY88_KP_SUB"  		,	KEY88_KP_SUB        	},
    { "KEY88_KP_DIVIDE"		,	KEY88_KP_DIVIDE     	},
    { "KEY88_RETURN"  		,	KEY88_RETURN        	},
    { "KEY88_HOME" 		,	KEY88_HOME          	},
    { "KEY88_UP" 		,	KEY88_UP            	},
    { "KEY88_RIGHT"		,	KEY88_RIGHT         	},
    { "KEY88_INS_DEL"		,	KEY88_INS_DEL       	},
    { "KEY88_GRAPH" 		,	KEY88_GRAPH         	},
    { "KEY88_KANA"		,	KEY88_KANA          	},
    { "KEY88_SHIFT"		,	KEY88_SHIFT         	},
    { "KEY88_CTRL"		,	KEY88_CTRL          	},
    { "KEY88_STOP"		,	KEY88_STOP          	},
    { "KEY88_ESC"		,	KEY88_ESC           	},
    { "KEY88_TAB"		,	KEY88_TAB           	},
    { "KEY88_DOWN"		,	KEY88_DOWN          	},
    { "KEY88_LEFT"		,	KEY88_LEFT          	},
    { "KEY88_HELP"		,	KEY88_HELP          	},
    { "KEY88_COPY"		,	KEY88_COPY          	},
    { "KEY88_CAPS"		,	KEY88_CAPS          	},
    { "KEY88_ROLLUP"		,	KEY88_ROLLUP        	},
    { "KEY88_ROLLDOWN"		,	KEY88_ROLLDOWN      	},
    { "KEY88_F1" 	   	,	KEY88_F1            	},
    { "KEY88_F2"		,	KEY88_F2            	},
    { "KEY88_F3"		,	KEY88_F3            	},
    { "KEY88_F4"		,	KEY88_F4            	},
    { "KEY88_F5"		,	KEY88_F5            	},
    { "KEY88_F11"		,	KEY88_F11           	},
    { "KEY88_F12"		,	KEY88_F12           	},
    { "KEY88_F13"		,	KEY88_F13           	},
    { "KEY88_F14"		,	KEY88_F14           	},
    { "KEY88_F15"		,	KEY88_F15           	},
    { "KEY88_F16"		,	KEY88_F16           	},
    { "KEY88_F17"		,	KEY88_F17           	},
    { "KEY88_F18"		,	KEY88_F18           	},
    { "KEY88_F19"		,	KEY88_F19           	},
    { "KEY88_F20"		,	KEY88_F20           	},
    { "KEY88_F6"		,	KEY88_F6            	},
    { "KEY88_F7"		,	KEY88_F7            	},
    { "KEY88_F8"		,	KEY88_F8            	},
    { "KEY88_F9"		,	KEY88_F9            	},
    { "KEY88_F10"		,	KEY88_F10           	},
    { "KEY88_BS"		,	KEY88_BS            	},
    { "KEY88_INS"		,	KEY88_INS           	},
    { "KEY88_DEL"		,	KEY88_DEL           	},
    { "KEY88_HENKAN"		,	KEY88_HENKAN        	},
    { "KEY88_KETTEI"		,	KEY88_KETTEI        	},
    { "KEY88_PC" 	 	,	KEY88_PC            	},
    { "KEY88_ZENKAKU"		,	KEY88_ZENKAKU       	},
    { "KEY88_RETURNL"		,	KEY88_RETURNL       	},
    { "KEY88_RETURNR"		,	KEY88_RETURNR       	},
    { "KEY88_SHIFTL"		,	KEY88_SHIFTL        	},
    { "KEY88_SHIFTR"		,	KEY88_SHIFTR        	},

    { "KEY88_SYS_MENU"		,	KEY88_SYS_MENU        	},
    { "KEY88_SYS_STATUS"	,	KEY88_SYS_STATUS       	},
  };

  int len;
  char *conv_end;
  unsigned long i;

  if( str==NULL ) return -1;
  len = strlen(str);
  if( len==0 ) return -1;


					/* 0〜9 で始まれば、数字に変換 */
  if( '0'<=str[0] && str[0]<='9' ){
    i = strtoul( str, &conv_end, 0 );		/* 10進,16進,8進数が可能 */
    if( *conv_end == '\0' ){
      if( ( 32 <= i && i <= 247 ) ){
	return i;
      }else{
	return KEY88_INVALID;
      }
    }
    return -1;
  }
					/* 3文字なら、テンキーかも */
  if( len==3 ){
    for( i=0; i<COUNTOF(tenkey_list); i++ ){
      if( strcmp( tenkey_list[i].name, str ) == 0 ){
	return tenkey_list[i].val;
      }
      if( my_strcmp( tenkey_list[i].name, str ) == 0 ){
	return tenkey_list[i].val;
      }
    }
  }
					/* 定義文字列に合致するのを探す */
  for( i=0; i<COUNTOF(list); i++ ){
    if( strcmp( list[i].name, str ) == 0 ){
      return list[i].val;
    }
  }
  for( i=0; i<COUNTOF(list); i++ ){
    if( my_strcmp( list[i].name, str ) == 0 ){
      return list[i].val;
    }
  }

  return -1;
}




/***********************************************************************
 * ステートロード／ステートセーブ
 ************************************************************************/
/* ver 0.6.2 以前と ver 0.6.3 以降でファンクションキーの機能が変わったので、
   ステートロード／ステートセーブの際に変換する。*/

enum {			/* ver 0.6.2以前の、ファンクションキーの機能	*/
  OLD_FN_FUNC,
  OLD_FN_FRATE_UP,
  OLD_FN_FRATE_DOWN,
  OLD_FN_VOLUME_UP,
  OLD_FN_VOLUME_DOWN,
  OLD_FN_PAUSE,
  OLD_FN_RESIZE,
  OLD_FN_NOWAIT,
  OLD_FN_SPEED_UP,
  OLD_FN_SPEED_DOWN,
  OLD_FN_MOUSE_HIDE,
  OLD_FN_FULLSCREEN,
  OLD_FN_IMAGE_NEXT1,
  OLD_FN_IMAGE_PREV1,
  OLD_FN_IMAGE_NEXT2,
  OLD_FN_IMAGE_PREV2,
  OLD_FN_NUMLOCK,
  OLD_FN_RESET,
  OLD_FN_KANA,
  OLD_FN_ROMAJI,
  OLD_FN_CAPS,
  OLD_FN_KETTEI,
  OLD_FN_HENKAN,
  OLD_FN_ZENKAKU,
  OLD_FN_PC,
  OLD_FN_SNAPSHOT,
  OLD_FN_STOP,
  OLD_FN_COPY,
  OLD_FN_STATUS,
  OLD_FN_MENU,
  OLD_FN_end
};
static struct{		/* ver 0.6.3以降との、機能の対応表 */
	int	old;		int	now;
} func_f_convert[] =
{
  {	OLD_FN_FUNC,		FN_FUNC,	},
  {	OLD_FN_FRATE_UP,	FN_FRATE_UP,	},
  {	OLD_FN_FRATE_DOWN,	FN_FRATE_DOWN,	},
  {	OLD_FN_VOLUME_UP,	FN_VOLUME_UP,	},
  {	OLD_FN_VOLUME_DOWN,	FN_VOLUME_DOWN, },
  {	OLD_FN_PAUSE,		FN_PAUSE,	},
  {	OLD_FN_RESIZE,		FN_RESIZE,	},
  {	OLD_FN_NOWAIT,		FN_NOWAIT,	},
  {	OLD_FN_SPEED_UP,	FN_SPEED_UP,	},
  {	OLD_FN_SPEED_DOWN,	FN_SPEED_DOWN,	},
  {	OLD_FN_MOUSE_HIDE,	FN_FUNC,	},
  {	OLD_FN_FULLSCREEN,	FN_FULLSCREEN,	},
  {	OLD_FN_IMAGE_NEXT1,	FN_IMAGE_NEXT1, },
  {	OLD_FN_IMAGE_PREV1,	FN_IMAGE_PREV1, },
  {	OLD_FN_IMAGE_NEXT2,	FN_IMAGE_NEXT2, },
  {	OLD_FN_IMAGE_PREV2,	FN_IMAGE_PREV2, },
  {	OLD_FN_NUMLOCK,		FN_NUMLOCK,	},
  {	OLD_FN_RESET,		FN_RESET,	},
  {	OLD_FN_KANA,		FN_KANA,	},
  {	OLD_FN_ROMAJI,		FN_ROMAJI,	},
  {	OLD_FN_CAPS,		FN_CAPS,	},
  {	OLD_FN_KETTEI,		KEY88_KETTEI,	},
  {	OLD_FN_HENKAN,		KEY88_HENKAN,	},
  {	OLD_FN_ZENKAKU,		KEY88_ZENKAKU,	},
  {	OLD_FN_PC,		KEY88_PC,	},
  {	OLD_FN_SNAPSHOT,	FN_SNAPSHOT,	},
  {	OLD_FN_STOP,		KEY88_STOP,	},
  {	OLD_FN_COPY,		KEY88_COPY,	},
  {	OLD_FN_STATUS,		FN_STATUS,	},
  {	OLD_FN_MENU,		FN_MENU,	},
  {	OLD_FN_FUNC,		FN_MAX_SPEED,	},
  {	OLD_FN_FUNC,		FN_MAX_CLOCK,	},
  {	OLD_FN_FUNC,		FN_MAX_BOOST,	},
};
static	int	old_func_f[ 1 + 20 ];
static	void	function_new2old( void )
{
  int i, j;
  for( i=1; i<=20; i++ ){
    old_func_f[i] = OLD_FN_FUNC;
    for( j=0; j<COUNTOF(func_f_convert); j++ ){
      if( function_f[i] == func_f_convert[j].now ){
	old_func_f[i] = func_f_convert[j].old;
	break;
      }
    }
  }
}
static	void	function_old2new( void )
{
  int i, j;
  for( i=1; i<=20; i++ ){
    function_f[i] = FN_FUNC;
    for( j=0; j<COUNTOF(func_f_convert); j++ ){
      if( old_func_f[i] == func_f_convert[j].old ){
	function_f[i] = func_f_convert[j].now;
	break;
      }
    }
  }
}



#define	SID	"KYBD"
#define	SID2	"KYB2"
#define	SID3	"KYB3"

static	T_SUSPEND_W	suspend_keyboard_work[] =
{
  { TYPE_INT,	&jop1_step		},
  { TYPE_INT,	&jop1_dx		},
  { TYPE_INT,	&jop1_dy		},

  { TYPE_INT,	&romaji_input_mode	},

  { TYPE_INT,	&mouse_mode		},
  { TYPE_INT,	&mouse_key_mode		},
  { TYPE_INT,	&mouse_key_assign[ 0]	},
  { TYPE_INT,	&mouse_key_assign[ 1]	},
  { TYPE_INT,	&mouse_key_assign[ 2]	},
  { TYPE_INT,	&mouse_key_assign[ 3]	},
  { TYPE_INT,	&mouse_key_assign[ 4]	},
  { TYPE_INT,	&mouse_key_assign[ 5]	},

  { TYPE_INT,	&joy_key_mode		},
  { TYPE_INT,	&joy_key_assign[ 0]	},
  { TYPE_INT,	&joy_key_assign[ 1]	},
  { TYPE_INT,	&joy_key_assign[ 2]	},
  { TYPE_INT,	&joy_key_assign[ 3]	},
  { TYPE_INT,	&joy_key_assign[ 4]	},
  { TYPE_INT,	&joy_key_assign[ 5]	},
  { TYPE_INT,	&joy_key_assign[ 6]	},
  { TYPE_INT,	&joy_key_assign[ 7]	},
  { TYPE_INT,	&joy_key_assign[ 8]	},
  { TYPE_INT,	&joy_key_assign[ 9]	},
  { TYPE_INT,	&joy_key_assign[10]	},
  { TYPE_INT,	&joy_key_assign[11]	},
  { TYPE_INT,	&joy_swap_button	},

  { TYPE_INT,	&cursor_key_mode	},
  { TYPE_INT,	&cursor_key_assign[0]	},
  { TYPE_INT,	&cursor_key_assign[1]	},
  { TYPE_INT,	&cursor_key_assign[2]	},
  { TYPE_INT,	&cursor_key_assign[3]	},

  { TYPE_INT,	&tenkey_emu		},
  { TYPE_INT,	&numlock_emu		},

  { TYPE_INT,	&old_func_f[ 1]		},
  { TYPE_INT,	&old_func_f[ 2]		},
  { TYPE_INT,	&old_func_f[ 3]		},
  { TYPE_INT,	&old_func_f[ 4]		},
  { TYPE_INT,	&old_func_f[ 5]		},
  { TYPE_INT,	&old_func_f[ 6]		},
  { TYPE_INT,	&old_func_f[ 7]		},
  { TYPE_INT,	&old_func_f[ 8]		},
  { TYPE_INT,	&old_func_f[ 9]		},
  { TYPE_INT,	&old_func_f[10]		},
  { TYPE_INT,	&old_func_f[11]		},
  { TYPE_INT,	&old_func_f[12]		},
  { TYPE_INT,	&old_func_f[13]		},
  { TYPE_INT,	&old_func_f[14]		},
  { TYPE_INT,	&old_func_f[15]		},
  { TYPE_INT,	&old_func_f[16]		},
  { TYPE_INT,	&old_func_f[17]		},
  { TYPE_INT,	&old_func_f[18]		},
  { TYPE_INT,	&old_func_f[19]		},
  { TYPE_INT,	&old_func_f[20]		},

  { TYPE_INT,	&romaji_type		},

  { TYPE_END,	0			},
};

static	T_SUSPEND_W	suspend_keyboard_work2[] =
{
  { TYPE_INT,	&jop1_time		},
  { TYPE_END,	0			},
};

static	T_SUSPEND_W	suspend_keyboard_work3[] =
{
  { TYPE_INT,	&function_f[ 1]		},
  { TYPE_INT,	&function_f[ 2]		},
  { TYPE_INT,	&function_f[ 3]		},
  { TYPE_INT,	&function_f[ 4]		},
  { TYPE_INT,	&function_f[ 5]		},
  { TYPE_INT,	&function_f[ 6]		},
  { TYPE_INT,	&function_f[ 7]		},
  { TYPE_INT,	&function_f[ 8]		},
  { TYPE_INT,	&function_f[ 9]		},
  { TYPE_INT,	&function_f[10]		},
  { TYPE_INT,	&function_f[11]		},
  { TYPE_INT,	&function_f[12]		},
  { TYPE_INT,	&function_f[13]		},
  { TYPE_INT,	&function_f[14]		},
  { TYPE_INT,	&function_f[15]		},
  { TYPE_INT,	&function_f[16]		},
  { TYPE_INT,	&function_f[17]		},
  { TYPE_INT,	&function_f[18]		},
  { TYPE_INT,	&function_f[19]		},
  { TYPE_INT,	&function_f[20]		},
  { TYPE_END,	0			},
};


int	statesave_keyboard( void )
{
  function_new2old();

  if( statesave_table( SID, suspend_keyboard_work ) != STATE_OK ) return FALSE;

  if( statesave_table( SID2,suspend_keyboard_work2) != STATE_OK ) return FALSE;

  if( statesave_table( SID3,suspend_keyboard_work3) != STATE_OK ) return FALSE;

  return TRUE;
}

int	stateload_keyboard( void )
{
  if( stateload_table( SID, suspend_keyboard_work ) != STATE_OK ) return FALSE;

  if( stateload_table( SID2,suspend_keyboard_work2) != STATE_OK ){

    /* 旧バージョンなら、みのがす */

    printf( "stateload : Statefile is old. (ver 0.6.0 or 1?)\n" );

    goto NOT_HAVE_SID2;
  }

  if( stateload_table( SID3,suspend_keyboard_work3) != STATE_OK ){

    /* 旧バージョンなら、みのがす */

    printf( "stateload : Statefile is old. (ver 0.6.0, 1 or 2?)\n" );

    goto NOT_HAVE_SID3;
  }

  return TRUE;



 NOT_HAVE_SID2:
  /* この関数の呼び出し以前に、 stateload_pc88main と stateload_intr が
     呼び出されていなければ、以下の初期化は意味がない */

  jop1_time = state_of_cpu + z80main_cpu.state0;


 NOT_HAVE_SID3:
  /* function_f[] を差し替える */
  function_old2new();


  return TRUE;
}
