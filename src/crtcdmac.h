#ifndef CRTCDMAC_H_INCLUDED
#define CRTCDMAC_H_INCLUDED



		/* CRTC側から見たアトリビュート	*/

#define MONO_SECRET	0x01
#define MONO_BLINK	0x02
#define MONO_REVERSE	0x04
#define MONO_UPPER	0x10
#define MONO_UNDER	0x20
#define MONO_GRAPH	0x80
#define COLOR_SWITCH	0x08
#define COLOR_GRAPH	0x10
#define COLOR_B		0x20
#define COLOR_R		0x40
#define COLOR_G		0x80

		/* 内部表現で使用するアトリビュート */

#define ATTR_REVERSE	0x01			/* 反転			*/
#define ATTR_SECRET	0x02			/* 表示/非表示		*/
#define ATTR_UPPER	0x04			/* アッパーライン	*/
#define ATTR_LOWER	0x08			/* アンダーライン	*/
#define ATTR_GRAPH	0x10			/* グラフィックモード	*/
#define ATTR_B		0x20			/* 色 Blue		*/
#define ATTR_R		0x40			/* 色 Reg		*/
#define ATTR_G		0x80			/* 色 Green		*/

#define MONO_MASK	0x0f
#define COLOR_MASK	0xe0



extern	int	crtc_active;		/* CRTCの状態 0:CRTC作動 1:CRTC停止 */
extern	int	crtc_intr_mask;		/* CRTCの割込マスク ==3 で表示	    */
extern	int	crtc_cursor[2];		/* カーソル位置 非表示の時は(-1,-1) */
extern	byte	crtc_format[5];		/* CRTC 期化時のフォーマット	    */

extern	int	crtc_reverse_display;	/* 真…反転表示 / 偽…通常表示	*/
extern	int	crtc_skip_line;		/* 真…1行飛ばし表示 / 偽…通常 */
extern	int	crtc_cursor_style;	/* ブロック / アンダライン	*/
extern	int	crtc_cursor_blink;	/* 真…点滅する 偽…点滅しない	*/
extern	int	crtc_attr_non_separate;	/* 真…VRAM、ATTR が交互に並ぶ	*/
extern	int	crtc_attr_color;	/* 真…カラー 偽…白黒		*/
extern	int	crtc_attr_non_special;	/* 偽…行の終りに ATTR が並ぶ	*/

extern	int	CRTC_SZ_LINES;		/* 表示する桁数 (20/25)		*/
#define		CRTC_SZ_COLUMNS	(80)	/* 表示する行数 (80固定)	*/
extern	int	crtc_sz_lines;		/* 桁数 (20〜25)		*/
extern	int	crtc_sz_columns;	/* 行数 (2〜80)			*/
extern	int	crtc_sz_attrs;		/* 属性量 (1〜20)		*/
extern	int	crtc_byte_per_line;	/* 1行あたりのメモリ バイト数	*/
extern	int	crtc_font_height;	/* フォントの高さ ドット数(8/10)*/


extern	pair	dmac_address[4];
extern	pair	dmac_counter[4];
#define	text_dma_addr	dmac_address[2]

extern	int	dmac_mode;



	/**** テキスト表示 ****/

enum {
  TEXT_DISABLE,		/* テキスト表示なし				*/
  TEXT_ATTR_ONLY,	/*  〃    但し属性のみ有効 (白黒グラフィック時)	*/
  TEXT_ENABLE,		/* テキスト表示あり				*/
  End_of_TEXT
};
extern	int	text_display;		/* テキスト表示状態フラグ	*/
extern	int	blink_cycle;		/* 点滅の周期	8/16/24/32	*/
extern	int	blink_counter;		/* 点滅制御カウンタ		*/


/* このマクロは、CRTC,DMAC設定時および、I/O 31H / 53H 出力時に呼ぶ	*/

#define	set_text_display()						\
	do{								\
	  if( (dmac_mode & 0x4) && (crtc_active) && crtc_intr_mask==3){	\
	    if( !(grph_pile & GRPH_PILE_TEXT) ){			\
	      text_display = TEXT_ENABLE;				\
	    }else{							\
	      if( grph_ctrl & GRPH_CTRL_COLOR )				\
		text_display = TEXT_DISABLE;				\
	      else							\
		text_display = TEXT_ATTR_ONLY;				\
	    }								\
	  }else{							\
	    text_display = TEXT_DISABLE;				\
	  }								\
	}while(0)



extern	int	dma_wait_count;		/* DMAで消費するサイクル数	*/

#define	SET_DMA_WAIT_COUNT()	dma_wait_count =			\
					crtc_byte_per_line * crtc_sz_lines

#define	RESET_DMA_WAIT_COUNT()	dma_wait_count = 0



void	crtc_init( void );

void	crtc_out_command( byte data );
void	crtc_out_parameter( byte data );
byte	crtc_in_status( void );
byte	crtc_in_parameter( void );


void	dmac_init( void );

void	dmac_out_mode( byte data );
byte	dmac_in_status( void );
void	dmac_out_address( byte addr, byte data );
void	dmac_out_counter( byte addr, byte data );
byte	dmac_in_address( byte addr );
byte	dmac_in_counter( byte addr );





#undef	SUPPORT_CRTC_SEND_SYNC_SIGNAL

#ifdef	SUPPORT_CRTC_SEND_SYNC_SIGNAL

void	crtc_send_sync_signal( int flag );
#define	set_crtc_sync_bit()	crtc_send_sync_signal( 1 )
#define	clr_crtc_sync_bit()	crtc_send_sync_signal( 0 )

#else

#define	set_crtc_sync_bit()	((void)0)
#define	clr_crtc_sync_bit()	((void)0)

#endif


#endif	/* CRTCDMAC_H_INCLUDED */
