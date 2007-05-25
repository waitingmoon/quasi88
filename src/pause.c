/************************************************************************/
/*									*/
/* 一時停止処理 (OS依存)						*/
/*									*/
/*	変数 pause_by_focus_out により処理が変わる			*/
/*	・pause_by_focus_out == 0 の時					*/
/*		ESCが押されると解除。	画面中央に PAUSEと表示		*/
/*	・pause_by_focus_out != 0 の時					*/
/*		X のマウスが画面内に入ると解除				*/
/*									*/
/* 【関数】								*/
/* void pause_init( void )						*/
/*									*/
/* void pause_main( void )						*/
/*									*/
/************************************************************************/
#include <stdio.h>

#include "quasi88.h"
#include "pause.h"

#include "emu.h"
#include "initval.h"
#include "status.h"
#include "screen.h"
#include "wait.h"
#include "event.h"


int	need_focus = FALSE;			/* フォーカスアウト停止あり */


static	int	pause_by_focus_out = FALSE;

/*
 * エミュ処理中に、フォーカスが無くなった (-focus指定時は、ポーズ開始)
 */
void	pause_event_focus_out_when_exec( void )
{
  if( need_focus ){				/* -focus 指定時は */
    pause_by_focus_out = TRUE;
    set_emu_mode( PAUSE );			/* ここで PAUSE する */
  }
}

/*
 * ポーズ中に、フォーカスを得た
 */
void	pause_event_focus_in_when_pause( void )
{
  if( pause_by_focus_out ){
    set_emu_mode( GO );
  }
}

/*
 * ポーズ中に、ポーズ終了のキー(ESCキー)押下検知した
 */
void	pause_event_key_on_esc( void )
{
  set_emu_mode( GO );
}

/*
 * ポーズ中に、メニュー開始のキー押下検知した
 */
void	pause_event_key_on_menu( void )
{
  set_emu_mode( MENU );
}









static	void	pause_init( void )
{
  status_message( 0, 0, " PAUSE " );
  status_message( 1, 0, "<ESC> key to return" );
  status_message( 2, 0, NULL );
  draw_screen_force();
}


void	pause_main( void )
{
  pause_init();

  while( next_emu_mode() == PAUSE ){

    event_handle();	/* イベントを処理する */
    wait_menu();	/* しばし寝て待つ     */

  }

  pause_by_focus_out = FALSE;
}
