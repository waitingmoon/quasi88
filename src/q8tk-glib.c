/************************************************************************/
/*									*/
/* QUASI88 メニュー用 Tool Kit						*/
/*				Graphic lib				*/
/*									*/
/*	Q8TK の API ( q8tk_XXX() 関数 ) 内部で呼ばれる関数群		*/
/*	menu_screen[][]に然るべき値をセットする。			*/
/*									*/
/************************************************************************/
#include <stdio.h>
#include <string.h>

#include "quasi88.h"
#include "memory.h"		/* has_kanji_rom	*/

#include "q8tk.h"
#include "q8tk-glib.h"


/********************************************************/
/* ワーク						*/
/********************************************************/
int		menu_screen_current;
T_Q8GR_SCREEN	menu_screen[2][ Q8GR_SCREEN_Y ][ Q8GR_SCREEN_X ];
static	int	menu_mouse_x;
static	int	menu_mouse_y;


/********************************************************/
/*							*/
/********************************************************/
void	q8gr_init( void )
{
  int	i, j;

  memset( menu_screen, 0, sizeof(menu_screen) );

  for( j=0; j<Q8GR_SCREEN_Y; j++ ){
    for( i=0; i<Q8GR_SCREEN_X; i++ ){
      menu_screen[ 1 ][ j ][ i ].font_type  = FONT_UNUSED;
    }
  }
  menu_screen_current = 0;

  q8gr_clear_screen();
}



/********************************************************/
/* menu_screen[][]をクリア				*/
/********************************************************/
void	q8gr_clear_screen( void )
{
  int	i, j;
  T_Q8GR_SCREEN	*p = &menu_screen[ menu_screen_current ][0][0];

  for( j=0; j<Q8GR_SCREEN_Y; j++ ){
    for( i=0; i<Q8GR_SCREEN_X; i++ ){
      p->background = Q8GR_PALETTE_BACKGROUND;
      p->foreground = Q8GR_PALETTE_FOREGROUND;
      p->mouse      = FALSE;
      p->reverse    = FALSE;
      p->underline  = FALSE;
      p->font_type  = FONT_ANK;
      p->addr       = 0;
      p ++;
    }
  }
  menu_mouse_x = -1;
  menu_mouse_y = -1;

  q8gr_reset_screen_mask();

  q8gr_set_cursor_exist( FALSE );
}



/********************************************************/
/* スクリーンのマスキング				*/
/********************************************************/
static	int	screen_mask_x0, screen_mask_x1;
static	int	screen_mask_y0, screen_mask_y1;



#define	CHECK_MASK_X_FOR(x)	if     ( (x) < screen_mask_x0 ) continue;\
				else if( (x) >=screen_mask_x1 ) break
#define	CHECK_MASK_Y_FOR(y)	if     ( (y) < screen_mask_y0 ) continue;\
				else if( (y) >=screen_mask_y1 ) break
#define	CHECK_MASK_X(x)		( (x)<screen_mask_x0 || (x)>=screen_mask_x1 )
#define	CHECK_MASK_Y(x)		( (y)<screen_mask_y0 || (y)>=screen_mask_y1 )


void	q8gr_set_screen_mask( int x, int y, int sx, int sy )
{
  screen_mask_x0 = x;
  screen_mask_y0 = y;
  screen_mask_x1 = x + sx;
  screen_mask_y1 = y + sy;
}
void	q8gr_reset_screen_mask( void )
{
  screen_mask_x0 = 0;
  screen_mask_y0 = 0;
  screen_mask_x1 = Q8GR_SCREEN_X;
  screen_mask_y1 = Q8GR_SCREEN_Y;
}




/********************************************************/
/* フォーカス用のスクリーン情報				*/
/********************************************************/
static	void	*focus_screen[ Q8GR_SCREEN_Y ][ Q8GR_SCREEN_X ];


void	q8gr_clear_focus_screen( void )
{
  int	i, j;

  for( j=0; j<Q8GR_SCREEN_Y; j++ )
    for( i=0; i<Q8GR_SCREEN_X; i++ )
      focus_screen[j][i] = NULL;
}

void	q8gr_set_focus_screen( int x, int y, int sx, int sy, void *p )
{
  int	i, j;

  if( p ){
    for( j=y; j<y+sy; j++ ){
      CHECK_MASK_Y_FOR(j);
      for( i=x; i<x+sx; i++ ){
	CHECK_MASK_X_FOR(i);
	focus_screen[j][i] = p;
      }
    }
  }
}
void	*q8gr_get_focus_screen( int x, int y )
{
  if( 0<=x && x<Q8GR_SCREEN_X &&
      0<=y && y<Q8GR_SCREEN_Y ){
    return focus_screen[y][x];
  }else{
    return NULL;
  }
}
int	q8gr_scan_focus_screen( void *p )
{
  int	i, j;

  for( j=0; j<Q8GR_SCREEN_Y; j++ ){
    for( i=0; i<Q8GR_SCREEN_X; i++ ){
      if( focus_screen[j][i]==p ) return TRUE;
    }
  }
  return FALSE;
}





/*----------------------------------------------------------------------
 * (スクロールド)ウインドウ・(トグル)ボタン・フレーム・オプションメニュー
 *----------------------------------------------------------------------
 *        ｘ
 *   ｙ  ┌──────────┓↑ 	左上から光があたっているような
 *       │                    ┃｜ 	立体感をだす。
 *       │                    ┃sy 
 *       │                    ┃｜ 
 *       └━━━━━━━━━━┛↓ 
 *        ←────sx────→
 *----------------------------------------------------------------------*/
static	void	draw_normal_box( int x, int y, int sx, int sy, int shadow_type)
{
  int	i,j,c,fg;
  int	light, shadow;

  switch( shadow_type ){
  case Q8TK_SHADOW_NONE:			/* 枠線なし */
    light  = Q8GR_PALETTE_BACKGROUND;
    shadow = Q8GR_PALETTE_BACKGROUND;
    break;
  case Q8TK_SHADOW_IN:				/* 全体がへこんでいる */
    light  = Q8GR_PALETTE_SHADOW;
    shadow = Q8GR_PALETTE_LIGHT;
    break;
  case Q8TK_SHADOW_OUT:				/* 全体が盛り上がっている */
    light  = Q8GR_PALETTE_LIGHT;
    shadow = Q8GR_PALETTE_SHADOW;
    break;
  case Q8TK_SHADOW_ETCHED_IN:			/* 枠だけへこんでいる */
    light  = Q8GR_PALETTE_LIGHT;
    shadow = Q8GR_PALETTE_LIGHT;
    break;
  case Q8TK_SHADOW_ETCHED_OUT:			/* 枠だけ盛り上がっている */
  default:
    light  = Q8GR_PALETTE_SHADOW;
    shadow = Q8GR_PALETTE_SHADOW;
    break;
  }

  for( j=y; j<y+sy; j++ ){
    CHECK_MASK_Y_FOR(j);
    if( j==y || j==y+sy-1 ){
      for( i=x; i<x+sx; i++ ){
	CHECK_MASK_X_FOR(i);

	if      ( i==x      && j==y      ){		/* 左上 ┌ */
	  c  = Q8GR_G_7;
	  fg = light;
	}else if( i==x+sx-1 && j==y      ){		/* 右上 ┐ */
	  c  = Q8GR_G_9;
	  fg = shadow;
	}else if( i==x      && j==y+sy-1 ){		/* 左下 └ */
	  c  = Q8GR_G_1;
	  fg = light;
	}else if( i==x+sx-1 && j==y+sy-1 ){		/* 右下 ┘ */
	  c  = Q8GR_G_3;
	  fg = shadow;
	}else{
	  c  = Q8GR_G__;
	  if( j==y ) fg = light;			/* 上   ─ */
	  else       fg = shadow;			/* 下   ─ */
	}

	q8gr_putc( i, j, fg, Q8GR_PALETTE_BACKGROUND, FALSE, FALSE, c );
      }
    }else{
      for( i=x; i<x+sx; i++ ){
	CHECK_MASK_X_FOR(i);

	if( i==x || i==x+sx-1 ){
	  c  = Q8GR_G_I;
	  if( i==x ) fg = light;			/* 左   │ */
	  else       fg = shadow;			/* 右   │ */
	}else{						/* 間      */
	  c  = (Uint)' ';
	  fg = Q8GR_PALETTE_FOREGROUND;
	}

	q8gr_putc( i, j, fg, Q8GR_PALETTE_BACKGROUND, FALSE, FALSE, c );
      }
    }
  }
}



void	q8gr_draw_window( int x, int y, int sx, int sy, int shadow_type )
{
  draw_normal_box( x, y, sx, sy, shadow_type );
}

void	q8gr_draw_button( int x, int y, int sx, int sy, int condition, void *p)
{
  int shadow_type;
  if( condition == Q8TK_BUTTON_OFF ) shadow_type = Q8TK_SHADOW_OUT;
  else                               shadow_type = Q8TK_SHADOW_IN;

  draw_normal_box( x, y, sx, sy, shadow_type );

  q8gr_set_focus_screen( x, y, sx, sy, p );
}

void	q8gr_draw_frame( int x, int y, int sx, int sy, int shadow_type,
			 int code, const char *str )
{
  draw_normal_box( x, y, sx, sy, shadow_type );

  q8gr_puts( x+1, y, Q8GR_PALETTE_FOREGROUND, Q8GR_PALETTE_BACKGROUND,
	     FALSE, FALSE, code, str );
}

void	q8gr_draw_scrolled_window( int x, int y, int sx, int sy,
				   int shadow_type, void *p )
{
  draw_normal_box( x, y, sx, sy, shadow_type );

  q8gr_set_focus_screen( x, y, sx, sy, p );
  if( sx >= 2  &&  sy >= 2 ){
    q8gr_set_focus_screen( x+1, y+1, sx-2, sy-2, p );
  }
}



/*----------------------------------------------------------------------
 * チェックボタン
 *----------------------------------------------------------------------
 *        ｘ
 *   ｙ     □  文字列の部分
 *        ←─→
 *	  ボタン
 *----------------------------------------------------------------------*/
void	q8gr_draw_check_button( int x, int y, int condition, void *p )
{
  q8gr_putc( x  , y, Q8GR_PALETTE_FOREGROUND, Q8GR_PALETTE_BACKGROUND,
	     FALSE, FALSE, Q8GR_L_RIGHT );

  q8gr_putc( x+1, y, Q8GR_PALETTE_FONT_FG,    Q8GR_PALETTE_FONT_BG,
	     FALSE, FALSE, (condition==Q8TK_BUTTON_OFF) ? Q8GR_B_UL
							: Q8GR_B_BOX );

  q8gr_putc( x+2, y, Q8GR_PALETTE_FOREGROUND, Q8GR_PALETTE_BACKGROUND,
	     FALSE, FALSE, Q8GR_L_LEFT );

#if 0	/* チェックボタン部のみ、クリックに反応する */

  q8gr_set_focus_screen( x+1, y, 1, 1, p );

#else	/* ラベルを子に持つ場合、そのラベルをクリックしても反応する */

  if( ((Q8tkWidget *)p)->child &&
      ((Q8tkWidget *)p)->child->type == Q8TK_TYPE_LABEL &&
      ((Q8tkWidget *)p)->child->visible &&
      ((Q8tkWidget *)p)->child->name ){

    int len = q8gr_strlen( ((Q8tkWidget *)p)->child->code,
			   ((Q8tkWidget *)p)->child->name );

    q8gr_set_focus_screen( x+1, y, 1+1+len, 1, p );
  }else{
    q8gr_set_focus_screen( x+1, y, 1, 1, p );
  }
#endif
}


/*----------------------------------------------------------------------
 * ラジオボタン
 *----------------------------------------------------------------------
 *        ｘ
 *   ｙ     ○  文字列の部分
 *        ←─→
 *	  ボタン
 *----------------------------------------------------------------------*/
void	q8gr_draw_radio_button( int x, int y, int condition, void *p )
{
  q8gr_putc( x  , y, Q8GR_PALETTE_FOREGROUND, Q8GR_PALETTE_BACKGROUND, 
	     FALSE, FALSE, ' ' );

  q8gr_putc( x+1, y, Q8GR_PALETTE_FOREGROUND, Q8GR_PALETTE_BACKGROUND, 
	     FALSE, FALSE, (condition==Q8TK_BUTTON_OFF) ? Q8GR_B_OFF
							    : Q8GR_B_ON   );
  q8gr_putc( x+2, y, Q8GR_PALETTE_FOREGROUND, Q8GR_PALETTE_BACKGROUND, 
	     FALSE, FALSE, ' ' );


#if 0	/* ラジオボタン部のみ、クリックに反応する */

  q8gr_set_focus_screen( x+1, y, 1, 1, p );

#else	/* ラベルを子に持つ場合、そのラベルをクリックしても反応する */

  if( ((Q8tkWidget *)p)->child &&
      ((Q8tkWidget *)p)->child->type == Q8TK_TYPE_LABEL &&
      ((Q8tkWidget *)p)->child->visible &&
      ((Q8tkWidget *)p)->child->name ){

    int len = q8gr_strlen( ((Q8tkWidget *)p)->child->code,
			   ((Q8tkWidget *)p)->child->name );

    q8gr_set_focus_screen( x+1, y, 1+1+len, 1, p );
  }else{
    q8gr_set_focus_screen( x+1, y, 1, 1, p );
  }
#endif
}



/*----------------------------------------------------------------------
 * ノートブック
 *----------------------------------------------------------------------
 *        ｘ
 *   ｙ  ┌──┓━━┓━━┓    ↑
 *       │タブ┃タブ│タブ│	 ｜
 *       │    └──┴──┴─┓｜
 *       │                    ┃sy
 *       │                    ┃｜
 *       │                    ┃｜
 *       └━━━━━━━━━━┛↓
 *        ←────sx────→
 *----------------------------------------------------------------------*/
static	struct{
  int	x, y;
  int	x0, x1;
  int	selected;
} note_w;
void	q8gr_draw_notebook( int x, int y, int sx, int sy )
{
  note_w.x  = x;
  note_w.y  = y;
  note_w.x0 = x;
  note_w.x1 = x+sx-1;
  note_w.selected = FALSE;

  q8gr_draw_button( x, y+2, sx, sy-2, Q8TK_BUTTON_OFF, NULL );
}
void	q8gr_draw_notepage( int code, const char *tag,
			    int select_flag, int active_flag, void *p )
{
  int	i,len;
  int	bg = Q8GR_PALETTE_BACKGROUND;
  int	light  = Q8GR_PALETTE_LIGHT;
  int	shadow = Q8GR_PALETTE_SHADOW;
  int	focus_x = note_w.x;

  len = q8gr_strlen( code, tag );

  if( select_flag ){

    q8gr_puts( note_w.x+1, note_w.y+1, Q8GR_PALETTE_FOREGROUND, bg,
	       FALSE, active_flag, code, tag );

    q8gr_putc( note_w.x, note_w.y,   light, bg, FALSE, FALSE, Q8GR_C_7 );
    q8gr_putc( note_w.x, note_w.y+1, light, bg, FALSE, FALSE, Q8GR_G_I );
    if( note_w.x==note_w.x0 )
      q8gr_putc( note_w.x, note_w.y+2, light, bg, FALSE, FALSE, Q8GR_G_I );
    else
      q8gr_putc( note_w.x, note_w.y+2, light, bg, FALSE, FALSE, Q8GR_G_3 );

    note_w.x ++;
    for( i=0; i<len; i++, note_w.x++ ){
      q8gr_putc( note_w.x, note_w.y,   light, bg, FALSE, FALSE, Q8GR_G__ );
      q8gr_putc( note_w.x, note_w.y+2, light, bg, FALSE, FALSE, ' ' );
    }

    q8gr_putc( note_w.x, note_w.y,   shadow, bg, FALSE, FALSE, Q8GR_C_9 );
    q8gr_putc( note_w.x, note_w.y+1, shadow, bg, FALSE, FALSE, Q8GR_G_I );
    if( note_w.x==note_w.x1 )
      q8gr_putc( note_w.x, note_w.y+2, shadow, bg, FALSE, FALSE, Q8GR_G_I);
    else
      q8gr_putc( note_w.x, note_w.y+2, light,  bg, FALSE, FALSE, Q8GR_G_1);
    note_w.x ++;

    note_w.selected = TRUE;

  }else if( note_w.selected==FALSE ){

    q8gr_puts( note_w.x+1, note_w.y+1, Q8GR_PALETTE_FOREGROUND, bg,
	       FALSE, active_flag, code, tag );

    q8gr_putc( note_w.x, note_w.y,   shadow, bg, FALSE, FALSE, Q8GR_C_7 );
    q8gr_putc( note_w.x, note_w.y+1, shadow, bg, FALSE, FALSE, Q8GR_G_I );
    if( note_w.x==note_w.x0 )
      q8gr_putc( note_w.x, note_w.y+2, light, bg, FALSE, FALSE, Q8GR_G_4 );
    else
      q8gr_putc( note_w.x, note_w.y+2, light, bg, FALSE, FALSE, Q8GR_G_2 );

    note_w.x ++;
    for( i=0; i<len; i++, note_w.x++ ){
      q8gr_putc( note_w.x, note_w.y, shadow, bg, FALSE, FALSE, Q8GR_G__ );
    }

  }else{

    q8gr_puts( note_w.x, note_w.y+1, Q8GR_PALETTE_FOREGROUND, bg,
	       FALSE, active_flag, code, tag );

    for( i=0; i<len; i++, note_w.x++ ){
      q8gr_putc( note_w.x, note_w.y, shadow, bg, FALSE, FALSE, Q8GR_G__ );
    }

    q8gr_putc( note_w.x, note_w.y,   shadow, bg, FALSE, FALSE, Q8GR_C_9 );
    q8gr_putc( note_w.x, note_w.y+1, light,  bg, FALSE, FALSE, Q8GR_G_I );
    if( note_w.x==note_w.x1 )
      q8gr_putc( note_w.x, note_w.y+2, shadow, bg, FALSE, FALSE, Q8GR_G_6);
    else
      q8gr_putc( note_w.x, note_w.y+2, light,  bg, FALSE, FALSE, Q8GR_G_2);
    note_w.x ++;

  }


  q8gr_set_focus_screen( focus_x, note_w.y, note_w.x-focus_x, 2, p );
}


/*----------------------------------------------------------------------
 * 垂直・水平区切り線
 *----------------------------------------------------------------------
 *        ｘ             ｘ
 *   ｙ   ┃↑        ｙ ━━━━━━━━━━
 *        ┃｜           ←───width ──→
 *        ┃height
 *        ┃｜
 *        ┃↓
 *----------------------------------------------------------------------*/
void	q8gr_draw_vseparator( int x, int y, int height )
{
  int	j;
  if( !CHECK_MASK_X(x) ){
    for( j=y; j<y+height; j++ ){
      CHECK_MASK_Y_FOR(j);
      q8gr_putc( x, j, Q8GR_PALETTE_FOREGROUND, Q8GR_PALETTE_BACKGROUND,
		 FALSE, FALSE, Q8GR_G_I );
    }
  }
}
void	q8gr_draw_hseparator( int x, int y, int width )
{
  int	i;
  if( !CHECK_MASK_Y(y) ){
    for( i=x; i<x+width; i++ ){
      CHECK_MASK_X_FOR(i);
      q8gr_putc( i, y, Q8GR_PALETTE_FOREGROUND, Q8GR_PALETTE_BACKGROUND,
		 FALSE, FALSE, Q8GR_G__ );
    }
  }
}

/*----------------------------------------------------------------------
 * エントリ
 *----------------------------------------------------------------------
 *          ｘ
 *          ←─width →
 *   y      文字列の部分
 *
 *----------------------------------------------------------------------*/
void	q8gr_draw_entry( int x, int y, int width, int code, const char *text,
			 int disp_pos, int cursor_pos, void *p )
{
  q8gr_strings( x, y,
		Q8GR_PALETTE_FONT_FG, Q8GR_PALETTE_FONT_BG, 
		FALSE, FALSE, cursor_pos,
		code, text, disp_pos, width );

  q8gr_set_focus_screen( x, y, width, 1, p );
}


/*----------------------------------------------------------------------
 * コンボ
 *----------------------------------------------------------------------
 *        ｘ
 *   ｙ   文字列の部分  ▼		矢印だけを表示する。
 *        ←─width →←─→		文字列は entry として別途表示する
 *                     矢印
 *----------------------------------------------------------------------*/
void	q8gr_draw_combo( int x, int y, int width, int active, void *p )
{
#if 0
  int fg = (active) ? Q8GR_PALETTE_FONT_FG : Q8GR_PALETTE_FOREGROUND;

  q8gr_putc( x+width,  y, fg, Q8GR_PALETTE_BACKGROUND, TRUE, FALSE, ' ' );
  q8gr_putc( x+width+1,y, fg, Q8GR_PALETTE_BACKGROUND, TRUE, FALSE, Q8GR_A_D );
  q8gr_putc( x+width+2,y, fg, Q8GR_PALETTE_BACKGROUND, TRUE, FALSE, ' ' );

  q8gr_set_focus_screen( x+width, y, 3, 1, p );


#else	/* 矢印部を2文字にしてみよう */
  int fg = (active) ? Q8GR_PALETTE_FONT_FG : Q8GR_PALETTE_FOREGROUND;

  q8gr_putc( x+width,  y, fg, Q8GR_PALETTE_BACKGROUND, FALSE, FALSE, 0xe6 );
  q8gr_putc( x+width+1,y, fg, Q8GR_PALETTE_BACKGROUND, FALSE, FALSE, 0xe7 );

  q8gr_set_focus_screen( x+width, y, 2, 1, p );
#endif
}


/*----------------------------------------------------------------------
 * リストアイテム
 *----------------------------------------------------------------------
 *        ｘ
 *   ｙ   文字列の部分
 *        ←─width →
 *----------------------------------------------------------------------*/
void	q8gr_draw_list_item( int x, int y, int width, int active,
			     int reverse, int underline,
			     int code, const char *text, void *p )
{
  int	fg = (active) ? Q8GR_PALETTE_FONT_FG : Q8GR_PALETTE_FOREGROUND;

  q8gr_strings( x, y,
		fg, Q8GR_PALETTE_FONT_BG,
		reverse, underline, -1,
		code, text, 0, width );

  q8gr_set_focus_screen( x, y, width, 1, p );
}


/*----------------------------------------------------------------------
 * 水平スケール・垂直スケール
 *----------------------------------------------------------------------
 *        ｘ             ｘ
 *   ｙ   ↑↑        ｙ □□□□■□□□□→
 *        □｜           ←───length──→
 *        ■height
 *        □｜
 *        ↓↓
 *----------------------------------------------------------------------*/
static	void	draw_adjustment( int x, int y, int active, 
				 Q8Adjust *adj, void *p )
{
  int	i, fg = Q8GR_PALETTE_FOREGROUND;
  if( active ) fg = Q8GR_PALETTE_SCALE_ACT;

  adj->x = x;
  adj->y = y;

  if( adj->horizontal ){			/* HORIZONTAL */

    if( adj->arrow ){
      q8gr_putc( x, y, fg, Q8GR_PALETTE_SCALE_SLD,
		 FALSE, FALSE, Q8GR_A_L );
      x++;
    }
    for( i=0; i<adj->length; i++ )
      if( i==adj->pos ){
	q8gr_putc( x+i, y, fg, Q8GR_PALETTE_SCALE_SLD,
		   FALSE, FALSE, Q8GR_B_B );
      }else{
	q8gr_putc( x+i, y, fg, Q8GR_PALETTE_SCALE_BAR,
		   FALSE, FALSE, ' ' );
      }
    if( adj->arrow ){
      q8gr_putc( x+i, y, fg, Q8GR_PALETTE_SCALE_SLD,
		 FALSE, FALSE, Q8GR_A_R );
      x--;
    }
    q8gr_set_focus_screen( x, y, adj->length +(adj->arrow?2:0), 1, p );

  }else{				/* Virtival */

    if( adj->arrow ){
      q8gr_putc( x, y, fg, Q8GR_PALETTE_SCALE_SLD,
		 FALSE, FALSE, Q8GR_A_U );
      y++;
    }
    for( i=0; i<adj->length; i++ )
      if( i==adj->pos ){
	q8gr_putc( x, y+i, fg, Q8GR_PALETTE_SCALE_SLD,
		   FALSE, FALSE, Q8GR_B_B );
      }else{
	q8gr_putc( x, y+i, fg, Q8GR_PALETTE_SCALE_BAR,
		   FALSE, FALSE, ' ' );
      }
    if( adj->arrow ){
      q8gr_putc( x, y+i, fg, Q8GR_PALETTE_SCALE_SLD,
		 FALSE, FALSE, Q8GR_A_D );
      y--;
    }
    q8gr_set_focus_screen( x, y, 1, adj->length +(adj->arrow?2:0), p );
  }

}



void	q8gr_draw_hscale( int x, int y, Q8Adjust *adj, int active,
			  int draw_value, int value_pos, void *p )
{
  if( draw_value ){
    int  vx, vy;
    char valstr[8];
    int	len = adj->length + (adj->arrow?2:0);

    if     ( adj->value < -99 ) strcpy( valstr, "-**" );
    else if( adj->value > 999 ) strcpy( valstr, "***" );
    else                        sprintf( valstr, "%3d", adj->value );

    switch( value_pos ){
    case Q8TK_POS_LEFT:
      vx = x;	vy = y;
      x += 4;
      break;
    case Q8TK_POS_RIGHT:
      vx = x + len+1;	vy = y;
      break;
    case Q8TK_POS_TOP:
      vx = x + ((adj->pos+3 > len) ? (len-3) : adj->pos);
      vy = y;
      y += 1;
      break;
    case Q8TK_POS_BOTTOM:
    default:
      vx = x + ((adj->pos+3 > len) ? (len-3) : adj->pos);
      vy = y +1;
      break;
    }

    q8gr_puts( vx, vy, Q8GR_PALETTE_FOREGROUND, Q8GR_PALETTE_BACKGROUND,
	       FALSE, FALSE, Q8TK_KANJI_ANK, valstr );

  }

  draw_adjustment( x, y, active, adj, p );
}


void	q8gr_draw_vscale( int x, int y, Q8Adjust *adj, int active,
			  int draw_value, int value_pos, void *p )
{
  if( draw_value ){
    int  vx, vy;
    char valstr[8];

    if     ( adj->value < -99 ) strcpy( valstr, "-**" );
    else if( adj->value > 999 ) strcpy( valstr, "***" );
    else                        sprintf( valstr, "%3d", adj->value );

    switch( value_pos ){
    case Q8TK_POS_LEFT:
      vx = x;	vy = y + adj->pos + (adj->arrow?1:0);
      x += 4;
      break;
    case Q8TK_POS_RIGHT:
      vx = x+1;	vy = y + adj->pos + (adj->arrow?1:0);
      break;
    case Q8TK_POS_TOP:
      vx = x;	vy = y;
      x += 1;	y += 1;
      break;
    case Q8TK_POS_BOTTOM:
    default:
      vx = x;	vy = y + adj->length + (adj->arrow?2:0);
      x += 1;
      break;
    }

    q8gr_puts( vx, vy, Q8GR_PALETTE_FOREGROUND, Q8GR_PALETTE_BACKGROUND,
	       FALSE, FALSE, Q8TK_KANJI_ANK, valstr );

  }

  draw_adjustment( x, y, active, adj, p );
}





/************************************************************************/
/* ワーク menu_screen[][] を実際に操作する関数				*/
/*	なお、表示バイトとは、半角1バイト、全角2バイトと数える		*/
/************************************************************************/


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 * ASCIIコード、JIS漢字コードを、内蔵ROMアドレスに変換
 *- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void kanji2addr( int code, unsigned int *addr, unsigned int *type )
{
  if( has_kanji_rom == FALSE ){	/* ---- 漢字ROMない場合 ---- */

    if     ( code < 0x100 )		/* 半角文字 */
    {
      *addr = (code << 3);
      *type = FONT_ANK;
    }
    else{				/* 全角文字 */
      *addr = 0;
      *type = FONT_KNJXL;
    }

  }else{			/* ---- 漢字ROMある場合 ---- */

    if     ( code < 0x100 )		/* 半角文字 */
    {
      if( ( 0x20 <= code && code <= 0x7f ) ||	/* ASCII    */
	  ( 0xa0 <= code && code <= 0xdf ) ){	/* カタカナ */

	*addr = (code << 3);
	*type = FONT_HALF;

      }else{				/* コントロール文字,グラフィック文字 */
	*addr = (code << 2) | 0x0800;
	*type = FONT_QUART;
      }
    }
    else if( code < 0x3000 )		/* 非漢字 */
    {
      *addr = ( ((code&0x0060)<<7) | ((code&0x0700)<<1) | ((code&0x001f)<<4) );
      *type = FONT_KNJ1L;
    }
    else if( code < 0x5000 )		/* 第一水準漢字 */
    {
      *addr = ( ((code&0x0060)<<9) | ((code&0x1f00)<<1) | ((code&0x001f)<<4) );
      *type = FONT_KNJ1L;
    }
    else if( code < 0x7000 )		/* 第二水準漢字 その1 */
    {
      *addr = ( ((code&0x0060)<<9) | ((code&0x0f00)<<1) | ((code&0x001f)<<4) 
		| (code&0x2000) );
      *type = FONT_KNJ2L;
    }
    else				/* 第二水準漢字 その2 */
    {
      *addr = ( ((code&0x0060)<<7) | ((code&0x0700)<<1) | ((code&0x001f)<<4) );
      *type = FONT_KNJ2L;
    }
  }
}

/*----------------------------------------------------------------------
 * 簡略式 文字列表示 (EUC/SJIS/ANK)
 *----------------------------------------------------------------------*/
int	q8gr_puts( int x, int y, int fg, int bg, int reverse, int underline,
		   int code, const char *str )
{
  return q8gr_strings( x, y, fg, bg, reverse, underline, -1,
		       code, str, 0, 0 );
}

/*----------------------------------------------------------------------
 * 汎用文字列表示 (EUC/SJIS/ANK)
 *
 *	x, y, bg, fg       … 表示座標 および、前景色・背景色
 *	reverse, underline … 真なら反転 および アンダーライン
 *	cursor_pos	   … カーソル表示バイト位置 (-1なら無し)
 *	code, str          … 文字コード、文字列
 *	start              … 表示開始バイト位置 (0〜)
 *	width              … 表示バイト数。 width==0 なら全て表示する
 *	                      文字列が表示バイト数に満たない場合は空白を表示
 *	戻り値             … 実際に表示した文字バイト数
 *			      (width!=0 なら、widthと同じはず)
 *----------------------------------------------------------------------*/
int	q8gr_strings( int x, int y, int fg, int bg, int reverse, int underline,
		      int cursor_pos, 
		      int code, const char *str, int start, int width )
{
  const unsigned char  *p  = (const unsigned char *)str;
  unsigned int		h, c, type, addr, rev;
  int w, pos = 0;
  int count = 0;


  /* -------- width バイト分、表示 (width==0なら全て表示) -------- */

  while( (h = (unsigned int)*p ++ ) ){

    /* ======== 1バイト、ないし 2バイト の文字コードを取得 ======== */

    if( code == Q8TK_KANJI_EUC ){	/* - - - - EUC - - - - */

      if( h < 0x80 ){				/* ASCII */

	c = h;

      }else if( h == 0x8e ){			/* 半角カナ */

	c = *p ++;					/* 0xa0〜0xdf以外 */
	if( c == '\0' ) break;				/* でも気にしない */

      }else{					/* 漢字 */

	c = *p ++;					/* 範囲外         */
	if( c == '\0' ) break;				/* でも気にしない */

	/* EUC → JIS */
	c = ((h & 0x7f) << 8) | (c & 0x7f);

      }

    }
    else if( code == Q8TK_KANJI_SJIS ){	/* - - - - SJIS - - - - */

      if( (h >= 0x81 && h <= 0x9f) ||		 /* 漢字 */ 
	  (h >= 0xe0 && h <= 0xfc) ){

	c = *p ++;					/* 2バイト目が範囲外 */
	if( c == '\0' ) break;				/* でも気にしない    */

	c = (h << 8) | c;

	/* SJIS → JIS */
	if( 0xe000 <= c ) c -= 0x4000;
	c = ((((c & 0xff00) - 0x8100)<<1) | (c & 0x00ff) ) & 0xffff;
	if( (c & 0x00ff) >= 0x80 ) c -= 1;
	if( (c & 0x00ff) >= 0x9e ) c += 0x100 - 0x9e;
	else                       c -= 0x40;
	c += 0x2121;

      }else{					/* ANK */

	c = h;

      }

    }
    else{				/* - - - - ANK - - - - */

      c = h;

    }

    /* ======== 描画開始チェック  ======== */

    if( c < 0x100 ) w = 1;		/* 1バイト文字 */
    else            w = 2;		/* 2バイト文字 */
      
    if( count == 0 ){			/* 未描画 */

      if( pos < start && pos + w <= start ){	/* 描画領域未達 */
	pos += w;
	continue;
      }
      else if( pos < start ){			/* 描画領域干渉 */
	c = 0x8e;	/* ダミー文字を */
	w = 1;		/* 1バイト描画  */
      }
      else{					/* 描画領域到達 */
	;
      }
    }
    
#if 0	/* select_start 〜 select_end を反転させる (文字列の範囲指定)	*/
    if( 0 <= select_start ){
      if( pos == select_start ){
	reverse = !(reverse);
      }
      if( 0 <= select_end ){
	if( pos == select_end ){
	  reverse = !(reverse);
	}
      }
    }
#endif

    rev = reverse;
    if( 0 <= cursor_pos ){		/* カーソル位置 */
      if( pos==cursor_pos ){
	q8gr_set_cursor_exist( TRUE );
	if( q8gr_get_cursor_blink() ){
	  rev = !(rev);
	}
      }
    }

    pos += w;


    /* ======== 文字コードをフォント種別／アドレスに変換  ======== */

    kanji2addr( c, &addr, &type );



    /* ======== menu_screen[][] にセット  ======== */

    if( !CHECK_MASK_Y(y) ){
      if( !CHECK_MASK_X(x) ){
	menu_screen[menu_screen_current][y][x].background = bg;
	menu_screen[menu_screen_current][y][x].foreground = fg;
	menu_screen[menu_screen_current][y][x].reverse    = rev;
	menu_screen[menu_screen_current][y][x].underline  = underline;
	menu_screen[menu_screen_current][y][x].font_type  = type;
	menu_screen[menu_screen_current][y][x].addr       = addr;
      }
      if( type >= FONT_2_BYTE ){
	x++;
	count ++;
	if( width && width <= count ) break;
	if( !CHECK_MASK_X(x) ){
	  menu_screen[menu_screen_current][y][x].background = bg;
	  menu_screen[menu_screen_current][y][x].foreground = fg;
	  menu_screen[menu_screen_current][y][x].reverse    = rev;
	  menu_screen[menu_screen_current][y][x].underline  = underline;
	  menu_screen[menu_screen_current][y][x].font_type  = type + 1;
	  menu_screen[menu_screen_current][y][x].addr       = addr;
	}
      }
    }
    x++;
    count ++;
    if( width && width <= count ) break;
  }


  /* -------- width 指定時、 余った部分はスペースを表示 -------- */

  if( width ){
    for( ; count < width; ){

#if 0	/* select_start 〜 select_end を反転させる (文字列の範囲指定)	*/
      if( 0 <= select_start ){
	if( pos == select_start ){
	  reverse = !(reverse);
	}
	if( 0 <= select_end ){
	  if( pos == select_end ){
	    reverse = !(reverse);
	  }
	}
      }
#endif

      rev = reverse;
      if( 0 <= cursor_pos ){		/* カーソル位置 */
	if( pos==cursor_pos ){
	  q8gr_set_cursor_exist( TRUE );
	  if( q8gr_get_cursor_blink() ){
	    rev = !(rev);
	  }
	}
      }

      q8gr_putc( x, y, fg, bg, rev, underline, ' ' );
      
      pos ++;
      x ++;
      count ++;
    }
  }
  return count;
}

/*----------------------------------------------------------------------
 * ANK文字 putc
 *	x, y, bg, fg  … 表示座標 および、前景色・背景色
 *	reverse, unsigned … 真なら反転 および アンダーライン
 *	c … 文字コード (0x00〜0xff)
 *----------------------------------------------------------------------*/
void	q8gr_putc( int x, int y, int fg, int bg,
		   int reverse, int underline, int c )
{
  if( !CHECK_MASK_X(x) && !CHECK_MASK_Y(y) ){
    menu_screen[menu_screen_current][y][x].background = bg;
    menu_screen[menu_screen_current][y][x].foreground = fg;
    menu_screen[menu_screen_current][y][x].reverse    = reverse;
    menu_screen[menu_screen_current][y][x].underline  = underline;
    menu_screen[menu_screen_current][y][x].font_type  = FONT_ANK;
    menu_screen[menu_screen_current][y][x].addr       = (c&0xff) << 3;
  }
}


/*----------------------------------------------------------------------
 * 文字列の表示バイト長を返す
 *	code, str … 文字コード, 文字列,
 *----------------------------------------------------------------------*/
int	q8gr_strlen( int code, const char *str )
{
  const unsigned char  *p  = (const unsigned char *)str;
  unsigned int          h;
  int count = 0;

  if( code == Q8TK_KANJI_EUC ){

    while( ( h = (unsigned int)*p ++ ) ){

      if( h < 0x80 ){				/* ASCII */
	count ++;
      }else if( h == 0x8e ){			/* 半角カナ */
	if( *p == '\0' ) break;				/* 0xa0〜0xdf以外 */
	p ++;						/* でも気にしない */
	count ++;
      }else{					/* 漢字 */
	if( *p == '\0' ) break;				/* 範囲外         */
	p ++;						/* でも気にしない */
	count += 2;
      }
    }

  }else if( code == Q8TK_KANJI_SJIS ){

    while( ( h = (unsigned int)*p ++ ) ){

      if( (h >= 0x81 && h <= 0x9f) ||		 /* 漢字 */ 
	  (h >= 0xe0 && h <= 0xfc) ){
	if( *p == '\0' ) break;				/* 2バイト目が範囲外 */
	p ++;						/* でも気にしない    */
	count += 2;
      }else{
	count ++;
      }
    }

  }else{

    count = (int)strlen( str );

  }
  return count;
}


/*----------------------------------------------------------------------
 * 文字列の 表示バイト pos が、文字のどの部分にあたるかを返す
 *	code, str, pos … 文字コード, 文字列, チェックする表示バイト位置
 *	戻り値	0 = 1バイト文字
 *		1 = 2バイト文字の前半
 *		2 = 2バイト文字の後半
 *----------------------------------------------------------------------*/
int	q8gr_strchk( int code, const char *str, int pos )
{
  const unsigned char  *p  = (const unsigned char *)str;
  unsigned int          h;
  int count = 0;
  int type = 0;

  if( code == Q8TK_KANJI_EUC ){

    while( ( h = (unsigned int)*p ++ ) ){

      if( h < 0x80 ){				/* ASCII */
	type = 0;
      }else if( h == 0x8e ){			/* 半角カナ */
	if( *p == '\0' ) break;				/* 0xa0〜0xdf以外 */
	p ++;						/* でも気にしない */
	type = 0;
      }else{					/* 漢字 */
	if( *p == '\0' ) break;				/* 範囲外         */
	p ++;						/* でも気にしない */
	type = 1;
      }

      if( pos == count ){
	return type;
      }
      if( type==1 ){
	count ++;
	if( pos == count ){
	  return 2;
	}
      }
      count ++;
    }
    return 0;

  }else if( code == Q8TK_KANJI_SJIS ){

    while( ( h = (unsigned int)*p ++ ) ){

      if( (h >= 0x81 && h <= 0x9f) ||		 /* 漢字 */ 
	  (h >= 0xe0 && h <= 0xfc) ){
	if( *p == '\0' ) break;				/* 2バイト目が範囲外 */
	p ++;						/* でも気にしない    */
	type = 1;
      }else{
	type = 0;
      }
      if( pos == count ){
	return type;
      }
      if( type==1 ){
	count ++;
	if( pos == count ){
	  return 2;
	}
      }
      count ++;
    }
    return 0;

  }else{

    return 0;

  }
}


/*--------------------------------------------------------------
 * エントリ用文字列削除
 *	code … 文字コード
 *	str  … 文字列。このワークを直接書き換える
 *	del_pos … 削除するバイト位置。以降も文字列は詰める。
 *	戻り値 … 削除したバイト数(半角=1、全角=2、削除なし=0 )
 *--------------------------------------------------------------*/
int	q8gr_strdel( int code, char *str, int del_pos )
{
  const unsigned char  *p  = (const unsigned char *)str;
  unsigned int          h, w, s;
  int pos[2] = { 0, 0 };
  int adr[2] = { 0, 0 };

  if( del_pos < 0 ) return 0;

  if( q8gr_strchk( code, str, del_pos ) == 2 ) del_pos --;

  if( code == Q8TK_KANJI_EUC ){

    while( ( h = (unsigned int)*p ++ ) ){

      if( h < 0x80 ){				/* ASCII */
	w = 1;
	s = 1;
      }else if( h == 0x8e ){			/* 半角カナ */
	if( *p == '\0' ) return 0;			/* 0xa0〜0xdf以外 */
	p ++;						/* でも気にしない */
	w = 1;
	s = 2;
      }else{					/* 漢字 */
	if( *p == '\0' ) return 0;			/* 範囲外         */
	p ++;						/* でも気にしない */
	w = 2;
	s = 2;
      }

      pos[1] = pos[0] + w;
      adr[1] = adr[0] + s;
      if( pos[0] == del_pos ){
	memmove( &str[ adr[0] ], &str[ adr[1] ], strlen( &str[ adr[1] ] ) + 1 );
	return w;
      }
      pos[0] = pos[1];
      adr[0] = adr[1];

    }

  }else if( code == Q8TK_KANJI_SJIS ){

    while( ( h = (unsigned int)*p ++ ) ){

      if( (h >= 0x81 && h <= 0x9f) ||		 /* 漢字 */ 
	  (h >= 0xe0 && h <= 0xfc) ){
	if( *p == '\0' ) return 0;			/* 2バイト目が範囲外 */
	p ++;						/* でも気にしない    */
	w = 2;
	s = 2;
      }else{
	w = 1;
	s = 1;
      }

      pos[1] = pos[0] + w;
      adr[1] = adr[0] + s;
      if( pos[0] == del_pos ){
	memmove( &str[ adr[0] ], &str[ adr[1] ], strlen( &str[ adr[1] ] ) + 1 );
	return w;
      }
      pos[0] = pos[1];
      adr[0] = adr[1];

    }

  }else{
    memmove( &str[ del_pos ], &str[ del_pos+1 ], strlen( &str[ del_pos+1 ] ) + 1 );
    return 1;
  }

  return 0;
}



/*--------------------------------------------------------------
 * 文字列を size バイト分、コピー
 *	code … 文字コード
 *	dst  … コピー先文字列
 *	src  … コピー元文字列
 *	size … コピーするサイズ
 *		2バイト文字の途中でサイズに達した場合、
 *		その文字はコピーされない。
 *	※ strncpy 同様、途中で指定サイズに達した場合は末端は \0 にならない
 *	※ strncpy と違い、指定サイズに満たない場合でも \0 で埋めたりはしない
 *--------------------------------------------------------------*/
void	q8gr_strncpy( int code, char *dst, const char *src, int size )
{
  unsigned int h;

  if( size < 0 ) return;

  if( code == Q8TK_KANJI_EUC ){

    while( size ){
      h = (unsigned int)*src ++;

      if( h < 0x80 ){				/* ASCII */
	;
      }else{
	if( size < 2 ){
	  h = '\0';
	}else{
	  if( h == 0x8e ){			/* 半角カナ */
	    if( *src == '\0' ) h = '\0';		/* 0xa0〜0xdf以外 */
							/* でも気にしない */
	  }else{				/* 漢字 */
	    if( *src == '\0' ) h = '\0';		/* 範囲外         */
	    						/* でも気にしない */
	  }
	  *dst ++ = h;
	  h = (unsigned int)*src ++;
	}
      }

      *dst ++ = h;
      size --;
      if( h == 0 ) break;
    }

  }else if( code == Q8TK_KANJI_SJIS ){

    while( size ){
      h = (unsigned int)*src ++;

      if( (h >= 0x81 && h <= 0x9f) ||		 /* 漢字 */ 
	  (h >= 0xe0 && h <= 0xfc) ){
	if( size < 2 ){
	  h = '\0';
	}else{
	  if( *src == '\0' ) h = '\0';			/* 2バイト目が範囲外 */
	  *dst = h;					/* でも気にしない    */
	  h = (unsigned int)*src ++;
	}
      }else{					/* ANK */
	;
      }

      *dst ++ = h;
      size --;
      if( h == 0 ) break;
    }

  }else{
    strncpy( dst, src, size );
  }
}




void	q8gr_draw_mouse( int x, int y )
{
  if( 0 <= menu_mouse_x ){
    menu_screen[menu_screen_current][menu_mouse_y][menu_mouse_x].mouse = FALSE;
  }


  if( 0 <= x  &&  x < Q8GR_SCREEN_X &&
      0 <= y  &&  y < Q8GR_SCREEN_Y ){

    menu_screen[menu_screen_current][y][x].mouse = TRUE;
    menu_mouse_x = x;
    menu_mouse_y = y;

  }else{
    menu_mouse_x = -1;
    menu_mouse_y = -1;
  }

}








/*----------------------------------------------------------------------
 * タイトルロゴ
 *----------------------------------------------------------------------
 *          ｘ
 *   y      ここに表示。サイズは 24文字x3行 の予定
 *
 *----------------------------------------------------------------------*/
void	q8gr_draw_logo( int x, int y )
{
  int i, j;
  int fg = Q8GR_PALETTE_LOGO_FG;
  int bg = Q8GR_PALETTE_LOGO_BG;
  int  c = 0, addr;

  for( j = 0; j < Q8GR_LOGO_H; j ++ ){
    for( i = 0; i < Q8GR_LOGO_W; i ++ ){

      addr = ((c / Q8GR_LOGO_W) * Q8GR_LOGO_W*16 ) + (c % Q8GR_LOGO_W);

      if( !CHECK_MASK_Y(y+j) ){
	if( !CHECK_MASK_X(x+i) ){
	  menu_screen[menu_screen_current][y+j][x+i].background = bg;
	  menu_screen[menu_screen_current][y+j][x+i].foreground = fg;
	  menu_screen[menu_screen_current][y+j][x+i].reverse    = FALSE;
	  menu_screen[menu_screen_current][y+j][x+i].underline  = FALSE;
	  menu_screen[menu_screen_current][y+j][x+i].font_type  = FONT_LOGO;
	  menu_screen[menu_screen_current][y+j][x+i].addr       = addr;
	}
      }

      c++;
    }
  }
}
    
/* ロゴの実体はこれ。 192x48ドット == 24文字x3行 */

byte	q8gr_logo[ Q8GR_LOGO_W * Q8GR_LOGO_H * 16 ] =
{
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x07,0xf0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x1f,0xfc,0x00,0x7f,0xc0,0x0f,0xf8,0x00,0x7f,0xc0,0x00,0x00,0x0f,0xf1,0xff,0x00,0x01,0xf8,0x00,0x00,0x07,0xe0,0x00,
0x00,0x7f,0xff,0x00,0x7f,0xc0,0x0f,0xf8,0x01,0xff,0xf0,0x00,0x00,0x7f,0xf1,0xff,0x00,0x07,0xfe,0x00,0x00,0x1f,0xf8,0x00,
0x00,0xff,0xff,0x80,0x7f,0xc0,0x0f,0xf8,0x07,0xff,0xfc,0x00,0x01,0xff,0xf1,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x01,0xff,0xff,0xc0,0x7f,0xc0,0x0f,0xf8,0x0f,0xff,0xfe,0x00,0x03,0xff,0xf1,0xff,0x00,0x7f,0xff,0xe0,0x01,0xff,0xff,0x80,
0x03,0xff,0xff,0xe0,0x7f,0xc0,0x0f,0xf8,0x1f,0xff,0xff,0x00,0x07,0xff,0xf1,0xff,0x00,0xff,0xff,0xf0,0x03,0xff,0xff,0xc0,
0x07,0xff,0xff,0xf0,0x7f,0xc0,0x0f,0xf8,0x3f,0xff,0xff,0x80,0x0f,0xff,0xf1,0xff,0x00,0xff,0xff,0xf0,0x03,0xff,0xff,0xc0,
0x07,0xff,0xff,0xf0,0x7f,0xc0,0x0f,0xf8,0x3f,0xff,0xff,0x80,0x0f,0xff,0xf1,0xff,0x01,0xff,0xff,0xf8,0x07,0xff,0xff,0xe0,
0x0f,0xff,0xff,0xf8,0x7f,0xc0,0x0f,0xf8,0x7f,0xff,0xff,0xc0,0x1f,0xff,0xf1,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x0f,0xff,0xff,0xf8,0x7f,0xc0,0x0f,0xf8,0x7f,0xff,0xff,0xc0,0x1f,0xff,0xf1,0xff,0x03,0xff,0xff,0xfc,0x0f,0xff,0xff,0xf0,
0x0f,0xfe,0x3f,0xf8,0x7f,0xc0,0x0f,0xf8,0x7f,0xf1,0xff,0xc0,0x1f,0xfe,0x01,0xff,0x03,0xff,0x9f,0xfc,0x0f,0xfe,0x7f,0xf0,
0x1f,0xfc,0x1f,0xfc,0x7f,0xc0,0x0f,0xf8,0xff,0xe0,0xff,0xe0,0x3f,0xf8,0x01,0xff,0x07,0xfe,0x07,0xfe,0x1f,0xf8,0x1f,0xf8,
0x1f,0xfc,0x1f,0xfc,0x7f,0xc0,0x0f,0xf8,0xff,0xc0,0x7f,0xe0,0x3f,0xf0,0x01,0xff,0x07,0xfc,0x03,0xfe,0x1f,0xf0,0x0f,0xf8,
0x1f,0xf8,0x0f,0xfc,0x7f,0xc0,0x0f,0xf8,0xff,0xc0,0x7f,0xe0,0x3f,0xe0,0x01,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x3f,0xf0,0x07,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x80,0x3f,0xf0,0x3f,0xe0,0x01,0xff,0x07,0xfc,0x03,0xfe,0x1f,0xf0,0x0f,0xf8,
0x3f,0xf0,0x07,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x80,0x3f,0xf0,0x3f,0xe0,0x01,0xff,0x07,0xfc,0x03,0xfe,0x1f,0xf0,0x0f,0xf8,
0x3f,0xf0,0x07,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x80,0x3f,0xf0,0x3f,0xe0,0x01,0xff,0x07,0xfe,0x07,0xfe,0x1f,0xf8,0x1f,0xf8,
0x3f,0xe0,0x03,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x00,0x1f,0xf0,0x3f,0xe0,0x01,0xff,0x03,0xff,0x9f,0xfc,0x0f,0xfe,0x7f,0xf0,
0x3f,0xe0,0x03,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x00,0x1f,0xf0,0x3f,0xf0,0x01,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x3f,0xe0,0x03,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x00,0x1f,0xf0,0x3f,0xf0,0x01,0xff,0x03,0xff,0xff,0xfc,0x0f,0xff,0xff,0xf0,
0x3f,0xe0,0x03,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x00,0x1f,0xf0,0x1f,0xf8,0x01,0xff,0x01,0xff,0xff,0xf8,0x07,0xff,0xff,0xe0,
0x3f,0xe0,0x03,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x00,0x1f,0xf0,0x1f,0xfc,0x01,0xff,0x01,0xff,0xff,0xf8,0x07,0xff,0xff,0xe0,
0x3f,0xe0,0x03,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x00,0x1f,0xf0,0x0f,0xfe,0x01,0xff,0x01,0xff,0xff,0xf8,0x07,0xff,0xff,0xe0,
0x3f,0xe0,0x03,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x00,0x1f,0xf0,0x07,0xff,0x01,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x3f,0xe0,0x03,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x3f,0xff,0xf0,0x07,0xff,0x81,0xff,0x03,0xff,0xff,0xfc,0x0f,0xff,0xff,0xf0,
0x3f,0xe0,0x03,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x3f,0xff,0xf0,0x03,0xff,0xc1,0xff,0x07,0xff,0xff,0xfe,0x1f,0xff,0xff,0xf8,
0x3f,0xe0,0x13,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x3f,0xff,0xf0,0x01,0xff,0xc1,0xff,0x07,0xff,0x9f,0xfe,0x1f,0xfe,0x7f,0xf8,
0x3f,0xe0,0x3b,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x3f,0xff,0xf0,0x00,0xff,0xe1,0xff,0x0f,0xfe,0x07,0xff,0x3f,0xf8,0x1f,0xfc,
0x3f,0xe0,0x7f,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x3f,0xff,0xf0,0x00,0x7f,0xe1,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x3f,0xf0,0xff,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x3f,0xff,0xf0,0x00,0x3f,0xf1,0xff,0x0f,0xfc,0x03,0xff,0x3f,0xf0,0x0f,0xfc,
0x3f,0xf1,0xff,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x3f,0xff,0xf0,0x00,0x3f,0xf1,0xff,0x0f,0xf8,0x01,0xff,0x3f,0xe0,0x07,0xfc,
0x3f,0xf1,0xff,0xfe,0x7f,0xc0,0x0f,0xf9,0xff,0x3f,0xff,0xf0,0x00,0x1f,0xf1,0xff,0x0f,0xf8,0x01,0xff,0x3f,0xe0,0x07,0xfc,
0x1f,0xf8,0xff,0xfc,0x7f,0xc0,0x0f,0xf9,0xff,0x3f,0xff,0xf0,0x00,0x1f,0xf1,0xff,0x0f,0xf8,0x01,0xff,0x3f,0xe0,0x07,0xfc,
0x1f,0xfc,0x7f,0xfc,0x7f,0xe0,0x1f,0xf9,0xff,0x00,0x1f,0xf0,0x00,0x3f,0xf1,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x1f,0xfc,0x7f,0xfc,0x7f,0xf8,0x7f,0xf9,0xff,0x00,0x1f,0xf0,0x00,0x7f,0xf1,0xff,0x0f,0xfc,0x03,0xff,0x3f,0xf0,0x0f,0xfc,
0x0f,0xfe,0x3f,0xf8,0x7f,0xff,0xff,0xf9,0xff,0x00,0x1f,0xf0,0x01,0xff,0xf1,0xff,0x0f,0xfe,0x07,0xff,0x3f,0xf8,0x1f,0xfc,
0x0f,0xff,0xff,0xf8,0x7f,0xff,0xff,0xf9,0xff,0x00,0x1f,0xf0,0x0f,0xff,0xf1,0xff,0x0f,0xff,0x9f,0xff,0x3f,0xfe,0x7f,0xfc,
0x0f,0xff,0xff,0xfc,0x3f,0xff,0xff,0xf1,0xff,0x00,0x1f,0xf3,0xff,0xff,0xe1,0xff,0x07,0xff,0xff,0xfe,0x1f,0xff,0xff,0xf8,
0x07,0xff,0xff,0xfe,0x3f,0xff,0xff,0xf1,0xff,0x00,0x1f,0xf3,0xff,0xff,0xe1,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x07,0xff,0xff,0xff,0x1f,0xff,0xff,0xe1,0xff,0x00,0x1f,0xf3,0xff,0xff,0xc1,0xff,0x03,0xff,0xff,0xfc,0x0f,0xff,0xff,0xf0,
0x03,0xff,0xff,0xff,0x1f,0xff,0xff,0xe1,0xff,0x00,0x1f,0xf3,0xff,0xff,0x81,0xff,0x01,0xff,0xff,0xf8,0x07,0xff,0xff,0xe0,
0x01,0xff,0xff,0xfe,0x0f,0xff,0xff,0xc1,0xff,0x00,0x1f,0xf3,0xff,0xff,0x01,0xff,0x00,0xff,0xff,0xf0,0x03,0xff,0xff,0xc0,
0x00,0xff,0xff,0xfc,0x07,0xff,0xff,0x81,0xff,0x00,0x1f,0xf3,0xff,0xfe,0x01,0xff,0x00,0x7f,0xff,0xe0,0x01,0xff,0xff,0x80,
0x00,0x7f,0xff,0x78,0x01,0xff,0xfe,0x01,0xff,0x00,0x1f,0xf3,0xff,0xf8,0x01,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x1f,0xfc,0x30,0x00,0xff,0xfc,0x01,0xff,0x00,0x1f,0xf3,0xff,0xc0,0x01,0xff,0x00,0x0f,0xff,0x00,0x00,0x3f,0xfc,0x00,
0x00,0x07,0xf0,0x00,0x00,0x1f,0xe0,0x01,0xff,0x00,0x1f,0xf3,0xfe,0x00,0x01,0xff,0x00,0x03,0xfc,0x00,0x00,0x0f,0xf0,0x00,
};
