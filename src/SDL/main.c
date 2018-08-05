/************************************************************************/
/*									*/
/*				QUASI88					*/
/*									*/
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "quasi88.h"
#include "initval.h"

#include "file-op.h"
#include "getconf.h"

#include "device.h"
#include "snddrv.h"	/* xmame_system_init(), xmame_system_term() */
#include "suspend.h"
#include "menu.h"


static	void	the_end( void );

int	main( int argc, char *argv[] )
{
  int	x = 1;

	/* エンディアンネスチェック */

#ifdef LSB_FIRST
  if( *(char *)&x != 1 ){
    fprintf( stderr,
	     "%s CAN'T EXCUTE !\n"
	     "This machine is Big-Endian.\n"
	     "Compile again comment-out 'LSB_FIRST = 1' in Makefile.\n",
	     argv[0] );
    return -1;
  }
#else
  if( *(char *)&x == 1 ){
    fprintf( stderr,
	     "%s CAN'T EXCUTE !\n"
	     "This machine is Little-Endian.\n"
	     "Compile again comment-in 'LSB_FIRST = 1' in Makefile.\n",
	     argv[0] );
    return -1;
  }
#endif


  sdl_system_init();		/* SDL関連の初期化 */

  xmame_system_init();		/* XMAMEサウンド関連初期化 (引数処理の前に!) */

  quasi88_atexit( the_end );


  if( osd_environment() ){		/* 環境設定	*/

    if( config_init( argc, argv ) ){	/* 引数処理	*/

      quasi88();			/* PC-8801 エミュレーション */
    }
  }


  xmame_system_term();		/* XMAMEサウンド関連後始末 */

  sdl_system_term();		/* SDL関連後始末 */

  return 0;
}



/*
 * 予期せず終了…
 */
static	void	the_end( void )
{
  xmame_system_term();		/* XMAMEサウンド関連後始末 */
  sdl_system_term();		/* SDL関連後始末 */
}








/***********************************************************************
 * ステートロード／ステートセーブ
 ************************************************************************/

/*	他の情報すべてがロード or セーブされた後に呼び出される。
 *	必要に応じて、システム固有の情報を付加してもいいかと。
 */

int	stateload_system( void )
{
  return TRUE;
}
int	statesave_system( void )
{
  return TRUE;
}



/***********************************************************************
 * メニュー画面に表示する、システム固有メッセージ
 ************************************************************************/

static int about_lang;
static int about_line;

/*	引数 japanese が真なら、日本語表示を要求
 *	戻値 漢字コード 0〜2、-1なら指定なし
 */
int	about_msg_init( int japanese )
{
  about_lang = ( japanese ) ? 1 : 0;
  about_line = 0;

  return -1;		/* 文字コード指定なし */
}

/*	メッセージの文字列を返す
 *	これ以上メッセージが無い場合は、NULL
 */
const char *about_msg( void ){ return NULL; }
