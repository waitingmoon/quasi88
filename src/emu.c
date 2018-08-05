/************************************************************************/
/*									*/
/* モードに応じて、処理関数を呼び出す。					*/
/*	モードは、実行(EXEC)、メニュー(MENU)、モニター(MONITOR)、	*/
/*	停止(PAUSE) に大きくわかれる。					*/
/*									*/
/************************************************************************/

#include <stdio.h>

#include "quasi88.h"
#include "initval.h"
#include "emu.h"

#include "pc88cpu.h"

#include "screen.h"
#include "keyboard.h"
#include "intr.h"
#include "event.h"
#include "menu.h"
#include "monitor.h"
#include "pause.h"
#include "wait.h"
#include "suspend.h"
#include "status.h"
#include "graph.h"
#include "snddrv.h"




break_t		break_point[2][NR_BP];	/* ブレークポイント		*/
break_drive_t	break_point_fdc[NR_BP];	/* FDC ブレークポイント		*/


int	cpu_timing	= DEFAULT_CPU;		/* SUB-CPU 駆動方式	*/

int	select_main_cpu = TRUE;			/* -cpu 0 実行するCPU	*/
						/* 真なら MAIN CPUを実行*/

int	dual_cpu_count	= 0;			/* -cpu 1 同時処理STEP数*/
int	CPU_1_COUNT	= 4000;			/* その、初期値		*/

int	cpu_slice_us    = 5;			/* -cpu 2 処理時分割(us)*/
						/* 10>でSILPHEEDが動かん*/

int	trace_counter	= 1;			/* TRACE 時のカウンタ	*/


static	int	main_state   = 0;
static	int	sub_state    = 0;
#define	JACKUP	(256)


static	int	emu_mode	= EXEC;		/* エミュレータ処理状態	*/
static	int	emu_mode_execute= GO;
static	int	emu_mode_next	= EXEC;
static	int	emu_rest_step;

void	set_emu_mode( int mode )
{
  if( mode == EXEC ) mode = GO;

  if( mode == GO      ||
      mode == STEP    ||
      mode == TRACE   ||
      mode == TRACE_CHANGE ){

    emu_mode_next    = EXEC;
    emu_mode_execute = mode;

  }else{

    emu_mode_next    = mode;
    emu_rest_step    = 1;

  }

  CPU_BREAKOFF();
}

int	get_emu_mode( void )
{
  return emu_mode;
}

int	next_emu_mode( void )
{
  return emu_mode_next;
}




/***********************************************************************
 * エミュレート処理の制御
 *	実行 / メニュー / モニター / 一時停止の切替え
 *
 ************************************************************************/

static	void	emu_init( void );
static	void	emu_main( void );

void	emu_reset( void )
{
  select_main_cpu = TRUE;
  dual_cpu_count  = 0;

  main_state   = 0;
  sub_state    = 0;
}


void	emu( void )
{
  int	i, j;
	/* ブレークポイントのワーク初期化 (モニターモード用) */
  for( j=0; j<2; j++ )
    for( i=0; i<NR_BP; i++ )
      break_point[j][i].type = BP_NONE;

  for( i=0; i<NR_BP; i++ )
    break_point_fdc[i].type = BP_NONE;



  for( ;; ){

    if( emu_mode != emu_mode_next ){
      emu_mode = emu_mode_next;
      if( emu_mode == EXEC ){
	emu_init();
      }
    }

    wait_vsync_reset();		/* モード切替の最初に、時間計測を初期化 */

    switch( emu_mode ){
    case EXEC:		/* ------------------------------ CPUを実行する	*/
      set_window_title( Q_TITLE " ver " Q_VERSION );
      xmame_sound_resume();
      event_init();
      keyboard_start();
      emu_main();
      break;

    case MONITOR:	/* ----------------------------- モニターモード	*/
#ifdef	USE_MONITOR
      set_window_title( Q_TITLE " (MONITOR)" );
      xmame_sound_suspend();
      event_init();
      keyboard_stop();
      monitor_main();
#else
      set_emu_mode( PAUSE );
#endif
      break;

    case MENU:		/* ----------------------------- メニューモード	*/
      set_window_title( Q_TITLE " ver " Q_VERSION );
      xmame_sound_suspend();
      event_init();
      keyboard_stop();
      menu_main();
      break;

    case PAUSE:		/* --------------------------------- 一時停止中	*/
      set_window_title( Q_TITLE " (PAUSE)" );
      xmame_sound_suspend();
      event_init();
      keyboard_stop();
      pause_main();
      break;

    case QUIT:		/* --------------------------------------- 終了	*/
      return;
    }

  }
}



/***********************************************************************
 * CPU実行処理 (EXEC) の制御
 *	-cpu <n> に応じて、動作を変える。
 *
 *	STEP  時は、1step だけ実行する。
 *	TRACE 時は、指定回数分、1step 実行する。
 *
 *	ブレークポイント指定時は、1step実行の度に PC がブレークポイントに
 *	達したかどうかを確認する。
 *
 ************************************************************************/

#define	INFINITY	(0)
#define	ONLY_1STEP	(1)

/*------------------------------------------------------------------------*/

/*
 * ブレークポイント (タイプ PC) の有無をチェックする
 */

INLINE	int	check_break_point_PC( void )
{
  int	i, j;

  for( i=0; i<NR_BP; i++ ) if( break_point[BP_MAIN][i].type == BP_PC ) break;
  for( j=0; j<NR_BP; j++ ) if( break_point[BP_SUB][j].type  == BP_PC ) break;

  if( i==NR_BP && j==NR_BP ) return FALSE;
  else                       return TRUE;
}

/*------------------------------------------------------------------------*/

/*
 * CPU を 1step 実行して、PCがブレークポイントに達したかチェックする
 *	ブレークポイント(タイプPC)未設定ならこの関数は使わず、z80_emu()を使う
 */

static	int	z80_emu_with_breakpoint( z80arch *z80, int unused )
{
  int i, cpu, states;

  states = z80_emu( z80, 1 );		/* 1step だけ実行 */

  if( z80==&z80main_cpu ) cpu = BP_MAIN;
  else                    cpu = BP_SUB;

  for( i=0; i<NR_BP; i++ ){
    if( break_point[cpu][i].type == BP_PC     &&
	break_point[cpu][i].addr == z80->PC.W ){

      if( i==BP_NUM_FOR_SYSTEM ){
	break_point[cpu][i].type = BP_NONE;
      }

      printf( "*** Break at %04x *** ( %s[#%d] : PC )\n",
	      z80->PC.W, (cpu==BP_MAIN)?"MAIN":"SUB", i+1 );
      if( cpu==BP_MAIN ) z80_debug( &z80main_cpu, "[MAIN CPU]\n" );
      else               z80_debug( &z80sub_cpu,  "[SUB CPU]\n"  );

      set_emu_mode( MONITOR );
    }							
  }							

  return states;
}

/*---------------------------------------------------------------------------*/
static	void	emu_init( void )
{
  status_message( 0, -1, NULL );
  status_message( 1, -1, NULL );
  status_message( 2, -1, NULL );

  draw_screen_force();
}
/*---------------------------------------------------------------------------*/

static	void	emu_main( void )
{
  int	passed_step;		/* 実行した step数 */
  int	target_step;		/* この step数に達するまで実行する */

  int	infinity, only_1step;

  int	wk;
  int	(*z80_exec)( z80arch *, int );


	/* ブレークポイント設定の有無で、呼び出す関数を変える */
  if( check_break_point_PC() ) z80_exec = z80_emu_with_breakpoint;
  else                         z80_exec = z80_emu;


	/* GO/TRACE/STEP/CHANGE に応じて処理の繰り返し回数を決定 */

  passed_step = 0;

  switch( emu_mode_execute ){
  default:
  case GO:
    target_step = 0;			/* 無限に実行 */
    infinity    = INFINITY;
    only_1step  = ONLY_1STEP;
    break;

  case TRACE:
    target_step = trace_counter;	/* 指定ステップ数実行 */
    infinity    = ONLY_1STEP;
    only_1step  = ONLY_1STEP;
    break;

  case STEP:
    target_step = 1;			/* 1ステップ実行 */
    infinity    = ONLY_1STEP;
    only_1step  = ONLY_1STEP;
    break;

  case TRACE_CHANGE:
    target_step = 0;			/* 無限に実行 */
    infinity    = ONLY_1STEP;
    only_1step  = ONLY_1STEP;
    break;
  }


  /* 実行する残りステップ数。
	TRACE / STEP の時は、指定されたステップ数。
	GO / TRACE_CHANGE なら 無限なので、 0。
		なお、途中でメニューに遷移した場合、強制的に 1 がセットされる。
		これにより無限に処理する場合でも、ループを抜けるようになる。 */
  emu_rest_step = target_step;




  switch( emu_mode_execute ){

  /*------------------------------------------------------------------------*/
  case GO:				/* ひたすら実行する           */
  case TRACE:				/* 指定したステップ、実行する */
  case STEP:				/* 1ステップだけ、実行する    */

    switch( cpu_timing ){

    case 0:		/* select_main_cpu で指定されたほうのCPUを無限実行 */
      for(;;){
	if( select_main_cpu ) (z80_exec)( &z80main_cpu, infinity );
	else                  (z80_exec)( &z80sub_cpu,  infinity );
	if( emu_rest_step ){
	  passed_step ++;
	  if( -- emu_rest_step <= 0 ) break;
	}
      }
      break;

    case 1:		/* dual_cpu_count==0 ならメインCPUを無限実行、*/
			/*               !=0 ならメインサブを交互実行 */
      for(;;){
	if( dual_cpu_count==0 ) (z80_exec)( &z80main_cpu, infinity   );
	else{
	  (z80_exec)( &z80main_cpu, only_1step );
	  (z80_exec)( &z80sub_cpu,  only_1step );
	  dual_cpu_count --;
	}
	if( emu_rest_step ){
	  passed_step ++;
	  if( -- emu_rest_step <= 0 ) break;
	}
      }
      break;

    case 2:		/* メインCPU、サブCPUを交互に 5us ずつ実行 */
      for(;;){
	if( main_state < 1*JACKUP  &&  sub_state < 1*JACKUP ){
	  main_state += (cpu_clock_mhz * cpu_slice_us) * JACKUP;
	  sub_state  += (3.9936        * cpu_slice_us) * JACKUP;
	}
	if( main_state >= 1*JACKUP ){
	  wk = (infinity==INFINITY) ? main_state/JACKUP : ONLY_1STEP;
	  main_state -= (z80_exec( &z80main_cpu, wk ) ) * JACKUP;
	}
	if( sub_state >= 1*JACKUP ){
	  wk = (infinity==INFINITY) ? sub_state/JACKUP : ONLY_1STEP;
	  sub_state  -= (z80_exec( &z80sub_cpu, wk ) ) * JACKUP;
	}
	if( emu_rest_step ){
	  passed_step ++;
	  if( -- emu_rest_step <= 0 ) break;
	}
      }
      break;

    }

	/* GO の場合、メニュー遷移などでここに抜けてくる */
	/* その他の場合、指定ステップ数処理した場合も、ここに抜けてくる */

    if( emu_mode_execute != GO ){
      if( passed_step >= target_step ){
				/* 規定ステップ実行完了したら、レジスタ表示 */
	switch( cpu_timing ){
	case 0:
	  if( select_main_cpu ) z80_debug( &z80main_cpu, "[MAIN CPU]\n" );
	  else                  z80_debug( &z80sub_cpu,  "[SUB CPU]\n" );
	  break;

	case 1:
	                       z80_debug( &z80main_cpu, "[MAIN CPU]\n" );
	  if( dual_cpu_count ) z80_debug( &z80sub_cpu,  "[SUB CPU]\n" );
	  break;

	case 2:
	  z80_debug( &z80main_cpu, "[MAIN CPU]\n" );
	  z80_debug( &z80sub_cpu,  "[SUB CPU]\n" );
	  break;
	}

	set_emu_mode( MONITOR );
      }

    }
    break;

  /*------------------------------------------------------------------------*/
    case TRACE_CHANGE:			/* CPUが切り替わるまで処理をする */
      if( cpu_timing >= 1 ){
	printf( "command 'trace change' can use when -cpu 0\n");
	set_emu_mode( MONITOR );
	break;
      }

      wk = select_main_cpu;
      while( wk==select_main_cpu ){
	if( select_main_cpu ) (z80_exec)( &z80main_cpu, infinity );
	else                  (z80_exec)( &z80sub_cpu,  infinity );
	if( emu_rest_step ){
	  passed_step ++;
	  if( -- emu_rest_step <= 0 ) break;
	}
      }
      if( wk != select_main_cpu ){
	if( select_main_cpu ) z80_debug( &z80main_cpu, "[MAIN CPU]\n" );
	else                  z80_debug( &z80sub_cpu,  "[SUB CPU]\n" );
	set_emu_mode( MONITOR );
      }
      break;
  }
}
















/***********************************************************************
 * ステートロード／ステートセーブ
 ************************************************************************/

#define	SID	"EMU "

static	T_SUSPEND_W	suspend_emu_work[] =
{
  { TYPE_INT,	&cpu_timing,		},
  { TYPE_INT,	&select_main_cpu,	},
  { TYPE_INT,	&dual_cpu_count,	},
  { TYPE_INT,	&CPU_1_COUNT,		},
  { TYPE_INT,	&cpu_slice_us,		},
  { TYPE_INT,	&main_state,		},
  { TYPE_INT,	&sub_state,		},

  { TYPE_END,	0			},
};


int	statesave_emu( void )
{
  if( statesave_table( SID, suspend_emu_work ) == STATE_OK ) return TRUE;
  else                                                       return FALSE;
}

int	stateload_emu( void )
{
  if( stateload_table( SID, suspend_emu_work ) == STATE_OK ) return TRUE;
  else                                                       return FALSE;

  /* リード後は必ずエミュに遷移でいいのか ? */
  set_emu_mode( EXEC );
}
