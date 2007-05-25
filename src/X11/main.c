/************************************************************************/
/*									*/
/*				QUASI88					*/
/*									*/
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>	/* setuid, getuid */

#include "quasi88.h"
#include "initval.h"

#include "file-op.h"
#include "getconf.h"

#include "device.h"
#include "snddrv.h"	/* xmame_system_init(), xmame_system_term() */
#include "suspend.h"
#include "snapshot.h"
#include "menu.h"


static	void	the_end( void );

int	main( int argc, char *argv[] )
{
  int	x = 1;

	/* root権限の必要な処理 (X11関連) を真っ先に行う */

  x11_system_init();  /* ここでエラーが出てもオプション解析するので先に進む */


	/* で、それが終われば、 root 権限を手放す */

  if( setuid( getuid() ) != 0 ){
    fprintf( stderr, "%s : setuid error\n", argv[0] );
    x11_system_term();
    return -1;
  }

  if( getuid() == 0 ) snapshot_cmd_enable = FALSE;


	/* エンディアンネスチェック */

#ifdef LSB_FIRST
  if( *(char *)&x != 1 ){
    fprintf( stderr,
	     "%s CAN'T EXCUTE !\n"
	     "This machine is Big-Endian.\n"
	     "Compile again comment-out 'LSB_FIRST = 1' in Makefile.\n",
	     argv[0] );
    x11_system_term();
    return -1;
  }
#else
  if( *(char *)&x == 1 ){
    fprintf( stderr,
	     "%s CAN'T EXCUTE !\n"
	     "This machine is Little-Endian.\n"
	     "Compile again comment-in 'LSB_FIRST = 1' in Makefile.\n",
	     argv[0] );
    x11_system_term();
    return -1;
  }
#endif


  xmame_system_init();		/* XMAMEサウンド関連初期化 (引数処理の前に!) */

  quasi88_atexit( the_end );


  if( osd_environment() ){		/* 環境設定	*/

    if( config_init( argc, argv ) ){	/* 引数処理	*/

      quasi88();			/* PC-8801 エミュレーション */
    }
  }


  xmame_system_term();		/* XMAMEサウンド関連後始末 */

  x11_system_term();		/* X11関連後始末 */

  return 0;
}



/*
 * 予期せず終了…
 */
static	void	the_end( void )
{
  xmame_system_term();		/* XMAMEサウンド関連後始末 */
  x11_system_term();		/* X11関連後始末 */
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
const char *about_msg( void )
{
  static const char *about_en[] =
  {
#ifdef	MITSHM
    "MIT-SHM ... Supported",
#endif  

#ifdef	USE_DGA
    "X11 DGA ... Supported",
#endif

#if	defined( JOY_NOTHING )

    "JOYSTICK ... Not supported",

#elif	defined( JOY_SDL )

    "JOYSTICK (SDL) ... Supported",

#elif	defined( JOY_LINUX_USB )

    "JOYSTICK (Linux USB-joystick) ... Supported",

#elif	defined( JOY_LINUX_USB )

    "JOYSTICK (BSD USB-joystick) ... Supported",
#endif  
  };


  static const char *about_jp[] =
  {
#ifdef	MITSHM
    "MIT-SHM がサポートされています",
#endif  

#ifdef	USE_DGA
    "X11 DGA がサポートされています",
#endif

#if	defined( JOY_NOTHING )

    "ジョイスティック はサポートされていません",

#elif	defined( JOY_SDL )

    "ジョイスティック (SDL) がサポートされています",

#elif	defined( JOY_LINUX_USB )

    "ジョイスティック (Linux USB-joystick) がサポートされています",

#elif	defined( JOY_LINUX_USB )

    "ジョイスティック (BSD USB-joystick) がサポートされています",
#endif  
  };


  if( about_lang ){
    if( about_line >= COUNTOF(about_jp) ) return NULL;
    else                                  return about_jp[ about_line ++ ];
  }else{
    if( about_line >= COUNTOF(about_en) ) return NULL;
    else                                  return about_en[ about_line ++ ];
  }
}
