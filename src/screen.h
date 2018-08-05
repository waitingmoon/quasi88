#ifndef SCREEN_H_INCLUDED
#define SCREEN_H_INCLUDED


/*
 *	PC-88 Related
 */

typedef struct{
  unsigned	char	blue;			/* B面輝度 (0〜7)	*/
  unsigned	char	red;			/* R面輝度 (0〜7)	*/
  unsigned 	char	green;			/* G面輝度 (0〜7)	*/
  unsigned 	char	padding;
} PC88_PALETTE_T;

extern	PC88_PALETTE_T	vram_bg_palette;	/* 背景パレット	*/
extern	PC88_PALETTE_T	vram_palette[8];	/* 各種パレット	*/

extern	byte	sys_ctrl;		/* OUT[30] SystemCtrl     	*/
extern	byte	grph_ctrl;		/* OUT[31] GraphCtrl      	*/
extern	byte	grph_pile;		/* OUT[53] 重ね合わせ     	*/

#define	SYS_CTRL_80		(0x01)		/* TEXT COLUMN80 / COLUMN40*/
#define	SYS_CTRL_MONO		(0x02)		/* TEXT MONO     / COLOR   */

#define	GRPH_CTRL_200		(0x01)		/* VRAM-MONO 200 / 400 line*/
#define	GRPH_CTRL_64RAM		(0x02)		/* RAM   64K-RAM / ROM-RAM */
#define	GRPH_CTRL_N		(0x04)		/* BASIC       N / N88     */
#define GRPH_CTRL_VDISP		(0x08)		/* VRAM  DISPLAY / UNDISP  */
#define GRPH_CTRL_COLOR		(0x10)		/* VRAM  COLOR   / MONO    */
#define	GRPH_CTRL_25		(0x20)		/* TEXT  LINE25  / LINE20  */

#define	GRPH_PILE_TEXT		(0x01)		/* 重ね合わせ 非表示 TEXT  */
#define	GRPH_PILE_BLUE		(0x02)		/*		       B   */
#define	GRPH_PILE_RED		(0x04)		/*		       R   */
#define	GRPH_PILE_GREEN		(0x08)		/*		       G   */



/*
 *	描画処理用ワーク
 */

	/* 描画差分管理 */

extern	char	screen_update[ 0x4000*2 ];	/* 画面表示差分更新フラグ */
extern	int	screen_update_force;		/* 画面強制更新フラグ	  */
extern	int	screen_update_palette;		/* パレット更新フラグ	  */

#define	set_screen_update( x )		screen_update[x] = 1
#define	set_screen_update_force()	screen_update_force = TRUE
#define	set_screen_update_palette()	do{				\
					  screen_update_palette = TRUE;	\
					  screen_update_force = TRUE;	\
					}while(0)


	/* テキスト処理 */

extern	int	text_attr_flipflop;
extern	Ushort	text_attr_buf[2][2048];


	/* その他 */

extern	int	frameskip_rate;		/* 画面表示の更新間隔		*/
extern	int	monitor_analog;		/* アナログモニター		*/

extern	int	use_auto_skip;		/* オートフレームスキップを使用する */
extern	int	do_skip_draw;		/* スクリーンへの描画をスキップする */
extern	int	already_skip_draw; 	/* スキップしたか		    */



/*
 *	表示デバイス用ワーク
 */


extern	int	have_mouse_cursor;	/* マウスカーソルがあるかどうか	*/
extern	int	hide_mouse;		/* マウスを隠すかどうか		*/
extern	int	grab_mouse;		/* グラブするかどうか		*/

extern	int	use_interlace;		/* インターレース表示		*/
extern	int	use_half_interp;	/* 画面サイズ半分時、色補間する */


enum {					/* 画面サイズ			*/
  SCREEN_SIZE_HALF,			/*		320 x 200	*/
  SCREEN_SIZE_FULL,			/*		640 x 400	*/
#ifdef	SUPPORT_DOUBLE
  SCREEN_SIZE_DOUBLE,			/*		1280x 800	*/
#endif
  SCREEN_SIZE_END
};

typedef	struct{				/* 画面サイズのリスト		*/
  int	w,  h;
  int	dw, dh;
} SCREEN_SIZE_TABLE;

extern const SCREEN_SIZE_TABLE screen_size_tbl[ SCREEN_SIZE_END ];

#define	STATUS_HEIGHT	(20)


extern	int	screen_size;		/* 画面サイズ指定		*/
extern	int	screen_size_max;
extern	int	use_fullscreen;		/* 全画面表示指定		*/

extern	int	WIDTH;			/* 描画バッファ横サイズ		*/
extern	int	HEIGHT;			/* 描画バッファ縦サイズ		*/
extern	int	DEPTH;			/* 色ビット数	(8/16/32)	*/
extern	int	SCREEN_W;		/* 画面横サイズ (320/640/1280)	*/
extern	int	SCREEN_H;		/* 画面縦サイズ (200/400/800)	*/

extern	char	*screen_buf;		/* 描画バッファ先頭		*/
extern	char	*screen_start;		/* 画面先頭			*/


extern	int	show_status;		/* ステータス表示有無		*/

extern	char	*status_buf;		/* ステータス全域 先頭		*/
extern	char	*status_start[3];	/* ステータス描画 先頭		*/
extern	int	status_sx[3];		/* ステータス描画サイズ		*/
extern	int	status_sy[3];




extern	Ulong	color_pixel[16];		/* 色コード		*/
extern	Ulong	color_half_pixel[16][16];	/* 色補完時の色コード	*/
extern	Ulong	black_pixel;			/* 黒の色コード		*/

enum {						/* ステータスに使う色	*/
  STATUS_BG,					/*	背景色(白)	*/
  STATUS_FG,					/*	前景色(黒)	*/
  STATUS_BLACK,					/*	黒色		*/
  STATUS_WHITE,					/*	白色		*/
  STATUS_RED,					/*	赤色		*/
  STATUS_GREEN,					/*	緑色		*/
  STATUS_COLOR_END
};
extern	Ulong	status_pixel[STATUS_COLOR_END];	/* ステータスの色コード	*/


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
                          WIDTH
	 ←───────────────────→
	┌────────────────────┐ ↑
	│                                        │ │
	│     ┌─────────┐             │ │
	│     │  ↑              │             │ │
	│     │←───────→│             │ │
	│     │  │   SCREEN_SX  │             │ │
	│     │  │              │             │ │HEIGHT
	│     │  │SCREEN_SY     │             │ │
	│     │  ↓              │             │ │
	│     └─────────┘             │ │
	│                                        │ │
	│                                        │ │
	│                                        │ ↓
	├──────┬──────┬──────┤ ↑
	│ステータス0 │ステータス1 │ステータス2 │ │STATUS_HEIGHT
	└──────┴──────┴──────┘ ↓

	screen_buf	描画バッファ全域の、先頭ポインタ
	WIDTH		描画バッファ全域の、横ピクセル数
	HEIGHT		        〃          縦ピクセル数

	screen_size	画面サイズ
	screen_start	画面バッファの、先頭ポインタ
	SCREEN_SX	画面バッファの、横ピクセル数 (320/640/1280)
	SCREEN_SY	      〃        縦ピクセル数 (200/400/800)

	DEPTH		色深度 (screen_buf, screen_start のビット幅、8/16/32)

	status_buf	ステータスバッファ全域の、先頭ポインタ
	status_start[3]	ステータス 0〜2 のバッファの、先頭ポインタ
	status_sx[3]		〃		      横ピクセル数
	status_sy[3]		〃		      縦ピクセル数


	※ ウインドウ表示の場合、
		WIDTH x (HEIGHT + STATUS_HEIGHT) のサイズで、
		ウインドウを生成します。
			( ステータス非表示なら、 WIDTH x HEIGHT )

	※ 全画面表示の場合、
		SCREEN_SX x (SCREEN_SY + STATUS_HEIGHT) 以上のサイズで
		全画面化します。
			( ステータス非表示なら、下の部分は黒で塗りつぶす )

   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/


/*
 *
 */

void	draw_screen( void );
void	draw_screen_force( void );

void	draw_status( void );


void	draw_menu_screen( void );
void	draw_menu_screen_force( void );



void	blink_ctrl_update( void );	/* ブリンクのワークを更新する	*/
void	reset_frame_counter( void );

void	screen_buf_init( void );
void	status_buf_init( void );

#endif	/* SCREEN_H_INCLUDED */
