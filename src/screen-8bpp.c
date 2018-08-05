/************************************************************************/
/*									*/
/* 画面の表示		8bpp						*/
/*									*/
/************************************************************************/

#include <string.h>

#include "quasi88.h"
#include "screen.h"
#include "screen-func.h"
#include "graph.h"
#include "memory.h"
#include "q8tk.h"


#define	SCREEN_WIDTH		WIDTH
#define	SCREEN_HEIGHT		HEIGHT
#define	SCREEN_SX		SCREEN_W
#define	SCREEN_SY		SCREEN_H
#define	SCREEN_TOP		screen_buf
#define	SCREEN_START		screen_start

#define	COLOR_PIXEL(x)		color_pixel[ x ]
#define	MIXED_PIXEL(a,b)	color_half_pixel[ a ][ b ]
#define	BLACK			black_pixel




#ifdef	SUPPORT_8BPP

#define	TYPE		bit8

/*===========================================================================
 * 等倍サイズ
 *===========================================================================*/

/*----------------------------------------------------------------------
 *			● 200ライン			標準
 *----------------------------------------------------------------------*/
#define	NORMAL
#define	COLOR						/* カラー640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C80x25_F_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C80x25_F_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C80x20_F_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C80x20_F_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C40x25_F_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C40x25_F_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C40x20_F_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C40x20_F_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	COLOR
#define	MONO						/* 白黒  640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M80x25_F_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M80x25_F_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M80x20_F_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M80x20_F_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M40x25_F_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M40x25_F_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M40x20_F_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M40x20_F_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	MONO
#define	UNDISP						/* 非表示640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U80x25_F_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U80x25_F_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U80x20_F_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U80x20_F_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U40x25_F_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U40x25_F_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U40x20_F_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U40x20_F_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	UNDISP
#undef	NORMAL

/*----------------------------------------------------------------------
 *			● 200ライン			ラインスキップ
 *----------------------------------------------------------------------*/
#define	SKIPLINE
#define	COLOR						/* カラー640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C80x25_F_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C80x25_F_S__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C80x20_F_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C80x20_F_S__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C40x25_F_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C40x25_F_S__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C40x20_F_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C40x20_F_S__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	COLOR
#define	MONO						/* 白黒  640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M80x25_F_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M80x25_F_S__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M80x20_F_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M80x20_F_S__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M40x25_F_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M40x25_F_S__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M40x20_F_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M40x20_F_S__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	MONO
#define	UNDISP						/* 非表示640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U80x25_F_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U80x25_F_S__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U80x20_F_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U80x20_F_S__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U40x25_F_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U40x25_F_S__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U40x20_F_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U40x20_F_S__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	UNDISP
#undef	SKIPLINE

/*----------------------------------------------------------------------
 *			● 200ライン			インターレース
 *----------------------------------------------------------------------*/
#define	INTERLACE
#define	COLOR						/* カラー640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C80x25_F_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C80x25_F_I__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C80x20_F_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C80x20_F_I__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C40x25_F_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C40x25_F_I__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C40x20_F_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C40x20_F_I__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	COLOR
#define	MONO						/* 白黒  640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M80x25_F_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M80x25_F_I__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M80x20_F_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M80x20_F_I__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M40x25_F_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M40x25_F_I__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M40x20_F_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M40x20_F_I__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	MONO
#define	UNDISP						/* 非表示640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U80x25_F_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U80x25_F_I__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U80x20_F_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U80x20_F_I__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-full-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U40x25_F_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U40x25_F_I__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U40x20_F_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U40x20_F_I__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	UNDISP
#undef	INTERLACE

/*----------------------------------------------------------------------
 *			● 400ライン			標準
 *----------------------------------------------------------------------*/
#define	HIRESO						/* 白黒  640x400 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-full-400.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_H80x25_F_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_H80x25_F_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-400.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_H80x20_F_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_H80x20_F_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-full-400.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_H40x25_F_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_H40x25_F_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-full-400.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_H40x20_F_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_H40x20_F_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	HIRESO


/*===========================================================================
 * 半分サイズ
 *===========================================================================*/

/*----------------------------------------------------------------------
 *			● 200ライン			標準
 *----------------------------------------------------------------------*/
#define	NORMAL
#define	COLOR						/* カラー640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-half-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C80x25_H_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C80x25_H_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-half-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C80x20_H_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C80x20_H_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-half-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C40x25_H_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C40x25_H_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-half-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C40x20_H_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C40x20_H_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	COLOR
#define	MONO						/* 白黒  640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-half-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M80x25_H_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M80x25_H_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-half-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M80x20_H_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M80x20_H_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-half-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M40x25_H_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M40x25_H_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-half-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M40x20_H_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M40x20_H_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	MONO
#define	UNDISP						/* 非表示640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-half-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U80x25_H_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U80x25_H_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-half-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U80x20_H_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U80x20_H_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-half-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U40x25_H_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U40x25_H_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-half-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U40x20_H_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U40x20_H_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	UNDISP
#undef	NORMAL

/*----------------------------------------------------------------------
 *			● 200ライン			色補完
 *----------------------------------------------------------------------*/
#define	INTERPOLATE
#define	COLOR						/* カラー640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-half-200-p.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C80x25_H_P__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C80x25_H_P__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-half-200-p.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C80x20_H_P__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C80x20_H_P__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-half-200-p.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C40x25_H_P__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C40x25_H_P__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-half-200-p.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C40x20_H_P__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C40x20_H_P__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	COLOR
#define	MONO						/* 白黒  640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-half-200-p.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M80x25_H_P__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M80x25_H_P__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-half-200-p.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M80x20_H_P__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M80x20_H_P__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-half-200-p.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M40x25_H_P__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M40x25_H_P__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-half-200-p.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M40x20_H_P__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M40x20_H_P__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	MONO
#define	UNDISP						/* 非表示640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-half-200-p.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U80x25_H_P__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U80x25_H_P__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-half-200-p.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U80x20_H_P__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U80x20_H_P__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-half-200-p.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U40x25_H_P__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U40x25_H_P__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-half-200-p.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U40x20_H_P__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U40x20_H_P__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	UNDISP
#undef	INTERPOLATE

/*----------------------------------------------------------------------
 *			● 400ライン
 *----------------------------------------------------------------------*/
#define	HIRESO						/* 白黒  640x400 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-half-400.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_H80x25_H_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_H80x25_H_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-half-400.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_H80x20_H_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_H80x20_H_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-half-400.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_H40x25_H_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_H40x25_H_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-half-400.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_H40x20_H_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_H40x20_H_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	HIRESO


/*===========================================================================
 * 二倍サイズ
 *===========================================================================*/
#ifdef	SUPPORT_DOUBLE
/*----------------------------------------------------------------------
 *			● 200ライン			標準
 *----------------------------------------------------------------------*/
#define	NORMAL
#define	COLOR						/* カラー640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C80x25_D_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C80x25_D_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C80x20_D_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C80x20_D_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C40x25_D_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C40x25_D_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C40x20_D_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C40x20_D_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	COLOR
#define	MONO						/* 白黒  640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M80x25_D_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M80x25_D_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M80x20_D_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M80x20_D_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M40x25_D_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M40x25_D_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M40x20_D_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M40x20_D_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	MONO
#define	UNDISP						/* 非表示640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U80x25_D_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U80x25_D_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U80x20_D_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U80x20_D_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U40x25_D_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U40x25_D_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U40x20_D_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U40x20_D_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	UNDISP
#undef	NORMAL

/*----------------------------------------------------------------------
 *			● 200ライン			ラインスキップ
 *----------------------------------------------------------------------*/
#define	SKIPLINE
#define	COLOR						/* カラー640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C80x25_D_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C80x25_D_S__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C80x20_D_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C80x20_D_S__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C40x25_D_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C40x25_D_S__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C40x20_D_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C40x20_D_S__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	COLOR
#define	MONO						/* 白黒  640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M80x25_D_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M80x25_D_S__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M80x20_D_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M80x20_D_S__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M40x25_D_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M40x25_D_S__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M40x20_D_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M40x20_D_S__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	MONO
#define	UNDISP						/* 非表示640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U80x25_D_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U80x25_D_S__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U80x20_D_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U80x20_D_S__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U40x25_D_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U40x25_D_S__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U40x20_D_S__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U40x20_D_S__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	UNDISP
#undef	SKIPLINE

/*----------------------------------------------------------------------
 *			● 200ライン			インターレース
 *----------------------------------------------------------------------*/
#define	INTERLACE
#define	COLOR						/* カラー640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C80x25_D_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C80x25_D_I__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C80x20_D_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C80x20_D_I__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C40x25_D_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C40x25_D_I__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_C40x20_D_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_C40x20_D_I__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	COLOR
#define	MONO						/* 白黒  640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M80x25_D_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M80x25_D_I__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M80x20_D_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M80x20_D_I__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M40x25_D_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M40x25_D_I__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_M40x20_D_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_M40x20_D_I__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	MONO
#define	UNDISP						/* 非表示640x200 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U80x25_D_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U80x25_D_I__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U80x20_D_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U80x20_D_I__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-double-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U40x25_D_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U40x25_D_I__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-200-i.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_U40x20_D_I__8 )
static	VRAM2SCREEN_ALL			( v2s_all_U40x20_D_I__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	UNDISP
#undef	INTERLACE

/*----------------------------------------------------------------------
 *			● 400ライン			標準
 *----------------------------------------------------------------------*/
#define	HIRESO						/* 白黒  640x400 */
#define	TEXT_WIDTH		80
#define	TEXT_HEIGHT			25
#include "screen-vram-double-400.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_H80x25_D_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_H80x25_D_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-400.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_H80x20_D_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_H80x20_D_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#define	TEXT_WIDTH		40
#define	TEXT_HEIGHT			25
#include "screen-vram-double-400.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_H40x25_D_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_H40x25_D_N__8 )
#undef	TEXT_HEIGHT
#define	TEXT_HEIGHT			20
#include "screen-vram-double-400.h"
static	VRAM2SCREEN_DIFF		( v2s_dif_H40x20_D_N__8 )
static	VRAM2SCREEN_ALL			( v2s_all_H40x20_D_N__8 )
#undef	TEXT_WIDTH
#undef	TEXT_HEIGHT
#undef	HIRESO

#endif	/* SUPPORT_DOUBLE */


/*===========================================================================
 * 画面消去
 *===========================================================================*/

#include "screen-vram-clear.h"
SCREEN_BUF_INIT		( screen_buf_init__8 )


/*===========================================================================
 * メニュー画面
 *===========================================================================*/

#include "screen-menu-full.h"
MENU2SCREEN		( menu2screen_F_N__8 )

#include "screen-menu-half.h"
MENU2SCREEN		( menu2screen_H_N__8 )

#include "screen-menu-half-p.h"
MENU2SCREEN		( menu2screen_H_P__8 )

#ifdef	SUPPORT_DOUBLE
#include "screen-menu-double.h"
MENU2SCREEN		( menu2screen_D_N__8 )
#endif


/*===========================================================================
 * ステータス
 *===========================================================================*/

#include "screen-status.h"
STATUS2SCREEN		( status2screen__8 )
STATUS_BUF_INIT		( status_buf_init__8 )
STATUS_BUF_CLEAR	( status_buf_clear__8 )


#undef	TYPE		/* bit8 */












/* ========================================================================= */
/* 等倍サイズ - 標準 */

int  ( *vram2screen_list_F_N__8[4][4][2] )( void ) =
{
  {
    { v2s_dif_C80x25_F_N__8, v2s_all_C80x25_F_N__8 },
    { v2s_dif_C80x20_F_N__8, v2s_all_C80x20_F_N__8 },
    { v2s_dif_C40x25_F_N__8, v2s_all_C40x25_F_N__8 },
    { v2s_dif_C40x20_F_N__8, v2s_all_C40x20_F_N__8 },
  },
  {
    { v2s_dif_M80x25_F_N__8, v2s_all_M80x25_F_N__8 },
    { v2s_dif_M80x20_F_N__8, v2s_all_M80x20_F_N__8 },
    { v2s_dif_M40x25_F_N__8, v2s_all_M40x25_F_N__8 },
    { v2s_dif_M40x20_F_N__8, v2s_all_M40x20_F_N__8 },
  },
  {
    { v2s_dif_U80x25_F_N__8, v2s_all_U80x25_F_N__8 },
    { v2s_dif_U80x20_F_N__8, v2s_all_U80x20_F_N__8 },
    { v2s_dif_U40x25_F_N__8, v2s_all_U40x25_F_N__8 },
    { v2s_dif_U40x20_F_N__8, v2s_all_U40x20_F_N__8 },
  },
  {
    { v2s_dif_H80x25_F_N__8, v2s_all_H80x25_F_N__8 },
    { v2s_dif_H80x20_F_N__8, v2s_all_H80x20_F_N__8 },
    { v2s_dif_H40x25_F_N__8, v2s_all_H40x25_F_N__8 },
    { v2s_dif_H40x20_F_N__8, v2s_all_H40x20_F_N__8 },
  },
};

/* 等倍サイズ - スキップライン */

int  ( *vram2screen_list_F_S__8[4][4][2] )( void ) =
{
  {
    { v2s_dif_C80x25_F_S__8, v2s_all_C80x25_F_S__8 },
    { v2s_dif_C80x20_F_S__8, v2s_all_C80x20_F_S__8 },
    { v2s_dif_C40x25_F_S__8, v2s_all_C40x25_F_S__8 },
    { v2s_dif_C40x20_F_S__8, v2s_all_C40x20_F_S__8 },
  },
  {
    { v2s_dif_M80x25_F_S__8, v2s_all_M80x25_F_S__8 },
    { v2s_dif_M80x20_F_S__8, v2s_all_M80x20_F_S__8 },
    { v2s_dif_M40x25_F_S__8, v2s_all_M40x25_F_S__8 },
    { v2s_dif_M40x20_F_S__8, v2s_all_M40x20_F_S__8 },
  },
  {
    { v2s_dif_U80x25_F_S__8, v2s_all_U80x25_F_S__8 },
    { v2s_dif_U80x20_F_S__8, v2s_all_U80x20_F_S__8 },
    { v2s_dif_U40x25_F_S__8, v2s_all_U40x25_F_S__8 },
    { v2s_dif_U40x20_F_S__8, v2s_all_U40x20_F_S__8 },
  },
  {
    { v2s_dif_H80x25_F_N__8, v2s_all_H80x25_F_N__8 },
    { v2s_dif_H80x20_F_N__8, v2s_all_H80x20_F_N__8 },
    { v2s_dif_H40x25_F_N__8, v2s_all_H40x25_F_N__8 },
    { v2s_dif_H40x20_F_N__8, v2s_all_H40x20_F_N__8 },
  },
};

/* 等倍サイズ - インターレース */

int  ( *vram2screen_list_F_I__8[4][4][2] )( void ) =
{
  {
    { v2s_dif_C80x25_F_I__8, v2s_all_C80x25_F_I__8 },
    { v2s_dif_C80x20_F_I__8, v2s_all_C80x20_F_I__8 },
    { v2s_dif_C40x25_F_I__8, v2s_all_C40x25_F_I__8 },
    { v2s_dif_C40x20_F_I__8, v2s_all_C40x20_F_I__8 },
  },
  {
    { v2s_dif_M80x25_F_I__8, v2s_all_M80x25_F_I__8 },
    { v2s_dif_M80x20_F_I__8, v2s_all_M80x20_F_I__8 },
    { v2s_dif_M40x25_F_I__8, v2s_all_M40x25_F_I__8 },
    { v2s_dif_M40x20_F_I__8, v2s_all_M40x20_F_I__8 },
  },
  {
    { v2s_dif_U80x25_F_I__8, v2s_all_U80x25_F_I__8 },
    { v2s_dif_U80x20_F_I__8, v2s_all_U80x20_F_I__8 },
    { v2s_dif_U40x25_F_I__8, v2s_all_U40x25_F_I__8 },
    { v2s_dif_U40x20_F_I__8, v2s_all_U40x20_F_I__8 },
  },
  {
    { v2s_dif_H80x25_F_N__8, v2s_all_H80x25_F_N__8 },
    { v2s_dif_H80x20_F_N__8, v2s_all_H80x20_F_N__8 },
    { v2s_dif_H40x25_F_N__8, v2s_all_H40x25_F_N__8 },
    { v2s_dif_H40x20_F_N__8, v2s_all_H40x20_F_N__8 },
  },
};

/* ========================================================================= */
/* 半分サイズ - 標準 */

int  ( *vram2screen_list_H_N__8[4][4][2] )( void ) =
{
  {
    { v2s_dif_C80x25_H_N__8, v2s_all_C80x25_H_N__8 },
    { v2s_dif_C80x20_H_N__8, v2s_all_C80x20_H_N__8 },
    { v2s_dif_C40x25_H_N__8, v2s_all_C40x25_H_N__8 },
    { v2s_dif_C40x20_H_N__8, v2s_all_C40x20_H_N__8 },
  },
  {
    { v2s_dif_M80x25_H_N__8, v2s_all_M80x25_H_N__8 },
    { v2s_dif_M80x20_H_N__8, v2s_all_M80x20_H_N__8 },
    { v2s_dif_M40x25_H_N__8, v2s_all_M40x25_H_N__8 },
    { v2s_dif_M40x20_H_N__8, v2s_all_M40x20_H_N__8 },
  },
  {
    { v2s_dif_U80x25_H_N__8, v2s_all_U80x25_H_N__8 },
    { v2s_dif_U80x20_H_N__8, v2s_all_U80x20_H_N__8 },
    { v2s_dif_U40x25_H_N__8, v2s_all_U40x25_H_N__8 },
    { v2s_dif_U40x20_H_N__8, v2s_all_U40x20_H_N__8 },
  },
  {
    { v2s_dif_H80x25_H_N__8, v2s_all_H80x25_H_N__8 },
    { v2s_dif_H80x20_H_N__8, v2s_all_H80x20_H_N__8 },
    { v2s_dif_H40x25_H_N__8, v2s_all_H40x25_H_N__8 },
    { v2s_dif_H40x20_H_N__8, v2s_all_H40x20_H_N__8 },
  },
};

/* 半分サイズ - 色補完 */

int  ( *vram2screen_list_H_P__8[4][4][2] )( void ) =
{
  {
    { v2s_dif_C80x25_H_P__8, v2s_all_C80x25_H_P__8 },
    { v2s_dif_C80x20_H_P__8, v2s_all_C80x20_H_P__8 },
    { v2s_dif_C40x25_H_P__8, v2s_all_C40x25_H_P__8 },
    { v2s_dif_C40x20_H_P__8, v2s_all_C40x20_H_P__8 },
  },
  {
    { v2s_dif_M80x25_H_P__8, v2s_all_M80x25_H_P__8 },
    { v2s_dif_M80x20_H_P__8, v2s_all_M80x20_H_P__8 },
    { v2s_dif_M40x25_H_P__8, v2s_all_M40x25_H_P__8 },
    { v2s_dif_M40x20_H_P__8, v2s_all_M40x20_H_P__8 },
  },
  {
    { v2s_dif_U80x25_H_P__8, v2s_all_U80x25_H_P__8 },
    { v2s_dif_U80x20_H_P__8, v2s_all_U80x20_H_P__8 },
    { v2s_dif_U40x25_H_P__8, v2s_all_U40x25_H_P__8 },
    { v2s_dif_U40x20_H_P__8, v2s_all_U40x20_H_P__8 },
  },
  {
    { v2s_dif_H80x25_H_N__8, v2s_all_H80x25_H_N__8 },
    { v2s_dif_H80x20_H_N__8, v2s_all_H80x20_H_N__8 },
    { v2s_dif_H40x25_H_N__8, v2s_all_H40x25_H_N__8 },
    { v2s_dif_H40x20_H_N__8, v2s_all_H40x20_H_N__8 },
  },
};

/* ========================================================================= */
#ifdef	SUPPORT_DOUBLE
/* 二倍サイズ - 標準 */

int  ( *vram2screen_list_D_N__8[4][4][2] )( void ) =
{
  {
    { v2s_dif_C80x25_D_N__8, v2s_all_C80x25_D_N__8 },
    { v2s_dif_C80x20_D_N__8, v2s_all_C80x20_D_N__8 },
    { v2s_dif_C40x25_D_N__8, v2s_all_C40x25_D_N__8 },
    { v2s_dif_C40x20_D_N__8, v2s_all_C40x20_D_N__8 },
  },
  {
    { v2s_dif_M80x25_D_N__8, v2s_all_M80x25_D_N__8 },
    { v2s_dif_M80x20_D_N__8, v2s_all_M80x20_D_N__8 },
    { v2s_dif_M40x25_D_N__8, v2s_all_M40x25_D_N__8 },
    { v2s_dif_M40x20_D_N__8, v2s_all_M40x20_D_N__8 },
  },
  {
    { v2s_dif_U80x25_D_N__8, v2s_all_U80x25_D_N__8 },
    { v2s_dif_U80x20_D_N__8, v2s_all_U80x20_D_N__8 },
    { v2s_dif_U40x25_D_N__8, v2s_all_U40x25_D_N__8 },
    { v2s_dif_U40x20_D_N__8, v2s_all_U40x20_D_N__8 },
  },
  {
    { v2s_dif_H80x25_D_N__8, v2s_all_H80x25_D_N__8 },
    { v2s_dif_H80x20_D_N__8, v2s_all_H80x20_D_N__8 },
    { v2s_dif_H40x25_D_N__8, v2s_all_H40x25_D_N__8 },
    { v2s_dif_H40x20_D_N__8, v2s_all_H40x20_D_N__8 },
  },
};

/* 二倍サイズ - スキップライン */

int  ( *vram2screen_list_D_S__8[4][4][2] )( void ) =
{
  {
    { v2s_dif_C80x25_D_S__8, v2s_all_C80x25_D_S__8 },
    { v2s_dif_C80x20_D_S__8, v2s_all_C80x20_D_S__8 },
    { v2s_dif_C40x25_D_S__8, v2s_all_C40x25_D_S__8 },
    { v2s_dif_C40x20_D_S__8, v2s_all_C40x20_D_S__8 },
  },
  {
    { v2s_dif_M80x25_D_S__8, v2s_all_M80x25_D_S__8 },
    { v2s_dif_M80x20_D_S__8, v2s_all_M80x20_D_S__8 },
    { v2s_dif_M40x25_D_S__8, v2s_all_M40x25_D_S__8 },
    { v2s_dif_M40x20_D_S__8, v2s_all_M40x20_D_S__8 },
  },
  {
    { v2s_dif_U80x25_D_S__8, v2s_all_U80x25_D_S__8 },
    { v2s_dif_U80x20_D_S__8, v2s_all_U80x20_D_S__8 },
    { v2s_dif_U40x25_D_S__8, v2s_all_U40x25_D_S__8 },
    { v2s_dif_U40x20_D_S__8, v2s_all_U40x20_D_S__8 },
  },
  {
    { v2s_dif_H80x25_D_N__8, v2s_all_H80x25_D_N__8 },
    { v2s_dif_H80x20_D_N__8, v2s_all_H80x20_D_N__8 },
    { v2s_dif_H40x25_D_N__8, v2s_all_H40x25_D_N__8 },
    { v2s_dif_H40x20_D_N__8, v2s_all_H40x20_D_N__8 },
  },
};

/* 二倍サイズ - インターレース */

int  ( *vram2screen_list_D_I__8[4][4][2] )( void ) =
{
  {
    { v2s_dif_C80x25_D_I__8, v2s_all_C80x25_D_I__8 },
    { v2s_dif_C80x20_D_I__8, v2s_all_C80x20_D_I__8 },
    { v2s_dif_C40x25_D_I__8, v2s_all_C40x25_D_I__8 },
    { v2s_dif_C40x20_D_I__8, v2s_all_C40x20_D_I__8 },
  },
  {
    { v2s_dif_M80x25_D_I__8, v2s_all_M80x25_D_I__8 },
    { v2s_dif_M80x20_D_I__8, v2s_all_M80x20_D_I__8 },
    { v2s_dif_M40x25_D_I__8, v2s_all_M40x25_D_I__8 },
    { v2s_dif_M40x20_D_I__8, v2s_all_M40x20_D_I__8 },
  },
  {
    { v2s_dif_U80x25_D_I__8, v2s_all_U80x25_D_I__8 },
    { v2s_dif_U80x20_D_I__8, v2s_all_U80x20_D_I__8 },
    { v2s_dif_U40x25_D_I__8, v2s_all_U40x25_D_I__8 },
    { v2s_dif_U40x20_D_I__8, v2s_all_U40x20_D_I__8 },
  },
  {
    { v2s_dif_H80x25_D_N__8, v2s_all_H80x25_D_N__8 },
    { v2s_dif_H80x20_D_N__8, v2s_all_H80x20_D_N__8 },
    { v2s_dif_H40x25_D_N__8, v2s_all_H40x25_D_N__8 },
    { v2s_dif_H40x20_D_N__8, v2s_all_H40x20_D_N__8 },
  },
};
#endif	/* SUPPORT_DOUBLE */

#endif	/* SUPPORT_8BPP */
