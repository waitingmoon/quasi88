/************************************************************************/
/* QUASI88 --- PC-8801 emulator						*/
/*	Copyright (c) 1998-2006 Showzoh Fukunaga			*/
/*	All rights reserved.						*/
/*									*/
/*	  このソフトは、UNIX + X Window System の環境で動作する、	*/
/*	PC-8801 のエミュレータです。					*/
/*									*/
/*	  このソフトの作成にあたり、Marat Fayzullin氏作の fMSX、	*/
/*	Nicola Salmoria氏 ( MAME/XMAME project) 作の mame/xmame、	*/
/*	ゆみたろ氏作の PC6001V のソースを参考にさせてもらいました。	*/
/*									*/
/*	＊注意＊							*/
/*	  サウンドドライバは、mame/xmame のソースを流用しています。	*/
/*	この部分のソースの著作権は、mame/xmame チームあるいはソースに	*/
/*	記載してある著作者にあります。					*/
/*									*/
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "quasi88.h"
#include "initval.h"

#include "pc88main.h"
#include "pc88sub.h"
#include "graph.h"
#include "memory.h"
#include "file-op.h"

#include "emu.h"
#include "drive.h"
#include "event.h"
#include "keyboard.h"
#include "monitor.h"
#include "snddrv.h"
#include "wait.h"
#include "status.h"
#include "suspend.h"
#include "snapshot.h"
#include "soundbd.h"
#include "screen.h"
#include "menu.h"


int	verbose_level	= DEFAULT_VERBOSE;	/* 冗長レベル		*/
int	verbose_proc    = FALSE;		/* 処理の進行状況の表示	*/
int	verbose_z80	= FALSE;		/* Z80処理エラーを表示	*/
int	verbose_io	= FALSE;		/* 未実装I/Oアクセス表示*/
int	verbose_pio	= FALSE;		/* PIO の不正使用を表示 */
int	verbose_fdc	= FALSE;		/* FDイメージ異常を報告	*/
int	verbose_wait	= FALSE;		/* ウエイト時の異常を報告 */
int	verbose_suspend = FALSE;		/* サスペンド時の異常を報告 */
int	verbose_snd	= FALSE;		/* サウンドのメッセージ	*/

char	file_disk[2][QUASI88_MAX_FILENAME];	/*ディスクイメージファイル名*/
int	image_disk[2];	 	  		/*イメージ番号0〜31,-1は自動*/
int	readonly_disk[2];			/*リードオンリーで開くなら真*/

char	file_tape[2][QUASI88_MAX_FILENAME];	/* テープ入出力のファイル名 */
char	file_prn[QUASI88_MAX_FILENAME];		/* パラレル出力のファイル名 */
char	file_sin[QUASI88_MAX_FILENAME];		/* シリアル出力のファイル名 */
char	file_sout[QUASI88_MAX_FILENAME];	/* シリアル入力のファイル名 */

int	file_coding = 0;			/* ファイル名の漢字コード   */
int	filename_synchronize = TRUE;		/* ファイル名を同調させる   */



static	void	disk_set( void );
static	void	bootup_work_init( void );

/*----------------------------------------------------------------------*/
/* ログを取るための準備／片付け						*/
/*----------------------------------------------------------------------*/

#if defined(PIO_FILE) || defined(FDC_FILE) || defined(MAIN_FILE) || defined(SUB_FILE)
FILE	*LOG = NULL;
#endif

static	void	debug_log_init( void )
{
#if defined(PIO_FILE) || defined(FDC_FILE) || defined(MAIN_FILE) || defined(SUB_FILE)
  LOG = fopen("log","w");
#endif
  if( verbose_proc ){
#if	defined( PIO_DISP ) || defined( PIO_FILE )
    printf("+ Support PIO logging. set variable \"pio_debug\" to 1.\n");
#endif
#if	defined( FDC_DISP ) || defined( FDC_FILE )
    printf("+ Support FDC logging. set variable \"fdc_debug\" to 1.\n");
#endif
#if	defined( MAIN_DISP ) || defined( MAIN_FILE )
    printf("+ Support Main Z80 logging. set variable \"main_debug\" to 1.\n");
#endif
#if	defined( SUB_DISP ) || defined( SUB_FILE )
    printf("+ Support Sub Z80 logging. set variable \"sub_debug\" to 1.\n");
#endif
  }
}

static	void	debug_log_finish( void )
{
#if defined(PIO_FILE) || defined(FDC_FILE) || defined(MAIN_FILE) || defined(SUB_FILE)
  if( LOG ) fclose(LOG);
#endif
}



/***********************************************************************
 *
 *			QUASI88 メイン関数
 *
 ************************************************************************/
static	int	proc;
int	quasi88( void )
{
  verbose_proc	= verbose_level & 0x01;
  verbose_z80	= verbose_level & 0x02;
  verbose_io	= verbose_level & 0x04;
  verbose_pio	= verbose_level & 0x08;
  verbose_fdc	= verbose_level & 0x10;
  verbose_wait	= verbose_level & 0x20;
  verbose_suspend=verbose_level & 0x40;
  verbose_snd	= verbose_level & 0x80;

  if( verbose_proc ) printf("\n"); fflush(NULL);
  proc = 0;


  stateload_init();			/* ステートロード関連初期化         */
  screen_snapshot_init();		/* 画面スナップショット関連初期化   */

  status_init();			/* ステータス表示のワーク初期化     */
  drive_init();				/* フロッピードライブのワーク初期化 */

  event_handle_init();


  if( memory_allocate() ){		/* エミュレート用メモリの確保	*/
    if( verbose_proc ) printf("\n"); fflush(NULL);
    proc = 1;

    set_signal();			/* INTシグナルの処理を設定	*/

    if( resume_flag ){			/* ステートロード		*/
      if( stateload() == FALSE ){
	fprintf( stderr, "stateload: Failed ! (filename = %s)\n", file_state );
	quasi88_exit();
      }
      if( verbose_proc ) printf("stateload...OK\n\n"); fflush(NULL);
    }

    if( graphic_system_init() ){	/* グラフィックシステム初期化	*/
      if( verbose_proc ) printf("\n"); fflush(NULL);
      proc = 2;

      screen_buf_init();

      if( xmame_sound_start() ){	/* サウンドドライバ初期化	*/
	if( verbose_proc ) printf("\n"); fflush(NULL);
	proc = 3;

	disk_set();
	bootup_work_init();

	if( file_tape[CLOAD][0] ) sio_open_tapeload( file_tape[CLOAD] );
	if( file_tape[CSAVE][0] ) sio_open_tapesave( file_tape[CSAVE] );
	if( file_sin[0] )         sio_open_serialin( file_sin );
	if( file_sout[0] )        sio_open_serialout( file_sout );
	if( file_prn[0] )         printer_open( file_prn );
					/* ステートロード時は 各々SEEK ? */

	key_record_playback_init();

	pc88main_init( (resume_flag) ? INIT_STATELOAD : INIT_POWERON );
	pc88sub_init(  (resume_flag) ? INIT_STATELOAD : INIT_POWERON );


	if( wait_vsync_init() ){	/* ウエイト用タイマー初期化	*/
	  if( verbose_proc ) printf("\n"); fflush(NULL);
	  proc = 4;

	  if( verbose_proc ) printf( "Running QUASI88...\n" );

	  if( resume_flag == 0 ) indicate_bootup_logo();
	  else                   indicate_stateload_logo();

	  debug_log_init();
	  {
	    emu();			/* エミュレート メイン */
	  }
	  debug_log_finish();

	  if( verbose_proc ) printf( "Shutting down.....\n" );
	  wait_vsync_term();
	}

	pc88main_term();
	pc88sub_term();

	disk_eject( 0 );
	disk_eject( 1 );

	sio_close_tapeload();
	sio_close_tapesave();
	sio_close_serialin();
	sio_close_serialout();
	printer_close();

	key_record_playback_term();

	xmame_sound_stop();
      }
      graphic_system_term();
    }
    memory_free();
  }



	/* 詳細表示をしないかった場合の、エラー表示 */

  if( !verbose_proc ){
    switch( proc ){
    case 0: printf( "memory allocate failed!\n" );		break;
    case 1: printf( "graphic system initialize failed!\n" );	break;
    case 2: printf( "sound system initialize failed!\n" );	break;
    case 3: printf( "timer initialize failed!\n" );		break;
    }
  }

  return 0;
}




/***********************************************************************
 * QUASI88 途中終了処理関数
 *	exit() の代わりに呼ぼう。
 ************************************************************************/
static	void (*exit_function)(void) = NULL;

void	quasi88_atexit( void (*function)(void) )
{
  exit_function = function;
}

void	quasi88_exit( void )
{
  switch( proc ){
  case 4:
    debug_log_finish();
    wait_vsync_term();			/* FALLTHROUGH */
  case 3:
    pc88main_term();
    pc88sub_term();
    disk_eject( 0 );
    disk_eject( 1 );
    sio_close_tapeload();
    sio_close_tapesave();
    sio_close_serialin();
    sio_close_serialout();
    printer_close();
    key_record_playback_term();
    xmame_sound_stop();			/* FALLTHROUGH */
  case 2:
    graphic_system_term();
  case 1:
    memory_free();			/* FALLTHROUGH */
  }

  if( exit_function ){
    (*exit_function)();
  }

  exit( -1 );
}





/***********************************************************************
 * QUASI88 起動中のリセット処理関数
 *
 ************************************************************************/
void	quasi88_reset( void )
{
  int	empty[2];

  pc88main_term();
  pc88sub_term();

  bootup_work_init();

  pc88main_init( INIT_RESET );
  pc88sub_init(  INIT_RESET );

  empty[0] = drive_check_empty(0);
  empty[1] = drive_check_empty(1);
  drive_reset();
  if( empty[0] ) drive_set_empty(0);
  if( empty[1] ) drive_set_empty(1);
  if( use_sound ) xmame_sound_reset();

  emu_reset();
}




/***********************************************************************
 * QUASI88 起動中のステートロード処理関数
 *
 ************************************************************************/
int	quasi88_stateload( void )
{
  int now_board, success;

  sio_close_tapeload();			/* イメージファイルを全て閉じる */
  sio_close_tapesave();
  sio_close_serialin();
  sio_close_serialout();
  printer_close();

  disk_eject( 0 );
  disk_eject( 1 );
  quasi88_reset();


  now_board = sound_board;

  success = stateload();		/* ステートロード実行 */

  if( now_board != sound_board ){ 	/* サウンドボードが変わったら */
    xmame_sound_resume();		/* 中断したサウンドを復帰後に */
    xmame_sound_stop();			/* サウンドを停止させる。     */
    xmame_sound_start();		/* そして、サウンド再初期化   */
  }


  if( success ){			/* ステートロード成功したら・・・ */

    disk_set();					/* イメージファイルを全て開く*/
    bootup_work_init();

    if( file_tape[CLOAD][0] ) sio_open_tapeload( file_tape[CLOAD] );
    if( file_tape[CSAVE][0] ) sio_open_tapesave( file_tape[CSAVE] );
    if( file_sin[0] )         sio_open_serialin( file_sin );
    if( file_sout[0] )        sio_open_serialout( file_sout );
    if( file_prn[0] )         printer_open( file_prn );

    pc88main_init( INIT_STATELOAD );		/* 各種ワーク初期化 */
    pc88sub_init(  INIT_STATELOAD );

  }else{				/* ステートロード失敗したら・・・ */

    quasi88_reset();				/* とりあえずリセット */
  }

  return success;
}








/*----------------------------------------------------------------------
 * file_disk[][] に設定されているディスクイメージをセットする
 *	( 起動時、およびステートロード時 )
 *----------------------------------------------------------------------*/
static	void	disk_set( void )
{
  int err0 = TRUE;
  int err1 = TRUE;

  if( file_disk[0][0] &&	/* ドライブ1,2 ともイメージ指定済みの場合 */
      file_disk[1][0] ){		/*	% quasi88 file file       */
					/*	% quasi88 file m m        */
					/*	% quasi88 file n file     */
					/*	% quasi88 file file m     */
					/*	% quasi88 file n file m   */
    int same = (strcmp( file_disk[0], file_disk[1] )==0) ? TRUE : FALSE;

    err0 = disk_insert( DRIVE_1,		/* ドライブ 1 をセット */
			file_disk[0],
			(image_disk[0]<0) ? 0 : image_disk[0],
			readonly_disk[0] );

    if( same ){					/* 同一ファイルの場合は */

      if( err0 == FALSE ){				/* 1: → 2: 転送 */
	err1 = disk_insert_A_to_B( DRIVE_1, DRIVE_2, 
				   (image_disk[1]<0) ? 0 : image_disk[1] );
      }

    }else{					/* 別ファイルの場合は */

      err1 = disk_insert( DRIVE_2,			/* ドライブ2 セット */
			  file_disk[1],
			  (image_disk[1]<0) ? 0 : image_disk[1],
			  readonly_disk[1] );
    }

    /* 両ドライブで同じファイル かつ イメージ指定自動の場合の処理 */
    if( err0 == FALSE && err1 == FALSE &&
	drive[DRIVE_1].fp == drive[DRIVE_2].fp  && 
	image_disk[0] < 0 && image_disk[1] < 0 ){
      disk_change_image( DRIVE_2, 1 );			/* 2: は イメージ2へ */
    }

  }else if( file_disk[0][0] ){	/* ドライブ1 だけ イメージ指定済みの場合 */
					/*	% quasi88 file		 */
					/*	% quasi88 file num       */
    err0 = disk_insert( DRIVE_1,
			file_disk[0],
			(image_disk[0]<0) ? 0 : image_disk[0],
			readonly_disk[0] );

    if( err0 == FALSE ){
      if( image_disk[0] < 0 &&			/* イメージ番号指定なしなら */
	  disk_image_num( DRIVE_1 ) >= 2 ){	/* ドライブ2にもセット      */

	err1 = disk_insert_A_to_B( DRIVE_1, DRIVE_2, 1 );
	if( err1 == FALSE ){
	  memcpy( file_disk[1], file_disk[0], QUASI88_MAX_FILENAME );
	}
      }
    }

  }else if( file_disk[1][0] ){	/* ドライブ2 だけ イメージ指定済みの場合 */
					/*	% quasi88 noexist file	 */
    err1 = disk_insert( DRIVE_2,
			file_disk[1],
			(image_disk[1]<0) ? 0 : image_disk[1],
			readonly_disk[1] );
  }



  /* オープンしなかった(出来なかった)場合は、ファイル名をクリア */
  if( err0 ) memset( file_disk[ 0 ], 0, QUASI88_MAX_FILENAME );
  if( err1 ) memset( file_disk[ 1 ], 0, QUASI88_MAX_FILENAME );


  /* ファイル名にあわせて、スナップショットファイル名も設定 */
  if( filename_synchronize ){
    if( err0 == FALSE || err1 == FALSE ){
      set_state_filename( FALSE );
      set_snapshot_filename( FALSE );
    }
  }

  if( verbose_proc ){
    int i;
    for( i=0; i<2; i++ ){
      if( disk_image_exist(i) ){
	printf("DRIVE %d: <= %s [%d]\n", i+1,
	     /*drive[i].filename, disk_image_selected(i)+1 );*/
	       file_disk[i],      disk_image_selected(i)+1 );
      }else{
	printf("DRIVE %d: <= (empty)\n", i+1 );
      }
    }
  }
}




/*----------------------------------------------------------------------
 * 各種変数初期化 (引数やPC8801のバージョンによって、変わるもの)
 *
 *----------------------------------------------------------------------*/
static	void	bootup_work_init( void )
{

	/* V1モードのバージョンの小数点以下を強制変更する */

  if( set_version ) ROM_VERSION = set_version;



	/* 起動デバイス(ROM/DISK)未定の時 */

  if( boot_from_rom==BOOT_AUTO ){
    if( disk_image_exist(0) ) boot_from_rom = FALSE; /* ディスク挿入時はDISK */
    else                      boot_from_rom = TRUE;  /* それ以外は、    ROM  */
  }


	/* 起動時の BASICモード未定の時	  */

  if( boot_basic==BASIC_AUTO ){			
    if( ROM_VERSION >= '4' )			/* SR 以降は、V2	  */
      boot_basic = BASIC_V2;
    else					/* それ以前は、V1S	  */
      boot_basic = BASIC_V1S;
  }


	/* サウンド(I/II)のポートを設定	 */

  if( sound_board == SOUND_II ){

    if     ( ROM_VERSION >= '8' )		/* FH/MH 以降は、44〜47H */
      sound_port = SD_PORT_44_45 | SD_PORT_46_47;
    else if( ROM_VERSION >= '4' )		/* SR 以降は、44〜45,A8〜ADH */
      sound_port = SD_PORT_44_45 | SD_PORT_A8_AD;
    else					/* それ以前は、  A8〜ADH */
      sound_port = SD_PORT_A8_AD;

  }else{

    if( ROM_VERSION >= '4' )			/* SR以降は、44〜45H	 */
      sound_port = SD_PORT_44_45;
    else					/* それ以前は、？？？	 */
      sound_port = SD_PORT_A8_AD;		/*	対応しないなら 0 */
  }

}





/***********************************************************************
 *	雑多な関数
 ************************************************************************/
#include <string.h>

/*===========================================================================
 * 大文字・小文字の区別なく、文字列比較 (stricmp/strcasecmp ?)
 *	戻り値: 一致時 == 0, 不一致時 != 0 (大小比較はなし)
 *===========================================================================*/
int	my_strcmp( const char *s, const char *d )
{
  if( s==NULL || d==NULL ) return 1;

  while( tolower(*s) == tolower(*d) ){
    if( *s == '\0' ) return 0;
    s++;
    d++;
  }
  return 1;
}

/*===========================================================================
 * 文字列 ct を 文字列 s に コピー (strlcpy ?)
 *	s の文字列終端は、必ず '\0' となり、s の長さは n-1 文字以下に収まる。
 *	余分な領域は \0 で埋められない。
 *	戻り値: なし
 *===========================================================================*/
void	my_strncpy( char *s, const char *ct, unsigned long n )
{
  s[0] = '\0';
  strncat( s, ct, n-1 );
}

/*===========================================================================
 * 文字列 ct を 文字列 s に 連結 (strlcat ?)
 *	s の文字列終端は、必ず '\0' となり、s の長さは n-1 文字以下に収まる。
 *	戻り値: なし
 *===========================================================================*/
void	my_strncat( char *s, const char *ct, unsigned long n )
{
  size_t used = strlen(s) + 1;

  if( n > used )
    strncat( s, ct, n - used );
}

/*===========================================================================
 * SJIS を EUC に変換 (かなり適当)
 *	*sjis_p の文字列を EUC に変換して、*euc_p に格納する。
 *
 *	注意！）この関数は、バッファあふれをチェックしていない。
 *		*euc_p は、*sjis_p の倍以上の長さがないと危険
 *===========================================================================*/
void	sjis2euc( char *euc_p, const char *sjis_p )
{
  int	h,l, h2, l2;

  while( ( h = (unsigned char)*sjis_p++ ) ){

    if( h < 0x80 ){				/* ASCII */

      *euc_p ++ = h;

    }else if( 0xa1 <= h && h <= 0xdf ){		/* 半角カナ */

      *euc_p ++ = (char)0x8e;
      *euc_p ++ = h;

    }else{					/* 全角文字 */

      if( ( l = (unsigned char)*sjis_p++ ) ){

	if( l <= 0x9e ){
	  if( h <= 0x9f ) h2 = (h - 0x71) *2 +1;
	  else            h2 = (h - 0xb1) *2 +1;
	  if( l >= 0x80 ) l2 = l - 0x1f -1;
	  else            l2 = l - 0x1f;
	}else{
	  if( h <= 0x9f ) h2 = (h - 0x70) *2;
	  else            h2 = (h - 0xb0) *2;
	  l2 = l - 0x7e;
	}
	*euc_p++ = 0x80 | h2;
	*euc_p++ = 0x80 | l2;

      }else{
	break;
      }

    }
  }

  *euc_p = '\0';
}


/*===========================================================================
 * EUC を SJIS に変換 (かなり適当)
 *	*euc_p の文字列を SJIS に変換して、*sjis_p に格納する。
 *
 *	注意！）この関数は、バッファあふれをチェックしていない。
 *		*sjis_p は、*euc_p と同等以上の長さがないと危険
 *===========================================================================*/

void	euc2sjis( char *sjis_p, const char *euc_p )
{
  int	h,l;

  while( ( h = (unsigned char)*euc_p++ ) ){

    if( h < 0x80 ){				/* ASCII */

      *sjis_p ++ = h;

    }else if( h==0x8e ){			/* 半角カナ */

      if( ( h = (unsigned char)*euc_p++ ) ){

	if( 0xa1 <= h && h <= 0xdf )
	  *sjis_p ++ = h;

      }else{
	break;
      }

    }else if( h & 0x80 ){			/* 全角文字 */

      if( ( l = (unsigned char)*euc_p++ ) ){

	if( l & 0x80 ){

	  h = (h & 0x7f) - 0x21;
	  l = (l & 0x7f) - 0x21;

	  if( h & 0x01 ) l += 0x9e;
	  else           l += 0x40;
	  if( l >= 0x7f ) l += 1;

	  h = (h>>1) + 0x81;

	  if( h >= 0xa0 ) h += 0x40;

	  *sjis_p++ = h;
	  *sjis_p++ = l;

	}

      }else{
	break;
      }

    }
  }

  *sjis_p = '\0';
}


/*===========================================================================
 * EUC文字列の長さを計算 (けっこう適当)
 *	ASCII・半角カナは1文字、全角漢字は2文字とする。
 *	文字列末の、\0 は長さに含めない。
 *===========================================================================*/

int	euclen( const char *euc_p )
{
  int	i = 0, h;

  while( ( h = (unsigned char)*euc_p++ ) ){

    if( h < 0x80 ){				/* ASCII */

      i++;

    }else if( h == 0x8e ){			/* 半角カナ */

      euc_p ++;
      i++;

    }else{					/* 漢字 */

      euc_p ++;
      i += 2;

    }
  }

  return i;
}

/*===========================================================================
 * 文字列 src をトークンに分割する。
 *	区切り文字は、スペースとタブ。先頭のスペース・タブは無視される。
 *	分割したトークンは ( \0 を付加して ) dst にコピーし、
 *	src の残りの文字列部分の先頭アドレスを返す。
 *	これ以上分割できない場合は dst には "\0" をコピーし、NULLを返す。
 *
 *	トークンに分割する際のルール
 *	  ・ 改行(\r or \n) ないし 終端(\0) で文字列は終わりとみなす。
 *	特別な文字
 *	  ・ スペースとタブは、区切り文字とする
 *	  ・ # はコメント文字とし、終端文字と同様に扱う。
 *	  ・ \ はエスケープ文字とする。スペース、タブ、#、"、\ の前に \ が
 *	     ある場合、これらは特別な文字とせずに通常の文字と同様に扱う。
 *	     他の文字の前に \ がある場合、単に \ はスキップされる。
 *	  ・ " は引用符文字とする。この文字で囲まれた部分の文字列について、
 *	     スペース、タブ、#、\ は特別な文字とみなされない。
 *	     ただし、 "" の2文字が続いている場合に限り、" とみなす。
 *===========================================================================*/
#define	COMMENT		'#'
#define	ESCAPE		'\\'
#define	QUOTE		'\"'
char	*my_strtok( char *dst, char *src )
{
  char *p = &src[0];
  char *q = &dst[0];

  int esc   = FALSE;			/* エスケープシーケンス処理中 */
  int quote = FALSE;			/* クォート文字処理中         */

  *q = '\0';

  while( *p==' ' || *p=='\t' ){		/* 先頭のスペース・タブをスキップ */
    p ++;
  }

  while(1){

    if( quote == FALSE ){		/* 通常部分の処理 */

      if( esc == FALSE ){
	if     ( *p=='\0' ||
		 *p=='\r' ||
		 *p=='\n' ||
		 *p==' '  ||
		 *p=='\t' ||
		 *p==COMMENT ){ *q = '\0';   break; }
	else if( *p==QUOTE   ){         p++; quote = TRUE; }
	else if( *p==ESCAPE  ){         p++; esc = TRUE;   }
	else                  { *q++ = *p++;               }
      }else{
	if     ( *p=='\0' ||
		 *p=='\r' ||
		 *p=='\n' )   { *q = '\0'; break; }
	else                  { *q++ = *p++; esc = FALSE;  }
      }

    }else{				/* " " で囲まれた部分の処理 */

	if     ( *p=='\0' ||
		 *p=='\r' ||
		 *p=='\n'   ){ *q = '\0';   break; }
	else if( *p==QUOTE  )
	  if( *(p+1)==QUOTE ){ *q++ = QUOTE; p+=2; quote = FALSE; }
	  else               {               p++;  quote = FALSE; }
	else                 { *q++ = *p++;                }

    }
  }

  if( *dst == '\0' ) return NULL;
  else               return p;
}





/***********************************************************************
 * 各種動作パラメータの変更
 *	これらの関数は、ショートカットキー処理や、機種依存部のイベント
 *	処理などから呼び出されることを *一応* 想定している。
 *
 *	メニュー画面の表示中に呼び出すと、メニュー表示と食い違いが生じる
 *	ので、メニュー中は呼び出さないように。エミュ実行中に呼び出すのが
 *	一番安全。うーん、いまいち。
 *
 *	if( get_emu_mode() == EXEC ){
 *	    quasi88_disk_insert_and_reset( file, FALSE );
 *	}
 *
 ************************************************************************/

#include "intr.h"
#include "q8tk.h"

/*======================================================================
 * 画面表示切り替え			MENU/MONITOR でも可
 *======================================================================*/
void	quasi88_change_screen( void )
{
  if( graphic_system_restart() ){
    screen_buf_init();
  }

  if( get_emu_mode() == MENU ) q8tk_misc_redraw();
  else                         draw_screen_force();
}

/*======================================================================
 * 画面スナップショット保存
 *======================================================================*/
void	quasi88_snapshot( void )
{
  if( get_emu_mode() == EXEC ){
    if( save_screen_snapshot() ){
      status_message( 1, 60, "Snapshot saved" );
    }else{
      status_message( 1, 60, "Snapshot failed!" );
    }
  }
}

/*======================================================================
 * ステータス表示切り替え		MENU/MONITOR でも可
 *======================================================================*/
void	quasi88_status( void )
{
  int result;

  show_status = ( now_status ) ? FALSE : TRUE;	/* 表示有無状態を反転 */

  result = set_status_window();			/* ステータス(非)表示 */

  if( result ){					/* その結果・・・ */

    status_reset( now_status );	/* ステータスのワークを再初期化 */

    status_buf_init();		/* 全画面時ステータス無しなら、消す必要あり */

    if( get_emu_mode() == MENU ){		/* 異様に醜い処理だ・・・ */
      if( result > 1 ) q8tk_misc_redraw();
      else             draw_status();
    }else{
      if( result > 1 ) draw_screen_force();
    }
  }
}

/*======================================================================
 * メニュー・ポーズ
 *======================================================================*/
void	quasi88_menu( void )
{
  if( get_emu_mode() != MENU ){
    set_emu_mode( MENU );
  }
}

void	quasi88_pause( void )
{
  if( get_emu_mode() == EXEC ){
    set_emu_mode( PAUSE );
  }
}




/*======================================================================
 * フレームスキップ数変更 : -frameskip の値を、変更する。
 *		変更した後は、しばらく画面にフレームレートを表示させる
 *======================================================================*/
static void change_framerate( int sign )
{
  int	i;
  char	str[32];

  static const char next[] = { 1, 2, 3, 4, 5, 6, 10, 12, 15, 20, 30, 60, };
  static const char prev[] = { 60, 30, 20, 15, 12, 10, 6, 5, 4, 3, 2, 1, };

  if( sign < 0 ){

    for( i=0; i<COUNTOF(next)-1; i++ )
      if( frameskip_rate <= next[i] ){ frameskip_rate = next[i+1];  break; }
    if( i==COUNTOF(next)-1 ) frameskip_rate = next[0];	/* ループさせよう */

  }else if( sign > 0 ){

    for( i=0; i<COUNTOF(prev)-1; i++ )
      if( frameskip_rate >= prev[i] ){ frameskip_rate = prev[i+1]; break; }
    if( i==COUNTOF(prev)-1 ) frameskip_rate = prev[0];	/* ループさせよう */

  }

  blink_ctrl_update();
  reset_frame_counter();

  sprintf( str, "FRAME RATE = %2d/sec", 60/frameskip_rate );
  status_message( 1, 60, str );					/* 1 sec */
}
void	quasi88_framerate_up( void )  { change_framerate( +1 ); }
void	quasi88_framerate_down( void ){ change_framerate( -1 ); }


/*======================================================================
 * ボリューム変更 : -vol の値を、変更する。
 *		変更した後は、しばらく画面に音量を表示させる
 *======================================================================*/
static void change_volume( int sign )
{
  char	str[32];

#ifdef USE_SOUND
  if( use_sound ){
    int diff = (sign>0) ? +1 : ( (sign<0)?-1:0);
    if (diff){
      int vol = xmame_get_sound_volume() + diff;
      if( vol >   0 ) vol = 0;
      if( vol < -32 ) vol = -32;
      xmame_set_sound_volume( vol );
    }
    
    sprintf( str, "VOLUME  %3d[db]", xmame_get_sound_volume() );
  }else
#endif
  {
    sprintf( str, "Sound not available !" );
  }
  status_message( 1, 60, str );					/* 1 sec */
}
void	quasi88_volume_up( void )  { change_volume( -1 ); }
void	quasi88_volume_down( void ){ change_volume( +1 ); }


/*======================================================================
 * ウェイト量変更 : -nowait, -speed の値を、変更する。
 *		変更した後は、速度比を表示させる。が、
 *		表示時間は速度比依存なので、結局表示時間は不定になる。
 *======================================================================*/
static void change_wait( int sign )
{
  int	time = 60;
  char	str[32];

  if( sign==0 ){

    no_wait ^= 1;
    if( no_wait ){sprintf( str, "WAIT  OFF" ); time *= 10; }
    else          sprintf( str, "WAIT  ON" );

  }else{

    if( sign < 0 ){
      wait_rate -= 10;
      if( wait_rate < 10 ) wait_rate = 10;
    }else{
      wait_rate += 10;
      if( wait_rate > 200 ) wait_rate = 200;
    }

    sprintf( str, "WAIT  %4d[%%]", wait_rate );

    wait_vsync_reset();
  }

  status_message( 1, time, str );				/* 1 sec */
}
void	quasi88_wait_up( void )  { change_wait( +1 ); }
void	quasi88_wait_down( void ){ change_wait( -1 ); }
void	quasi88_wait_none( void ){ change_wait(  0 ); }



/*======================================================================
 * 速度変更 (speed/clock/boost)
 *
 *======================================================================*/
void	quasi88_max_speed( int new_speed )
{
  char	str[32];

  if( !( 5<=new_speed || new_speed<=5000 ) ) new_speed = 1600;

  if( wait_rate < new_speed ) wait_rate = new_speed;
  else                        wait_rate = 100;
  wait_vsync_reset();
  no_wait = 0;

  sprintf( str, "WAIT  %4d[%%]", wait_rate );
  status_message( 1, 60, str );					/* 1 sec */
}
void	quasi88_max_clock( double new_clock )
{
  double def_clock = (boot_clock_4mhz ? CONST_4MHZ_CLOCK : CONST_8MHZ_CLOCK );
  char	str[32];

  if( !( 0.1<=new_clock && new_clock<1000.0) ) new_clock = CONST_4MHZ_CLOCK*16;

  if( cpu_clock_mhz < new_clock ) cpu_clock_mhz = new_clock;
  else                            cpu_clock_mhz = def_clock;
  interval_work_init_all();

  sprintf( str, "CLOCK %8.4f[MHz]", cpu_clock_mhz );
  status_message( 1, 60, str );					/* 1 sec */
}
void	quasi88_max_boost( int new_boost )
{
  char	str[32];

  if( !( 1<=new_boost || new_boost<=100 ) ) new_boost = 16;

  if( boost < new_boost ) boost_change( new_boost );
  else                    boost_change( 1 );
  

  sprintf( str, "BOOST [x%2d]", boost );
  status_message( 1, 60, str );					/* 1 sec */
}



/*======================================================================
 * ドライブを一時的に空の状態にする
 *		画面にメッセージを常時表示しておく
 *======================================================================*/
static void change_image_empty( int drv )
{
  char	str[48];

  if( disk_image_exist( drv ) ){

    drive_set_empty( drv );
    sprintf( str, "DRIVE %d:  <<<< Eject >>>>         ", drv+1 );
  }else{
    sprintf( str, "DRIVE %d:   --  No Disk  --        ", drv+1 );
  }

  status_message( 1, 0, str );				/* ∞ sec */
}
void	quasi88_drv1_image_empty( void ){ change_image_empty( 0 ); }
void	quasi88_drv2_image_empty( void ){ change_image_empty( 1 ); }

/*======================================================================
 * ドライブのイメージを次(前)のイメージに変更する
 *		変更した後は、しばらく画面にイメージ名を表示させる
 *======================================================================*/
static void change_image_change( int drv, int direction )
{
  char	str[48];
  int	img;

  if( disk_image_exist( drv ) ){

    img = disk_image_selected(drv);
    img += direction;
    if( img < 0 ) img = disk_image_num(drv)-1;
    if( img >= disk_image_num(drv) ) img = 0;

    drive_unset_empty( drv );
    disk_change_image( drv, img );


    sprintf( str, "DRIVE %d:  %-16s   %s  ",
	     drv+1,
	     drive[drv].image[ disk_image_selected(drv) ].name,
	     (drive[drv].image[ disk_image_selected(drv) ].protect)
							? "(p)" : "   " );
  }else{
    sprintf( str, "DRIVE %d:   --  No Disk  --        ", drv+1 );
  }

  status_message( 1, 60+60+30, str );			/* 2.5 sec */
}
void	quasi88_drv1_image_next( void ){ change_image_change( 0, +1 ); }
void	quasi88_drv1_image_prev( void ){ change_image_change( 0, -1 ); }
void	quasi88_drv2_image_next( void ){ change_image_change( 1, +1 ); }
void	quasi88_drv2_image_prev( void ){ change_image_change( 1, -1 ); }


/*======================================================================
 * ディスクイメージファイル設定
 *		・両ドライブに挿入して、リセット
 *		・両ドライブに挿入
 *		・指定ドライブに挿入
 *		・反対ドライブのイメージファイルを、挿入
 *		・両ドライブ取り出し
 *		・指定ドライブ取り出し
 *======================================================================*/
int	quasi88_disk_insert_and_reset( const char *filename, int ro )
{
  if( quasi88_disk_insert_all( filename, ro ) ){
    quasi88_reset();
    return TRUE;
  }
  return FALSE;
}
int	quasi88_disk_insert_all( const char *filename, int ro )
{
  quasi88_disk_eject_all();

  if( quasi88_disk_insert( DRIVE_1, filename, 0, ro ) ){

    boot_from_rom = FALSE;

    if( disk_image_num( DRIVE_1 ) > 1 ){
      quasi88_disk_insert_A_to_B( DRIVE_1, DRIVE_2, 1 );
    }
    return TRUE;
  }
  return FALSE;
}
int	quasi88_disk_insert( int drv, const char *filename, int image, int ro )
{
  int err;

  quasi88_disk_eject( drv );

  if( strlen( filename ) < QUASI88_MAX_FILENAME ){

    err = disk_insert( drv, filename, image, ro );

    if( err == FALSE ){
      strcpy( file_disk[ drv ], filename );
      readonly_disk[ drv ] = ro;

      if( filename_synchronize ){
	set_state_filename( FALSE );
	set_snapshot_filename( FALSE );
      }
    }

    return err ? FALSE : TRUE;
  }

  return FALSE;
}
int	quasi88_disk_insert_A_to_B( int src, int dst, int img )
{
  int err;

  quasi88_disk_eject( dst );

  err = disk_insert_A_to_B( src, dst, img );

  if( err == FALSE ){
    strcpy( file_disk[ dst ], file_disk[ src ] );
    readonly_disk[ dst ] = readonly_disk[ src ];

    if( filename_synchronize ){
      set_state_filename( FALSE );
      set_snapshot_filename( FALSE );
    }
  }

  return err ? FALSE : TRUE;
}
void	quasi88_disk_eject_all( void )
{
  int drv;

  for( drv=0; drv<2; drv++ ){
    quasi88_disk_eject( drv );
  }

  boot_from_rom = TRUE;
}
void	quasi88_disk_eject( int drv )
{
  if( disk_image_exist( drv ) ){
    disk_eject( drv );
    memset( file_disk[ drv ], 0, QUASI88_MAX_FILENAME );

    if( filename_synchronize ){
      set_state_filename( FALSE );
      set_snapshot_filename( FALSE );
    }
  }
}


/*======================================================================
 * テープイメージファイル設定
 *		・ロード用テープイメージファイルセット
 *		・ロード用テープイメージファイル巻き戻し
 *		・ロード用テープイメージファイル取り外し
 *		・セーブ用テープイメージファイルセット
 *		・セーブ用テープイメージファイル取り外し
 *======================================================================*/
int	quasi88_load_tape_insert( const char *filename )
{
  quasi88_load_tape_eject();

  if( strlen( filename ) < QUASI88_MAX_FILENAME &&
      sio_open_tapeload( filename ) ){

    strcpy( file_tape[ CLOAD ], filename );
    return TRUE;

  }
  return FALSE;
}
int	quasi88_load_tape_rewind( void )
{
  if( sio_tape_rewind() ){

    return TRUE;

  }
  quasi88_load_tape_eject();
  return FALSE;
}
void	quasi88_load_tape_eject( void )
{
  sio_close_tapeload();
  memset( file_tape[ CLOAD ], 0, QUASI88_MAX_FILENAME );
}

int	quasi88_save_tape_insert( const char *filename )
{
  quasi88_save_tape_eject();

  if( strlen( filename ) < QUASI88_MAX_FILENAME &&
      sio_open_tapesave( filename ) ){

    strcpy( file_tape[ CSAVE ], filename );
    return TRUE;

  }
  return FALSE;
}
void	quasi88_save_tape_eject( void )
{
  sio_close_tapesave();
  memset( file_tape[ CSAVE ], 0, QUASI88_MAX_FILENAME );
}

/*======================================================================
 * シリアル・パラレルイメージファイル設定
 *		・シリアル入力用ファイルセット
 *		・シリアル入力用ファイル取り外し
 *		・シリアル出力用ファイルセット
 *		・シリアル出力用ファイル取り外し
 *		・プリンタ出力用ファイルセット
 *		・プリンタ入力用ファイルセット
 *======================================================================*/
int	quasi88_serial_in_connect( const char *filename )
{
  quasi88_serial_in_remove();

  if( strlen( filename ) < QUASI88_MAX_FILENAME &&
      sio_open_serialin( filename ) ){

    strcpy( file_sin, filename );
    return TRUE;

  }
  return FALSE;
}
void	quasi88_serial_in_remove( void )
{
  sio_close_serialin();
  memset( file_sin, 0, QUASI88_MAX_FILENAME );
}
int	quasi88_serial_out_connect( const char *filename )
{
  quasi88_serial_out_remove();

  if( strlen( filename ) < QUASI88_MAX_FILENAME &&
      sio_open_serialout( filename ) ){

    strcpy( file_sout, filename );
    return TRUE;

  }
  return FALSE;
}
void	quasi88_serial_out_remove( void )
{
  sio_close_serialout();
  memset( file_sout, 0, QUASI88_MAX_FILENAME );
}
int	quasi88_printer_connect( const char *filename )
{
  quasi88_printer_remove();

  if( strlen( filename ) < QUASI88_MAX_FILENAME &&
      printer_open( filename ) ){

    strcpy( file_prn, filename );
    return TRUE;

  }
  return FALSE;
}
void	quasi88_printer_remove( void )
{
  printer_close();
  memset( file_prn, 0, QUASI88_MAX_FILENAME );
}











/***********************************************************************
 * 各種ファイルのフルパスを取得
 *	各種設定の処理 (機種依存部) から呼び出される・・・ハズ
 *
 *		指定の ディスクイメージ名の、フルパスを取得
 *		指定の ROMイメージ名の、     フルパスを取得
 *		指定の 共通設定ファイル名の、フルパスを取得
 *		指定の 個別設定ファイル名の、フルパスを取得
 *
 *	成功時は、 char * (mallocされた領域)、失敗時は NULL
 ************************************************************************/

/*
 * あるイメージのファイル名 imagename のディレクトリ部と拡張子部を
 * 取り除いたベース名を取り出し、
 * basedir と ベース名 と suffix を結合したファイル名を返す。
 * この返ってくる領域は、静的な領域なので注意 !
 */

static	char *get_concatenate_filename( const char *imagename,
					const char *basedir,
					const char *suffix )
{
  char *p;
  static char buf[ OSD_MAX_FILENAME ];
        char file[ OSD_MAX_FILENAME ];

  if( osd_path_split( imagename, buf, file, OSD_MAX_FILENAME ) ){

    size_t len = strlen( file );

    if( len >= 4 ){
      if      ( strcmp( &file[ len-4 ], ".d88" ) == 0 ||
		strcmp( &file[ len-4 ], ".D88" ) == 0 ){

	file[ len-4 ] = '\0';
      }
/*
      else if( strcmp( &file[ len-4 ], ".t88" ) == 0 ||
	       strcmp( &file[ len-4 ], ".T88" ) == 0 ||
	       strcmp( &file[ len-4 ], ".cmt" ) == 0 ||
	       strcmp( &file[ len-4 ], ".CMT" ) == 0 ){

	file[ len-4 ] = '\0';
      }
*/
    }

    if( strlen(file) + strlen(suffix) + 1 < OSD_MAX_FILENAME ){

      strcat( file, suffix );

      if( osd_path_join( basedir, file, buf, OSD_MAX_FILENAME ) ){
	return buf;
      }
    }
  }

  return NULL;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

char	*alloc_diskname( const char *filename )
{
  char *p;
  char dir [ OSD_MAX_FILENAME ];
  char file[ OSD_MAX_FILENAME ];
  const char *base;
  OSD_FILE *fp;
  int step;

		/* filename を dir と file に分ける */

  if( osd_path_split( filename, dir, file, OSD_MAX_FILENAME ) ){

    if( dir[0] == '\0' ){
		/* dir が空、つまり filename にパスの区切りが含まれない */

      step = 0;		/* dir_disk + filename で ファイル有無を判定	*/

    }else{
		/* filename にパス区切りが含まれる	または		*/
		/* 上記 step 0 で、ファイルが無かった場合		*/

      step = 1;		/* dir_cwd + filename で ファイル有無をチェック */
			/*	( filenameが絶対パスなら、 filename     */
			/*	  そのものでファイル有無チェックとなる)	*/
    }

  }else{
    return NULL;
  }


		/* step 0 → step 1 の順に、ファイル有無チェック */

  for( ; step < 2; step ++ ){

    if( step == 0 ) base = osd_dir_disk();
    else            base = osd_dir_cwd();

    if( base==NULL ) continue;

    if( osd_path_join( base, filename, file, OSD_MAX_FILENAME ) == FALSE ){
      return NULL;
    }

			/* 実際に open できるかをチェックする */
    fp = osd_fopen( FTYPE_DISK, file, "rb" );
    if( fp ){
      osd_fclose( fp );

      p = (char *)malloc( strlen(file) + 1 );
      if( p ){
	strcpy( p, file );
	return p;
      }
      break;
    }
  }

  return NULL;
}





char	*alloc_romname( const char *filename )
{
  char *p;
  char buf[ OSD_MAX_FILENAME ];
  OSD_FILE *fp;
  int step;
  const char *dir = osd_dir_rom(); 

	/* step 0 … filenameがあるかチェック			*/
	/* step 1 … dir_rom に、 filename があるかチェック	*/

  for( step=0; step<2; step++ ){

    if( step==0 ){

      if( OSD_MAX_FILENAME <= strlen(filename) ) return NULL;
      strcpy( buf, filename );

    }else{

      if( dir == NULL ||
	  osd_path_join( dir, filename, buf, OSD_MAX_FILENAME ) == FALSE ){

	return NULL;
      }
    }

		/* 実際に open できるかをチェックする */
    fp = osd_fopen( FTYPE_ROM, buf, "rb" );
    if( fp ){
      osd_fclose( fp );

      p = (char *)malloc( strlen(buf) + 1 );
      if( p ){
	strcpy( p, buf );
	return p;
      }
      break;
    }
  }
  return NULL;
}





char	*alloc_global_cfgname( void )
{
  const char *dir  = osd_dir_gcfg();
  const char *file = CONFIG_FILENAME  CONFIG_SUFFIX;
  char *p;
  char buf[ OSD_MAX_FILENAME ];


  if( dir == NULL ||
      osd_path_join( dir, file, buf, OSD_MAX_FILENAME ) == FALSE )

    return NULL;

  p = (char *)malloc( strlen(buf) + 1 );
  if( p ){
    strcpy( p, buf );
    return p;
  }

  return NULL;
}

char	*alloc_keyboard_cfgname( void )
{
  const char *dir  = osd_dir_gcfg();
  const char *file = KEYCONF_FILENAME  CONFIG_SUFFIX;
  char *p;
  char buf[ OSD_MAX_FILENAME ];


  if( dir == NULL ||
      osd_path_join( dir, file, buf, OSD_MAX_FILENAME ) == FALSE )

    return NULL;

  p = (char *)malloc( strlen(buf) + 1 );
  if( p ){
    strcpy( p, buf );
    return p;
  }

  return NULL;
}

char	*alloc_local_cfgname( const char *imagename )
{
  char *p   = NULL;
  char *buf;
  const char *dir = osd_dir_lcfg();

  if( dir==NULL ) return NULL;

  buf = get_concatenate_filename( imagename, dir, CONFIG_SUFFIX );

  if( buf ){
    p = (char *)malloc( strlen(buf) + 1 );
    if( p ){
      strcpy( p, buf );
    }
  }
  return p;
}




/***********************************************************************
 * ステートファイル、スナップショットファイルのファイル名に、
 * 初期文字列をセットする。
 *
 *
 *
 *
 ************************************************************************/

int	set_state_filename( int init )
{
  int result = FALSE;
  char *s, *buf;
  const char *dir;

  dir = osd_dir_state();
  if( dir==NULL ) dir = osd_dir_cwd();

  memset( file_state, 0, QUASI88_MAX_FILENAME );

  if( init == FALSE ){
    if     ( file_disk[0][0]     != '\0' ) s = file_disk[0];
    else if( file_disk[1][0]     != '\0' ) s = file_disk[1];
/*  else if( file_tape[CLOAD][0] != '\0' ) s = file_tape[CLOAD];*/
    else                                   s = STATE_FILENAME;
  }else{
    s = STATE_FILENAME;
  }

  buf = get_concatenate_filename( s, dir, STATE_SUFFIX );

  if( buf ){
    if( strlen( buf ) < QUASI88_MAX_FILENAME ){

      strcpy( file_state, buf );
      result = TRUE;
    }
  }

  if( result == FALSE ){
    strcpy( file_state, STATE_FILENAME STATE_SUFFIX );
  }
  return result;
}



int	set_snapshot_filename( int init )
{
  int result = FALSE;
  char *s, *buf;
  const char *dir;

  dir = osd_dir_snap();
  if( dir==NULL ) dir = osd_dir_cwd();

  memset( file_snap, 0, QUASI88_MAX_FILENAME );

  if( init == FALSE ){
    if     ( file_disk[0][0]     != '\0' ) s = file_disk[0];
    else if( file_disk[1][0]     != '\0' ) s = file_disk[1];
/*  else if( file_tape[CLOAD][0] != '\0' ) s = file_tape[CLOAD];*/
    else                                   s = SNAPSHOT_FILENAME;
  }else{
    s = SNAPSHOT_FILENAME;
  }

  buf = get_concatenate_filename( s, dir, "" );

  if( buf ){
    if( strlen( buf ) < QUASI88_MAX_FILENAME ){

      strcpy( file_snap, buf );
      result = TRUE;
    }
  }

  if( result == FALSE ){
    strcpy( file_snap, SNAPSHOT_FILENAME );
  }
  return result;
}
