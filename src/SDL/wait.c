/************************************************************************/
/*									*/
/* ウエイト調整用関数 (OS依存)						*/
/*									*/
/* 【関数】								*/
/*									*/
/* int  wait_vsync_init( void )		初期化 (起動時に呼び出される)	*/
/* void wait_vsync_term( void )		終了   (終了時に呼び出される)	*/
/*									*/
/* void	wait_vsync_reset( void )	計測リセット (設定変更時)	*/
/* void wait_vsync( void )		ウェイトする (設定間隔経過待ち)	*/
/*									*/
/* void wait_menu( void )		メニュー用ウェイト(1/60sec待つ)	*/
/*									*/
/************************************************************************/
#include <stdio.h>

#include <SDL.h>

#include "quasi88.h"
#include "initval.h"
#include "wait.h"
#include "suspend.h"

#include "screen.h"	/* auto_skip... */



/*
 * 自動フレームスキップ		( by floi, thanks ! )
 */

static	int	skip_counter = 0;		/* 連続何回スキップしたか */
static	int	skip_count_max = 15;		/* これ以上連続スキップしたら
						   一旦、強制的に描画する */


/*
 * ウェイト処理関数群
 */

static	int	wait_counter = 0;		/* 連続何回時間オーバーしたか*/
static	int	wait_count_max = 10;		/* これ以上連続オーバーしたら
						   一旦,時刻調整を初期化する */

/* 時刻調整は、us単位で行なう。でも変数の型が long なので 4295 秒で値が
   戻って(wrap)しまい、この時の 1フレームはタイミングが狂う。
   なので、可能ならば 64bit型(long long)にしてみよう。 */

#ifdef SDL_HAS_64BIT_TYPE
typedef	Sint64		T_WAIT_TICK;
#else
typedef	long		T_WAIT_TICK;
#endif

static	T_WAIT_TICK	next_time;		/* 次フレームの時刻 */
static	T_WAIT_TICK	delta_time;		/* 1 フレームの時間 */



/* ---- 現在時刻を取得する (usec単位) ---- */

#define	GET_TICK()	( (T_WAIT_TICK)SDL_GetTicks() * 1000 )





/****************************************************************************
 * ウェイト処理初期化
 *****************************************************************************/
int	wait_vsync_init( void )
{
  if( ! SDL_WasInit( SDL_INIT_TIMER ) ){
    if( SDL_InitSubSystem( SDL_INIT_TIMER ) != 0 ){
      if( verbose_wait ) printf( "Error Wait (SDL)\n" );
      return FALSE;
    }
  }

  wait_vsync_reset();
  return TRUE;
}



/****************************************************************************
 * ウェイト処理終了
 *****************************************************************************/
void	wait_vsync_term( void )
{
}



/****************************************************************************
 * ウェイト処理再初期化
 *****************************************************************************/
void	wait_vsync_reset( void )
{
  wait_counter = 0;

  delta_time = (T_WAIT_TICK)(1000000.0/( CONST_VSYNC_FREQ * wait_rate / 100 ));
  next_time  = GET_TICK() + delta_time;

  /* delta_time >= 1000000us (1sec) になると、ちょっとまずい */
}



/****************************************************************************
 * ウェイト処理
 *****************************************************************************/
void	wait_vsync( void )
{
  int	on_time = FALSE;
  int	slept   = FALSE;
  T_WAIT_TICK	diff_ms;


  diff_ms = ( next_time - GET_TICK() ) / 1000;

  if( diff_ms > 0 ){	    /* まだ時間が余っているなら */
			    /* diff_ms ミリ秒、ウェイト */

    if( wait_by_sleep ){	/* 時間が来るまで sleep する場合 */

#if 1	/* 方法 1) */
      SDL_Delay( diff_ms );		/* diff_ms ミリ秒、ディレイ   */
      slept = TRUE;

#else	/* 方法 2) */
      if( diff_ms < 10 ){		/* 10ms未満ならビジーウェイト */
	while( GET_TICK() <= next_time )
	  ;
      }else{				/* 10ms以上ならディレイ       */
	SDL_Delay( diff_ms );
	slept = TRUE;
      }
#endif

    }else{			/* 時間が来るまで Tick を監視する場合 */

      while( GET_TICK() <= next_time )
	;
    }

    on_time = TRUE;
  }


  if( slept == FALSE ){		/* 一度も SDL_Delay しなかった場合 */
    SDL_Delay( 1 );				/* for AUDIO thread ?? */
  }


  next_time += delta_time;


  if( on_time ){			/* 時間内に処理できた */
    wait_counter = 0;
  }else{				/* 時間内に処理できていない */
    wait_counter ++;
    if( wait_counter >= wait_count_max ){	/* 遅れがひどい場合は */
      wait_vsync_reset();			/* ウェイトを初期化   */
    }
  }



  /*
   * 自動フレームスキップ処理		( by floi, thanks ! )
   */
  if( use_auto_skip ){
    if( on_time ){			/* 時間内に処理できた */
      skip_counter = 0;
      do_skip_draw = FALSE;
      if( already_skip_draw ){		/* 既に描画をスキップしていたら */
	already_skip_draw = FALSE;
	reset_frame_counter();		/* 次は必ず描画する */
      }
    }else{				/* 時間内に処理できていない */
      skip_counter++;
      if( skip_counter >= skip_count_max ){	/* スキップしすぎ */
	skip_counter = 0;
	do_skip_draw = FALSE;
	already_skip_draw = FALSE;
	reset_frame_counter();			/* 次は必ず描画する */
      }else{
	do_skip_draw = TRUE;			/* 描画をスキップする必要有り*/
      }
    }
  }

  return;
}



/****************************************************************************
 * メニュー用のウェイト
 *	約 1/60 秒ほど待つ。精度は不要だが、可能なら必ず sleep させるべし
 *****************************************************************************/
void	wait_menu( void )
{
  SDL_Delay( 20 );
}
