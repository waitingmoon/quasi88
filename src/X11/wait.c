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

/* select, usleep, nanosleep のいずれかのシステムコールを使用するので、
   以下のどれか一つを残して、他はコメントアウトする */

#define USE_SELECT
/* #define USE_USLEEP */
/* #define USE_NANOSLEEP */


#include <stdio.h>

#include <sys/types.h>		/* select                        */
#include <sys/time.h>		/* select           gettimeofday */
#include <unistd.h>		/* select usleep                 */
#include <time.h>		/*        nanosleep clock        */

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
   64bit型(long long)にすれば問題はないんだけども、標準じゃないので保留 */

typedef	long		T_WAIT_TICK;

static	T_WAIT_TICK	next_time;		/* 次フレームの時刻 */
static	T_WAIT_TICK	delta_time;		/* 1 フレームの時間 */

static	T_WAIT_TICK	sleep_min_time = 100;	/* sleep 可能な最小時間 */



/* ---- 指定された時間 (usec単位) sleep する ---- */

INLINE	void	delay_usec( unsigned int usec )
{
#if	defined( USE_SELECT )		/* select を使う */

  struct timeval tv;
  tv.tv_sec  = 0;
  tv.tv_usec = usec;
  select( 0, NULL, NULL, NULL, &tv );

#elif	defined( USE_USLEEP )		/* usleep を使う */

  usleep( usec );

#elif	defined( USE_NANOSLEEP )	/* nanosleep を使う */

  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = usec * 1000;
  nanosleep( &ts, NULL );

#else					/* どれも使えない ! */
  wait_by_sleep = FALSE; /* X_X; */
#endif
}



/* ---- 現在時刻を取得する (usec単位) ---- */
static int tick_error = FALSE;


#ifdef  HAVE_GETTIMEOFDAY		/* gettimeofday() を使う */

static struct timeval start_tv;

INLINE	void		set_tick( void )
{
  if( gettimeofday( &start_tv, 0 ) ){
    if( verbose_wait ) printf( "Clock Error\n" );
    tick_error = TRUE;
    start_tv.tv_sec  = 0;
    start_tv.tv_usec = 0;
  }
}

INLINE	T_WAIT_TICK	get_tick( void )
{
  struct timeval tv;
  if( gettimeofday( &tv, 0 ) ){
    if( verbose_wait ){ if( tick_error==FALSE ) printf( "Clock Error\n" ); }
    tick_error = TRUE;
    tv.tv_sec  = 1;
    tv.tv_usec = 1;
  }

  return ( (T_WAIT_TICK)(tv.tv_sec  - start_tv.tv_sec) * 1000000 +
	   (T_WAIT_TICK)(tv.tv_usec - start_tv.tv_usec) );
}


#else					/* clock() を使う */

/* #define CLOCK_SLICE	CLK_TCK */		/* これじゃ駄目？ */
#define	CLOCK_SLICE	CLOCKS_PER_SEC		/* こっちが正解？ */

INLINE	void		set_tick( void )
{
}

INLINE	T_WAIT_TICK	get_tick( void )
{
  clock_t t = clock();
  if( t == (clock_t)-1 ){
    if( verbose_wait ){ if( tick_error==FALSE ) printf( "Clock Error\n" ); }
    tick_error = TRUE;
    t = CLOCK_SLICE;
  }

  return ( (T_WAIT_TICK) (t / CLOCK_SLICE) * 1000000 +
	   (T_WAIT_TICK)((t % CLOCK_SLICE) * 1000000.0 / CLOCK_SLICE) );
}

#endif





/****************************************************************************
 * ウェイト処理初期化
 *****************************************************************************/
int	wait_vsync_init( void )
{
  if( verbose_proc ){
#ifdef  HAVE_GETTIMEOFDAY
    printf("\nTimer start (gettimeofday(2) - based)\n" );
#else
    printf("\nTimer start (clock(3) - based)\n" );
#endif
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
  set_tick();

  sleep_min_time = wait_sleep_min_us;
  wait_counter = 0;

  delta_time = (T_WAIT_TICK)(1000000.0/( CONST_VSYNC_FREQ * wait_rate / 100 ));
  next_time  = get_tick() + delta_time;

  /* delta_time >= 1000000us (1sec) になると、ちょっとまずい */
}



/****************************************************************************
 * ウェイト処理
 *****************************************************************************/
void	wait_vsync( void )
{
  int	on_time = FALSE;
  T_WAIT_TICK	diff_us;


  diff_us = next_time - get_tick();

  if( tick_error == FALSE ){

    if( diff_us > 0 ){	    /* まだ時間が余っているなら */
			    /* diff_us μミリ秒ウェイト */

      if( wait_by_sleep ){	/* 時間が来るまで sleep する場合 */

	if( diff_us < sleep_min_time ){	/* 残り僅かならビジーウェイト */
	  while( tick_error == FALSE ){
	    if( next_time <= get_tick() )
	      break;
	  }
	}else{				/* 残り多ければディレイ       */
	  delay_usec( diff_us );
	}

      }else{			/* 時間が来るまで Tick を監視する場合 */

	while( tick_error == FALSE ){
	  if( next_time <= get_tick() )
	    break;
	}
      }

      on_time = TRUE;
    }
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
/*if(skip_counter)printf("%x\n",skip_counter);*/
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
#if defined( USE_SELECT ) || defined( USE_USLEEP ) || defined( USE_NANOSLEEP )

  delay_usec( 1000000 / 60 );

#else

  next_time = get_tick() + (T_WAIT_TICK)(1000000/60);

  while( tick_error == FALSE ){
    if( next_time <= get_tick() )
      break;
  }

#endif
}














#ifdef  HAVE_GETTIMEOFDAY
void print_gettimeofday(void)
{
  struct timeval tv;
  if( gettimeofday( &tv, 0 ) == 0 ){
    printf( "%ld.%06ld\n", tv.tv_sec, tv.tv_usec );
  }
}

void print_lapse(int flag)
{
  static struct timeval t0, t1, dt;

  if( t0.tv_sec == 0 || flag < 0 ){
    gettimeofday( &t0, 0 );
  }else{
    gettimeofday( &t1, 0 );

    if( flag > 0 ){
      dt.tv_sec  = t1.tv_sec  - t0.tv_sec;
      dt.tv_usec = t1.tv_usec - t0.tv_usec;
      if( dt.tv_usec < 0 ){
	dt.tv_sec --;
	dt.tv_usec += 1000000; 
      }
      printf( "%ld.%06ld\n", dt.tv_sec, dt.tv_usec );
    }

    t0.tv_sec  = t1.tv_sec;
    t0.tv_usec = t1.tv_usec;
  }
}
#endif
