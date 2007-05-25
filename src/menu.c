/************************************************************************/
/*									*/
/* メニューモード							*/
/*									*/
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "quasi88.h"
#include "initval.h"
#include "menu.h"

#include "pc88main.h"
#include "pc88sub.h"
#include "graph.h"
#include "intr.h"
#include "keyboard.h"
#include "memory.h"
#include "screen.h"

#include "emu.h"
#include "drive.h"
#include "image.h"
#include "status.h"
#include "monitor.h"
#include "snddrv.h"
#include "wait.h"
#include "file-op.h"
#include "suspend.h"
#include "snapshot.h"
#include "fdc.h"
#include "soundbd.h"

#include "event.h"
#include "q8tk.h"



int	menu_lang	= LANG_JAPAN;		/* メニューの言語           */
int	menu_readonly	= FALSE;		/* ディスク選択ダイアログの */
						/* 初期状態は ReadOnly ?    */
int	menu_swapdrv	= FALSE;		/* ドライブの表示順序       */


/*--------------------------------------------------------------*/
/* メニューでの表示メッセージは全て、このファイルの中に		*/
/*--------------------------------------------------------------*/
#include "message.h"




/****************************************************************/
/* ワーク							*/
/****************************************************************/

static	int	menu_last_page = 0;	/* 前回時のメニュータグを記憶 */

static	int	menu_boot_dipsw;	/* リセット時の設定を記憶 */
static	int	menu_boot_from_rom;
static	int	menu_boot_basic;
static	int	menu_boot_clock_4mhz;
static	int	menu_boot_clock_async;
static	int	menu_boot_version;
static	int	menu_boot_baudrate;
static	int	menu_boot_sound_board;
static	int	menu_boot_use_fmgen;

					/* 起動デバイスの制御に必要 */
static	Q8tkWidget	*widget_reset_boot;
static	Q8tkWidget	*widget_dipsw_b_boot_disk;
static	Q8tkWidget	*widget_dipsw_b_boot_rom;

static	Q8tkWidget	*menu_accel;	/* メインメニューのキー定義 */

static	int	quickres_basic;		/* 簡易リセットの設定を記憶 */
static	int	quickres_clock;





/*===========================================================================
 * ファイル操作エラーメッセージのダイアログ処理
 *===========================================================================*/
static	void	cb_file_error_dialog_ok( Q8tkWidget *dummy_0, void *dummy_1 )
{
  dialog_destroy();
}

static	void	start_file_error_dialog( int drv, int result )
{
  char wk[128];
  const t_menulabel *l = (drv<0) ? data_err_file : data_err_drive;

  if( result==ERR_NO ) return;
  if( drv<0 ) sprintf( wk, GET_LABEL( l, result ) );
  else        sprintf( wk, GET_LABEL( l, result ), drv+1 );

  dialog_create();
  {
    dialog_set_title( wk );
    dialog_set_separator();
    dialog_set_button( GET_LABEL( l, ERR_NO ),
		       cb_file_error_dialog_ok, NULL );
  }
  dialog_start();
}

/*===========================================================================
 * ディスク挿入 & 排出
 *===========================================================================*/
static void sub_misc_suspend_change( void );
static void sub_misc_snapshot_change( void );

/*===========================================================================
 *
 *	メインページ	リセット
 *
 *===========================================================================*/

/*----------------------------------------------------------------------*/
						 /* BASICモード切り替え */
static	int	get_reset_basic( void )
{
  return menu_boot_basic;
}
static	void	cb_reset_basic( Q8tkWidget *dummy, void *p )
{
  menu_boot_basic = (int)p;
}


static	Q8tkWidget	*menu_reset_basic( void )
{
  Q8tkWidget	*box;

  box = PACK_VBOX( NULL );
  {
    PACK_RADIO_BUTTONS( box,
			data_reset_basic, COUNTOF(data_reset_basic),
			get_reset_basic(), cb_reset_basic );
  }

  return box;
}

/*----------------------------------------------------------------------*/
						       /* CLOCK切り替え */
static	int	get_reset_clock( void )
{
  if( menu_boot_clock_4mhz ) return CLOCK_4MHZ;
  else                       return CLOCK_8MHZ;
}
static	void	cb_reset_clock( Q8tkWidget *dummy, void *p )
{
  if( (int)p == CLOCK_4MHZ ) menu_boot_clock_4mhz = TRUE;
  else                       menu_boot_clock_4mhz = FALSE;
}
static	int	get_reset_clock_async( void )
{
  return menu_boot_clock_async;
}
static	void	cb_reset_clock_async( Q8tkWidget *widget, void *dummy )
{
  int	async = ( Q8TK_TOGGLE_BUTTON(widget)->active ) ? TRUE : FALSE;
  menu_boot_clock_async = async;
}


static	Q8tkWidget	*menu_reset_clock( void )
{
  Q8tkWidget	*box,*box2;

  box = PACK_VBOX( NULL );
  {
    PACK_RADIO_BUTTONS( box,
			data_reset_clock, COUNTOF(data_reset_clock),
			get_reset_clock(), cb_reset_clock );
    PACK_LABEL( box, "" );
    box2 = PACK_HBOX( box );
    {
      PACK_LABEL( box2, "  " );
      PACK_CHECK_BUTTON( box2,
			 GET_LABEL( data_reset_clock_async, 0 ),
			 get_reset_clock_async(),
			 cb_reset_clock_async, NULL );
    }
  }

  return box;
}

/*----------------------------------------------------------------------*/
						  /* バージョン切り替え */
static	int	get_reset_version( void )
{
  return menu_boot_version;
}
static	void	cb_reset_version( Q8tkWidget *widget, void *dummy )
{
  menu_boot_version = *(q8tk_combo_get_text(widget));
}


static	Q8tkWidget	*menu_reset_version( void )
{
  Q8tkWidget	*combo;
  char		wk[4];

  wk[0] = get_reset_version();
  wk[1] = '\0';

  combo = PACK_COMBO( NULL,
		      data_reset_version, COUNTOF(data_reset_version),
		      get_reset_version(), wk, 4,
		      cb_reset_version, NULL,
		      NULL, NULL );
  return combo;
}

/*----------------------------------------------------------------------*/
						/* ディップスイッチ設定 */
static	void	dipsw_create( void );
static	void	dipsw_start( void );
static	void	dipsw_finish( void );

static	void	cb_reset_dipsw( Q8tkWidget *dummy_0, void *dummy_1 )
{
  dipsw_start();
}

static	void	set_reset_dipsw_boot( void )
{
  const t_menulabel  *l = data_reset_boot;

  if( widget_reset_boot ){
    q8tk_label_set( widget_reset_boot,
		    ( menu_boot_from_rom ? GET_LABEL( l, 1 )
					 : GET_LABEL( l, 0 ) ) );
  }
}


static	Q8tkWidget	*menu_reset_dipsw( void )
{
  Q8tkWidget	*vbox;
  Q8tkWidget	*button;
  const t_menulabel *l = data_reset;

  vbox = PACK_VBOX( NULL );
  {
    button = PACK_BUTTON( vbox,
			  GET_LABEL( l, DATA_RESET_DIPSW_BTN ),
			  cb_reset_dipsw, NULL );
    q8tk_misc_set_placement( button, Q8TK_PLACEMENT_X_CENTER,
				     Q8TK_PLACEMENT_Y_CENTER );

    widget_reset_boot = PACK_LABEL( vbox, "" );
    set_reset_dipsw_boot();
  }
  return vbox;
}

/*----------------------------------------------------------------------*/
								/* その他 */
static	int	get_reset_misc_sb( void )
{
  return menu_boot_sound_board;
}
static	void	cb_reset_misc_sb( Q8tkWidget *dummy, void *p )
{
  menu_boot_sound_board = (int)p;
}


static	Q8tkWidget	*menu_reset_misc( void )
{
  Q8tkWidget	*box;

  box = PACK_VBOX( NULL );
  {
    PACK_RADIO_BUTTONS( box,
			data_reset_misc_sb, COUNTOF(data_reset_misc_sb),
			get_reset_misc_sb(), cb_reset_misc_sb );
  }

  return box;
}

/*----------------------------------------------------------------------*/
							/* サウンドドライバ */
#if	defined(USE_SOUND) && defined(USE_FMGEN)
static	int	get_reset_snddrv( void )
{
  return menu_boot_use_fmgen;
}
static	void	cb_reset_snddrv( Q8tkWidget *dummy, void *p )
{
  menu_boot_use_fmgen = (int)p;
}


static	Q8tkWidget	*menu_reset_snddrv( void )
{
  Q8tkWidget	*box;

  box = PACK_VBOX( NULL );
  {
    PACK_RADIO_BUTTONS( box,
			data_reset_snddrv, COUNTOF(data_reset_snddrv),
			get_reset_snddrv(), cb_reset_snddrv );
  }

  return box;
}
#endif

/*----------------------------------------------------------------------*/
							    /* リセット */
static	void	cb_reset_now( Q8tkWidget *dummy_0, void *dummy_1 )
{
  int now_board = sound_board;

#if	defined(USE_SOUND) && defined(USE_FMGEN)
  int now_fmgen   = use_fmgen;
  use_fmgen       = menu_boot_use_fmgen;
#endif

  boot_dipsw      = menu_boot_dipsw;
  boot_from_rom   = menu_boot_from_rom;
  boot_basic      = menu_boot_basic;
  boot_clock_4mhz = menu_boot_clock_4mhz;
  set_version     = menu_boot_version;
  baudrate_sw     = menu_boot_baudrate;
  sound_board     = menu_boot_sound_board;

  if( menu_boot_clock_async == FALSE ){
    cpu_clock_mhz = boot_clock_4mhz ? CONST_4MHZ_CLOCK : CONST_8MHZ_CLOCK;
  }

  quasi88_reset();


#if	defined(USE_SOUND) && defined(USE_FMGEN)
  if( ( now_fmgen != use_fmgen )  ||
      ( now_board != sound_board ) )
#else
  if( now_board != sound_board )
#endif
  {
    if( memory_allocate_additional() == FALSE ){
      quasi88_exit();	/* 失敗！ */
    }
    xmame_sound_resume();		/* 中断したサウンドを復帰後に */
    xmame_sound_stop();			/* サウンドを停止させる。     */
    xmame_sound_start();		/* そして、サウンド再初期化   */
  }

  set_emu_mode( GO );

  q8tk_main_quit();

#if 0
printf(  "boot_dipsw      %04x\n",boot_dipsw    );
printf(  "boot_from_rom   %d\n",boot_from_rom   );
printf(  "boot_basic      %d\n",boot_basic      );
printf(  "boot_clock_4mhz %d\n",boot_clock_4mhz );
printf(  "ROM_VERSION     %c\n",ROM_VERSION     );
printf(  "baudrate_sw     %d\n",baudrate_sw     );
#endif
}

/*----------------------------------------------------------------------*/
						   /* 現在のBASICモード */
static	Q8tkWidget	*menu_reset_current( void )
{
  static const char *type[] = {
    "PC-8801",
    "PC-8801",
    "PC-8801",
    "PC-8801mkII",
    "PC-8801mkIISR",
    "PC-8801mkIITR/FR/MR",
    "PC-8801mkIITR/FR/MR",
    "PC-8801mkIITR/FR/MR",
    "PC-8801FH/MH",
    "PC-8801FA/MA/FE/MA2/FE2/MC",
  };
  static const char *basic[] = { " N ", "V1S", "V1H", " V2", };
  static const char *clock[] = { "8MHz", "4MHz", };
  const char *t = "";
  const char *b = "";
  const char *c = "";
  int i;
  char wk[80], ext[40];

  i = (get_reset_version() & 0xff) - '0';
  if( 0 <= i  &&  i< COUNTOF(type) ) t = type[ i ];

  i = get_reset_basic();
  if( 0 <= i  &&  i< COUNTOF(basic) ) b = basic[ i ];

  i = get_reset_clock();
  if( 0 <= i  &&  i< COUNTOF(clock) ) c = clock[ i ];

  ext[0] = 0;
  {
    if( sound_port ){
      if( ext[0]==0 ) strcat( ext, "(" );
      else            strcat( ext, ", " );
      if( sound_board == SOUND_I ) strcat( ext, "OPN" );
      else                         strcat( ext, "OPNA" );
    }

    if( use_extram ){
      if( ext[0]==0 ) strcat( ext, "(" );
      else            strcat( ext, ", " );
      sprintf( wk, "%dKB", use_extram * 128 );
      strcat( ext, wk );
      strcat( ext, GET_LABEL(data_reset_current,0) );/* ExtRAM*/
    }

    if( use_jisho_rom ){
      if( ext[0]==0 ) strcat( ext, "(" );
      else            strcat( ext, ", " );
      strcat( ext, GET_LABEL(data_reset_current,1) );/*DictROM*/
    }
  }
  if( ext[0] ) strcat( ext, ")" );


  sprintf( wk, " %-30s  %4s  %4s  %30s ",
	   t, b, c, ext );

  return PACK_LABEL( NULL, wk );
}

/*======================================================================*/

static	Q8tkWidget	*menu_reset( void )
{
  Q8tkWidget *hbox, *vbox;
  Q8tkWidget *w, *f;
  const t_menulabel *l = data_reset;

  dipsw_create();		/* ディップスイッチウインドウ生成 */

  vbox = PACK_VBOX( NULL );
  {
    f = PACK_FRAME( vbox, "", menu_reset_current() );
    q8tk_frame_set_shadow_type( f, Q8TK_SHADOW_ETCHED_OUT );

    hbox = PACK_HBOX( vbox );
    {
      PACK_FRAME( hbox,
		  GET_LABEL( l, DATA_RESET_BASIC ), menu_reset_basic() );

      PACK_FRAME( hbox,
		  GET_LABEL( l, DATA_RESET_CLOCK ), menu_reset_clock() );

      PACK_FRAME( hbox,
		  GET_LABEL( l, DATA_RESET_VERSION ), menu_reset_version() );

      PACK_FRAME( hbox,
		  GET_LABEL( l, DATA_RESET_DIPSW ), menu_reset_dipsw() );
    }

    hbox = PACK_HBOX( vbox );
    {
      PACK_FRAME( hbox,
		  GET_LABEL( l, DATA_RESET_MISC ), menu_reset_misc() );

#if	defined(USE_SOUND) && defined(USE_FMGEN)
      if( xmame_sound_is_enable() ){
	PACK_FRAME( hbox,
		    GET_LABEL( l, DATA_RESET_SNDDRV ), menu_reset_snddrv() );
      }
#endif
    }

    PACK_LABEL( vbox, GET_LABEL( l, DATA_RESET_NOTICE ) );

    w = PACK_BUTTON( vbox,
		     GET_LABEL( data_reset, DATA_RESET_NOW ),
		     cb_reset_now, NULL );
    q8tk_misc_set_placement( w, Q8TK_PLACEMENT_X_RIGHT, 0 );
  }

  return vbox;
}



/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
 *
 *	サブウインドウ	DIPSW
 *
 * = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

static	Q8tkWidget	*dipsw_window;
static	Q8tkWidget	*dipsw[4];
static	Q8tkWidget	*dipsw_accel;

enum {
  DIPSW_WIN,
  DIPSW_FRAME,
  DIPSW_VBOX,
  DIPSW_QUIT
};

/*----------------------------------------------------------------------*/
					    /* ディップスイッチ切り替え */
static	int	get_dipsw_b( int p )
{
  int	shift = data_dipsw_b[p].val;

  return ( (p<<1) | ( (menu_boot_dipsw >> shift) & 1 ) );
}
static	void	cb_dipsw_b( Q8tkWidget *dummy, void *p )
{
  int	shift = data_dipsw_b[ (int)p >> 1 ].val;
  int	on    = (int)p & 1;

  if( on ) menu_boot_dipsw |=  (1 << shift );
  else     menu_boot_dipsw &= ~(1 << shift );
}
static	int	get_dipsw_b2( void )
{
  return ( menu_boot_from_rom ? TRUE : FALSE );
}
static	void	cb_dipsw_b2( Q8tkWidget *dummy, void *p )
{
  if( (int)p ) menu_boot_from_rom = TRUE;
  else         menu_boot_from_rom = FALSE;

  set_reset_dipsw_boot();
}


static	Q8tkWidget	*menu_dipsw_b( void )
{
  int	i;
  Q8tkWidget	*vbox, *hbox;
  Q8tkWidget	*b = NULL;
  const t_dipsw *pd;
  const t_menudata *p;


  vbox = PACK_VBOX( NULL );
  {
    pd = data_dipsw_b;
    for( i=0; i<COUNTOF(data_dipsw_b); i++, pd++ ){

      hbox = PACK_HBOX( vbox );
      {
	PACK_LABEL( hbox, GET_LABEL( pd, 0 ) );

	PACK_RADIO_BUTTONS( hbox,
			    pd->p, 2,
			    get_dipsw_b(i), cb_dipsw_b );
      }
    }

    hbox = PACK_HBOX( vbox );
    {
      pd = data_dipsw_b2;
      p  = pd->p;

      PACK_LABEL( hbox, GET_LABEL( pd, 0 ) );

      for( i=0; i<2; i++, p++ ){
	b = PACK_RADIO_BUTTON( hbox,
			       b,
			       GET_LABEL( p, 0 ), 
			       (get_dipsw_b2() == p->val) ? TRUE : FALSE,
			       cb_dipsw_b2, (void *)(p->val) );

	if( i==0 ) widget_dipsw_b_boot_disk = b;	/* これらのボタンは */
	else       widget_dipsw_b_boot_rom  = b;	/* 覚えておく       */
      }
    }
  }

  return vbox;
}

/*----------------------------------------------------------------------*/
				   /* ディップスイッチ切り替え(RS-232C) */
static	int	get_dipsw_r( int p )
{
  int	shift = data_dipsw_r[p].val;

  return ( (p<<1) | ( (menu_boot_dipsw >> shift) & 1 ) );
}
static	void	cb_dipsw_r( Q8tkWidget *dummy, void *p )
{
  int	shift = data_dipsw_r[ (int)p >> 1 ].val;
  int	on    = (int)p & 1;

  if( on ) menu_boot_dipsw |=  (1 << shift );
  else     menu_boot_dipsw &= ~(1 << shift );
}
static	int	get_dipsw_r_baudrate( void )
{
  return menu_boot_baudrate;
}
static	void	cb_dipsw_r_baudrate( Q8tkWidget *widget, void *dummy )
{
  int	i;
  const t_menudata *p = data_dipsw_r_baudrate;
  const char       *combo_str = q8tk_combo_get_text(widget);

  for( i=0; i<COUNTOF(data_dipsw_r_baudrate); i++, p++ ){
    if( strcmp( p->str[menu_lang], combo_str ) == 0 ){
      menu_boot_baudrate = p->val;
      return;
    }
  }
}


static	Q8tkWidget	*menu_dipsw_r( void )
{
  int	i;
  Q8tkWidget	*vbox, *hbox;
  const t_dipsw *pd;

  vbox = PACK_VBOX( NULL );
  {
    hbox = PACK_HBOX( vbox );
    {
      PACK_LABEL( hbox, GET_LABEL( data_dipsw_r2, 0 ) );

      PACK_COMBO( hbox,
		  data_dipsw_r_baudrate,
		  COUNTOF(data_dipsw_r_baudrate),
		  get_dipsw_r_baudrate(), NULL, 8,
		  cb_dipsw_r_baudrate, NULL,
		  NULL, NULL );
    }

    pd = data_dipsw_r;
    for( i=0; i<COUNTOF(data_dipsw_r); i++, pd++ ){

      hbox = PACK_HBOX( vbox );
      {
	PACK_LABEL( hbox, GET_LABEL( data_dipsw_r, i ) );

	PACK_RADIO_BUTTONS( hbox,
			    pd->p, 2,
			    get_dipsw_r(i), cb_dipsw_r );
      }
    }
  }

  return vbox;
}

/*----------------------------------------------------------------------*/

static	void	dipsw_create( void )
{
  Q8tkWidget *vbox;
  const t_menulabel *l = data_dipsw;

  vbox = PACK_VBOX( NULL );
  {
    PACK_FRAME( vbox, GET_LABEL( l, DATA_DIPSW_B ), menu_dipsw_b() );

    PACK_FRAME( vbox, GET_LABEL( l, DATA_DIPSW_R ), menu_dipsw_r() );
  }

  dipsw_window = vbox;
}

static	void	cb_reset_dipsw_end( Q8tkWidget *dummy_0, void *dummy_1 )
{
  dipsw_finish();
}

static	void	dipsw_start( void )
{
  Q8tkWidget	*w, *f, *x, *b;
  const t_menulabel *l = data_reset;

  {						/* メインとなるウインドウ */
    w = q8tk_window_new( Q8TK_WINDOW_DIALOG );
    dipsw_accel = q8tk_accel_group_new();
    q8tk_accel_group_attach( dipsw_accel, w );
  }
  {						/* に、フレームを乗せて */
    f = q8tk_frame_new( GET_LABEL( l, DATA_RESET_DIPSW_SET ) );
    q8tk_container_add( w, f );
    q8tk_widget_show( f );
  }
  {						/* それにボックスを乗せる */
    x = q8tk_vbox_new();
    q8tk_container_add( f, x );
    q8tk_widget_show( x );
							/* ボックスには     */
    {							/* DIPSWメニュー と */
      q8tk_box_pack_start( x, dipsw_window );
    }
    {							/* 終了ボタンを配置 */
      b = PACK_BUTTON( x,
		       GET_LABEL( l, DATA_RESET_DIPSW_QUIT ),
		       cb_reset_dipsw_end, NULL );

      q8tk_accel_group_add( dipsw_accel, Q8TK_KEY_ESC, b, "clicked" );
    }
  }

  q8tk_widget_show( w );
  q8tk_grab_add( w );

  q8tk_widget_grab_default( b );


  dipsw[ DIPSW_WIN   ] = w;	/* ダイアログを閉じたときに備えて */
  dipsw[ DIPSW_FRAME ] = f;	/* ウィジットを覚えておきます     */
  dipsw[ DIPSW_VBOX  ] = x;
  dipsw[ DIPSW_QUIT  ] = b;
}

/* ディップスイッチ設定ウインドウの消去 */

static	void	dipsw_finish( void )
{
  q8tk_widget_destroy( dipsw[ DIPSW_QUIT ] );
  q8tk_widget_destroy( dipsw[ DIPSW_VBOX ] );
  q8tk_widget_destroy( dipsw[ DIPSW_FRAME ] );

  q8tk_grab_remove( dipsw[ DIPSW_WIN ] );
  q8tk_widget_destroy( dipsw[ DIPSW_WIN ] );
  q8tk_widget_destroy( dipsw_accel );
}





/*===========================================================================
 *
 *	メインページ	CPU設定
 *
 *===========================================================================*/

/*----------------------------------------------------------------------*/
						     /* CPU処理切り替え */
static	int	get_cpu_cpu( void )
{
  return cpu_timing;
}
static	void	cb_cpu_cpu( Q8tkWidget *dummy, void *p )
{
  cpu_timing = (int)p;
}


static	Q8tkWidget	*menu_cpu_cpu( void )
{
  Q8tkWidget	*vbox;

  vbox = PACK_VBOX( NULL );
  {						/* radio_button ... */
    PACK_RADIO_BUTTONS( vbox,
			data_cpu_cpu, COUNTOF(data_cpu_cpu),
			get_cpu_cpu(), cb_cpu_cpu );
  }

  return vbox;
}

/*----------------------------------------------------------------------*/
								/* 説明 */
static	Q8tkWidget	*help_widget[5];
static	Q8tkWidget	*help_string[40];
static	int		help_string_cnt;	
static	Q8tkWidget	*help_accel;

enum {
  HELP_WIN,
  HELP_VBOX,
  HELP_SWIN,
  HELP_BOARD,
  HELP_QUIT
};

static	void	help_finish( void );
static	void	cb_cpu_help_end( Q8tkWidget *dummy_0, void *dummy_1 )
{
  help_finish();
}

static	void	cb_cpu_help( Q8tkWidget *dummy_0, void *dummy_1 )
{
  Q8tkWidget	*w, *swin, *x, *b, *z;

  {						/* メインとなるウインドウ */
    w = q8tk_window_new( Q8TK_WINDOW_DIALOG );
    help_accel = q8tk_accel_group_new();
    q8tk_accel_group_attach( help_accel, w );
  }
  {						/* それにボックスを乗せる */
    x = q8tk_vbox_new();
    q8tk_container_add( w, x );
    q8tk_widget_show( x );
							/* ボックスには     */
    {							/* SCRLウインドウと */
      swin  = q8tk_scrolled_window_new( NULL, NULL );
      q8tk_widget_show( swin );
      q8tk_scrolled_window_set_policy( swin, Q8TK_POLICY_NEVER,
				       Q8TK_POLICY_AUTOMATIC );
      q8tk_misc_set_size( swin, 71, 20 );
      q8tk_box_pack_start( x, swin );
    }
    {							/* 終了ボタンを配置 */
      b = PACK_BUTTON( x,
		       " O K ",
		       cb_cpu_help_end, NULL );
      q8tk_misc_set_placement( b, Q8TK_PLACEMENT_X_CENTER,
			       Q8TK_PLACEMENT_Y_CENTER );

      q8tk_accel_group_add( help_accel, Q8TK_KEY_ESC, b, "clicked" );
    }
  }

  {							/* SCRLウインドウに */
    int i;
    const char **s = (menu_lang==LANG_JAPAN) ? help_jp : help_en;
    z = q8tk_vbox_new();				/* VBOXを作って     */
    q8tk_container_add( swin, z );
    q8tk_widget_show( z );

    for( i=0; i<COUNTOF(help_string); i++ ){		/* 説明ラベルを配置 */
      if( s[i] == NULL ) break;
      help_string[i] = q8tk_label_new( s[i] );
      q8tk_widget_show( help_string[i] );
      q8tk_box_pack_start( z, help_string[i] );
    }
    help_string_cnt = i;
  }

  q8tk_widget_show( w );
  q8tk_grab_add( w );

  q8tk_widget_grab_default( b );


  help_widget[ HELP_WIN   ] = w;	/* ダイアログを閉じたときに備えて */
  help_widget[ HELP_VBOX  ] = x;	/* ウィジットを覚えておきます     */
  help_widget[ HELP_SWIN  ] = swin;
  help_widget[ HELP_BOARD ] = z;
  help_widget[ HELP_QUIT  ] = b;
}

/* 説明ウインドウの消去 */

static	void	help_finish( void )
{
  int i;
  for( i=0; i<help_string_cnt; i++ )
    q8tk_widget_destroy( help_string[ i ] );

  q8tk_widget_destroy( help_widget[ HELP_QUIT  ] );
  q8tk_widget_destroy( help_widget[ HELP_BOARD ] );
  q8tk_widget_destroy( help_widget[ HELP_SWIN  ] );
  q8tk_widget_destroy( help_widget[ HELP_VBOX  ] );

  q8tk_grab_remove( help_widget[ HELP_WIN ] );
  q8tk_widget_destroy( help_widget[ HELP_WIN ] );
  q8tk_widget_destroy( help_accel );
}



static	Q8tkWidget	*menu_cpu_help( void )
{
  Q8tkWidget	*button;
  const t_menulabel *l = data_cpu;

  button = PACK_BUTTON( NULL,
			GET_LABEL( l, DATA_CPU_HELP ),
			cb_cpu_help, NULL );
  q8tk_misc_set_placement( button, Q8TK_PLACEMENT_X_CENTER,
				   Q8TK_PLACEMENT_Y_CENTER );
  return button;
}

/*----------------------------------------------------------------------*/
						 /* CPUクロック切り替え */
static	double	get_cpu_clock( void )
{
  return cpu_clock_mhz;
}
static	void	cb_cpu_clock( Q8tkWidget *widget, void *mode )
{
  int	i;
  const t_menudata *p = data_cpu_clock_combo;
  const char       *combo_str = q8tk_combo_get_text(widget);
  char buf[16], *conv_end;
  double new_clock = -1.0;

  for( i=0; i<COUNTOF(data_cpu_clock_combo); i++, p++ ){
    if( strcmp( p->str[menu_lang], combo_str )==0 ){
      new_clock = (double)p->val / 1000000.0;
      break;
    }
  }

  if( new_clock <= 0.0 ){
    strncpy( buf, combo_str, 15 );
    buf[15] = '\0';

    if( strlen(buf)==0 && (int)mode==0 ){
      new_clock = boot_clock_4mhz ? CONST_4MHZ_CLOCK
				  : CONST_8MHZ_CLOCK;
    }else{
      new_clock = strtod( buf, &conv_end );
      if( new_clock == 0.0 && (int)mode==0 ){
	new_clock = boot_clock_4mhz ? CONST_4MHZ_CLOCK
				    : CONST_8MHZ_CLOCK;
      }else if( *conv_end != '\0' ){
	new_clock = -1.0;
      }
    }
  }

  if( 0.1 <= new_clock && new_clock < 1000.0 ){
    cpu_clock_mhz = new_clock;
    interval_work_init_all();
  }

  if( (int)mode == 0 ){
    sprintf( buf, "%8.4f", get_cpu_clock() );
    q8tk_combo_set_text( widget, buf );
  }
}


static	Q8tkWidget	*menu_cpu_clock( void )
{
  Q8tkWidget	*vbox, *hbox;
  const t_menulabel *p = data_cpu_clock;
  char	buf[16];

  vbox = PACK_VBOX( NULL );
  {
    hbox = PACK_HBOX( vbox );
    {
      PACK_LABEL( hbox, GET_LABEL( p, DATA_CPU_CLOCK_CLOCK ) );

      sprintf( buf, "%8.4f", get_cpu_clock() );
      PACK_COMBO( hbox,
		  data_cpu_clock_combo, COUNTOF(data_cpu_clock_combo),
		  get_cpu_clock(), buf, 9,
		  cb_cpu_clock, (void *)0,
		  cb_cpu_clock, (void *)1 );

      PACK_LABEL( hbox, GET_LABEL(p, DATA_CPU_CLOCK_MHZ) );

      PACK_LABEL( hbox, GET_LABEL(p, DATA_CPU_CLOCK_INFO) );
    }
  }

  return vbox;
}

/*----------------------------------------------------------------------*/
							/* ウエイト変更 */
static	int	get_cpu_nowait( void )
{
  return no_wait;
}
static	void	cb_cpu_nowait( Q8tkWidget *widget, void *dummy )
{
  int	key = ( Q8TK_TOGGLE_BUTTON(widget)->active ) ? TRUE : FALSE;
  no_wait = key;
}

static	int	get_cpu_wait( void )
{
  return wait_rate;
}
static	void	cb_cpu_wait( Q8tkWidget *widget, void *mode )
{
  int	i;
  const t_menudata *p = data_cpu_wait_combo;
  const char       *combo_str = q8tk_combo_get_text(widget);
  char buf[16], *conv_end;
  int new_rate = -1;

  for( i=0; i<COUNTOF(data_cpu_wait_combo); i++, p++ ){
    if( strcmp( p->str[menu_lang], combo_str )==0 ){
      new_rate = p->val;
      break;
    }
  }

  if( new_rate <= 0 ){
    strncpy( buf, combo_str, 15 );
    buf[15] = '\0';

    if( strlen(buf)==0 && (int)mode==0 ){
      new_rate = 100;
    }else{
      new_rate = strtoul( buf, &conv_end, 10 );
      if( new_rate == 0 && (int)mode==0 ){
	new_rate = 100;
      }else if( *conv_end != '\0' ){
	new_rate = -1;
      }
    }
  }

  if( 5 <= new_rate && new_rate <= 5000 ){
    wait_rate = new_rate;
  }

  if( (int)mode == 0 ){
    sprintf( buf, "%4d", get_cpu_wait() );
    q8tk_combo_set_text( widget, buf );
  }
}


static	Q8tkWidget	*menu_cpu_wait( void )
{
  Q8tkWidget	*vbox, *hbox, *button;
  const t_menulabel *p = data_cpu_wait;
  char	buf[16];

  vbox = PACK_VBOX( NULL );
  {
    hbox = PACK_HBOX( vbox );			/* label, entry, label */
    {
      PACK_LABEL( hbox, GET_LABEL( p, DATA_CPU_WAIT_RATE ) );

      sprintf( buf, "%4d", get_cpu_wait() );
      PACK_COMBO( hbox,
		  data_cpu_wait_combo, COUNTOF(data_cpu_wait_combo),
		  get_cpu_wait(), buf, 5,
		  cb_cpu_wait, (void *)0,
		  cb_cpu_wait, (void *)1 );

      PACK_LABEL( hbox, GET_LABEL( p, DATA_CPU_WAIT_PERCENT ) );

      PACK_LABEL( hbox, GET_LABEL( p, DATA_CPU_WAIT_INFO ) );
    }

    button = PACK_CHECK_BUTTON( vbox,		/* check_button */
				GET_LABEL( p, DATA_CPU_WAIT_NOWAIT ),
				get_cpu_nowait(),
				cb_cpu_nowait, NULL );
    q8tk_misc_set_placement( button, Q8TK_PLACEMENT_X_RIGHT,
				     Q8TK_PLACEMENT_Y_CENTER );
  }

  return vbox;
}

/*----------------------------------------------------------------------*/
							    /* ブースト */
static	int	get_cpu_boost( void )
{
  return	boost;
}
static	void	cb_cpu_boost( Q8tkWidget *widget, void *mode )
{
  int	i;
  const t_menudata *p = data_cpu_boost_combo;
  const char       *combo_str = q8tk_combo_get_text(widget);
  char	buf[16], *conv_end;
  int	new_boost = -1;

  for( i=0; i<COUNTOF(data_cpu_boost_combo); i++, p++ ){
    if( strcmp( p->str[menu_lang], combo_str )==0 ){
      new_boost = p->val;
      break;
    }
  }

  if( new_boost <= 0 ){
    strncpy( buf, combo_str, 15 );
    buf[15] = '\0';

    if( strlen(buf)==0 && (int)mode==0 ){
      new_boost = 1;
    }else{
      new_boost = strtoul( buf, &conv_end, 10 );
      if( new_boost == 0 && (int)mode==0 ){
	new_boost = 1;
      }else if( *conv_end != '\0' ){
	new_boost = -1;
      }
    }
  }

  if( 1 <= new_boost && new_boost <= 100 ){
    if( boost != new_boost ){
      boost_change( new_boost );
    }
  }

  if( (int)mode == 0 ){
    sprintf( buf, "%4d", get_cpu_boost() );
    q8tk_combo_set_text( widget, buf );
  }
}

static	Q8tkWidget	*menu_cpu_boost( void )
{
  Q8tkWidget	*hbox;
  char		buf[8];
  const t_menulabel *p = data_cpu_boost;

  hbox = PACK_HBOX( NULL );
  {
    PACK_LABEL( hbox, GET_LABEL( p, DATA_CPU_BOOST_MAGNIFY ) );

    sprintf( buf, "%4d", get_cpu_boost() );
    PACK_COMBO( hbox,
		data_cpu_boost_combo, COUNTOF(data_cpu_boost_combo),
		get_cpu_boost(), buf, 5,
		cb_cpu_boost, (void*)0,
		cb_cpu_boost, (void*)1 );

    PACK_LABEL( hbox, GET_LABEL( p, DATA_CPU_BOOST_UNIT ) );

    PACK_LABEL( hbox, GET_LABEL( p, DATA_CPU_BOOST_INFO ) );
  }

  return hbox;
}

/*----------------------------------------------------------------------*/
						      /* 各種設定の変更 */
static	int	get_cpu_misc( int type )
{
  switch( type ){
  case DATA_CPU_MISC_FDCWAIT:	return ( fdc_wait==0 ) ? FALSE : TRUE;
  case DATA_CPU_MISC_HSBASIC:	return highspeed_mode;
  case DATA_CPU_MISC_MEMWAIT:	return memory_wait;
  }
  return FALSE;
}
static	void	cb_cpu_misc( Q8tkWidget *widget, void *p )
{
  int	key = ( Q8TK_TOGGLE_BUTTON(widget)->active ) ? TRUE : FALSE;

  switch( (int)p ){
  case DATA_CPU_MISC_FDCWAIT: fdc_wait       = (key)? 1 : 0;        return;
  case DATA_CPU_MISC_HSBASIC: highspeed_mode = (key)? TRUE :FALSE;  return;
  case DATA_CPU_MISC_MEMWAIT: memory_wait    = (key)? TRUE :FALSE;  return;
  }
}


static	Q8tkWidget	*menu_cpu_misc( void )
{
  int	i;
  Q8tkWidget	*vbox, *l;
  const t_menudata *p = data_cpu_misc;

  vbox = PACK_VBOX( NULL );
  {						/* check_button, label ... */
    for( i=0; i<COUNTOF(data_cpu_misc); i++, p++ ){
      if( p->val >= 0 ){
	PACK_CHECK_BUTTON( vbox,
			   GET_LABEL( p, 0 ),
			   get_cpu_misc( p->val ),
			   cb_cpu_misc, (void *)( p->val ) );
      }else{
	l = PACK_LABEL( vbox, GET_LABEL( p, 0 ) );
	q8tk_misc_set_placement( l, Q8TK_PLACEMENT_X_RIGHT, 0 );
      }
    }
  }

  return vbox;
}

/*======================================================================*/

static	Q8tkWidget	*menu_cpu( void )
{
  Q8tkWidget *vbox, *hbox, *vbox2;
  Q8tkWidget *f;
  const t_menulabel *l = data_cpu;

  vbox = PACK_VBOX( NULL );
  {
    hbox = PACK_HBOX( vbox );
    {
      PACK_FRAME( hbox, GET_LABEL( l, DATA_CPU_CPU ), menu_cpu_cpu() );

      f = PACK_FRAME( hbox, "              ", menu_cpu_help() );
      q8tk_frame_set_shadow_type( f, Q8TK_SHADOW_NONE );
    }

    hbox = PACK_HBOX( vbox );
    {
      vbox2 = PACK_VBOX( hbox );
      {
	PACK_FRAME( vbox2, GET_LABEL( l, DATA_CPU_CLOCK ), menu_cpu_clock() );

	PACK_FRAME( vbox2, GET_LABEL( l, DATA_CPU_WAIT ), menu_cpu_wait() );

	PACK_FRAME( vbox2, GET_LABEL( l, DATA_CPU_BOOST ), menu_cpu_boost() );
      }

      f = PACK_FRAME( hbox, "", menu_cpu_misc() );
      q8tk_frame_set_shadow_type( f, Q8TK_SHADOW_NONE );
    }
  }

  return vbox;
}








/*===========================================================================
 *
 *	メインページ	画面
 *
 *===========================================================================*/

/*----------------------------------------------------------------------*/
						  /* フレームレート変更 */
static	int	get_graph_frate( void )
{
  return	frameskip_rate;
}
static	void	cb_graph_frate( Q8tkWidget *widget, void *label )
{
  int	i;
  const t_menudata *p = data_graph_frate;
  const char       *combo_str = q8tk_combo_get_text(widget);
  char  str[32];

  for( i=0; i<COUNTOF(data_graph_frate); i++, p++ ){
    if( strcmp( p->str[menu_lang], combo_str )==0 ){
      sprintf( str, " fps (-frameskip %d)", p->val );
      q8tk_label_set( (Q8tkWidget*)label, str );

      frameskip_rate = p->val;
      blink_ctrl_update();
      return;
    }
  }
}
						/* thanks floi ! */
static	int	get_graph_autoskip( void )
{
  return use_auto_skip;
}
static	void	cb_graph_autoskip( Q8tkWidget *widget, void *dummy )
{
  int	key = ( Q8TK_TOGGLE_BUTTON(widget)->active ) ? TRUE : FALSE;
  use_auto_skip = key;
}


static	Q8tkWidget	*menu_graph_frate( void )
{
  Q8tkWidget	*vbox, *hbox, *combo, *label;
  char		wk[32];

  vbox = PACK_VBOX( NULL );
  {
    hbox = PACK_HBOX( vbox );			/* combo, label */
    {
      label = q8tk_label_new(" fps");
      {
	sprintf( wk, "%6.3f", 60.0f / get_graph_frate() );
	combo = PACK_COMBO( hbox,
			    data_graph_frate, COUNTOF(data_graph_frate),
			    get_graph_frate(), wk, 6,
			    cb_graph_frate, label,
			    NULL, NULL );
      }
      {
	q8tk_box_pack_start( hbox, label );
	q8tk_widget_show( label );
	cb_graph_frate( combo, (void*)label );
      }
    }

    PACK_LABEL( vbox, "" );			/* 空行 */
							/* thanks floi ! */
    PACK_CHECK_BUTTON( vbox,			/* check_button */
		       GET_LABEL( data_graph_autoskip, 0 ),
		       get_graph_autoskip(),
		       cb_graph_autoskip, NULL );
  }

  return vbox;
}

/*----------------------------------------------------------------------*/
						  /* 画面サイズ切り替え */
static	int	get_graph_resize( void )
{
  return screen_size;
}
static	void	cb_graph_resize( Q8tkWidget *dummy, void *p )
{
  if( screen_size != (int)p &&
      (int)p <= screen_size_max ){
    screen_size = (int)p;

    quasi88_change_screen();
  }
}
static	int	get_graph_fullscreen( void )
{
  return use_fullscreen;
}
static	void	cb_graph_fullscreen( Q8tkWidget *widget, void *dummy )
{
  use_fullscreen = ( Q8TK_TOGGLE_BUTTON(widget)->active ) ? TRUE : FALSE;

  quasi88_change_screen();
  q8tk_set_cursor( set_mouse_state() ? FALSE : TRUE );
}


static	Q8tkWidget	*menu_graph_resize( void )
{
  Q8tkWidget	*vbox;
  int	i = COUNTOF(data_graph_resize);
  if( i > screen_size_max + 1 )
    i = screen_size_max + 1;

  vbox = PACK_VBOX( NULL );
  {						/* radio_button .... */
    PACK_RADIO_BUTTONS( PACK_HBOX( vbox ),	
			data_graph_resize, i,
			get_graph_resize(), cb_graph_resize );

    if( enable_fullscreen > 0 ){

      PACK_LABEL( vbox, "" );			/* 空行 */

      PACK_CHECK_BUTTON( vbox,			/* check_button */
			 GET_LABEL( data_graph_fullscreen, 0 ),
			 get_graph_fullscreen(),
			 cb_graph_fullscreen, NULL );
    }
  }

  return vbox;
}

/*----------------------------------------------------------------------*/
						      /* 各種設定の変更 */
static	int	get_graph_misc( int type )
{
  switch( type ){
  case DATA_GRAPH_MISC_HIDE_MOUSE: return hide_mouse;
  case DATA_GRAPH_MISC_15K:        return (monitor_15k==0x02) ? TRUE : FALSE;
  case DATA_GRAPH_MISC_DIGITAL:    return ( !monitor_analog ) ? TRUE : FALSE;
  case DATA_GRAPH_MISC_NOINTERP:   return ( !use_half_interp) ? TRUE : FALSE;
  }
  return FALSE;
}
static	void	cb_graph_misc( Q8tkWidget *widget, void *p )
{
  int	key = ( Q8TK_TOGGLE_BUTTON(widget)->active ) ? TRUE : FALSE;

  switch( (int)p ){
  case DATA_GRAPH_MISC_HIDE_MOUSE:hide_mouse     = key;                return;
  case DATA_GRAPH_MISC_15K:	  monitor_15k    = (key)? 0x02 : 0x00; return;
  case DATA_GRAPH_MISC_DIGITAL:   monitor_analog = (key)? FALSE: TRUE; return;

  case DATA_GRAPH_MISC_NOINTERP:  use_half_interp= (key)? FALSE: TRUE;
				  set_half_interp();		       return;
  }
}
static	int	get_graph_misc2( void )
{
  return use_interlace;
}
static	void	cb_graph_misc2( Q8tkWidget *button, void *p )
{
  use_interlace = (int)p;
}


static	Q8tkWidget	*menu_graph_misc( void )
{
  Q8tkWidget	*hbox, *vbox;
  const t_menudata *p = data_graph_misc;
  int i       = COUNTOF(data_graph_misc);

  if( have_mouse_cursor == FALSE ){ p++, i--; }

  hbox = PACK_HBOX( NULL );
  {
    PACK_LABEL( hbox, " " );

    vbox = PACK_VBOX( hbox );
    {						/* check_button ... */
      vbox = PACK_VBOX( vbox );
      PACK_CHECK_BUTTONS( vbox,
			  p, i,
			  get_graph_misc, cb_graph_misc );
      PACK_LABEL( vbox, " " );
						/* radio_button ... */
      PACK_RADIO_BUTTONS( vbox,
			  data_graph_misc2, COUNTOF(data_graph_misc2),
			  get_graph_misc2(), cb_graph_misc2 );
    }

    PACK_LABEL( hbox, " " );
  }
  return hbox;
}

/*----------------------------------------------------------------------*/
						     /* PCG有無切り替え */
static	int	get_graph_pcg( void )
{
  return use_pcg;
}
static	void	cb_graph_pcg( Q8tkWidget *dummy, void *p )
{
  use_pcg = (int)p;
  memory_set_font();
}


static	Q8tkWidget	*menu_graph_pcg( void )
{
  Q8tkWidget	*hbox;

  hbox = PACK_HBOX( NULL );
  {
    PACK_RADIO_BUTTONS( hbox,
			data_graph_pcg, COUNTOF(data_graph_pcg),
			get_graph_pcg(), cb_graph_pcg );
  }

  return hbox;
}

/*----------------------------------------------------------------------*/
						    /* フォント切り替え */
static	int	get_graph_font( void )
{
  return font_type;
}
static	void	cb_graph_font( Q8tkWidget *button, void *p )
{
  font_type = (int)p;
  memory_set_font();
}


static	Q8tkWidget	*menu_graph_font( void )
{
  Q8tkWidget	*vbox;
  t_menudata data_graph_font[3];

  data_graph_font[0] = data_graph_font1[ (font_loaded & 1) ? 1 : 0 ];
  data_graph_font[1] = data_graph_font2[ (font_loaded & 2) ? 1 : 0 ];
  data_graph_font[2] = data_graph_font3[ (font_loaded & 4) ? 1 : 0 ];

  vbox = PACK_VBOX( NULL );
  {						/* radio_button ... */
    PACK_RADIO_BUTTONS( vbox,
			data_graph_font, COUNTOF(data_graph_font),
			get_graph_font(), cb_graph_font );
  }

  return vbox;
}

/*======================================================================*/

static	Q8tkWidget	*menu_graph( void )
{
  Q8tkWidget *vbox, *hbox, *vbox2;
  Q8tkWidget *f;
  const t_menulabel *l = data_graph;

  vbox = PACK_VBOX( NULL );
  {
    PACK_FRAME( vbox, GET_LABEL( l, DATA_GRAPH_FRATE ), menu_graph_frate() );

    PACK_FRAME( vbox, GET_LABEL( l, DATA_GRAPH_RESIZE ), menu_graph_resize() );

    hbox = PACK_HBOX( vbox );
    {
      q8tk_box_pack_start( hbox, menu_graph_misc() ); /*フレームには乗せない*/

      vbox2 = PACK_VBOX( hbox );
      {
	PACK_FRAME( vbox2, GET_LABEL( l, DATA_GRAPH_PCG  ), menu_graph_pcg() );
	PACK_FRAME( vbox2, GET_LABEL( l, DATA_GRAPH_FONT ), menu_graph_font());
      }
    }
  }

  return vbox;
}








/*===========================================================================
 *
 *	メインページ	音量
 *
 *===========================================================================*/

/*----------------------------------------------------------------------*/
						      /* ボリューム変更 */
static	int	get_volume( int type )
{
  switch( type ){
  case VOL_TOTAL:  return  xmame_get_sound_volume();
  case VOL_FM:     return  xmame_get_mixer_volume( XMAME_MIXER_FM );
  case VOL_PSG:    return  xmame_get_mixer_volume( XMAME_MIXER_PSG );
  case VOL_BEEP:   return  xmame_get_mixer_volume( XMAME_MIXER_BEEP );
  case VOL_RHYTHM: return  xmame_get_mixer_volume( XMAME_MIXER_RHYTHM );
  case VOL_ADPCM:  return  xmame_get_mixer_volume( XMAME_MIXER_ADPCM );
  case VOL_FMPSG:  return  xmame_get_mixer_volume( XMAME_MIXER_FMPSG );
  }
  return 0;
}
static	void	cb_volume( Q8tkWidget *widget, void *p )
{
  int	vol = Q8TK_ADJUSTMENT( widget )->value;

  switch( (int)p ){
  case VOL_TOTAL:   xmame_set_sound_volume( vol );			break;
  case VOL_FM:      xmame_set_mixer_volume( XMAME_MIXER_FM, vol );	break;
  case VOL_PSG:     xmame_set_mixer_volume( XMAME_MIXER_PSG, vol );	break;
  case VOL_BEEP:    xmame_set_mixer_volume( XMAME_MIXER_BEEP, vol );	break;
  case VOL_RHYTHM:  xmame_set_mixer_volume( XMAME_MIXER_RHYTHM, vol );	break;
  case VOL_ADPCM:   xmame_set_mixer_volume( XMAME_MIXER_ADPCM, vol );	break;
  case VOL_FMPSG:   xmame_set_mixer_volume( XMAME_MIXER_FMPSG, vol );	break;
  }
}


INLINE	Q8tkWidget	*menu_volume_unit( const t_volume *p, int count )
{
  int	i;
  Q8tkWidget	*vbox, *hbox;

  vbox = PACK_VBOX( NULL );
  {
    for( i=0; i<count; i++, p++ ){

      hbox = PACK_HBOX( vbox );
      {
	PACK_LABEL( hbox, GET_LABEL( p, 0 ) );

	PACK_HSCALE( hbox,
		     p,
		     get_volume( p->val ),
		     cb_volume, (void*)( p->val ) );
      }
    }
  }

  return vbox;
}


static	Q8tkWidget	*menu_volume_total( void )
{
  return menu_volume_unit( data_volume_total, COUNTOF(data_volume_total) );
}
static	Q8tkWidget	*menu_volume_level( void )
{
  return menu_volume_unit( data_volume_level, COUNTOF(data_volume_level) );
}
static	Q8tkWidget	*menu_volume_rhythm( void )
{
  return menu_volume_unit( data_volume_rhythm, COUNTOF(data_volume_rhythm) );
}
static	Q8tkWidget	*menu_volume_fmgen( void )
{
  return menu_volume_unit( data_volume_fmgen, COUNTOF(data_volume_fmgen) );
}

/*----------------------------------------------------------------------*/
					    /* サウンドなし時メッセージ */

static	Q8tkWidget	*menu_volume_no_available( void )
{
  int type;
  Q8tkWidget	*l;

#ifdef	USE_SOUND
  type = 2;
#else
  type = 0;
#endif

  if( sound_board==SOUND_II ){
    type |= 1;
  }

  if( 0 <= type && type <= 3 ){
    l = q8tk_label_new( GET_LABEL( data_volume_no, type ) );
  }else{
    l = q8tk_label_new( "???" );
  }

  q8tk_widget_show( l );

  return l;
}

/*----------------------------------------------------------------------*/
					    /* サウンドドライバ種別表示 */

static	Q8tkWidget	*menu_volume_type( void )
{
  int type;
  Q8tkWidget	*l;

#if	defined(USE_SOUND) && defined(USE_FMGEN)
  if( use_fmgen ){
    type = 2;
  }else
#endif
#if	defined(USE_SOUND)
  {
    type = 0;
  }
#else
  type = -1;
#endif

  if( sound_board==SOUND_II ){
    type |= 1;
  }

  if( 0 <= type && type <= 3 ){
    l = q8tk_label_new( GET_LABEL( data_volume_type, type ) );
  }else{
    l = q8tk_label_new( "???" );
  }

  q8tk_widget_show( l );

  return l;
}

/*----------------------------------------------------------------------*/

static	Q8tkWidget	*menu_volume( void )
{
  Q8tkWidget *vbox;
  Q8tkWidget *w;
  const t_menulabel *l = data_volume;

  if( xmame_sound_is_enable()==FALSE ){

    w = PACK_FRAME( NULL, "", menu_volume_no_available() );
    q8tk_frame_set_shadow_type( w, Q8TK_SHADOW_ETCHED_OUT );

    return w;
  }

  vbox = PACK_VBOX( NULL );
  {
    w = PACK_FRAME( vbox, "", menu_volume_type() );
    q8tk_frame_set_shadow_type( w, Q8TK_SHADOW_ETCHED_OUT );

    if( xmame_volume_is_variable() ){
      PACK_FRAME( vbox, GET_LABEL(l, DATA_VOLUME_TOTAL), menu_volume_total() );
    }

#if	defined(USE_SOUND) && defined(USE_FMGEN)

    if( use_fmgen ) w = menu_volume_fmgen();
    else            w = menu_volume_level();
    PACK_FRAME( vbox, GET_LABEL( l, DATA_VOLUME_LEVEL ), w );

    if( use_fmgen ){
      ;
    }else if( sound_board==SOUND_II ){
      PACK_FRAME( vbox, GET_LABEL( l, DATA_VOLUME_SD2 ), menu_volume_rhythm());
    }

#else

    PACK_FRAME( vbox, GET_LABEL( l, DATA_VOLUME_LEVEL ), menu_volume_level() );

    if( sound_board==SOUND_II ){
      PACK_FRAME( vbox, GET_LABEL( l, DATA_VOLUME_SD2 ), menu_volume_rhythm());
    }

#endif
  }

  return vbox;
}




/*===========================================================================
 *
 *	メインページ	ディスク
 *
 *===========================================================================*/

/*----------------------------------------------------------------------*/

typedef struct{
  Q8tkWidget	*list;			/* イメージ一覧のリスト	*/
  Q8tkWidget	*button[2];		/* ボタンと		*/
  Q8tkWidget	*label[2];		/* そのラベル (2個)	*/
  int		func[2];		/* ボタンの機能 IMG_xxx	*/
  Q8tkWidget	*stat_label;		/* 情報 - Busy/Ready	*/
  Q8tkWidget	*attr_label;		/* 情報 - RO/RW属性	*/
  Q8tkWidget	*num_label;		/* 情報 - イメージ数	*/
} T_DISK_INFO;

static	T_DISK_INFO	disk_info[2];	/* 2ドライブ分のワーク	*/

static	char		disk_filename[ QUASI88_MAX_FILENAME ];

static	int		disk_drv;	/* 操作するドライブの番号 */
static	int		disk_img;	/* 操作するイメージの番号 */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static	void	set_disk_widget( void );


/* BOOT from DISK で、DISK を CLOSE した時や、
   BOOT from ROM  で、DISK を OPEN した時は、 DIP-SW 設定を強制変更 */
static	void	disk_update_dipsw_b_boot( void )
{
  if( disk_image_exist(0) || disk_image_exist(1) ){
    q8tk_toggle_button_set_state( widget_dipsw_b_boot_disk, TRUE );
  }else{
    q8tk_toggle_button_set_state( widget_dipsw_b_boot_rom,  TRUE );
  }
  set_reset_dipsw_boot();

  /* リセットしないでメニューモードを抜けると設定が保存されないので・・・ */
  boot_from_rom = menu_boot_from_rom;			/* thanks floi ! */
}


/*----------------------------------------------------------------------*/
/* 属性変更の各種処理							*/

enum {
  ATTR_RENAME,		/* drive[drv] のイメージ img をリネーム		*/
  ATTR_PROTECT,		/* drive[drv] のイメージ img をプロテクト	*/
  ATTR_FORMAT,		/* drive[drv] のイメージ img をアンフォーマット	*/
  ATTR_UNFORMAT,	/* drive[drv] のイメージ img をフォーマット	*/
  ATTR_APPEND,		/* drive[drv] に最後にイメージを追加		*/
  ATTR_CREATE		/* 新規にディスクイメージファイルを作成		*/
};

static	void	sub_disk_attr_file_ctrl( int drv, int img, int cmd, char *c )
{
  int	ro = FALSE;
  int	result = -1;
  OSD_FILE *fp;


  if( cmd != ATTR_CREATE ){		/* ドライブのファイルを変更する場合 */

    fp = drive[ drv ].fp;			/* そのファイルポインタを取得*/
    if( drive[ drv ].read_only ){
      ro = TRUE;
    }

  }else{				/* 別のファイルを更新する場合 */

    fp = osd_fopen( FTYPE_DISK, c, "r+b" );		/* "r+b" でオープン */
    if( fp == NULL ){
      fp = osd_fopen( FTYPE_DISK, c, "rb" );		/* "rb" でオープン */
      if( fp ) ro = TRUE;
    }

    if( fp ){						/* オープンできたら */
      if     ( fp == drive[ 0 ].fp ) drv = 0;		/* すでにドライブに */
      else if( fp == drive[ 1 ].fp ) drv = 1;		/* 開いてないかを   */
      else                           drv = -1;		/* チェックする     */
    }
    else{						/* オープンできない */
      fp = osd_fopen( FTYPE_DISK, c, "ab" );		/* 時は、新規に作成 */
      drv = -1;
    }

  }


  if( fp == NULL ){			/* オープン失敗 */
    start_file_error_dialog( drv, ERR_CANT_OPEN );
    return;
  }
  else if( ro ){			/* リードオンリーなので処理不可 */
    if( drv < 0 ) osd_fclose( fp );
    if( cmd != ATTR_CREATE ) start_file_error_dialog( drv, ERR_READ_ONLY );
    else                     start_file_error_dialog(  -1, ERR_READ_ONLY );
    return;
  }
  else if( drv>=0 &&			/* 壊れたイメージが含まれるので不可 */
	   drive[ drv ].detect_broken_image ){
    start_file_error_dialog( drv, ERR_MAYBE_BROKEN );
    return;
  }


#if 0
  if( cmd==ATTR_CREATE || cmd==ATTR_APPEND ){
    /* この処理に時間がかかるような場合、メッセージをだす？？ */
    /* この処理がそんなに時間がかかることはない？？ */
  }
#endif

		/* 開いたファイルに対して、処理 */

  switch( cmd ){
  case ATTR_RENAME:	result = d88_write_name( fp, drv, img, c );	break;
  case ATTR_PROTECT:	result = d88_write_protect( fp, drv, img, c );	break;
  case ATTR_FORMAT:	result = d88_write_format( fp, drv, img );	break;
  case ATTR_UNFORMAT:	result = d88_write_unformat( fp, drv, img );	break;
  case ATTR_APPEND:
  case ATTR_CREATE:	result = d88_append_blank( fp, drv );		break;
  }

		/* その結果 */

  switch( result ){
  case D88_SUCCESS:	result = ERR_NO;			break;
  case D88_NO_IMAGE:	result = ERR_MAYBE_BROKEN;		break;
  case D88_BAD_IMAGE:	result = ERR_MAYBE_BROKEN;		break;
  case D88_ERR_READ:	result = ERR_MAYBE_BROKEN;		break;
  case D88_ERR_SEEK:	result = ERR_SEEK;			break;
  case D88_ERR_WRITE:	result = ERR_WRITE;			break;
  default:		result = ERR_UNEXPECTED;		break;
  }

		/* 終了処理。なお、エラー時はメッセージを出す */

  if( drv < 0 ){		/* 新規オープンしたファイルを更新した場合 */
    osd_fclose( fp );			/* ファイルを閉じて終わり	  */

  }else{			/* ドライブのファイルを更新した場合	  */
    if( result == ERR_NO ){		/* メニュー画面を更新せねば	  */
      set_disk_widget();
      if( cmd != ATTR_CREATE ) disk_update_dipsw_b_boot();
    }
  }

  if( result != ERR_NO ){
    start_file_error_dialog( drv, result );
  }

  return;
}

/*----------------------------------------------------------------------*/
/* 「リネーム」ダイアログ						*/

static	void	cb_disk_attr_rename_activate( Q8tkWidget *dummy, void *p )
{
  char	wk[16 + 1];

  if( (int)p ){			/* dialog_destroy() の前にエントリをゲット */
    strncpy( wk, dialog_get_entry(), 16 );
    wk[16] = '\0';
  }

  dialog_destroy();

  if( (int)p ){
    sub_disk_attr_file_ctrl( disk_drv, disk_img, ATTR_RENAME, wk );
  }
}
static	void	sub_disk_attr_rename( const char *image_name )
{
  int save_code;
  const t_menulabel *l = data_disk_attr_rename;


  dialog_create();
  {
    dialog_set_title( GET_LABEL( l, DATA_DISK_ATTR_RENAME_TITLE1 +disk_drv ) );

    save_code = q8tk_set_kanjicode( Q8TK_KANJI_SJIS );
    dialog_set_title( image_name );
    q8tk_set_kanjicode( save_code );

    dialog_set_title( GET_LABEL( l, DATA_DISK_ATTR_RENAME_TITLE2 ) );

    dialog_set_separator();

    save_code = q8tk_set_kanjicode( Q8TK_KANJI_SJIS );
    dialog_set_entry( drive[disk_drv].image[disk_img].name,
		      16,
		      cb_disk_attr_rename_activate, (void*)TRUE );
    q8tk_set_kanjicode( save_code );

    dialog_set_button( GET_LABEL( l, DATA_DISK_ATTR_RENAME_OK ),
		       cb_disk_attr_rename_activate, (void*)TRUE );

    dialog_set_button( GET_LABEL( l, DATA_DISK_ATTR_RENAME_CANCEL ),
		       cb_disk_attr_rename_activate, (void*)FALSE );

    dialog_accel_key( Q8TK_KEY_ESC );
  }
  dialog_start();
}

/*----------------------------------------------------------------------*/
/* 「プロテクト」ダイアログ						*/

static	void	cb_disk_attr_protect_clicked( Q8tkWidget *dummy, void *p )
{
  char	c;

  dialog_destroy();

  if( (int)p ){
    if( (int)p == 1 ) c = DISK_PROTECT_TRUE;
    else              c = DISK_PROTECT_FALSE;

    sub_disk_attr_file_ctrl( disk_drv, disk_img, ATTR_PROTECT, &c );
  }
}
static	void	sub_disk_attr_protect( const char *image_name )
{
  int save_code;
  const t_menulabel *l = data_disk_attr_protect;

  dialog_create();
  {
    dialog_set_title( GET_LABEL( l, DATA_DISK_ATTR_PROTECT_TITLE1 +disk_drv ));

    save_code = q8tk_set_kanjicode( Q8TK_KANJI_SJIS );
    dialog_set_title( image_name );
    q8tk_set_kanjicode( save_code );

    dialog_set_title( GET_LABEL( l, DATA_DISK_ATTR_PROTECT_TITLE2 ) );

    dialog_set_separator();

    dialog_set_button( GET_LABEL( l, DATA_DISK_ATTR_PROTECT_SET ),
		       cb_disk_attr_protect_clicked, (void*)1 );

    dialog_set_button( GET_LABEL( l, DATA_DISK_ATTR_PROTECT_UNSET ),
		       cb_disk_attr_protect_clicked, (void*)2 );

    dialog_set_button( GET_LABEL( l, DATA_DISK_ATTR_PROTECT_CANCEL ),
		       cb_disk_attr_protect_clicked, (void*)0 );

    dialog_accel_key( Q8TK_KEY_ESC );
  }
  dialog_start();
}

/*----------------------------------------------------------------------*/
/* 「フォーマット」ダイアログ						*/

static	void	cb_disk_attr_format_clicked( Q8tkWidget *dummy, void *p )
{
  dialog_destroy();

  if( (int)p ){
    if( (int)p == 1 )
      sub_disk_attr_file_ctrl( disk_drv, disk_img, ATTR_FORMAT,   NULL );
    else 
      sub_disk_attr_file_ctrl( disk_drv, disk_img, ATTR_UNFORMAT, NULL );
  }
}
static	void	sub_disk_attr_format( const char *image_name )
{
  int save_code;
  const t_menulabel *l = data_disk_attr_format;

  dialog_create();
  {
    dialog_set_title( GET_LABEL( l, DATA_DISK_ATTR_FORMAT_TITLE1 +disk_drv ) );

    save_code = q8tk_set_kanjicode( Q8TK_KANJI_SJIS );
    dialog_set_title( image_name );
    q8tk_set_kanjicode( save_code );

    dialog_set_title( GET_LABEL( l, DATA_DISK_ATTR_FORMAT_TITLE2 ) );

    dialog_set_title( GET_LABEL( l, DATA_DISK_ATTR_FORMAT_WARNING ) );

    dialog_set_separator();

    dialog_set_button( GET_LABEL( l, DATA_DISK_ATTR_FORMAT_DO ),
		       cb_disk_attr_format_clicked, (void*)1 );

    dialog_set_button( GET_LABEL( l, DATA_DISK_ATTR_FORMAT_NOT ),
		       cb_disk_attr_format_clicked, (void*)2 );

    dialog_set_button( GET_LABEL( l, DATA_DISK_ATTR_FORMAT_CANCEL ),
		       cb_disk_attr_format_clicked, (void*)0 );

    dialog_accel_key( Q8TK_KEY_ESC );
  }
  dialog_start();
}

/*----------------------------------------------------------------------*/
/* 「ブランクディスク」ダイアログ					*/

static	void	cb_disk_attr_blank_clicked( Q8tkWidget *dummy, void *p )
{
  dialog_destroy();

  if( (int)p ){
    sub_disk_attr_file_ctrl( disk_drv, disk_img, ATTR_APPEND, NULL );
  }
}
static	void	sub_disk_attr_blank( void )
{
  const t_menulabel *l = data_disk_attr_blank;

  dialog_create();
  {
    dialog_set_title( GET_LABEL( l, DATA_DISK_ATTR_BLANK_TITLE1 +disk_drv ) );

    dialog_set_title( GET_LABEL( l, DATA_DISK_ATTR_BLANK_TITLE2 ) );

    dialog_set_separator();

    dialog_set_button( GET_LABEL( l, DATA_DISK_ATTR_BLANK_OK ),
		       cb_disk_attr_blank_clicked, (void*)TRUE );

    dialog_set_button( GET_LABEL( l, DATA_DISK_ATTR_BLANK_CANCEL ),
		       cb_disk_attr_blank_clicked, (void*)FALSE );

    dialog_accel_key( Q8TK_KEY_ESC );
  }
  dialog_start();
}

/*----------------------------------------------------------------------*/
/* 「属性変更」 ボタン押下時の処理  -  詳細選択のダイアログを開く	*/

static char disk_attr_image_name[20];
static	void	cb_disk_attr_clicked( Q8tkWidget *dummy, void *p )
{
  char *name = disk_attr_image_name;

  dialog_destroy();

  switch( (int)p ){
  case DATA_DISK_ATTR_RENAME:	sub_disk_attr_rename( name );	break;
  case DATA_DISK_ATTR_PROTECT:	sub_disk_attr_protect( name );	break;
  case DATA_DISK_ATTR_FORMAT:	sub_disk_attr_format( name );	break;
  case DATA_DISK_ATTR_BLANK:	sub_disk_attr_blank();		break;
  }
}


static void sub_disk_attr( void )
{
  int save_code;
  const t_menulabel *l = data_disk_attr;

  sprintf( disk_attr_image_name,		/* イメージ名をセット */
	   "\"%-16s\"", drive[disk_drv].image[disk_img].name );

  dialog_create();
  {
    dialog_set_title( GET_LABEL( l, DATA_DISK_ATTR_TITLE1 +disk_drv ) );

    save_code = q8tk_set_kanjicode( Q8TK_KANJI_SJIS );
    dialog_set_title( disk_attr_image_name );
    q8tk_set_kanjicode( save_code );

    dialog_set_title( GET_LABEL( l, DATA_DISK_ATTR_TITLE2 ) );

    dialog_set_separator();

    dialog_set_button( GET_LABEL( l, DATA_DISK_ATTR_RENAME ),
		       cb_disk_attr_clicked, (void*)DATA_DISK_ATTR_RENAME );

    dialog_set_button( GET_LABEL( l, DATA_DISK_ATTR_PROTECT ),
		       cb_disk_attr_clicked, (void*)DATA_DISK_ATTR_PROTECT );

    dialog_set_button( GET_LABEL( l, DATA_DISK_ATTR_FORMAT ),
		       cb_disk_attr_clicked, (void*)DATA_DISK_ATTR_FORMAT );

    dialog_set_button( GET_LABEL( l, DATA_DISK_ATTR_BLANK ),
		       cb_disk_attr_clicked, (void*)DATA_DISK_ATTR_BLANK );

    dialog_set_button( GET_LABEL( l, DATA_DISK_ATTR_CANCEL ),
		       cb_disk_attr_clicked, (void*)DATA_DISK_ATTR_CANCEL );

    dialog_accel_key( Q8TK_KEY_ESC );
  }
  dialog_start();
}




/*----------------------------------------------------------------------*/
/* 「イメージファイルを開く」 ボタン押下時の処理			*/

static	int	disk_open_ro;
static	int	disk_open_cmd;
static void sub_disk_open_ok( void );

static void sub_disk_open( int cmd )
{
  const char *initial;
  int	num;
  const t_menulabel *l = (disk_drv == 0) ? data_disk_open_drv1
					 : data_disk_open_drv2;

  disk_open_cmd = cmd;
  num = (cmd==IMG_OPEN) ? DATA_DISK_OPEN_OPEN : DATA_DISK_OPEN_BOTH;


  if     ( file_disk[disk_drv  ][0] != '\0' ) initial = file_disk[disk_drv  ];
  else if( file_disk[disk_drv^1][0] != '\0' ) initial = file_disk[disk_drv^1];
  else{
    initial = osd_dir_disk();
    if( initial==NULL ) initial = osd_dir_cwd();
  }


  START_FILE_SELECTION( GET_LABEL( l, num ),
			(menu_readonly) ? 1 : 0,    /* ReadOnly の選択が可 */
			initial,

			sub_disk_open_ok,
			disk_filename,
			QUASI88_MAX_FILENAME,
			&disk_open_ro );
}

static void sub_disk_open_ok( void )
{
  if( disk_open_cmd == IMG_OPEN ){

    if( quasi88_disk_insert( disk_drv, disk_filename, 0, disk_open_ro )
	== FALSE ){
      start_file_error_dialog( disk_drv, ERR_CANT_OPEN );
    }
    else{
      if( disk_same_file() ){		/* 反対側と同じファイルだった場合 */
	int dst = disk_drv;
	int src = disk_drv^1;
	int img;

	if( drive[ src ].empty ){		/* 反対側ドライブ 空なら */
	  img = 0;				/*        最初のイメージ */
	}else{
	  if( disk_image_num( src ) == 1 ){	/* イメージが1個の場合は */
	    img = -1;				/*        ドライブ 空に  */

	  }else{				/* イメージが複数あれば  */
						/*        次(前)イメージ */
	    img = disk_image_selected( src ) + ((dst==DRIVE_1) ? -1 : +1);
	    if( (img < 0) || 
		(disk_image_num( dst ) -1 < img) ) img = -1;
	  }
	}
	if( img < 0 ) drive_set_empty( dst );
	else          disk_change_image( dst, img );
      }
    }

  }else{	/*   IMG_BOTH */

    if( quasi88_disk_insert_all( disk_filename, disk_open_ro ) == FALSE ){

      disk_drv = 0;
      start_file_error_dialog( disk_drv, ERR_CANT_OPEN );

    }

  }

  if( filename_synchronize ){
    sub_misc_suspend_change();
    sub_misc_snapshot_change();
  }
  set_disk_widget();
  disk_update_dipsw_b_boot();
}

/*----------------------------------------------------------------------*/
/* 「イメージファイルを閉じる」 ボタン押下時の処理			*/

static void sub_disk_close( void )
{
  quasi88_disk_eject( disk_drv );

  if( filename_synchronize ){
    sub_misc_suspend_change();
    sub_misc_snapshot_change();
  }
  set_disk_widget();
  disk_update_dipsw_b_boot();
}

/*----------------------------------------------------------------------*/
/* 「反対ドライブと同じファイルを開く」 ボタン押下時の処理		*/

static void sub_disk_copy( void )
{
  int	dst = disk_drv;
  int	src = disk_drv^1;
  int	img;

  if( ! disk_image_exist( src ) ) return;

  if( drive[ src ].empty ){			/* 反対側ドライブ 空なら */
    img = 0;					/*        最初のイメージ */
  }else{
    if( disk_image_num( src ) == 1 ){		/* イメージが1個の場合は */
      img = -1;					/*        ドライブ 空に  */

    }else{					/* イメージが複数あれば  */
						/*        次(前)イメージ */
      img = disk_image_selected( src ) + ((dst==DRIVE_1) ? -1 : +1);
      if( (img < 0) || 
	  (disk_image_num( dst ) -1 < img) ) img = -1;
    }
  }

  if( quasi88_disk_insert_A_to_B( src, dst, img ) == FALSE ){
    start_file_error_dialog( disk_drv, ERR_CANT_OPEN );
  }

  if( filename_synchronize ){
    sub_misc_suspend_change();
    sub_misc_snapshot_change();
  }
  set_disk_widget();
  disk_update_dipsw_b_boot();
}





/*----------------------------------------------------------------------*/
/* イメージのリストアイテム選択時の、コールバック関数			*/

static	void	cb_disk_image( Q8tkWidget *dummy, void *p )
{
  int	drv = ( (int)p ) & 0xff;
  int	img = ( (int)p ) >> 8;

  if( img < 0 ){			/* img==-1 で <<なし>> */
    drive_set_empty( drv );
  }else{				/* img>=0 なら イメージ番号 */
    drive_unset_empty( drv );
    disk_change_image( drv, img );
  }
}

/*----------------------------------------------------------------------*/
/* ドライブ毎に存在するボタンの、コールバック関数			*/

static	void	cb_disk_button( Q8tkWidget *dummy, void *p )
{
  int	drv    = ( (int)p ) & 0xff;
  int	button = ( (int)p ) >> 8;

  disk_drv = drv;
  disk_img = disk_image_selected( drv );

  switch( disk_info[drv].func[button] ){
  case IMG_OPEN:
  case IMG_BOTH:
    sub_disk_open( disk_info[drv].func[button] );
    break;
  case IMG_CLOSE:
    sub_disk_close();
    break;
  case IMG_COPY:
    sub_disk_copy();
    break;
  case IMG_ATTR:
    if( ! drive_check_empty( drv ) ){	     /* イメージ<<なし>>選択時は無効 */
      sub_disk_attr();
    }
    break;
  }
}

/*----------------------------------------------------------------------*/
/* ファイルを開く毎に、disk_info[] に情報をセット			*/
/*		(イメージのリスト生成、ボタン・情報のラベルをセット)	*/

static	void	set_disk_widget( void )
{
  int	i, drv, save_code;
  Q8tkWidget *item;
  T_DISK_INFO	*w;
  const t_menulabel *inf = data_disk_info;
  const t_menulabel *l   = data_disk_image;
  const t_menulabel *btn;
  char	wk[40], wk2[20];
  const char *s;


  for( drv=0; drv<2; drv++ ){
    w   = &disk_info[drv];

    if( menu_swapdrv ){
      btn = (drv==0) ? data_disk_button_drv1swap : data_disk_button_drv2swap;
    }else{
      btn = (drv==0) ? data_disk_button_drv1 : data_disk_button_drv2;
    }

		/* イメージ名の LIST ITEM 生成 */

    q8tk_listbox_clear_items( w->list, 0, -1 );

    item = q8tk_list_item_new_with_label( GET_LABEL(l, DATA_DISK_IMAGE_EMPTY));
    q8tk_widget_show( item );
    q8tk_container_add( w->list, item );		/* <<なし>> ITEM */
    q8tk_signal_connect( item, "select",
			 cb_disk_image, (void *)( (-1 <<8) +drv ) );

    if( disk_image_exist( drv ) ){		/* ---- ディスク挿入済 ---- */
      save_code = q8tk_set_kanjicode( Q8TK_KANJI_SJIS );
      {
	for( i=0; i<disk_image_num(drv); i++ ){
	  sprintf( wk, "%3d  %-16s  %s ",	/* イメージNo イメージ名 RW */
		   i+1,
		   drive[drv].image[i].name,
		   (drive[drv].image[i].protect) ? "RO" : "RW" );

	  item = q8tk_list_item_new_with_label( wk );
	  q8tk_widget_show( item );
	  q8tk_container_add( w->list, item );
	  q8tk_signal_connect( item, "select",
			       cb_disk_image, (void *)( (i<<8) +drv ) );
	}
      }
      q8tk_set_kanjicode( save_code );

				/* <<なし>> or 選択image の ITEM をセレクト */
      if( drive_check_empty( drv ) ) i = 0;
      else                           i = disk_image_selected( drv ) + 1;
      q8tk_listbox_select_item( w->list, i );

    }else{					/* ---- ドライブ空っぽ ---- */
      q8tk_listbox_select_item( w->list, 0 );		    /* <<なし>> ITEM */
    }

		/* ボタンの機能 「閉じる」「属性変更」 / 「開く」「開く」 */

    if( disk_image_exist( drv ) ){
      w->func[0] = IMG_CLOSE;
      w->func[1] = IMG_ATTR;
    }else{
      w->func[0] = ( disk_image_exist( drv^1 ) ) ? IMG_COPY : IMG_BOTH;
      w->func[1] = IMG_OPEN;
    }
    q8tk_label_set( w->label[0], GET_LABEL( btn, w->func[0] ) );
    q8tk_label_set( w->label[1], GET_LABEL( btn, w->func[1] ) );

		/* 情報 - Busy/Ready */

    if( get_drive_ready(drv) ) s = GET_LABEL( inf, DATA_DISK_INFO_STAT_READY );
    else                       s = GET_LABEL( inf, DATA_DISK_INFO_STAT_BUSY  );
    q8tk_label_set( w->stat_label, s );
    q8tk_label_set_reverse( w->stat_label,	/* BUSYなら反転表示 */
			  ( get_drive_ready(drv) ) ? FALSE : TRUE );

		/* 情報 - RO/RW属性 */

    if( disk_image_exist( drv ) ){
      if( drive[drv].read_only ) s = GET_LABEL( inf, DATA_DISK_INFO_ATTR_RO );
      else                       s = GET_LABEL( inf, DATA_DISK_INFO_ATTR_RW );
    }else{
      s = "";
    }
    q8tk_label_set( w->attr_label, s );
    q8tk_label_set_color( w->attr_label,	/* ReadOnlyなら赤色表示 */
			  ( drive[drv].read_only ) ? Q8GR_PALETTE_RED : -1 );

		/* 情報 - イメージ数 */

    if( disk_image_exist( drv ) ){
      if( drive[drv].detect_broken_image ){		/* 破損あり */
	s = GET_LABEL( inf, DATA_DISK_INFO_NR_BROKEN );
      }else
      if( drive[drv].over_image ||			/* イメージ多過ぎ */
	  disk_image_num( drv ) > 99 ){
	s = GET_LABEL( inf, DATA_DISK_INFO_NR_OVER );
      }else{
	s = "";
      }
      sprintf( wk, "%2d%s",
	       (disk_image_num(drv)>99) ? 99 : disk_image_num(drv), s );
      sprintf( wk2, "%9.9s", wk );			/* 9文字右詰めに変換 */
    }else{
      wk2[0] = '\0';
    }
    q8tk_label_set( w->num_label,  wk2 );
  }
}


/*----------------------------------------------------------------------*/
/* 「ブランク作成」 ボタン押下時の処理					*/

static	void	sub_disk_blank_ok( void );
static	void	cb_disk_blank_warn_clicked( Q8tkWidget *, void * );


static	void	cb_disk_blank( Q8tkWidget *dummy_0, void *dummy_1 )
{
  const char *initial;
  const t_menulabel *l = data_disk_blank;

  if     ( file_disk[DRIVE_1][0] != '\0' ) initial = file_disk[DRIVE_1];
  else if( file_disk[DRIVE_2][0] != '\0' ) initial = file_disk[DRIVE_2];
  else{
    initial = osd_dir_disk();
    if( initial==NULL ) initial = osd_dir_cwd();
  }


  START_FILE_SELECTION( GET_LABEL( l, DATA_DISK_BLANK_FSEL ),
			-1,	/* ReadOnly の選択は不可 */
			initial,

			sub_disk_blank_ok,
			disk_filename,
			QUASI88_MAX_FILENAME,
			NULL );
}

static	void	sub_disk_blank_ok( void )
{
  const t_menulabel *l = data_disk_blank;

  switch( osd_file_stat( disk_filename ) ){

  case FILE_STAT_NOEXIST:
    /* ファイルを新規に作成し、ブランクを作成 */
    sub_disk_attr_file_ctrl( 0, 0, ATTR_CREATE, disk_filename );
    break;

  case FILE_STAT_DIR:
    /* ディレクトリなので、ブランクは追加できない */
    start_file_error_dialog( -1, ERR_CANT_OPEN );
    break;

  default:
    /* すでにファイルが存在します。ブランクを追加しますか？ */
    dialog_create();
    {
      dialog_set_title( GET_LABEL( l, DATA_DISK_BLANK_WARN_0 ) );

      dialog_set_title( GET_LABEL( l, DATA_DISK_BLANK_WARN_1 ) );

      dialog_set_separator();

      dialog_set_button( GET_LABEL( l, DATA_DISK_BLANK_WARN_APPEND ),
			 cb_disk_blank_warn_clicked, (void*)TRUE );

      dialog_set_button( GET_LABEL( l, DATA_DISK_BLANK_WARN_CANCEL ),
			 cb_disk_blank_warn_clicked, (void*)FALSE );

      dialog_accel_key( Q8TK_KEY_ESC );
    }
    dialog_start();
    break;
  }
}

static	void	cb_disk_blank_warn_clicked( Q8tkWidget *dummy, void *p )
{
  dialog_destroy();

  if( (int)p ){
    /* ファイルに、ブランクを追記 */
    sub_disk_attr_file_ctrl( 0, 0, ATTR_CREATE, disk_filename );
  }
}

/*----------------------------------------------------------------------*/
/* 「ファイル名確認」 ボタン押下時の処理				*/

static	void	cb_disk_fname_dialog_ok( Q8tkWidget *dummy_0, void *dummy_1 )
{
  dialog_destroy();
}

static	void	cb_disk_fname( Q8tkWidget *dummy, void *p )
{
  const t_menulabel *l = data_disk_fname;
  char filename[72];
  int save_code;
  size_t len;
  const char *none = "(No Image File)";

  dialog_create();
  {
    dialog_set_title( GET_LABEL( l, DATA_DISK_FNAME_TITEL ) );
    dialog_set_title( GET_LABEL( l, DATA_DISK_FNAME_LINE ) );

    {
      save_code = q8tk_set_kanjicode( osd_kanji_code() );

      len = strlen( none );
      if( file_disk[0][0] ) len = MAX( len, strlen( file_disk[0] ) );
      if( file_disk[1][0] ) len = MAX( len, strlen( file_disk[1] ) );
      len = MIN( len, sizeof(filename)-1 - 5 );
      len += 5;					/* 5 == strlen("[1:] ") */

      strcpy( filename, "[1:] " );
      strncat( filename, ( file_disk[0][0] ) ? file_disk[0] : none,
	       len - strlen(filename) );
      while( strlen(filename) < len ){ strcat( filename, " " ); }
      dialog_set_title( filename );

      strcpy( filename, "[2:] " );
      strncat( filename, ( file_disk[1][0] ) ? file_disk[1] : none,
	       len - strlen(filename) );
      while( strlen(filename) < len ){ strcat( filename, " " ); }
      dialog_set_title( filename );

      q8tk_set_kanjicode( save_code );
    }

    if( disk_image_exist(0) && disk_same_file() ){
      dialog_set_title( GET_LABEL( l, DATA_DISK_FNAME_SAME ) );
    }

    if( (disk_image_exist(0) && drive[0].read_only ) ||
	(disk_image_exist(1) && drive[1].read_only ) ){
      dialog_set_title( GET_LABEL( l, DATA_DISK_FNAME_SEP ) );
      dialog_set_title( GET_LABEL( l, DATA_DISK_FNAME_RO ) );

      if( fdc_ignore_readonly == FALSE ){
	dialog_set_title( GET_LABEL( l, DATA_DISK_FNAME_RO_1 ) );
	dialog_set_title( GET_LABEL( l, DATA_DISK_FNAME_RO_2 ) );
      }else{
	dialog_set_title( GET_LABEL( l, DATA_DISK_FNAME_RO_X ) );
	dialog_set_title( GET_LABEL( l, DATA_DISK_FNAME_RO_Y ) );
      }
    }


    dialog_set_separator();

    dialog_set_button( GET_LABEL( l, DATA_DISK_FNAME_OK ),
		       cb_disk_fname_dialog_ok, NULL );

    dialog_accel_key( Q8TK_KEY_ESC );
  }
  dialog_start();
}


/*======================================================================*/

static	Q8tkWidget	*menu_disk( void )
{
  Q8tkWidget	*hbox, *vbox, *swin, *lab;
  Q8tkWidget	*f, *vx, *hx;
  T_DISK_INFO	*w;
  int	i,j,k;
  const t_menulabel *l;


  hbox = PACK_HBOX( NULL );
  {
    for( k=0; k<COUNTOF(disk_info); k++ ){

      if( menu_swapdrv ){ i = k ^ 1; }
      else              { i = k;     }

      w = &disk_info[i];
      {
	vbox = PACK_VBOX( hbox );
	{
	  lab = PACK_LABEL( vbox, GET_LABEL( data_disk_image_drive, i ) );

	  if( menu_swapdrv )
	    q8tk_misc_set_placement( lab, Q8TK_PLACEMENT_X_RIGHT, 0 );

	  {
	    swin  = q8tk_scrolled_window_new( NULL, NULL );
	    q8tk_widget_show( swin );
	    q8tk_scrolled_window_set_policy( swin, Q8TK_POLICY_NEVER,
					     Q8TK_POLICY_AUTOMATIC );
	    q8tk_misc_set_size( swin, 29, 11 );

	    w->list = q8tk_listbox_new();
	    q8tk_widget_show( w->list );
	    q8tk_container_add( swin, w->list );

	    q8tk_box_pack_start( vbox, swin );
	  }

	  for( j=0; j<2; j++ ){

	    w->label[j] = q8tk_label_new( "" );	/* 空ラベルのウィジット確保 */
	    q8tk_widget_show( w->label[j] );
	    w->button[j] = q8tk_button_new();
	    q8tk_widget_show( w->button[j] );
	    q8tk_container_add( w->button[j], w->label[j] );
	    q8tk_signal_connect( w->button[j], "clicked",
				 cb_disk_button, (void *)( (j<<8) + i ) );

	    q8tk_box_pack_start( vbox, w->button[j] );
	  }
	}
      }

      PACK_VSEP( hbox );
    }

    {
      vbox = PACK_VBOX( hbox );
      {
	l = data_disk_info;
	for( i=0; i<COUNTOF(disk_info); i++ ){
	  w = &disk_info[i];

	  vx = PACK_VBOX( NULL );
	  {
	    hx = PACK_HBOX( vx );
	    {
	      PACK_LABEL( hx, GET_LABEL( l, DATA_DISK_INFO_STAT ) );
						/* 空ラベルのウィジット確保 */
	      w->stat_label = PACK_LABEL( hx, "" );
	    }

	    hx = PACK_HBOX( vx );
	    {
	      PACK_LABEL( hx, GET_LABEL( l, DATA_DISK_INFO_ATTR ) );
						/* 空ラベルのウィジット確保 */
	      w->attr_label = PACK_LABEL( hx, "" );
	    }

	    hx = PACK_HBOX( vx );
	    {
		PACK_LABEL( hx, GET_LABEL( l, DATA_DISK_INFO_NR ) );
						/* 空ラベルのウィジット確保 */
		w->num_label = PACK_LABEL( hx, "" );
		q8tk_misc_set_placement( w->num_label, Q8TK_PLACEMENT_X_RIGHT,
					               0 );
	    }
	  }

	  f = PACK_FRAME( vbox, GET_LABEL( data_disk_info_drive, i ), vx );
	  q8tk_frame_set_shadow_type( f, Q8TK_SHADOW_IN );
	}

	hx = PACK_HBOX( vbox );
	{
	  PACK_BUTTON( hx, GET_LABEL( data_disk_fname, DATA_DISK_FNAME ),
		       cb_disk_fname, NULL );
	}

	for( i=0; i<2; i++ )			/* 位置調整のためダミーを何個か */
	  PACK_LABEL( vbox, "" );
						/* button */
	PACK_BUTTON( vbox,
		     GET_LABEL( data_disk_image, DATA_DISK_IMAGE_BLANK ),
		     cb_disk_blank, NULL );
      }
    }
  }


  set_disk_widget();

  return	hbox;
}



/*===========================================================================
 *
 *	メインページ	キー設定
 *
 *===========================================================================*/

/*----------------------------------------------------------------------*/
static Q8tkWidget *fkey_widget[20+1][2];

				    /* ファンクションキー割り当ての変更 */
static	int	get_key_fkey( int fn_key )
{
  return ( function_f[ fn_key ] < 0x20 ) ? function_f[ fn_key ] : FN_FUNC;
}
static	void	cb_key_fkey( Q8tkWidget *widget, void *fn_key )
{
  int	i;
  const t_menudata *p = data_key_fkey_fn;
  const char       *combo_str = q8tk_combo_get_text(widget);

  for( i=0; i<COUNTOF(data_key_fkey_fn); i++, p++ ){
    if( strcmp( p->str[menu_lang], combo_str )==0 ){
      function_f[ (int)fn_key ] = p->val;
      q8tk_combo_set_text( fkey_widget[(int)fn_key][1],
			   keymap_assign[0].str );
      return;
    }
  }
}

static	int	get_key_fkey2( int fn_key )
{
  return ( function_f[ fn_key ] >= 0x20 ) ? function_f[ fn_key ] : KEY88_INVALID;
}
static	void	cb_key_fkey2( Q8tkWidget *widget, void *fn_key )
{
  int	i;
  const t_keymap *q = keymap_assign;
  const char     *combo_str = q8tk_combo_get_text(widget);

  for( i=0; i<COUNTOF(keymap_assign); i++, q++ ){
    if( strcmp( q->str, combo_str )==0 ){
      function_f[ (int)fn_key ] = q->code;
      q8tk_combo_set_text( fkey_widget[(int)fn_key][0],
			   data_key_fkey_fn[0].str[menu_lang] );
      return;
    }
  }
}


static	Q8tkWidget	*menu_key_fkey( void )
{
  int	i;
  Q8tkWidget	*vbox, *hbox;
  const t_menudata *p = data_key_fkey;

  vbox = PACK_VBOX( NULL );
  {
    for( i=0; i<COUNTOF(data_key_fkey); i++, p++ ){

      hbox  = PACK_HBOX( vbox );
      {
	PACK_LABEL( hbox, GET_LABEL( p, 0 ) );

	fkey_widget[p->val][0] =
		PACK_COMBO( hbox,
			    data_key_fkey_fn, COUNTOF(data_key_fkey_fn),
			    get_key_fkey( p->val ), NULL, 42,
			    cb_key_fkey, (void*)( p->val ),
			    NULL, NULL );

	fkey_widget[p->val][1] =
		MAKE_KEY_COMBO( hbox,
				&data_key_fkey2[i],
				get_key_fkey2,
				cb_key_fkey2 );
      }
    }
  }

  return vbox;
}

/*----------------------------------------------------------------------*/
						      /* キー設定の変更 */
static	int	get_key_cfg( int type )
{
  switch( type ){
  case DATA_KEY_CFG_TENKEY:	return	tenkey_emu;
  case DATA_KEY_CFG_NUMLOCK:	return	numlock_emu;
  }
  return FALSE;
}
static	void	cb_key_cfg( Q8tkWidget *widget, void *type )
{
  int	key = ( Q8TK_TOGGLE_BUTTON(widget)->active ) ? TRUE : FALSE;

  switch( (int)type ){
  case DATA_KEY_CFG_TENKEY:	tenkey_emu  = (key) ? TRUE : FALSE;	break;
  case DATA_KEY_CFG_NUMLOCK:	numlock_emu = (key) ? TRUE : FALSE;	break;
  }
}


static	Q8tkWidget	*menu_key_cfg( void )
{
  Q8tkWidget	*vbox;

  vbox = PACK_VBOX( NULL );
  {
    PACK_CHECK_BUTTONS( vbox,
			data_key_cfg, COUNTOF(data_key_cfg),
			get_key_cfg, cb_key_cfg );
  }

  return vbox;
}

/*----------------------------------------------------------------------*/
					      /* ソフトウェアキーボード */
static	void	keymap_start( void );
static	void	keymap_finish( void );

static	void	cb_key_softkeyboard( Q8tkWidget *dummy_0, void *dummy_1 )
{
  keymap_start();
}

static	Q8tkWidget	*menu_key_softkeyboard( void )
{
  Q8tkWidget	*button;
  const t_menulabel *l = data_skey_set;

  button = PACK_BUTTON( NULL,
			GET_LABEL( l, DATA_SKEY_BUTTON_SETUP),
			cb_key_softkeyboard, NULL );

  return button;
}



/*----------------------------------------------------------------------*/
					    /* カーソルキーカスタマイズ */
				     /* original idea by floi, thanks ! */
static	Q8tkWidget	*cursor_key_widget;
static	int	get_key_cursor_key_mode( void )
{
  return cursor_key_mode;
}
static	void	cb_key_cursor_key_mode( Q8tkWidget *dummy, void *p )
{
  int m = (int)p;

  if( cursor_key_mode != 2 && m == 2 ) q8tk_widget_show( cursor_key_widget );
  if( cursor_key_mode == 2 && m != 2 ) q8tk_widget_hide( cursor_key_widget );

  cursor_key_mode = m;
}
static	int	get_key_cursor_key( int type )
{
  return cursor_key_assign[ type ];
}
static	void	cb_key_cursor_key( Q8tkWidget *widget, void *type )
{
  int	i;
  const t_keymap *q = keymap_assign;
  const char     *combo_str = q8tk_combo_get_text(widget);

  for( i=0; i<COUNTOF(keymap_assign); i++, q++ ){
    if( strcmp( q->str, combo_str )==0 ){
      cursor_key_assign[ (int)type ] = q->code;
      return;
    }
  }
}


static	Q8tkWidget	*menu_key_cursor( void )
{
  Q8tkWidget	*hbox;

  hbox = PACK_HBOX( NULL );
  {						/* radio_button ... */
    PACK_RADIO_BUTTONS( PACK_VBOX( hbox ),
			data_key_cursor_mode, COUNTOF(data_key_cursor_mode),
			get_key_cursor_key_mode(), cb_key_cursor_key_mode );

    PACK_VSEP( hbox );				/* vseparator */

    cursor_key_widget = 
      PACK_KEY_ASSIN( hbox,			/* combo x 4 */
		      data_key_cursor, COUNTOF(data_key_cursor),
		      get_key_cursor_key, cb_key_cursor_key );

    if( get_key_cursor_key_mode() != 2 ){
      q8tk_widget_hide( cursor_key_widget );
    }
  }

  return hbox;
}

/*----------------------------------------------------------------------*/

static	Q8tkWidget	*menu_key( void )
{
  Q8tkWidget *vbox, *hbox, *w;
  const t_menulabel *l = data_key;

  vbox = PACK_VBOX( NULL );
  {
    PACK_FRAME( vbox, GET_LABEL( l, DATA_KEY_FKEY ), menu_key_fkey() );

    PACK_FRAME( vbox, GET_LABEL( l, DATA_KEY_CURSOR ), menu_key_cursor() );

    hbox = PACK_HBOX( vbox );
    {
      PACK_FRAME( hbox, GET_LABEL( l, DATA_KEY_CFG ), menu_key_cfg() );

      w = menu_key_softkeyboard();
      PACK_FRAME( hbox, GET_LABEL( l, DATA_KEY_SKEY ), w );
      q8tk_misc_set_placement( w, Q8TK_PLACEMENT_X_CENTER,
			          Q8TK_PLACEMENT_Y_CENTER );
    }
  }

  return vbox;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
 *
 *	サブウインドウ	ソフトウェアキーボード
 *
 * = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

static	Q8tkWidget	*keymap[129];
static	int		keymap_num;
static	Q8tkWidget	*keymap_accel;

enum {			/* keymap[] は以下のウィジットの保存に使う */
  KEYMAP_WIN,

  KEYMAP_VBOX,
  KEYMAP_SCRL,
  KEYMAP_SEP,
  KEYMAP_HBOX,

  KEYMAP_BTN_1,
  KEYMAP_BTN_2,

  KEYMAP_LINES,
  KEYMAP_LINE_1,
  KEYMAP_LINE_2,
  KEYMAP_LINE_3,
  KEYMAP_LINE_4,
  KEYMAP_LINE_5,
  KEYMAP_LINE_6,

  KEYMAP_KEY
};

/*----------------------------------------------------------------------*/

static	int	get_key_softkey( int code )
{
  return is_key_pressed( code );
}
static	void	cb_key_softkey( Q8tkWidget *button, void *code )
{
  if( Q8TK_TOGGLE_BUTTON(button)->active ) do_key_press  ( (int)code );
  else                                     do_key_release( (int)code );
}

static	void	cb_key_softkey_release( Q8tkWidget *dummy_0, void *dummy_1 )
{
  do_key_all_release();		/* 全てのキーを離した状態にする         */
  keymap_finish();		/* ソフトウェアキーの全ウィジットを消滅 */
}

static	void	cb_key_softkey_end( Q8tkWidget *dummy_0, void *dummy_1 )
{
  do_key_bug();			/* 複数キー同時押し時のハードバグを再現 */
  keymap_finish();		/* ソフトウェアキーの全ウィジットを消滅 */
}


/* ソフトウフェアキーボード ウインドウ生成・表示 */

static	void	keymap_start( void )
{
  Q8tkWidget *w, *v, *s, *l, *h, *b1, *b2, *vx, *hx, *n;
  int i,j;
  int model = ( ROM_VERSION < '8' ) ? 0 : 1;

  for( i=0; i<COUNTOF(keymap); i++ ) keymap[i] = NULL;

  {						/* メインとなるウインドウ */
    w = q8tk_window_new( Q8TK_WINDOW_DIALOG );
    keymap_accel = q8tk_accel_group_new();
    q8tk_accel_group_attach( keymap_accel, w );
  }

  {						/* に、ボックスを乗せる */
    v = q8tk_vbox_new();
    q8tk_container_add( w, v );
    q8tk_widget_show( v );
  }

  {							/* ボックスには     */
    {							/* スクロール付 WIN */
      s = q8tk_scrolled_window_new( NULL, NULL );
      q8tk_box_pack_start( v, s );
      q8tk_misc_set_size( s, 80, 21 );
      q8tk_scrolled_window_set_policy( s, Q8TK_POLICY_AUTOMATIC,
					  Q8TK_POLICY_NEVER     );
      q8tk_widget_show( s );
    }
    {							/* 見栄えのための空行*/
      l = q8tk_label_new( "" );
      q8tk_box_pack_start( v, l );
      q8tk_widget_show( l );
    }
    {							/* ボタン配置用 HBOX */
      h = q8tk_hbox_new();
      q8tk_box_pack_start( v, h );
      q8tk_misc_set_placement( h, Q8TK_PLACEMENT_X_CENTER, 0 );
      q8tk_widget_show( h );

      {								/* HBOXには */
	const t_menulabel *l = data_skey_set;
	{							/* ボタン 1 */
	  b1 = q8tk_button_new_with_label( GET_LABEL(l,DATA_SKEY_BUTTON_OFF) );
	  q8tk_signal_connect( b1, "clicked", cb_key_softkey_release, NULL );
	  q8tk_box_pack_start( h, b1 );
	  q8tk_widget_show( b1 );
	}
	{							/* ボタン 2 */
	  b2 = q8tk_button_new_with_label( GET_LABEL(l,DATA_SKEY_BUTTON_QUIT));
	  q8tk_signal_connect( b2, "clicked", cb_key_softkey_end, NULL );
	  q8tk_box_pack_start( h, b2 );
	  q8tk_widget_show( b2 );
	  q8tk_accel_group_add( keymap_accel, Q8TK_KEY_ESC, b2, "clicked" );
	}
      }
    }
  }

  /* スクロール付 WIN に、キートップの文字のかかれた、ボタンを並べる */

  vx = q8tk_vbox_new();			/* キー6列分を格納する VBOX を配置 */
  q8tk_container_add( s, vx );
  q8tk_widget_show( vx );

  keymap[ KEYMAP_WIN   ] = w;
  keymap[ KEYMAP_VBOX  ] = v;
  keymap[ KEYMAP_SCRL  ] = s;
  keymap[ KEYMAP_SEP   ] = l;
  keymap[ KEYMAP_HBOX  ] = h;
  keymap[ KEYMAP_BTN_1 ] = b1;
  keymap[ KEYMAP_BTN_2 ] = b2;
  keymap[ KEYMAP_LINES ] = vx;

  keymap_num = KEYMAP_KEY;


  for( j=0; j<6; j++ ){			/* キー6列分繰り返し */

    const t_keymap *p = keymap_line[ model ][ j ];

    hx = q8tk_hbox_new();		/* キー複数個を格納するためのHBOXに */
    q8tk_box_pack_start( vx, hx );
    q8tk_widget_show( hx );
    keymap[ KEYMAP_LINE_1 + j ] = hx;

    for( i=0; p[ i ].str; i++ ){	/* キーを1個づつ配置しておく*/

      if( keymap_num >= COUNTOF( keymap ) )	/* トラップ */
	{ fprintf( stderr, "%s %d\n", __FILE__, __LINE__ ); break; }
      
      if( p[i].code )				/* キートップ文字 (ボタン) */
      {
	n = q8tk_toggle_button_new_with_label( p[i].str );
	if( get_key_softkey( p[i].code ) ){
	  q8tk_toggle_button_set_state( n, TRUE );
	}
	q8tk_signal_connect( n, "toggled", cb_key_softkey, (void *)p[i].code );
      }
      else					/* パディング用空白 (ラベル) */
      {
	n = q8tk_label_new( p[i].str );
      }
      q8tk_box_pack_start( hx, n );
      q8tk_widget_show( n );

      keymap[ keymap_num ++ ] = n;
    }
  }


  q8tk_widget_show( w );
  q8tk_grab_add( w );

  q8tk_widget_grab_default( b2 );
}


/* キーマップダイアログの終了・消滅 */

static	void	keymap_finish( void )
{
  int	i;
  for( i=keymap_num-1; i; i-- ){
    if( keymap[i] ){
      q8tk_widget_destroy( keymap[i] );
    }
  }

  q8tk_grab_remove( keymap[ KEYMAP_WIN ] );
  q8tk_widget_destroy( keymap[ KEYMAP_WIN ] );
  q8tk_widget_destroy( keymap_accel );
}








/*===========================================================================
 *
 *	メインページ	マウス
 *
 *===========================================================================*/

/*----------------------------------------------------------------------*/
						/* マウスモード切り替え */
static	int	get_mouse_mode( void )
{
  return mouse_mode;
}
static	void	cb_mouse_mode( Q8tkWidget *widget, void *dummy )
{
  int	i;
  const t_menudata *p = data_mouse_mode;
  const char       *combo_str = q8tk_combo_get_text(widget);

  for( i=0; i<COUNTOF(data_mouse_mode); i++, p++ ){
    if( strcmp( p->str[menu_lang], combo_str )==0 ){
      mouse_mode = p->val;
      return;
    }
  }
}


static	Q8tkWidget	*menu_mouse_mode( void )
{
  Q8tkWidget	*hbox;


  hbox  = PACK_HBOX( NULL );
  {
    PACK_LABEL( hbox, GET_LABEL( data_mouse_mode_msg, 0 ) );

    PACK_COMBO( hbox,
		data_mouse_mode, COUNTOF(data_mouse_mode),
		get_mouse_mode(), NULL, 0,
		cb_mouse_mode, NULL,
		NULL, NULL );
  }

  return hbox;
}

/*----------------------------------------------------------------------*/
						  /* マウス入力設定変更 */
static	Q8tkWidget	*mouse_mouse_widget;
static	int	get_mouse_mouse_key_mode( void )
{
  return mouse_key_mode;
}
static	void	cb_mouse_mouse_key_mode( Q8tkWidget *dummy, void *p )
{
  int m = (int)p;

  if( mouse_key_mode != 2 && m == 2 ) q8tk_widget_show( mouse_mouse_widget );
  if( mouse_key_mode == 2 && m != 2 ) q8tk_widget_hide( mouse_mouse_widget );

  mouse_key_mode = m;
}
static	int	get_mouse_mouse_key( int type )
{
  return mouse_key_assign[ type ];
}
static	void	cb_mouse_mouse_key( Q8tkWidget *widget, void *type )
{
  int	i;
  const t_keymap *q = keymap_assign;
  const char     *combo_str = q8tk_combo_get_text(widget);

  for( i=0; i<COUNTOF(keymap_assign); i++, q++ ){
    if( strcmp( q->str, combo_str )==0 ){
      mouse_key_assign[ (int)type ] = q->code;
      return;
    }
  }
}


static	Q8tkWidget	*menu_mouse_mouse( void )
{
  Q8tkWidget	*hbox;

  hbox = PACK_HBOX( NULL );
  {						/* radio_button ... */
    PACK_RADIO_BUTTONS( PACK_VBOX( hbox ),
			data_mouse_mouse_key_mode,
			COUNTOF(data_mouse_mouse_key_mode),
			get_mouse_mouse_key_mode(),
			cb_mouse_mouse_key_mode );

    PACK_VSEP( hbox );				/* vseparator */

    mouse_mouse_widget = 
      PACK_KEY_ASSIN( hbox,			/* combo x 6 */
		      data_mouse_mouse, COUNTOF(data_mouse_mouse),
		      get_mouse_mouse_key, cb_mouse_mouse_key );

    if( get_mouse_mouse_key_mode() != 2 ){
      q8tk_widget_hide( mouse_mouse_widget );
    }
  }

  return hbox;
}

/*----------------------------------------------------------------------*/
					/* ジョイスティック入力設定変更 */
static	Q8tkWidget	*mouse_joy_widget;
static	int	get_mouse_joy_key_mode( void )
{
  return joy_key_mode;
}
static	void	cb_mouse_joy_key_mode( Q8tkWidget *dummy, void *p )
{
  int m = (int)p;

  if( joy_key_mode != 2 && m == 2 ) q8tk_widget_show( mouse_joy_widget );
  if( joy_key_mode == 2 && m != 2 ) q8tk_widget_hide( mouse_joy_widget );

  joy_key_mode = m;
}
static	int	get_mouse_joy_key( int type )
{
  return joy_key_assign[ type ];
}
static	void	cb_mouse_joy_key( Q8tkWidget *widget, void *type )
{
  int	i;
  const t_keymap *q = keymap_assign;
  const char     *combo_str = q8tk_combo_get_text(widget);

  for( i=0; i<COUNTOF(keymap_assign); i++, q++ ){
    if( strcmp( q->str, combo_str )==0 ){
      joy_key_assign[ (int)type ] = q->code;
      return;
    }
  }
}
static	int	get_joystick_swap( void )
{
  return joy_swap_button;
}
static	void	cb_joystick_swap( Q8tkWidget *widget, void *dummy )
{
  int	key = ( Q8TK_TOGGLE_BUTTON(widget)->active ) ? TRUE : FALSE;

  joy_swap_button = key;
}


static	Q8tkWidget	*menu_mouse_joy( void )
{
  Q8tkWidget	*vbox, *hbox;

  vbox = PACK_VBOX( NULL );
  {
    hbox = PACK_HBOX( vbox );
    {
      PACK_RADIO_BUTTONS( PACK_VBOX( hbox ),		/* radio ... */
			  data_mouse_joy_key_mode,
			  COUNTOF(data_mouse_joy_key_mode),
			  get_mouse_joy_key_mode(),
			  cb_mouse_joy_key_mode );

      PACK_VSEP( hbox );				/* vseparator */

      mouse_joy_widget =
	PACK_KEY_ASSIN( hbox,				/* combo x 6 */
			data_mouse_joy, COUNTOF(data_mouse_joy),
			get_mouse_joy_key, cb_mouse_joy_key );

      if( get_mouse_joy_key_mode() != 2 ){
	q8tk_widget_hide( mouse_joy_widget );
      }
    }

    PACK_LABEL( vbox, "                            " );	/* 空行 */

    PACK_CHECK_BUTTON( vbox,				/* check */
		       GET_LABEL( data_mouse_joystick_swap, 0 ),
		       get_joystick_swap(),
		       cb_joystick_swap, NULL );
  }

  return vbox;
}

/*----------------------------------------------------------------------*/

static	Q8tkWidget	*menu_mouse( void )
{
  Q8tkWidget *vbox;
  const t_menulabel *l = data_mouse;

  vbox = PACK_VBOX( NULL );
  {
    PACK_FRAME( vbox, GET_LABEL( l, DATA_MOUSE_MODE ), menu_mouse_mode() );

    PACK_FRAME( vbox, GET_LABEL( l, DATA_MOUSE_MOUSE ), menu_mouse_mouse() );

    PACK_FRAME( vbox, GET_LABEL( l, DATA_MOUSE_JOYSTICK ), menu_mouse_joy() );
  }

  return vbox;
}



/*===========================================================================
 *
 *	メインページ	テープ
 *
 *===========================================================================*/

/*----------------------------------------------------------------------*/
				      /* ロードイメージ・セーブイメージ */
int			tape_mode;
static	char		tape_filename[ QUASI88_MAX_FILENAME ];

static	Q8tkWidget	*tape_name[2];
static	Q8tkWidget	*tape_rate[2];

/*----------------------------------------------------------------------*/
static	void	set_tape_name( int c )
{
  q8tk_entry_set_text( tape_name[ c ],
		       ( (file_tape[ c ][0]=='\0') ? "(No File)"
						   : file_tape[ c ] ) );
}
static	void	set_tape_rate( int c  )
{
  char buf[16];
  long cur, end;

  if( c == CLOAD ){

    if( sio_tape_pos( &cur, &end ) ){
      if( end == 0 ){
	sprintf( buf, "   END " );
      }else{
	sprintf( buf, "   %3ld%%", cur * 100 / end );
      }
    }else{
      sprintf( buf, "   ---%%" );
    }

    q8tk_label_set( tape_rate[ c ], buf );
  }
}


/*----------------------------------------------------------------------*/
/* 「EJECT」ボタン押下時の処理						*/

static	void	cb_tape_eject_do( Q8tkWidget *dummy, void *c )
{
  if( (int)c == CLOAD ){
    quasi88_load_tape_eject();
  }else{
    quasi88_save_tape_eject();
  }

  set_tape_name( (int)c );
  set_tape_rate( (int)c );
}

/*----------------------------------------------------------------------*/
/* 「REW」ボタン押下時の処理						*/

static	void	cb_tape_rew_do( Q8tkWidget *dummy, void *c )
{
  if( (int)c == CLOAD ){
					/* イメージを巻き戻す */
    if( quasi88_load_tape_rewind() ){				/* 成功 */
      ;
    }else{							/* 失敗 */
      set_tape_name( (int)c );
    }
    set_tape_rate( (int)c );
  }
}

/*----------------------------------------------------------------------*/
/* 「OPEN」ボタン押下時の処理						*/

static	void	sub_tape_open( void );
static	void	sub_tape_open_do( void );
static	void	cb_tape_open_warn_clicked( Q8tkWidget *, void * );

static	void	cb_tape_open( Q8tkWidget *dummy, void *c )
{
  const char *initial;
  const t_menulabel *l = ( (int)c == CLOAD ) ? data_tape_load : data_tape_save;

				/* 今から生成するファイルセレクションは */
  tape_mode = (int)c;		/* LOAD用 か SAVE用か を覚えておく      */

  if( file_tape[ (int)c ][0] != '\0' ) initial = file_tape[ (int)c ];
  else{
    initial = osd_dir_tape();
    if( initial==NULL ) initial = osd_dir_cwd();
  }


  START_FILE_SELECTION( GET_LABEL( l, DATA_TAPE_FSEL ),
			-1,	/* ReadOnly の選択は不可 */
			initial,

			sub_tape_open,
			tape_filename,
			QUASI88_MAX_FILENAME,
			NULL );
}

static	void	sub_tape_open( void )
{
  const t_menulabel *l = data_tape_save;

  switch( osd_file_stat( tape_filename ) ){

  case FILE_STAT_NOEXIST:
    if( tape_mode == CLOAD ){			/* ファイル無いのでエラー   */
      start_file_error_dialog( -1, ERR_CANT_OPEN );
    }else{	      				/* ファイル無いので新規作成 */
      sub_tape_open_do();
    }
    break;

  case FILE_STAT_DIR:
    /* ディレクトリなので、開いちゃだめ */
    start_file_error_dialog( -1, ERR_CANT_OPEN );
    break;

  default:
    if( tape_mode == CSAVE ){
      /* すでにファイルが存在します。イメージを追記しますか？ */
      dialog_create();
      {
	dialog_set_title( GET_LABEL( l, DATA_TAPE_WARN_0 ) );

	dialog_set_title( GET_LABEL( l, DATA_TAPE_WARN_1 ) );

	dialog_set_separator();

	dialog_set_button( GET_LABEL( l, DATA_TAPE_WARN_APPEND ),
			   cb_tape_open_warn_clicked, (void*)TRUE );

	dialog_set_button( GET_LABEL( l, DATA_TAPE_WARN_CANCEL ),
			   cb_tape_open_warn_clicked, (void*)FALSE );

	dialog_accel_key( Q8TK_KEY_ESC );
      }
      dialog_start();
    }else{
      sub_tape_open_do();
    }
    break;
  }
}

static	void	sub_tape_open_do( void )
{
  int result, c = tape_mode;

  if( c == CLOAD ){			/* テープを開く */
    result = quasi88_load_tape_insert( tape_filename );
  }else{
    result = quasi88_save_tape_insert( tape_filename );
  }

  set_tape_name( c );
  set_tape_rate( c );


  if( result == 0 ){
    start_file_error_dialog( -1, ERR_CANT_OPEN );
  }
}

static	void	cb_tape_open_warn_clicked( Q8tkWidget *dummy, void *p )
{
  dialog_destroy();

  if( (int)p ){
    sub_tape_open_do();
  }
}




/*----------------------------------------------------------------------*/

INLINE	Q8tkWidget	*menu_tape_image_unit( const t_menulabel *l, int c )
{
  int save_code;
  Q8tkWidget	*vbox, *hbox, *w, *e;

  vbox = PACK_VBOX( NULL );
  {
    hbox = PACK_HBOX( vbox );
    {
      w = PACK_LABEL( hbox, GET_LABEL( l, DATA_TAPE_FOR ) );
      q8tk_misc_set_placement( w, Q8TK_PLACEMENT_X_CENTER,
			          Q8TK_PLACEMENT_Y_CENTER );

      {
	save_code = q8tk_set_kanjicode( osd_kanji_code() );

	e = PACK_ENTRY( hbox,
			QUASI88_MAX_FILENAME, 65, NULL,
			NULL, NULL, NULL, NULL );
	q8tk_entry_set_editable( e, FALSE );

	tape_name[ c ] = e;
	set_tape_name( c );

	q8tk_set_kanjicode( save_code );
      }
    }

    hbox = PACK_HBOX( vbox );
    {
      PACK_BUTTON( hbox,
		   GET_LABEL( l, DATA_TAPE_CHANGE ),
		   cb_tape_open, (void*)c );

      PACK_VSEP( hbox );

      PACK_BUTTON( hbox,
		   GET_LABEL( l, DATA_TAPE_EJECT ),
		   cb_tape_eject_do, (void*)c );

      if( c == CLOAD ){
	PACK_BUTTON( hbox,
		     GET_LABEL( l, DATA_TAPE_REWIND ),
		     cb_tape_rew_do, (void*)c );
      }
      if( c == CLOAD ){
	w = PACK_LABEL( hbox, "" );
	q8tk_misc_set_placement( w, Q8TK_PLACEMENT_X_CENTER,
				    Q8TK_PLACEMENT_Y_CENTER );
	tape_rate[ c ] = w;
	set_tape_rate( c );
      }
    }
  }

  return vbox;
}

static	Q8tkWidget	*menu_tape_image( void )
{
  Q8tkWidget *vbox, *w;

  vbox = PACK_VBOX( NULL );
  {
    q8tk_box_pack_start( vbox, menu_tape_image_unit( data_tape_load, CLOAD ) );

    PACK_HSEP( vbox );

    q8tk_box_pack_start( vbox, menu_tape_image_unit( data_tape_save, CSAVE ) );
  }

  return vbox;
}

/*----------------------------------------------------------------------*/
					    /* テープ処理モード切り替え */
static	int	get_tape_intr( void )
{
  return cmt_intr;
}
static	void	cb_tape_intr( Q8tkWidget *dummy, void *p )
{
  cmt_intr = (int)p;
}


static	Q8tkWidget	*menu_tape_intr( void )
{
  Q8tkWidget	*vbox;

  vbox = PACK_VBOX( NULL );
  {
    PACK_RADIO_BUTTONS( vbox,
			data_tape_intr, COUNTOF(data_tape_intr),
			get_tape_intr(), cb_tape_intr );
  }

  return vbox;
}

/*======================================================================*/

static	Q8tkWidget	*menu_tape( void )
{
  Q8tkWidget *vbox;
  const t_menulabel *l = data_tape;

  vbox = PACK_VBOX( NULL );
  {
    PACK_FRAME( vbox, GET_LABEL( l, DATA_TAPE_IMAGE ), menu_tape_image() );

    PACK_FRAME( vbox, GET_LABEL( l, DATA_TAPE_INTR ), menu_tape_intr() );
  }

  return vbox;
}



/*===========================================================================
 *
 *	メインページ	その他
 *
 *===========================================================================*/

/*----------------------------------------------------------------------*/
							  /* サスペンド */

static	Q8tkWidget	*misc_suspend_entry;
static	Q8tkWidget	*misc_suspend_combo;

/*	サスペンド時のメッセージダイアログを消す			  */
static	void	cb_misc_suspend_dialog_ok( Q8tkWidget *dummy, void *result )
{
  dialog_destroy();

  if( (int)result == DATA_MISC_RESUME_OK ){
    set_emu_mode( GO );
    q8tk_main_quit();
  }
}

/*	サスペンド実行後のメッセージダイアログ				  */
static	void	sub_misc_suspend_dialog( int result )
{
  const t_menulabel *l = data_misc_suspend_err;
  char filename[72];
  int save_code;
  size_t len;
  const char *none = "(No Image File)";

  dialog_create();
  {
    dialog_set_title( GET_LABEL( l, result ) );	/* 結果表示 */

    if( result == DATA_MISC_SUSPEND_OK ||	/* 成功時はイメージ名表示 */
	result == DATA_MISC_RESUME_OK  ){

      dialog_set_title( GET_LABEL( l, DATA_MISC_SUSPEND_LINE ) );
      dialog_set_title( GET_LABEL( l, DATA_MISC_SUSPEND_INFO ) );

      save_code = q8tk_set_kanjicode( osd_kanji_code() );

      len = strlen( none );
      if( file_disk[0][0] ) len = MAX( len, strlen( file_disk[0] ) );
      if( file_disk[1][0] ) len = MAX( len, strlen( file_disk[1] ) );
      if( file_tape[0][0] ) len = MAX( len, strlen( file_tape[0] ) );
      if( file_tape[1][0] ) len = MAX( len, strlen( file_tape[1] ) );
      len = MIN( len, sizeof(filename)-1 - 11 );
      len += 11;				/* 11==strlen("[DRIVE 1:] ") */

      strcpy( filename, "[DRIVE 1:] " );
      strncat( filename, ( file_disk[0][0] ) ? file_disk[0] : none,
	       len - strlen(filename) );
      while( strlen(filename) < len ){ strcat( filename, " " ); }
      dialog_set_title( filename );

      strcpy( filename, "[DRIVE 2:] " );
      strncat( filename, ( file_disk[1][0] ) ? file_disk[1] : none,
	       len - strlen(filename) );
      while( strlen(filename) < len ){ strcat( filename, " " ); }
      dialog_set_title( filename );

      strcpy( filename, "[TapeLOAD] " );
      strncat( filename, ( file_tape[0][0] ) ? file_tape[0] : none,
	       len - strlen(filename) );
      while( strlen(filename) < len ){ strcat( filename, " " ); }
      dialog_set_title( filename );

      strcpy( filename, "[TapeSAVE] " );
      strncat( filename, ( file_tape[1][0] ) ? file_tape[1] : none,
	       len - strlen(filename) );
      while( strlen(filename) < len ){ strcat( filename, " " ); }
      dialog_set_title( filename );

      q8tk_set_kanjicode( save_code );
    }

    dialog_set_separator();

    dialog_set_button( GET_LABEL( l, DATA_MISC_SUSPEND_AGREE ),
		       cb_misc_suspend_dialog_ok, (void*)result );

    dialog_accel_key( Q8TK_KEY_ESC );
  }
  dialog_start();
}

/*	レジューム実行前のメッセージダイアログ				  */
static	void	sub_misc_suspend_not_access( void )
{
  const t_menulabel *l = data_misc_suspend_err;

  dialog_create();
  {
    dialog_set_title( GET_LABEL( l, DATA_MISC_RESUME_CANTOPEN ) );

    dialog_set_separator();

    dialog_set_button( GET_LABEL( l, DATA_MISC_SUSPEND_AGREE ),
		       cb_misc_suspend_dialog_ok,
		       (void*)DATA_MISC_SUSPEND_AGREE );

    dialog_accel_key( Q8TK_KEY_ESC );
  }
  dialog_start();
}

/*	サスペンド実行前のメッセージダイアログ				  */
static	void	cb_misc_suspend_overwrite( Q8tkWidget *dummy_0, void *dummy_1);
static	void	sub_misc_suspend_really( void )
{
  const t_menulabel *l = data_misc_suspend_err;

  dialog_create();
  {
    dialog_set_title( GET_LABEL( l, DATA_MISC_SUSPEND_REALLY ) );

    dialog_set_separator();

    dialog_set_button( GET_LABEL( l, DATA_MISC_SUSPEND_OVERWRITE ),
		       cb_misc_suspend_overwrite, NULL );
    dialog_set_button( GET_LABEL( l, DATA_MISC_SUSPEND_CANCEL ),
		       cb_misc_suspend_dialog_ok,
		       (void*)DATA_MISC_SUSPEND_CANCEL );

    dialog_accel_key( Q8TK_KEY_ESC );
  }
  dialog_start();
}

static	void	cb_misc_suspend_overwrite( Q8tkWidget *dummy_0, void *dummy_1 )
{
  dialog_destroy();
  {
    if( statesave() ){
      sub_misc_suspend_dialog( DATA_MISC_SUSPEND_OK );		/* 成功 */
    }else{
      sub_misc_suspend_dialog( DATA_MISC_SUSPEND_ERR );		/* 失敗 */
    }
  }
}

/*----------------------------------------------------------------------*/
/*	サスペンド処理 (「セーブ」クリック時 )				*/
static	void	cb_misc_suspend_save( Q8tkWidget *dummy_0, void *dummy_1 )
{
#if 0	/* いちいち上書き確認してくるのはうざい？ */
  if( statesave_check() ){				/* ファイルある */
    sub_misc_suspend_really();
  }else
#endif
  {
    if( statesave() ){
      sub_misc_suspend_dialog( DATA_MISC_SUSPEND_OK );		/* 成功 */
    }else{
      sub_misc_suspend_dialog( DATA_MISC_SUSPEND_ERR );		/* 失敗 */
    }
  }
}

/*----------------------------------------------------------------------*/
/*	サスペンド処理 (「ロード」クリック時 )				*/
static	void	cb_misc_suspend_load( Q8tkWidget *dummy_0, void *dummy_1 )
{
  if( stateload_check() == FALSE ){			/* ファイルなし */
    sub_misc_suspend_not_access();
  }else{
    if( quasi88_stateload() ){
      sub_misc_suspend_dialog( DATA_MISC_RESUME_OK );		/* 成功 */
    }else{
      sub_misc_suspend_dialog( DATA_MISC_RESUME_ERR );		/* 失敗 */
    }
  }
}

/*----------------------------------------------------------------------*/
/*	ファイル名チェック						*/

static int get_misc_suspend_num( void )
{
  const char  *ssfx = STATE_SUFFIX;		/* ".sta" */
  const size_t nsfx = strlen(STATE_SUFFIX);	/* 4      */

  if( strlen(file_state) > nsfx &&
      my_strcmp( &file_state[ strlen(file_state)-nsfx ], ssfx ) == 0 ){

    if( strlen(file_state) > nsfx+2 &&	/* ファイル名が xxx-N.sta */
	'-'  ==  file_state[ strlen(file_state) -nsfx -2 ]   &&
	isdigit( file_state[ strlen(file_state) -nsfx -1 ] ) ){

      return file_state[ strlen(file_state) -nsfx -1 ];    /* '0'〜'9'を返す */

    }else{				/* ファイル名が xxx.sta */
      return 0;
    }
  }else{				/* ファイル名が その他 */
    return -1;
  }
}

/*----------------------------------------------------------------------*/
/*	ファイル名前変更。エントリー changed (入力)時に呼ばれる。       */
/*		(ファイルセレクションでの変更時はこれは呼ばれない)      */

static void cb_misc_suspend_entry_change( Q8tkWidget *widget, void *dummy )
{
  int i, j;
  char buf[4];

  strncpy( file_state, q8tk_entry_get_text( widget ),
	   QUASI88_MAX_FILENAME-1 );
  file_state[ QUASI88_MAX_FILENAME-1 ] = '\0';

  i = get_misc_suspend_num();		/* 名前が .sta で終れば、コンボ変更 */
  if( i > 0 ){ buf[0] = i;   }
  else       { buf[0] = ' '; }
  buf[1] = '\0';
  if( *(q8tk_combo_get_text( misc_suspend_combo )) != buf[0] ){
    q8tk_combo_set_text( misc_suspend_combo, buf );
  }
}

/*----------------------------------------------------------------------*/
/*	ファイル選択処理。ファイルセレクションを使用			*/

static void sub_misc_suspend_change( void );

static	void	cb_misc_suspend_fsel( Q8tkWidget *dummy_0, void *dummy_1 )
{
  const t_menulabel *l = data_misc_suspend;


  START_FILE_SELECTION( GET_LABEL( l, DATA_MISC_SUSPEND_FSEL ),
			-1,	/* ReadOnly の選択は不可 */
			file_state,

			sub_misc_suspend_change,
			file_state,
			QUASI88_MAX_FILENAME,
			NULL );
}

static void sub_misc_suspend_change( void )
{
  int i;
  char buf[4];

  q8tk_entry_set_text( misc_suspend_entry, file_state );

  i = get_misc_suspend_num();		/* 名前が .sta で終れば、コンボ変更 */
  if( i > 0 ){ buf[0] = i;   }
  else       { buf[0] = ' '; }
  buf[1] = '\0';
  q8tk_combo_set_text( misc_suspend_combo, buf );
}


static void cb_misc_suspend_num( Q8tkWidget *widget, void *dummy )
{
  const char  *ssfx = STATE_SUFFIX;		/* ".sta"   */
  const size_t nsfx = strlen(STATE_SUFFIX);	/* 4        */
  char        buf[] = "-N" STATE_SUFFIX;	/* "-N.sta" */
  int len;

  int	i, chg = FALSE;
  const t_menudata *p = data_misc_suspend_num;
  const char       *combo_str = q8tk_combo_get_text(widget);

  for( i=0; i<COUNTOF(data_misc_suspend_num); i++, p++ ){
    if( strcmp( p->str[menu_lang], combo_str ) == 0 ){

      i = get_misc_suspend_num();
      buf[1] = p->val;

				/* ファイル名から .sta ないし -N.sta を削除 */
      len = strlen(file_state);
      if( i>0 ){						/* xxx-N.sta */
	file_state[ strlen(file_state) -nsfx -2 ] = '\0';
      }else if( i==0 ){						/* xxx.sta   */
	file_state[ strlen(file_state) -nsfx    ] = '\0';
      }else{							/* xxx       */
	;
      }

      if( i>0 ){			/* 元のファイル名が xxx-N.sta */

	if( p->val == 0 ){			/* xxx-N.sta -> xxx.sta */
	  strcat( file_state, ssfx );
	}else{					/* xxx-N.sta -> xxx-M.sta */
	  strcat( file_state, buf );
	}
	chg = TRUE;

      }else if( i==0 ){			/* 元のファイル名が xxx.sta */

	if( p->val ){				/* xxx.sta -> xxx-N.sta */
	  if( len + 2 < QUASI88_MAX_FILENAME ){
	    strcat( file_state, buf );
	    chg = TRUE;
	  }
	}

      }else{				/* 元のファイル名が その他 xxx */

	if( p->val == 0 ){			/* xxx -> xxx.sta */
	  if( len + nsfx < QUASI88_MAX_FILENAME ){
	    strcat( file_state, ssfx );
	    chg = TRUE;
	  }
	}else{					/* xxx -> xxx-N.sta */
	  if( len + nsfx +2 < QUASI88_MAX_FILENAME ){
	    strcat( file_state, buf );
	    chg = TRUE;
	  }
	}
      }
      if( chg ){
	q8tk_entry_set_text( misc_suspend_entry, file_state );
	q8tk_entry_set_position( misc_suspend_entry, QUASI88_MAX_FILENAME );
      }
      return;
    }
  }
}

/*----------------------------------------------------------------------*/

static	Q8tkWidget	*menu_misc_suspend( void )
{
  Q8tkWidget	*vbox, *hbox;
  Q8tkWidget	*w, *e;
  const t_menulabel *l = data_misc_suspend;
  int save_code;

  vbox = PACK_VBOX( NULL );
  {
    hbox = PACK_HBOX( vbox );
    {
      {
	save_code = q8tk_set_kanjicode( osd_kanji_code() );

	e = PACK_ENTRY( hbox,
			QUASI88_MAX_FILENAME, 67, file_state,
			NULL, NULL,
			cb_misc_suspend_entry_change, NULL );
/*      q8tk_entry_set_position( e, 0 );*/
	misc_suspend_entry = e;

	q8tk_set_kanjicode( save_code );
      }

      PACK_LABEL( hbox, " " );

      PACK_BUTTON( hbox,
		   GET_LABEL( l, DATA_MISC_SUSPEND_CHANGE ),
		   cb_misc_suspend_fsel, NULL );
    }

/*  PACK_HSEP( vbox );*/

    hbox = PACK_HBOX( vbox );
    {
      PACK_BUTTON( hbox,
		   GET_LABEL( l, DATA_MISC_SUSPEND_SAVE ),
		   cb_misc_suspend_save, NULL );

      PACK_LABEL( hbox, " " );
      PACK_VSEP( hbox );
      PACK_LABEL( hbox, " " );

      PACK_BUTTON( hbox,
		   GET_LABEL( l, DATA_MISC_SUSPEND_LOAD ),
		   cb_misc_suspend_load, NULL );

      w = PACK_LABEL( hbox, GET_LABEL( l, DATA_MISC_SUSPEND_NUMBER ) );
      q8tk_misc_set_placement( w, Q8TK_PLACEMENT_X_CENTER,
			          Q8TK_PLACEMENT_Y_CENTER );

      w = PACK_COMBO( hbox,
		      data_misc_suspend_num, COUNTOF(data_misc_suspend_num),
		      get_misc_suspend_num(), " ", 0,
		      cb_misc_suspend_num, NULL,
		      NULL, NULL );
      q8tk_misc_set_placement( w, Q8TK_PLACEMENT_X_CENTER,
			       Q8TK_PLACEMENT_Y_CENTER );
      misc_suspend_combo = w;
    }
  }

  return vbox;
}



/*----------------------------------------------------------------------*/
					 /* スクリーン スナップショット */

static	Q8tkWidget	*misc_snapshot_entry;


/*----------------------------------------------------------------------*/
/*	スナップショット セーブ (「実行」クリック時 )			*/
static	void	cb_misc_snapshot_do( void )
{
  size_t len = strlen( file_snap );

  save_screen_snapshot();

  if( len != strlen( file_snap ) ) 
    q8tk_entry_set_text( misc_snapshot_entry, file_snap );
}

/*----------------------------------------------------------------------*/
/*	画像フォーマット切り替え					*/
static	int	get_misc_snapshot_format( void )
{
  return snapshot_format;
}
static	void	cb_misc_snapshot_format( Q8tkWidget *dummy, void *p )
{
  snapshot_format = (int)p;
}



/*----------------------------------------------------------------------*/
/*	ファイル名前変更。エントリー changed (入力)時に呼ばれる。	*/
/*		(ファイルセレクションでの変更時はこれは呼ばれない)	*/

static void cb_misc_snapshot_entry_change( Q8tkWidget *widget, void *dummy )
{
  strncpy( file_snap, q8tk_entry_get_text( widget ),
	   QUASI88_MAX_FILENAME-1 );
  file_snap[ QUASI88_MAX_FILENAME-1 ] = '\0';
}

/*----------------------------------------------------------------------*/
/*	ファイル選択処理。ファイルセレクションを使用			*/

static void sub_misc_snapshot_change( void );

static	void	cb_misc_snapshot_fsel( Q8tkWidget *dummy_0, void *dummy_1 )
{
  const t_menulabel *l = data_misc_snapshot;


  START_FILE_SELECTION( GET_LABEL( l, DATA_MISC_SNAPSHOT_FSEL ),
			-1,	/* ReadOnly の選択は不可 */
			file_snap,

			sub_misc_snapshot_change,
			file_snap,
			QUASI88_MAX_FILENAME,
			NULL );
}

static void sub_misc_snapshot_change( void )
{
  q8tk_entry_set_text( misc_snapshot_entry, file_snap );
}


/*----------------------------------------------------------------------*/
#ifdef	USE_SSS_CMD
/*	コマンド実行状態変更 */
static	int	get_misc_snapshot_c_do( void )
{
  return snapshot_cmd_do;
}
static	void	cb_misc_snapshot_c_do( Q8tkWidget *widget, void *dummy )
{
  int	key = ( Q8TK_TOGGLE_BUTTON(widget)->active ) ? TRUE : FALSE;
  snapshot_cmd_do = key;
}

/*	コマンド変更。エントリー changed (入力)時に呼ばれる。  */
static void cb_misc_snapshot_c_entry_change( Q8tkWidget *widget, void *dummy )
{
  strncpy( snapshot_cmd, q8tk_entry_get_text( widget ),
	   SNAPSHOT_CMD_SIZE-1 );
  snapshot_cmd[ SNAPSHOT_CMD_SIZE-1 ] = '\0';
}
#endif

/*----------------------------------------------------------------------*/
static	Q8tkWidget	*menu_misc_snapshot( void )
{
  Q8tkWidget	*hbox, *vbox, *hbox2, *vbox2;
  Q8tkWidget	*e, *w;
  const t_menulabel *l = data_misc_snapshot;
  int save_code;

  vbox = PACK_VBOX( NULL );
  {
    hbox = PACK_HBOX( vbox );
    {
      {
	save_code = q8tk_set_kanjicode( osd_kanji_code() );

	e = PACK_ENTRY( hbox,
			QUASI88_MAX_FILENAME, 67, file_snap,
			NULL, NULL,
			cb_misc_snapshot_entry_change, NULL );
/*      q8tk_entry_set_position( e, 0 );*/
	misc_snapshot_entry = e; 

	q8tk_set_kanjicode( save_code );
      }
      PACK_LABEL( hbox, " " );

      PACK_BUTTON( hbox,
		   GET_LABEL( l, DATA_MISC_SNAPSHOT_CHANGE ),
		   cb_misc_snapshot_fsel, NULL );
    }

    hbox = PACK_HBOX( vbox );
    {
      PACK_BUTTON( hbox,
		   GET_LABEL( l, DATA_MISC_SNAPSHOT_BUTTON ),
		   cb_misc_snapshot_do, NULL );

      PACK_VSEP( hbox );

      vbox2 = PACK_VBOX( hbox );
      {
	if( snapshot_cmd_enable == FALSE ){
	  PACK_LABEL( vbox2, "" );
	}

	hbox2 = PACK_HBOX( vbox2 );
	{
	  PACK_LABEL( hbox2, GET_LABEL( l, DATA_MISC_SNAPSHOT_FORMAT ) );

	  PACK_RADIO_BUTTONS( PACK_HBOX( hbox2 ),
			      data_misc_snapshot_format,
			      COUNTOF(data_misc_snapshot_format),
			      get_misc_snapshot_format(),
			      cb_misc_snapshot_format );
	}

#ifdef	USE_SSS_CMD
	if( snapshot_cmd_enable ){

	  PACK_LABEL( vbox2, "" );			/* 空行 */

	  hbox2 = PACK_HBOX( vbox2 );
	  {
	    PACK_CHECK_BUTTON( hbox2,
			       GET_LABEL( l, DATA_MISC_SNAPSHOT_CMD ),
			       get_misc_snapshot_c_do(),
			       cb_misc_snapshot_c_do, NULL );

	    PACK_ENTRY( hbox2,
			SNAPSHOT_CMD_SIZE, 41, snapshot_cmd,
			NULL, NULL,
			cb_misc_snapshot_c_entry_change, NULL );
	  }
	}
#endif	/* USE_SSS_CMD */
      }
    }
  }

  return vbox;
}

/*----------------------------------------------------------------------*/
						    /* ファイル名合わせ */
static	int	get_misc_sync( void )
{
  return filename_synchronize;
}
static	void	cb_misc_sync( Q8tkWidget *widget, void *dummy )
{
  filename_synchronize = ( Q8TK_TOGGLE_BUTTON(widget)->active ) ? TRUE : FALSE;
}



/*======================================================================*/

static	Q8tkWidget	*menu_misc( void )
{
  Q8tkWidget *vbox, *w;
  const t_menulabel *l = data_misc;

  vbox = PACK_VBOX( NULL );
  {
    PACK_FRAME( vbox, GET_LABEL( l, DATA_MISC_SUSPEND ), menu_misc_suspend() );

    PACK_FRAME( vbox, GET_LABEL( l, DATA_MISC_SNAPSHOT), menu_misc_snapshot());

    PACK_LABEL( vbox, "" );			/* 空行 */

    PACK_CHECK_BUTTON( vbox,			/* check_button */
		       GET_LABEL( data_misc_sync, 0 ),
		       get_misc_sync(),
		       cb_misc_sync, NULL );
  }

  return vbox;
}










/*===========================================================================
 *
 *	メインページ	バージョン情報
 *
 *===========================================================================*/

static	Q8tkWidget	*menu_about( void )
{
  int i;
  Q8tkWidget *vx, *hx, *vbox, *swin, *hbox, *w;

  vx = PACK_VBOX( NULL );
  {
    hx = PACK_HBOX( vx );				/* 上半分にロゴ表示 */
    {
      PACK_LABEL( hx, " " );/*indent*/

      if( strcmp( Q_TITLE, "QUASI88" )==0 ){
	w = q8tk_logo_new();
	q8tk_widget_show( w );
	q8tk_box_pack_start( hx, w );
      }else{
	PACK_LABEL( hx, Q_TITLE );
      }

      vbox = PACK_VBOX( hx );
      {
	i = Q8GR_LOGO_H;
#ifdef	USE_MONITOR
	PACK_LABEL( vbox, "  (with monitor-mode)" );
	i--;
#endif
	for( ; i>1; i-- ) PACK_LABEL( vbox, "" );

	PACK_LABEL( vbox, "  ver. " Q_VERSION  "  < " Q_COMMENT " >" );
      }
    }
							/* 下半分は情報表示 */
    swin  = q8tk_scrolled_window_new( NULL, NULL );
    {
      hbox = PACK_HBOX( NULL );
      {
	PACK_LABEL( hbox, " " );/*indent*/

	vbox = PACK_VBOX( hbox );
	{
	  {			/* サウンドに関する情報表示 */
	    const char *(*s) = (menu_lang==0) ? data_about_en : data_about_jp;

	    for( i=0; s[i]; i++ ){
	      PACK_LABEL( vbox, s[i] );
	    }
	  }

	  {			/* システム依存部に関する情報表示 */
	    const char *s;
	    int save_code = 0, code = about_msg_init( menu_lang );

	    if( code >= 0 ){ save_code = q8tk_set_kanjicode( code ); }

	    PACK_LABEL( vbox, "" );
	    while( (s = about_msg() ) ){
	      PACK_LABEL( vbox, s );
	    }

	    if( code >= 0 ){ q8tk_set_kanjicode( save_code ); }
	  }
	}
      }
      q8tk_container_add( swin, hbox );
    }

    q8tk_scrolled_window_set_policy( swin, Q8TK_POLICY_AUTOMATIC,
					   Q8TK_POLICY_AUTOMATIC );
    q8tk_misc_set_size( swin, 78, 18-Q8GR_LOGO_H );
    q8tk_widget_show( swin );
    q8tk_box_pack_start( vx, swin );
  }

  return vx;
}










/*===========================================================================
 *
 *	メインウインドウ
 *
 *===========================================================================*/

/*----------------------------------------------------------------------*/
				     /* NOTEBOOK に張り付ける、各ページ */
static	struct{
  int		data_num;
  Q8tkWidget	*(*menu_func)(void);
} menu_page[] =
{
  { DATA_TOP_RESET,	menu_reset,	},
  { DATA_TOP_CPU,	menu_cpu,	},
  { DATA_TOP_GRAPH,	menu_graph,	},
  { DATA_TOP_VOLUME,	menu_volume,	},
  { DATA_TOP_DISK,	menu_disk,	},
  { DATA_TOP_KEY,	menu_key,	},
  { DATA_TOP_MOUSE,	menu_mouse,	},
  { DATA_TOP_TAPE,	menu_tape,	},
  { DATA_TOP_MISC,	menu_misc,	},
  { DATA_TOP_ABOUT,	menu_about,	},
};

/*----------------------------------------------------------------------*/
/* NOTEBOOK の各ページを、ファンクションキーで選択出来るように、
   アクセラレータキーを設定する。そのため、ダミーウィジット利用 */

#define	cb_note_fake(fn,n)						     \
static	void	cb_note_fake_##fn( Q8tkWidget *dummy, Q8tkWidget *notebook ){\
  q8tk_notebook_set_page( notebook, n );				     \
}
cb_note_fake(f1,0)
cb_note_fake(f2,1)
cb_note_fake(f3,2)
cb_note_fake(f4,3)
cb_note_fake(f5,4)
cb_note_fake(f6,5)
cb_note_fake(f7,6)
cb_note_fake(f8,7)
cb_note_fake(f9,8)
cb_note_fake(f10,9)

     /* 以下のアクセラレータキー処理は、 floi氏 提供。 Thanks ! */
static	void	cb_note_fake_prev( Q8tkWidget *dummy, Q8tkWidget *notebook ){
  int	n = q8tk_notebook_current_page( notebook ) - 1;
  if( n < 0 ) n = COUNTOF(menu_page) - 1;
  q8tk_notebook_set_page( notebook, n );
}

static	void	cb_note_fake_next( Q8tkWidget *dummy, Q8tkWidget *notebook ){
  int   n = q8tk_notebook_current_page( notebook ) + 1;
  if( COUNTOF(menu_page) <= n ) n = 0;
  q8tk_notebook_set_page( notebook, n );
}

static	struct{
  int	key;
  void	(*cb_func)(Q8tkWidget *, Q8tkWidget *);
} menu_fkey[] =
{
  { Q8TK_KEY_F1,  cb_note_fake_f1,  },
  { Q8TK_KEY_F2,  cb_note_fake_f2,  },
  { Q8TK_KEY_F3,  cb_note_fake_f3,  },
  { Q8TK_KEY_F4,  cb_note_fake_f4,  },
  { Q8TK_KEY_F5,  cb_note_fake_f5,  },
  { Q8TK_KEY_F6,  cb_note_fake_f6,  },
  { Q8TK_KEY_F7,  cb_note_fake_f7,  },
  { Q8TK_KEY_F8,  cb_note_fake_f8,  },
  { Q8TK_KEY_F9,  cb_note_fake_f9,  },
  { Q8TK_KEY_F10, cb_note_fake_f10, },

  { Q8TK_KEY_HOME,      cb_note_fake_prev,  },
  { Q8TK_KEY_END,       cb_note_fake_next,  },
};

/*----------------------------------------------------------------------*/
						/* 簡易リセットボタン   */
static	Q8tkWidget	*quickres_widget;
static	int		quickres_hide		= 1;
static	Q8tkWidget	*quickres_vsep;
static	Q8tkWidget	*quickres_button;
static	Q8tkWidget	*quickres_label;
static	void	cb_quickres_stat( Q8tkWidget *dummy_0, void *dummy_1 )
{
  quickres_hide ^= 1;
  
  if( quickres_hide ){
    q8tk_widget_hide( quickres_widget );
    q8tk_widget_hide( quickres_vsep );
    q8tk_label_set( quickres_button->child,  ">>" );
    q8tk_label_set( quickres_label, "                   " );
  }else{
    q8tk_widget_show( quickres_widget );
    q8tk_widget_show( quickres_vsep );
    q8tk_label_set( quickres_button->child,  "<<" );
    q8tk_label_set( quickres_label, "" );
  }
}
static	void	cb_quickres_reset( Q8tkWidget *dummy_0, void *dummy_1 )
{
  boot_basic      = quickres_basic;
  boot_clock_4mhz = quickres_clock;
  if( menu_boot_clock_async == FALSE ){
    cpu_clock_mhz = boot_clock_4mhz ? CONST_4MHZ_CLOCK : CONST_8MHZ_CLOCK;
  }
  quasi88_reset();
  set_emu_mode( GO );
  q8tk_main_quit();
}
static	int	get_quickres_basic( void )
{
  return quickres_basic;
}
static	void	cb_quickres_basic( Q8tkWidget *dummy, void *p )
{
  quickres_basic = (int)p;
}
static	int	get_quickres_clock( void )
{
  return quickres_clock;
}
static	void	cb_quickres_clock( Q8tkWidget *dummy, void *p )
{
  quickres_clock = (int)p;
}

static	void	pack_quickres( Q8tkWidget *base_hbox )
{
  quickres_widget = PACK_HBOX( base_hbox );
  {
    PACK_RADIO_BUTTONS( PACK_VBOX( quickres_widget ),
			data_quickres_basic, COUNTOF(data_quickres_basic),
			get_quickres_basic(), cb_quickres_basic );

    PACK_RADIO_BUTTONS( PACK_VBOX( quickres_widget ),
			data_quickres_clock, COUNTOF(data_quickres_clock),
			get_quickres_clock(), cb_quickres_clock );

    PACK_BUTTON( quickres_widget,
		 GET_LABEL( data_quickres_reset, 0 ),
		 cb_quickres_reset, NULL );
  }
  quickres_vsep   = PACK_VSEP( base_hbox );
  quickres_button = PACK_BUTTON( base_hbox,
				 "<<",
				 cb_quickres_stat, NULL );
  quickres_label  = PACK_LABEL( base_hbox,
				"" );
  if( quickres_hide ){
    q8tk_widget_hide( quickres_widget );
    q8tk_widget_hide( quickres_vsep );
    q8tk_label_set( quickres_button->child,  ">>" );
    q8tk_label_set( quickres_label, "                   " );
  }

  PACK_LABEL( base_hbox,
	      (debug_mode)
	      ? (menu_lang ? " "             : "      " )
	      : (menu_lang ? "             " : "                 " ) );
}


/*----------------------------------------------------------------------*/
					/* メインウインドウ下部のボタン */
static	void	sub_top_quit( void );

static	int	get_top_status( void ){ return now_status; }
static	void	cb_top_status( Q8tkWidget *widget, void *dummy )
{
  /*int	key = ( Q8TK_TOGGLE_BUTTON(widget)->active ) ? TRUE : FALSE;*/
  quasi88_status();
}

static	void	cb_top_button( Q8tkWidget *dummy, void *p )
{
  switch( (int)p ){
  case DATA_TOP_EXIT:
    set_emu_mode( GO );
    break;
  case DATA_TOP_MONITOR:
    set_emu_mode( MONITOR );
    break;
  case DATA_TOP_QUIT:
    sub_top_quit();
    return;
  }
  q8tk_main_quit();
}

static	Q8tkWidget	*menu_top_button( void )
{
  int	i;
  Q8tkWidget *hbox, *b;
  const t_menudata *p = data_top_button;

  hbox = PACK_HBOX( NULL );
  {
    pack_quickres( hbox );


    b = PACK_CHECK_BUTTON( hbox,
			   GET_LABEL( data_top_status, 0 ),
			   get_top_status(),
			   cb_top_status, NULL );
    q8tk_misc_set_placement( b, 0, Q8TK_PLACEMENT_Y_CENTER );


    for( i=0; i<COUNTOF(data_top_button); i++, p++ ){

      if( p->val == DATA_TOP_MONITOR  &&  debug_mode == FALSE ) continue;

      b = PACK_BUTTON( hbox, GET_LABEL( p, 0 ),
		       cb_top_button, (void *)(p->val) );

      if( p->val == DATA_TOP_QUIT ){
	q8tk_accel_group_add( menu_accel, Q8TK_KEY_F12, b, "clicked" );
      }
      if( p->val == DATA_TOP_EXIT ){
	q8tk_accel_group_add( menu_accel, Q8TK_KEY_ESC, b, "clicked" );
      }
    }
  }
  q8tk_misc_set_placement( hbox, Q8TK_PLACEMENT_X_RIGHT, 0 );

  return hbox;
}


/*----------------------------------------------------------------------*/
				  /* QUITボタン押下時の、確認ダイアログ */

static	void	cb_top_quit_clicked( Q8tkWidget *dummy, void *p )
{
  dialog_destroy();

  if( (int)p ){
    set_emu_mode( QUIT );
    q8tk_main_quit();
  }
}
static	void	sub_top_quit( void )
{
  const t_menulabel *l = data_top_quit;

  dialog_create();
  {
    dialog_set_title( GET_LABEL( l, DATA_TOP_QUIT_TITLE ) );

    dialog_set_separator();

    dialog_set_button( GET_LABEL( l, DATA_TOP_QUIT_OK ),
		       cb_top_quit_clicked, (void*)TRUE );

    dialog_accel_key( Q8TK_KEY_F12 );

    dialog_set_button( GET_LABEL( l, DATA_TOP_QUIT_CANCEL ),
		       cb_top_quit_clicked, (void*)FALSE );

    dialog_accel_key( Q8TK_KEY_ESC );
  }
  dialog_start();
}

/*======================================================================*/

static Q8tkWidget *menu_top( void )
{
  int	i;
  const t_menudata *l = data_top;
  Q8tkWidget *note_fake[ COUNTOF(menu_fkey) ];
  Q8tkWidget *win, *vbox, *notebook, *w;

  win = q8tk_window_new( Q8TK_WINDOW_TOPLEVEL );
  menu_accel = q8tk_accel_group_new();
  q8tk_accel_group_attach( menu_accel, win );
  q8tk_widget_show( win );

  vbox = PACK_VBOX( NULL );
  {
    {
		/* 各メニューをノートページに乗せていく */

      notebook = q8tk_notebook_new();
      {
	for( i=0; i<COUNTOF(menu_page); i++ ){

	  w = (*menu_page[i].menu_func)();
	  q8tk_notebook_append( notebook, w,
				GET_LABEL( l, menu_page[i].data_num ) );

	  if( i<COUNTOF(menu_fkey) ){
	    note_fake[i] = q8tk_button_new();
	    q8tk_signal_connect( note_fake[i], "clicked",
				 menu_fkey[i].cb_func, notebook );
	    q8tk_accel_group_add( menu_accel, menu_fkey[i].key,
				  note_fake[i], "clicked" );
	  }
	}

	for( ; i < COUNTOF(menu_fkey); i++ ){
	  note_fake[i] = q8tk_button_new();
	  q8tk_signal_connect( note_fake[i], "clicked",
			       menu_fkey[i].cb_func, notebook );
	  q8tk_accel_group_add( menu_accel, menu_fkey[i].key,
				note_fake[i], "clicked" );
	}
      }
      q8tk_widget_show( notebook );
      q8tk_box_pack_start( vbox, notebook );
    }
    {
	/* おつぎは、ボタン */

      w = menu_top_button();
      q8tk_box_pack_start( vbox, w );
    }
  }
  q8tk_container_add( win, vbox );


	/* ノートブックを返します */
  return notebook;
}





/****************************************************************/
/* メニューモード メイン処理					*/
/****************************************************************/

static	void	menu_init( void )
{
  int	i;

  for( i=0; i<0x10; i++ ){			/* キースキャンワーク初期化 */
    if     ( i==0x08 ) key_scan[i] |= 0xdf;		/* カナは残す */
    else if( i==0x0a ) key_scan[i] |= 0x7f;		/* CAPSも残す */
    else               key_scan[i]  = 0xff;
  }

  menu_boot_dipsw      = boot_dipsw;
  menu_boot_from_rom   = boot_from_rom;
  menu_boot_basic      = boot_basic;
  menu_boot_clock_4mhz = boot_clock_4mhz;
  menu_boot_version    = ROM_VERSION;
  menu_boot_baudrate   = baudrate_sw;
  menu_boot_sound_board= sound_board;
#if	defined(USE_SOUND) && defined(USE_FMGEN)
  menu_boot_use_fmgen  = use_fmgen;
#endif

  widget_reset_boot    = NULL;

  quickres_basic = boot_basic;
  quickres_clock = boot_clock_4mhz;


  /*screen_buf_init();*/	/* 消さなくても全部書き換えるのでOK ? */

  status_message( 0, 0, " MENU " );
  status_message( 1, 0, "<ESC> key to return" );
  status_message( 2, 0, NULL );
  draw_status();
}



void	menu_main( void )
{
  Q8tkWidget *notebook;
  int mouse_cursor_exist;
  int cpu_timing_save = cpu_timing;

  menu_init();


  q8tk_init();
  {
    if     ( strcmp( menu_kanji_code, menu_kanji_code_euc  )==0 )
    {
      q8tk_set_kanjicode( Q8TK_KANJI_EUC );
    }
    else if( strcmp( menu_kanji_code, menu_kanji_code_sjis )==0 )
    {
      q8tk_set_kanjicode( Q8TK_KANJI_SJIS );
    }else
    {
      q8tk_set_kanjicode( Q8TK_KANJI_ANK );
    }

    mouse_cursor_exist = set_mouse_state();
    q8tk_set_cursor( mouse_cursor_exist ? FALSE : TRUE );

    notebook = menu_top();

    q8tk_notebook_set_page( notebook, menu_last_page );
    {
      q8tk_main();
    }
    menu_last_page = q8tk_notebook_current_page( notebook );
  }
  q8tk_term();


  if( cpu_timing_save != cpu_timing ){
    emu_reset();
  }

  pc88main_bus_setup();
  pc88sub_bus_setup();

  if( next_emu_mode() != QUIT ){
    screen_buf_init();
  }
}
