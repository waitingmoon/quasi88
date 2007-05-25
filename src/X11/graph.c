/***********************************************************************
 * グラフィック処理 (システム依存)
 *
 *	詳細は、 graph.h 参照
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>

#include "quasi88.h"
#include "graph.h"
#include "device.h"
#include "joystick.h"

#include "screen.h"
#include "emu.h"	/* get_emu_mode() */


/************************************************************************/

#ifdef MITSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

static	XShmSegmentInfo	SHMInfo;
int	use_SHM		= TRUE;		/* MIT-SHM を使用するかどうか	*/
#endif


#ifdef	USE_DGA
#include <unistd.h>
#include <sys/types.h>
#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>

static	char			*dga_addr;
static	int			dga_width;
static	int			dga_bank;
static	int			dga_ram;
static	XF86VidModeModeInfo	**dga_mode = NULL;
static	int			dga_mode_count;
static	int			dga_mode_fit;
static	int			dga_switched = FALSE;
#endif



int	now_screen_size;		/* 現在の、画面サイズ		*/

int	enable_fullscreen = 0;		/* 全画面表示可能かどうか	*/
					/*    1:可 0:不可 -1:全画面のみ	*/
int	now_fullscreen	  = FALSE;	/* 現在、全画面表示中なら真	*/

int	enable_half_interp = FALSE;	/* HALF時、色補間可能かどうか	*/
int	now_half_interp    = FALSE;	/* 現在、色補完中なら真		*/

int	now_status = FALSE;		/* 現在、ステータス表示中なら真	*/



static	int     SIZE_OF_DEPTH = 0;	/* 色バイト数	(1 or 2 or 4)	*/
static	int	display_width;		/* 実際の表示横サイズ		*/

static	int	screen_bx;		/* ボーダー(枠)の幅 x (ドット)	*/
static	int	screen_by;		/*	   〃	    y (ドット)	*/

	int	SCREEN_DX = 0;		/* ウインドウ左上と、		*/
	int	SCREEN_DY = 0;		/* 画面エリア左上とのオフセット	*/

	int	colormap_type   = 0;	/* カラーマップのタイプ	-1〜2	*/
static	int	now_colormap_type;	/* 実際に使ったタイプ   -1〜2	*/

					/* colormap_type<=0 時のワーク	*/
static	Ulong	color_h_pixel[15+14+13+12+11+10+9+8+7+6+5+4+3+2+1];
					/* colormap_type==2 時のワーク	*/
static	Ulong	alloc_color[16  +15+14+13+12+11+10+9+8+7+6+5+4+3+2+1];
static	int	nr_alloc_color;


	int	status_fg = 0x000000;	/* ステータス前景色		*/
	int	status_bg = 0xd6d6d6;	/* ステータス背景色		*/

static	int	exist_status_color;	/* ステータスの色確保成功	*/

	int	mouse_rel_move;		/* マウス相対移動量検知させるか	*/

static	int	now_grab_mouse;		/* 現在、カーソル グラブ中	*/


	Display	 *display;
static	Screen   *screen;
	Window	 window;
	Atom	 atom_WM_close_type;
	Atom	 atom_WM_close_data;
static	Colormap colormap, default_colormap;
static	XImage   *image;

static	GC       gc;
static	Visual   *visual;

static	Ulong	white_pixel;


/******************************************************************************

                          WIDTH
	 ←───────────────────→
	┌────────────────────┐ ↑
	│              ↑                        │ │
	│              │SCREEN_DY               │ │
	│              ↓                        │ │
	│←─────→┌─────────┐    │ │
	│   SCREEN_DX  │  ↑              │    │ │
	│              │←───────→│    │ │HEIGHT
	│              │  │   SCREEN_SX  │    │ │
	│              │  │              │    │ │
	│              │  │SCREEN_SY     │    │ │
	│              │  ↓              │    │ │
	│              └─────────┘    │ │
	│                                        │ ↓
	├──────┬──────┬──────┤ ↑
	│ステータス0 │ステータス1 │ステータス2 │ │STATUS_HEIGHT
	└──────┴──────┴──────┘ ↓
	    ステータス0〜2のサイズ比率は、 1:3:1

******************************************************************************/

static	void	set_wm_hints( int w, int h );

static	int	create_window( int first_time );
static	void	destroy_window( void );

static	int	create_image( void );
static	void	destroy_image( void );

static	void	create_colormap( void );
static	void	destroy_colormap( void );


static	void	alloc_pixmap_for_invisible_mouse( void );
static	void	free_pixmap_for_invisible_mouse( void );


enum {
  DGA_ERR_NONE = 0,
  DGA_ERR_AVAILABLE,
  DGA_ERR_ROOT_RIGHTS,
  DGA_ERR_LOCAL_DISPLAY,
  DGA_ERR_QUERY_VERSION,
  DGA_ERR_QUERY_EXTENSION,
  DGA_ERR_QUERY_DIRECT_VIDEO,
  DGA_ERR_QUERY_DIRECT_PRESENT,
  DGA_ERR_GET_VIDEO,
  DGA_ERR_MANY_BANKS,
  DGA_ERR_XVM_QUERY_VERSION,
  DGA_ERR_XVM_QUERY_EXTENSION
};
static	int	DGA_error = DGA_ERR_AVAILABLE;

/************************************************************************/
/* X11 システム初期化							*/
/************************************************************************/
void	x11_system_init( void )
{
  enable_fullscreen = 0;

  display = XOpenDisplay( NULL );
  if( !display ) return;


		/* DGA 初期化 */
#ifdef	USE_DGA
  {
    int     i, j;
    char *s;

    if( geteuid() ){
      DGA_error = DGA_ERR_ROOT_RIGHTS;
    }
    else
    if( !(s = getenv("DISPLAY")) || (s[0] != ':') ){
      DGA_error = DGA_ERR_LOCAL_DISPLAY;
    }
    else
    if( !XF86DGAQueryVersion( display, &i, &j ) ){
      DGA_error = DGA_ERR_QUERY_VERSION;
    }
    else
    if( !XF86DGAQueryExtension( display, &i, &j ) ){
      DGA_error = DGA_ERR_QUERY_EXTENSION;
    }
    else
    if( !XF86DGAQueryDirectVideo( display, DefaultScreen(display), &i ) ){
      DGA_error = DGA_ERR_QUERY_DIRECT_VIDEO;
    }
    else
    if( !(i & XF86DGADirectPresent) ){
      DGA_error = DGA_ERR_QUERY_DIRECT_PRESENT;
    }
    else
    if( !XF86DGAGetVideo( display, DefaultScreen(display),
			  &dga_addr, &dga_width, &dga_bank, &dga_ram ) ){
      DGA_error = DGA_ERR_GET_VIDEO;
    }
    else
    if( dga_ram * 1024 != dga_bank ){
      DGA_error = DGA_ERR_MANY_BANKS;
    }
    else
    if( !XF86VidModeQueryVersion( display, &i, &j ) ){
      DGA_error = DGA_ERR_XVM_QUERY_VERSION;
    }
    else
    if( !XF86VidModeQueryExtension( display, &i, &j ) ){
      DGA_error = DGA_ERR_XVM_QUERY_EXTENSION;
    }
    else
    {
      DGA_error = DGA_ERR_NONE;
      enable_fullscreen = 1;
    }
  }
#endif
}


static	void	x11_system_init_verbose( void )
{
  if( verbose_proc ){

    if( !display ){ printf("FAILED\n"); return; }
    else          { printf("OK");               }

#ifdef	USE_DGA
    printf("\n");
    printf("  DGA : ");

    if     ( DGA_error == DGA_ERR_NONE )
      printf("OK");
    else if( DGA_error == DGA_ERR_ROOT_RIGHTS )
      printf("FAILED (Must be suid root)");
    else if( DGA_error == DGA_ERR_LOCAL_DISPLAY )
      printf("FAILED (Only works on a local display)");
    else if( DGA_error == DGA_ERR_QUERY_VERSION )
      printf("FAILED (XF86DGAQueryVersion)");
    else if( DGA_error == DGA_ERR_QUERY_EXTENSION )
      printf("FAILED (XF86DGAQueryExtension)");
    else if( DGA_error == DGA_ERR_QUERY_DIRECT_VIDEO )
      printf("FAILED (XF86DGAQueryDirectVideo)");
    else if( DGA_error == DGA_ERR_QUERY_DIRECT_PRESENT )
      printf("FAILED (Xserver not support DirectVideo)");
    else if( DGA_error == DGA_ERR_GET_VIDEO )
      printf("FAILED (XF86DGAGetVideo)");
    else if( DGA_error == DGA_ERR_MANY_BANKS )
      printf("FAILED (banked graphics modes not supported)" );
    else if( DGA_error == DGA_ERR_XVM_QUERY_VERSION )
      printf("FAILED (XF86VidModeQueryVersion)");
    else if( DGA_error == DGA_ERR_XVM_QUERY_EXTENSION )
      printf("FAILED (XF86VidModeQueryExtension)");
    else
      printf("FAILED (Not Support)" );

    if( DGA_error==DGA_ERR_NONE ) printf(", full screen available\n");
    else                          printf(", full screen not available\n");

#else
    printf(" (full screen not supported)\n");
#endif
  }
}




/************************************************************************/
/* X11 システム終了							*/
/************************************************************************/
void	x11_system_term( void )
{
  if( display ){
    XAutoRepeatOn( display );		/* オートリピート設定をもとに戻す */

#ifndef	USE_DGA		/* DGA有効時、XCloseDisplayでエラーがでる。なぜに? */
    {
      XCloseDisplay( display );
    }
#endif
    display = NULL;
  }
}




/************************************************************************/
/* グラフィックシステムの初期化						*/
/************************************************************************/
int	graphic_system_init( void )
{
  int     i, count, h, w;
  XPixmapFormatValues *pixmap;

  if( verbose_proc ){
    printf( "Initializing Graphic System (X11) ... " );
    x11_system_init_verbose();
  }

  if( ! display ){
    return 0;
  }


  screen   = DefaultScreenOfDisplay( display );
  gc       = DefaultGCOfScreen( screen );
  visual   = DefaultVisualOfScreen( screen );


  pixmap = XListPixmapFormats( display, &count );
  if( pixmap == NULL ){
    if( verbose_proc ) printf("  X11 error (Out of memory ?)\n");
    return 0;
  }
  for( i=0; i<count; i++ ){
    if( pixmap[i].depth == DefaultDepthOfScreen( screen ) ){
      DEPTH = pixmap[i].depth;
      if     ( DEPTH<= 8 && pixmap[i].bits_per_pixel== 8 ) SIZE_OF_DEPTH = 1;
      else if( DEPTH<=16 && pixmap[i].bits_per_pixel==16 ) SIZE_OF_DEPTH = 2;
      else if( DEPTH<=32 && pixmap[i].bits_per_pixel==32 ) SIZE_OF_DEPTH = 4;
      else                                                 SIZE_OF_DEPTH = 0;
      break;
    }
  }
  XFree( pixmap );


  {				/* 非対応の depth なら弾く */
    const char *s = NULL;
    switch( SIZE_OF_DEPTH ){
    case 0:	s = "this bpp is not supported";	break;
#ifndef	SUPPORT_8BPP
    case 1:	s = "8bpp is not supported";		break;
#endif
#ifndef	SUPPORT_16BPP
    case 2:	s = "16bpp is not supported";		break;
#endif
#ifndef	SUPPORT_32BPP
    case 4:	s = "32bpp is not supported";		break;
#endif
    }
    if( s ){
      if( verbose_proc ) printf( "  %s\n",s );
      return 0;
    }
  }

  if( DEPTH<4 ){
    if( verbose_proc ) printf("  < 4bpp is not supported\n" );
    return 0;
  }



  /* screen_size, WIDTH, HEIGHT にコマンドラインで指定したウインドウサイズが
     セット済みなので、それをもとにボーダー(枠)のサイズを算出する */

  w = screen_size_tbl[ screen_size ].w;
  h = screen_size_tbl[ screen_size ].h;

  screen_bx = ( ( MAX( WIDTH,  w ) - w ) / 2 ) & ~7;	/* 8の倍数 */
  screen_by = ( ( MAX( HEIGHT, h ) - h ) / 2 ) & ~1;	/* 2の倍数 */


  /* Drag & Drop 初期化 */
  xdnd_initialize();

  if( create_window( TRUE ) ){

    if( use_joydevice ){
      joystick_init();		/* ジョイスティック初期化 */
    }
    return TRUE;

  }else{
    return FALSE;
  } 
}


/* ------------------------------------------------------------------------- */

static	int	create_window_WIN( int first_time );
static	int	create_window_DGA( int first_time );

static	int	create_window( int first_time )
{
  int create_ok = FALSE;

  if( use_fullscreen ){			/* 全画面表示指定時、DGAで初期化 */

    create_ok = create_window_DGA( first_time );

    if( create_ok ){
      now_fullscreen = TRUE;
    }else{					/* 失敗ならウインドウで再試行*/
      now_fullscreen = FALSE;
      first_time = TRUE;
    }

  }else{
    now_fullscreen = FALSE;
  }

  if( create_ok == FALSE ){		/* ウインドウで初期化 */
    create_ok = create_window_WIN( first_time );
  }


  if( create_ok ){			/* ウインドウ・全画面共通処理 */

    /* ステータス用のバッファなどを算出 */

    status_sx[0] = display_width / 5;
    status_sx[1] = display_width - status_sx[0]*2;
    status_sx[2] = display_width / 5;

    status_sy[0] = 
    status_sy[1] = 
    status_sy[2] = STATUS_HEIGHT - 3;

    status_buf = &screen_buf[ WIDTH * HEIGHT * SIZE_OF_DEPTH ];

    status_start[0] = status_buf + 3*(WIDTH * SIZE_OF_DEPTH);	/* 3ライン下 */
    status_start[1] = status_start[0] + ( status_sx[0] * SIZE_OF_DEPTH );
    status_start[2] = status_start[1] + ( status_sx[1] * SIZE_OF_DEPTH );


    /* マウス表示、グラブの設定 (ついでにキーリピートも) */
    set_mouse_state();


    /* HALFサイズ時の色補完有無を設定 */
    set_half_interp();

    now_status = show_status;


    /* Drag & Drop 受け付け開始 */
    xdnd_start();
  }

  return create_ok;
}



static	int	create_window_WIN( int first_time )
{
	/* ウインドウサイズをセット */

  now_screen_size = screen_size;

  WIDTH     = screen_size_tbl[ screen_size ].w + screen_bx * 2;
  HEIGHT    = screen_size_tbl[ screen_size ].h + screen_by * 2;
  SCREEN_W  = screen_size_tbl[ screen_size ].w;
  SCREEN_H  = screen_size_tbl[ screen_size ].h;
  SCREEN_DX = screen_bx;
  SCREEN_DY = screen_by;



  if( first_time ){	/* 初回 … まだウインドウは存在しない */

	/* ウインドウを開く */

    if( verbose_proc ) printf("  Opening window ... ");
    window = XCreateSimpleWindow( display, RootWindowOfScreen(screen), 0, 0,
				  WIDTH, 
				  HEIGHT + ((show_status) ?STATUS_HEIGHT :0),
				  0,
				  WhitePixelOfScreen( screen ),
				  BlackPixelOfScreen( screen ) );
    if( verbose_proc )
      printf( "%s (%dx%dx%d)\n", (window?"OK":"FAILED"),WIDTH,HEIGHT,DEPTH );

    if( !window ){
      return 0; 
    }

	/* ウインドウのタイトルを設定 */

    XStoreName( display, window, Q_TITLE " ver " Q_VERSION );

	/* ウインドウマネージャーへ特性(サイズ変更不可)を指示する */

    set_wm_hints( WIDTH, HEIGHT + ((show_status) ?STATUS_HEIGHT :0) );

	/* カラーを確保する */

    create_colormap();

	/* X のマウスカーソル表示／非表示を設定 */

    alloc_pixmap_for_invisible_mouse();

	/* イベントの設定 */

    XSelectInput( display, window,
		  FocusChangeMask|ExposureMask|KeyPressMask|KeyReleaseMask|
		  ButtonPressMask|ButtonReleaseMask|PointerMotionMask );


    /* ステータス ON/OFF (リサイズ) 時に、画面か消えないようにする(?) */
    {
      XSetWindowAttributes attributes;
      attributes.bit_gravity = NorthWestGravity;
      XChangeWindowAttributes( display, window, CWBitGravity, &attributes );
    }

	/* 強制終了、中断に備えて、アトムを設定 */

    atom_WM_close_type = XInternAtom( display, "WM_PROTOCOLS", False );
    atom_WM_close_data = XInternAtom( display, "WM_DELETE_WINDOW", False );
    XSetWMProtocols( display, window, &atom_WM_close_data, 1 );



  }else{		/* 2回目〜 … ウインドウはすでに存在する */

    Window child;
    int x,y;

	/* ウインドウのサイズを変更する */

    set_wm_hints( WIDTH, HEIGHT + ((show_status) ?STATUS_HEIGHT :0) );

    /* リサイズしてウインドウが画面外に出てしまったらイヤなので、その場合は
       ウインドウを画面内に移動させようと思う。が環境によっては XGetGeometry()
       を使ってもちゃんと座標が取得できないし、 XMoveWindow() を使っても、
       ウインドウの枠とかを考慮せずに移動する場合がある。ウインドウマネージャー
       が関わっているからだと思うのだが、どうするのが正しいんでしょう ? */
#if 1
    /* とりあえずルートウインドウからの相対位置を求めて、原点が上か左の画面外
       だったら移動させる。仮想ウインドウマネージャーでも大丈夫だろう */

    XTranslateCoordinates( display, window, DefaultRootWindow(display), 0, 0,
			   &x, &y, &child);
    if( x < 0 || y < 0 ){
      if( x < 0 ) x = 0;
      if( y < 0 ) y = 0;
      XMoveResizeWindow( display, window, x, y, 
			 WIDTH, HEIGHT + ((show_status) ?STATUS_HEIGHT :0) );
    }else
#endif
    {
      XResizeWindow( display, window, 
		     WIDTH, HEIGHT + ((show_status) ?STATUS_HEIGHT :0) );
    }

	/* image を破棄する */

    destroy_image();
  }

	/* スクリーンバッファ と image を確保 */

  if( image == NULL ){
    if( ! create_image() ){ destroy_window(); return 0; }
  }

	/* スクリーンバッファの、描画開始位置を設定 */

  screen_start = &screen_buf[ (WIDTH*SCREEN_DY + SCREEN_DX) * SIZE_OF_DEPTH ];

	/* ステータス表示位置算出のための、パラメータセット	*/

  display_width = WIDTH;



  XMapRaised( display, window );

  return 1;
}





#ifdef	USE_DGA

static int search_vidmode( int size, int bx, int by, int *diff )
{
  int w = screen_size_tbl[ size ].w + bx * 2;
  int h = screen_size_tbl[ size ].h + by * 2;

  int i,  fit = -1,  dif = 0;

  for( i=0; i<dga_mode_count; i++ ){
    if( w <= dga_mode[i]->hdisplay  &&  h <= dga_mode[i]->vdisplay ){

      if( fit == -1 ||
	  ((dga_mode[i]->hdisplay - w) < (dga_mode[fit]->hdisplay - w) ) ){
	fit = i;
	dif = (dga_mode[i]->hdisplay - w) + (dga_mode[fit]->hdisplay - w);
      }
    }
  }
  if( diff ) *diff = dif;
  return fit;
}



static	int	create_window_DGA( int first_time )
{
  int fit, size;
  int sz[3], mode[3], dif[3];

  if( DGA_error ){
    if( first_time ){
      if( verbose_proc ) printf("  can't use full screen mode\n");
    }
    return 0;
  }

  if( verbose_proc ) printf("  check & change VIDMODE ... ");


	/* モードラインを全て取得 (first_time時のみ) */

  if( dga_mode == NULL ){
    if( ! XF86VidModeGetAllModeLines( display, DefaultScreen(display),
				      &dga_mode_count, &dga_mode ) ){

      if( verbose_proc ) printf( "FAILED (XF86VidModeGetAllModeLines)\n" );
      return 0;
    }
    dga_mode_fit = 0;

#if 0	/* debug */
    { int i;
      for( i=0; i<dga_mode_count; i++ )
	printf("%3d: %dx%d\n",i,dga_mode[i]->hdisplay,dga_mode[i]->vdisplay);
    }
#endif
  }


	/* 最適なモードラインを選び、ウインドウサイズをセット */

#if 0	/* 『FULL+ボーダー』→『FULL』→『HALF』 の順に、モードラインを探す */
  
				/* ボーダー(枠)サイズが保持できるモードを探す*/
  sz[0]   = SCREEN_SIZE_FULL;
  mode[0] = search_vidmode( sz[0], screen_bx, 
			    	   MAX( screen_by, STATUS_HEIGHT ), &dif[0] );

				/* 通常サイズで表示できる最適モードを探す */
  sz[1]   = SCREEN_SIZE_FULL;
  mode[1] = search_vidmode( sz[1], screen_size_tbl[sz[1]].dw, 
			    	   screen_size_tbl[sz[1]].dh, &dif[1] );

				/* 半分サイズで表示できる最適モードを探す */
  sz[2]   = SCREEN_SIZE_HALF;
  mode[2] = search_vidmode( sz[2], screen_size_tbl[sz[2]].dw, 
			    	   screen_size_tbl[sz[2]].dh, &dif[1] );


  if     ( mode[0] >= 0  &&  		/* よりサイズが近いモードを選ぶ */
	   mode[1] >= 0 ){
    if( dif[0] <= dif[1] ){ fit = mode[0];  size = sz[0]; }
    else                  { fit = mode[1];  size = sz[1]; }
  }
  else if( mode[0] >= 0 ) { fit = mode[0];  size = sz[0]; }
  else if( mode[1] >= 0 ) { fit = mode[1];  size = sz[1]; }
  else if( mode[2] >= 0 ) { fit = mode[2];  size = sz[2]; }
  else{
    if( verbose_proc ) printf("FAILED (not found suitable size)\n");
    if( first_time ){ XFree( dga_mode ); dga_mode = NULL; }
    else            { destroy_window();  }
    return 0;
  }

#else	/* 指定サイズから小さくしていって、モードラインを探す */

  size = screen_size;
  size = size*2+1;
  for( ; size>=0; size -- ){
    mode[0] = search_vidmode( size/2,
			      (size & 1) ? screen_size_tbl[ size/2 ].dw : 0,
			      (size & 1) ? screen_size_tbl[ size/2 ].dh
							      : STATUS_HEIGHT,
			      NULL );
    if( mode[0] >= 0 ) break;
  }
  size = size/2;
  if( mode[0] < 0 ){
    if( verbose_proc ) printf("FAILED (not found suitable size)\n");
    if( first_time ){ XFree( dga_mode ); dga_mode = NULL; }
    else            { destroy_window();  }
    return 0;
  }
  fit = mode[0];
#endif

  if( dga_mode_fit != fit ){		/* 現在と違うモードなら切り替え */
    dga_mode_fit = fit;

    if( first_time == FALSE ){			/* DGA有効のままだと失敗する?*/
      XF86DGADirectVideo( display, DefaultScreen(display), 0 );
    }
    XF86VidModeSwitchToMode( display, DefaultScreen(display), dga_mode[fit] );
    dga_switched = TRUE;
  }

					/* 最適モードのサイズで設定 */
  now_screen_size = size;

  /* メモリのサイズは、 横:dga_width 縦:dga_bank/dga_width/SIZE_OF_DEPTH  */
  /* 横はアドレス算出に使用するのでこの値を、縦は実際の表示サイズを使用   */

  WIDTH     = dga_width;
  HEIGHT    = dga_mode[ fit ]->vdisplay - STATUS_HEIGHT;
  SCREEN_W  = screen_size_tbl[ size ].w;
  SCREEN_H  = screen_size_tbl[ size ].h;
  SCREEN_DX = ( dga_mode[ fit ]->hdisplay - SCREEN_W ) / 2;
  SCREEN_DY = ( dga_mode[ fit ]->vdisplay - SCREEN_H ) / 2;


#if 0	/* debug */
  printf( "[%d] %dx%d +%d+%d\n",fit,SCREEN_W,SCREEN_H,SCREEN_DX,SCREEN_DY);
#endif
  if( verbose_proc ) printf("OK\n");



  if( first_time ){	/* 初回 … DGAはまだ有効にしていない */

	/* DGAを有効にする */

    if( verbose_proc ) printf("  staring DGA ... ");
    XF86DGADirectVideo( display, DefaultScreen(display), 
			XF86DGADirectGraphics|
			XF86DGADirectMouse|
			XF86DGADirectKeyb);

    XF86DGASetViewPort( display, DefaultScreen(display), 0, 0 );
    if( verbose_proc ) printf( "OK\n" );

	/* キーボード・マウスをグラブする */

    window = DefaultRootWindow(display);

    XGrabKeyboard( display, window, True, GrabModeAsync,
		   GrabModeAsync,  CurrentTime);

    XGrabPointer( display, window, True,
		  PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
		  GrabModeAsync, GrabModeAsync, window, None, CurrentTime );

	/* カラーを確保する */

    create_colormap();

	/* X のマウスカーソル表示／非表示を設定 */

    alloc_pixmap_for_invisible_mouse();

	/* イベントを設定すると怒られるので、設定しない */

  }else{		/* 2回目〜 … DGA関連の設定済み */

	/* モード切替の際にDGAを無効にしてしまったので、再度有効にする */

    XF86DGADirectVideo( display, DefaultScreen(display), 
			XF86DGADirectGraphics|
			XF86DGADirectMouse|
			XF86DGADirectKeyb);
  }


	/* スクリーンバッファの、描画開始位置を設定 */

  screen_buf = dga_addr;
  screen_start = &screen_buf[ (WIDTH*SCREEN_DY +SCREEN_DX) *SIZE_OF_DEPTH ];

	/* ステータス表示位置算出のための、パラメータセット	*/

  display_width = dga_mode[ fit ]->hdisplay;


  return 1;
}
#else
static	int	create_window_DGA( int first_time )
{
  return FALSE;
}
#endif











/************************************************************************/
/* グラフィックシステムの終了						*/
/************************************************************************/
static int graphic_system_no_window = FALSE;
void	graphic_system_term( void )
{
  if( graphic_system_no_window == FALSE ){
    destroy_window();
  }

  joystick_term();		/* ジョイスティック終了 */
}


static	void	destroy_window( void )
{
  free_pixmap_for_invisible_mouse();

  destroy_colormap();

#ifdef	USE_DGA
  if( now_fullscreen ){
    XF86DGADirectVideo( display, DefaultScreen(display), 0 );

    if( dga_mode ){
      if( dga_switched ){
	XF86VidModeSwitchToMode( display, DefaultScreen(display), dga_mode[0]);
	/* XF86VidModeSwitchMode( display, DefaultScreen(display), -1 );
	   XF86VidModeSwitchMode( display, DefaultScreen(display), +1 ); */
      }
      XFree( dga_mode );
      dga_mode = NULL;
    }
    dga_switched = FALSE;

    XUngrabPointer(  display, CurrentTime );
    XUngrabKeyboard( display, CurrentTime );

  }else
#endif
  {
    destroy_image();

    XDestroyWindow( display, window );

    if( now_grab_mouse ){
      XUngrabPointer( display, CurrentTime );
    }
  }

  XSync( display, True );	/* 全イベント破棄 */
}








/*--------------------------------------------------------------------------*/
/*
 * ウインドウマネージャにサイズ変更不可を指示する。
 */
static	void	set_wm_hints( int w, int h )
{
#if 1
  XSizeHints Hints;
  XWMHints WMHints;

  Hints.flags      = PSize|PMinSize|PMaxSize;
  Hints.min_width  = Hints.max_width  = Hints.base_width  = w;
  Hints.min_height = Hints.max_height = Hints.base_height = h;
  WMHints.input = True;
  WMHints.flags = InputHint;

  XSetWMHints( display, window, &WMHints );
  XSetWMNormalHints( display, window, &Hints );

#else

  XSizeHints *Hints = XAllocSizeHints();
  XWMHints *WMHints = XAllocWMHints();

  if( Hints ){
    Hints->min_width  = Hints->max_width  = w;
    Hints->min_height = Hints->max_height = h;
    Hints->flags = PMinSize | PMaxSize;

    XSetWMNormalHints( display, window, Hints );
    XFree( Hints );
  }
  if( WMHints ){
    WMHints->input = True;
    WMHints->flags = InputHint;
    XSetWMHints( display, window, WMHints );
    XFree( WMHints );
  }
#endif
}




/*--------------------------------------------------------------------------*/

/*
 * イメージ image と 仮想VRAM screen_buf を生成する。
 *	MIT-SHMの場合、 image と screen_buf は同時に生成される。
 *	通常の場合、 screen_buf は malloc で確保する。
 */


#ifdef MITSHM
/* MIT-SHM の失敗をトラップ */
static	int	private_handler( Display *display, XErrorEvent *E )
{
  char	str[256];

  if( E->error_code == BadAccess ||
      E->error_code == BadAlloc  ){
    use_SHM = FALSE;
    return 0;
  }

  XGetErrorText( display, E->error_code, str, 256 );
  fprintf( stderr, "X Error (%s)\n", str );
  fprintf( stderr, " Error Code   %d\n", E->error_code );
  fprintf( stderr, " Request Code %d\n", E->request_code );
  fprintf( stderr, " Minor code   %d\n", E->minor_code );

  exit( -1 );

  return 1;
}
#endif


static	int	create_image( void )
{
#ifdef MITSHM

  if( use_SHM ){			/* MIS-SHM が実装されてるかを判定 */
    int	tmp;
    if( ! XQueryExtension( display, "MIT-SHM", &tmp, &tmp, &tmp ) ){
      if( verbose_proc ) printf("  X-Server not support MIT-SHM\n");
      use_SHM = FALSE;
    }
  }

  if( use_SHM ){

    if( verbose_proc ) printf("  Using shared memory (MIT-SHM):\n"
			      "    CreateImage...");
    image = XShmCreateImage( display, visual, DEPTH,
			     ZPixmap, NULL, &SHMInfo,
			     WIDTH, HEIGHT + STATUS_HEIGHT );

    if( image ){

      if( verbose_proc ) printf("GetInfo...");
      SHMInfo.shmid = shmget( IPC_PRIVATE,
			      image->bytes_per_line * image->height,
			      IPC_CREAT|0777 );
      if( SHMInfo.shmid<0 ){
	use_SHM = FALSE;
      }

      XSetErrorHandler( private_handler );	/* エラーハンドラを横取り */
						/* (XShmAttach()異常検出) */
      if( use_SHM ){

	if( verbose_proc ) printf("Allocate...");
	screen_buf = 
	  image->data = SHMInfo.shmaddr = (char *)shmat( SHMInfo.shmid, 0, 0 );
	if( !screen_buf ){
	  use_SHM = FALSE;
	}


	if( use_SHM ){
	  if( verbose_proc ) printf("Attach...");
	  SHMInfo.readOnly = False;

	  if( !XShmAttach( display, &SHMInfo ) ){
	    use_SHM = FALSE;
	  }

	  XSync( display, False );
	  /* sleep(2); */
	}
      }

      if( SHMInfo.shmid>=0 ) shmctl( SHMInfo.shmid, IPC_RMID, 0 );


      if( use_SHM ){				/* すべて成功 */
	if( verbose_proc ) printf("OK\n");
      }else{					/* どっかで失敗 */
	if( verbose_proc ) printf("FAILED(can't use shared memory)\n");
	if( SHMInfo.shmaddr ) shmdt( SHMInfo.shmaddr );
	XDestroyImage( image );
	image = NULL;
      }

      XSetErrorHandler( None );			/* エラーハンドラを戻す */

    }else{
      if( verbose_proc ) printf("FAILED(can't use shared memory)\n");
      use_SHM = FALSE;
    }
  }

  if( use_SHM == FALSE )
#endif
  {
	/* スクリーンバッファを確保 */

    if( verbose_proc ) printf("  Screen buffer: Memory allocate...");
    screen_buf = (char *)malloc( WIDTH*(HEIGHT+STATUS_HEIGHT) *SIZE_OF_DEPTH );
    if( verbose_proc ){ if( screen_buf==NULL ) printf( "FAILED\n"); }
    if( screen_buf == NULL ){
      return 0;
    }


	/* スクリーンバッファをイメージに割り当て */

    if( verbose_proc ) printf("CreateImage...");
    image = XCreateImage( display, visual, DEPTH,
			  ZPixmap, 0, screen_buf,
			  WIDTH, HEIGHT + STATUS_HEIGHT, 8, 0 );
    if( verbose_proc ) printf( "%s\n", (image ? "OK" : "FAILED") );
    if( image == NULL ){
      free( screen_buf );
      screen_buf = NULL;
      return 0;
    }

  }

  return 1;
}

/*--------------------------------------------------------------------------*/

static	void	destroy_image( void )
{

#ifdef MITSHM
  if( use_SHM ){
    XShmDetach( display, &SHMInfo );
    if( SHMInfo.shmaddr ) shmdt( SHMInfo.shmaddr );
    /*if( SHMInfo.shmid>=0 )shmctl( SHMInfo.shmid,IPC_RMID,0 );*/
  }
#endif

  if( image ){
    XDestroyImage( image );
    image = NULL;
  }

#if 0		/* screen_buf はもう不要なので、ここで free しようとしたが、
		   image のほうでまだ使用中らしく、free するとコケてしまう。
		   というか、 XDestroyImage してるのに………。
		   XSync をしてもだめな様子。じゃあ、いつ free するの ?? */
#ifdef MITSHM
  if( use_SHM == FALSE )
#endif
  {
    free( screen_buf );
    screen_buf = NULL;
  }
#endif
}


/*--------------------------------------------------------------------------*/

#define	SET_STATUS_COLOR( n, r, g, b )				\
		color[ n ].pixel = status_pixel[ n ];		\
		color[ n ].red   = (r<<8)|r;			\
		color[ n ].green = (g<<8)|g;			\
		color[ n ].blue  = (b<<8)|b;			\
		color[ n ].flags = DoRed|DoGreen|DoBlue;

/*
 * カラーを確保する
 * colormap_type == -1 共有カラーセルから16個確保
 *		     0 共有カラーセルから16+α 個確保
 *		     1 プレイベートカラーマップを確保	
 *		     2 カラーセルは、使用時に適宜確保
 */
static	void	create_colormap( void )
{
  Ulong   plane[1];	/* dummy */
  int     i, j, k;

  default_colormap = colormap = DefaultColormapOfScreen( screen );

  black_pixel = BlackPixelOfScreen( screen );
  white_pixel = WhitePixelOfScreen( screen );


  enable_half_interp = FALSE;
  exist_status_color = FALSE;


  if( now_fullscreen && colormap_type <= 0 ){	/* DGAの場合は、共有カラー */
    now_colormap_type = 1;			/* マップにする必要なし    */
  }else{
    now_colormap_type = colormap_type;
  }


  if( verbose_proc ) printf("  Colormap: ");

  switch( now_colormap_type ){

  case -1:		/* 共有カラーセルを使用・HALF色補完なし (除 DGA) */
  case 0:		/* 共有カラーセルを使用・HALF色補完あり (除 DGA) */

    if( verbose_proc ) printf("shared...");
    if( XAllocColorCells( display, colormap, True, plane, 0,
			  color_pixel, COUNTOF(color_pixel) ) ){
      if( verbose_proc ) printf("OK ");

      if( XAllocColorCells( display, colormap, True, plane, 0,
			    status_pixel, COUNTOF(status_pixel)) ){
	exist_status_color = TRUE;
      }

      if( now_colormap_type==-1 ){	/* -1 の時は、HALFサイズフィルタ */
	break;				/* リング用のカラーを確保しない  */
      }
					/*  0 の時は、カラー確保を試みる */
      if( XAllocColorCells( display, colormap, True, plane, 0,
			    color_h_pixel, COUNTOF(color_h_pixel) )){
	enable_half_interp = TRUE;
      }
      if( verbose_proc ){
	if( enable_half_interp ) printf("(available -interp)\n" );
	else                     printf("(can't use -interp)\n");
      }
      break;
    }
    if( verbose_proc ) printf("FAILED, ");
    now_colormap_type = 1;	/* FALLTHROUGH */


  case 1:		/* プレイベートカラーマップを確保 */

    if( verbose_proc ) printf("private...");

    if( visual->class == PseudoColor ||
        visual->class == DirectColor ){

      colormap = XCreateColormap( display, window, visual, AllocAll );

      if( now_fullscreen == FALSE )
	XSetWindowColormap( display, window, colormap );

      if( verbose_proc ) printf("OK ");

      if( visual->class == DirectColor ){ /* DirectColor の扱いがわからん… */
	int red_shift=0, green_shift=0, blue_shift=0;
	for( i=0; i<DEPTH; i++ )
	  if( visual->red_mask   & (1<<i) ){ red_shift   = i; break; }
	for( i=0; i<DEPTH; i++ )
	  if( visual->green_mask & (1<<i) ){ green_shift = i; break; }
	for( i=0; i<DEPTH; i++ )
	  if( visual->blue_mask  & (1<<i) ){ blue_shift  = i; break; }
	for( i=0; i<16; i++ ){
	  color_pixel[i] = ( ((i << blue_shift ) & visual->blue_mask)  |
			     ((i << red_shift  ) & visual->red_mask)   |
			     ((i << green_shift) & visual->green_mask) );
	}
	black_pixel = color_pixel[ 8];
	white_pixel = color_pixel[15];

	if( verbose_proc ) printf("(can't use -interp)\n");
	break;
      }

      for( i=0; i<16; i++ ){ 		/* カラーセル 0〜15番までの */
	color_pixel[ i ] = i;		/* 16色を使用する。         */
      }

      if( DEPTH <= 4 ){		/* 4bpp (16セル) 以下の場合、もう色がない */

	black_pixel = color_pixel[ 8];	/* 黒は、テキストの黒のセルで代用 */
	white_pixel = color_pixel[15];	/* 白も、テキストの白のセルで代用 */

	if( verbose_proc ) printf("(can't use -interp)\n");

      }else{			/* 5bpp (32セル) 以上ならステータス色もあり */

	black_pixel = 16;		/* 黒は、セル 16 に割り当てる	 */

	exist_status_color = TRUE;	/* ステータス色を 17以降に割り当て */
	k = 17;
	for( i=0; i<COUNTOF(status_pixel); i++ ){
	  status_pixel[i] = k ++;
	}
				/* 8bpp (256セル) 以上あれば、HALFサイズの */
				/* フィルタリング用 のカラーセルも確保可能 */
	if( DEPTH >= 8 ){
	  for( i=0; i<COUNTOF(color_h_pixel); i++ ){
	    color_h_pixel[i] = k++;
	  }
	  enable_half_interp = TRUE;
	}
	if( verbose_proc ){
	  if( enable_half_interp ) printf("(available -interp)\n" );
	  else                     printf("(can't use -interp)\n");
	}
      }
      break;
    }
    if( verbose_proc ) printf("FAILED, no color allocated\n");
    now_colormap_type = 2;	/* FALLTHROUGH */


  case 2:		/* 色は必要時に動的に確保 */

    nr_alloc_color = 0;

    if( visual->class == StaticColor ||		/* 色不足を避けるため、    */
        visual->class == TrueColor   ){		/* PseudoColor/DirectColor */
      enable_half_interp = TRUE;		/* では、 ステータス色と   */
      exist_status_color = TRUE;		/* HALF補完色は無しにする  */
    }

    break;
  }


  if( enable_half_interp    &&	/* HALFサイズフィルタリングカラーの準備 */
      now_colormap_type < 2 ){

    for( i=0; i<16; i++ ){
      color_half_pixel[i][i] = color_pixel[i];
    }
    k = 0;
    for( i=0; i<16; i++ ){
      for( j=i+1; j<16; j++ ){
	color_half_pixel[i][j] =
	  color_half_pixel[j][i] = color_h_pixel[ k ++ ];
      }
    }
  }


  if( now_colormap_type == 1 ){	/* プライベートカラーマップ時の、黒を定義 */
    XColor  color;
    color.pixel = black_pixel;
    color.blue  = color.green = color.red = 0;
    color.flags = DoRed|DoGreen|DoBlue;
    XStoreColor( display, colormap, &color );
  }

				/* ステータス用のカラーを定義 */
  if( exist_status_color ){
    XColor  color[ COUNTOF(status_pixel) ];

    SET_STATUS_COLOR( STATUS_BG,    ((status_bg>>16)&0xff),
				    ((status_bg>> 8)&0xff),
				    ((status_bg    )&0xff) );
    SET_STATUS_COLOR( STATUS_FG,    ((status_fg>>16)&0xff),
				    ((status_fg>> 8)&0xff),
				    ((status_fg    )&0xff) );
    SET_STATUS_COLOR( STATUS_BLACK, 0x00, 0x00, 0x00 );
    SET_STATUS_COLOR( STATUS_WHITE, 0xff, 0xff, 0xff );
    SET_STATUS_COLOR( STATUS_RED,   0xff, 0x00, 0x00 );
    SET_STATUS_COLOR( STATUS_GREEN, 0x00, 0xff, 0x00 );

    switch( now_colormap_type ){
    case -1:
    case 0:
    case 1:
      XStoreColors( display, colormap, color, COUNTOF(status_pixel) );
#ifdef	USE_DGA
      if( now_fullscreen ){
	XF86DGAInstallColormap( display, DefaultScreen(display), colormap);
      }
#endif
      break;

    case 2:
      for( i=0; i<COUNTOF(status_pixel); i++ ){
	if( XAllocColor( display, colormap, &color[i] )){
	  status_pixel[i] = color[i].pixel;
	}else{
	  XFreeColors( display, colormap, status_pixel, i, 0 );
	  exist_status_color = FALSE;
	  break;
	}
      }
      break;
    }
  }
  if( exist_status_color == FALSE ){
    status_pixel[ STATUS_BG    ] = black_pixel;
    status_pixel[ STATUS_FG    ] = white_pixel;
    status_pixel[ STATUS_BLACK ] = black_pixel;
    status_pixel[ STATUS_WHITE ] = white_pixel;
    status_pixel[ STATUS_RED   ] = white_pixel;
    status_pixel[ STATUS_GREEN ] = white_pixel;
  }

}

/*--------------------------------------------------------------------------*/

static	void	destroy_colormap( void )
{
  switch( now_colormap_type ){
  case -1:
  case 0:
    XFreeColors( display, colormap, color_pixel, COUNTOF(color_pixel), 0 );
    if( enable_half_interp )
      XFreeColors( display, colormap, color_h_pixel, COUNTOF(color_h_pixel),0);
    if( exist_status_color )
      XFreeColors( display, colormap, status_pixel, COUNTOF(status_pixel),0);
    break;

  case 1:	
    XFreeColormap( display, colormap );
#if 0		/* DGAでカラーマップをセットした場合、必ずデフォルトに戻す ! */
    XSetWindowColormap( display, window, default_colormap );
#endif
    break;

  case 2:
    if( nr_alloc_color )
      XFreeColors( display, colormap, alloc_color, nr_alloc_color, 0 );
    if( exist_status_color )
      XFreeColors( display, colormap, status_pixel, COUNTOF(status_pixel),0);
    break;
  }
}











/************************************************************************/
/* グラフィックシステムの再初期化					*/
/*	screen_size, use_fullscreen より、新たなウインドウを生成する。	*/
/*	再初期化に失敗したときは、どうしようもないので、強制終了する。	*/
/************************************************************************/
int	graphic_system_restart( void )
{
  int	is_first_time = FALSE;

  if( verbose_proc ) printf("Restarting Graphic System (X11)\n");


  if( now_fullscreen && use_fullscreen ){	/* DGA のまま、サイズ変更 */

    if( now_screen_size != screen_size ){		/* サイズ変更する */

	/* いちいち一旦終了する必要はないだろ */
/*    destroy_window();
      is_first_time = TRUE;*/

    }else
    {							/* しない */
      set_half_interp();
      return 0;
    }

  }else if( now_fullscreen != use_fullscreen ){	/* DGA ←→ ウインドウ変更 */

    destroy_window();				/* 現在のモードを終了	*/
    is_first_time = TRUE;			/* 1回目の初期化	*/

  }else{					/* ウインドウのままサイズ変更*/

    if( now_screen_size == screen_size ){	/* サイズ同じならこのまま */
      set_half_interp();
      return 0;
    }
    is_first_time = FALSE;			/* 2回目以降の初期化 */
  }



  if( ! create_window( is_first_time ) ){

    fprintf(stderr,"Sorry : Graphic System Fatal Error !!!\n");

    graphic_system_no_window = TRUE;
    quasi88_exit();
  }

  return 1;
}






/************************************************************************/
/* パレット設定								*/
/************************************************************************/
void	trans_palette( SYSTEM_PALETTE_T syspal[] )
{
  int     i, j, nr_color;
  XColor  color[16 +15+14+13+12+11+10+9+8+7+6+5+4+3+2+1];

		/* 88のパレット16色分を設定 */

  for( i=0; i<16; i++ ){
    color[i].pixel = color_pixel[i];
    color[i].red   = (unsigned short)syspal[i].red   << 8;
    color[i].green = (unsigned short)syspal[i].green << 8;
    color[i].blue  = (unsigned short)syspal[i].blue  << 8;
    color[i].flags = DoRed|DoGreen|DoBlue;
  }
  nr_color = 16;


	/* HALFサイズフィルタリング可能時はフィルタパレット値を計算 */
	/* (フィルタリング用には、120色を設定) */

  if( now_half_interp ){
    for( i=0; i<16; i++ ){
      for( j=i+1; j<16; j++ ){
	color[nr_color].pixel = color_half_pixel[i][j];
	color[nr_color].red   = (color[i].red  >>1) + (color[j].red  >>1);
	color[nr_color].green = (color[i].green>>1) + (color[j].green>>1);
	color[nr_color].blue  = (color[i].blue >>1) + (color[j].blue >>1);
	color[nr_color].flags = DoRed|DoGreen|DoBlue;
	nr_color++;
      }
    }
  }




  switch( now_colormap_type ){
  case -1:
  case 0:
  case 1:
    XStoreColors( display, colormap, color, nr_color );
#ifdef	USE_DGA
    if( now_fullscreen ){
      XF86DGAInstallColormap( display, DefaultScreen(display), colormap);
    }
#endif
    break;

  case 2:			/* 以前のカラーを解放して 新たなカラーを確保 */
    if( nr_alloc_color ){
      XFreeColors( display, colormap, alloc_color, nr_alloc_color, 0 );
      nr_alloc_color = 0;
    }

    for( i=0; i<16; i++ ){
      if( (color[i].blue | color[i].red | color[i].green) &&
	  XAllocColor( display, colormap, &color[i] ) ){

	color_pixel[i] = color[i].pixel;
	alloc_color[ nr_alloc_color ++ ] = color[i].pixel;
      }else{
	color_pixel[i] = black_pixel;
      }
    }

    if( now_half_interp ){
      for( i=0; i<16; i++ ){
	color_half_pixel[i][i] = color_pixel[i];
      }
      nr_color = 16;
      for( i=0; i<16; i++ ){
	for( j=i+1; j<16; j++ ){
	  if( XAllocColor( display, colormap, &color[nr_color] )){

	    color_half_pixel[i][j] = color[nr_color].pixel;
	    alloc_color[ nr_alloc_color ++ ] = color[nr_color].pixel;
	  }else{
	    color_half_pixel[i][j] = black_pixel;
	  }
	  color_half_pixel[j][i] = color_half_pixel[i][j];
	  nr_color++;
	}
      }
    }
  }
}






/************************************************************************/
/* 画面表示								*/
/*	ところで、 XSync って 拡張機能なの？ 使えない環境がある？	*/
/*	その場合は、 XFlush( display ) に置き換えればいいが……		*/
/************************************************************************/
/*
 *	ボーダー(枠)、ステータスも含めて全てを表示する
 */
void	put_image_all( void )
{
  if( ! now_fullscreen ){

#ifdef MITSHM
    if( use_SHM ){
      XShmPutImage( display, window, gc, image,
		    0, 0, 0, 0,
		    WIDTH, HEIGHT + ((now_status) ?STATUS_HEIGHT :0), 
		    False );
    }else
#endif
    {
      XPutImage( display, window, gc, image,
		 0, 0, 0, 0, 
		 WIDTH, HEIGHT + ((now_status) ?STATUS_HEIGHT :0) );
    }

    XSync( display, False );

  }else{
    /* フルスクリーン時の画面サイズは、 now_status に関わらず、
       WIDTH * (HEIGHT + STATUS_HEIGHT) だが、DGAなので処理する必要なし */
  }
}


/*
 *	VRAMの (x0,y0)-(x1,y1) および 指定されたステータスを表示する
 */
void	put_image( int x0, int y0, int x1, int y1, int st0, int st1, int st2 )
{
  if( ! now_fullscreen ){

    if( x0 >= 0 ){
      if      ( now_screen_size == SCREEN_SIZE_FULL ){
	;
      }else if( now_screen_size == SCREEN_SIZE_HALF ){
	x0 /= 2;  x1 /= 2;  y0 /= 2;  y1 /= 2;
      }else  /* now_screen_size == SCREEN_SIZE_DOUBLE */ {
	x0 *= 2;  x1 *= 2;  y0 *= 2;  y1 *= 2;
      }
    }

#ifdef MITSHM
    if( use_SHM ){
      if( x0 >= 0 ){ XShmPutImage( display, window, gc, image,
				   SCREEN_DX + x0, SCREEN_DY + y0,
				   SCREEN_DX + x0, SCREEN_DY + y0,
				   x1 - x0, y1 - y0,
				   False ); }
      if( now_status ){
	if( st0 ){ XShmPutImage( display, window, gc, image,
				 0, HEIGHT,
				 0, HEIGHT,
				 status_sx[0], STATUS_HEIGHT,
				 False ); }
	if( st1 ){ XShmPutImage( display, window, gc, image,
				 status_sx[0], HEIGHT,
				 status_sx[0], HEIGHT,
				 status_sx[1], STATUS_HEIGHT,
				 False ); }
	if( st2 ){ XShmPutImage( display, window, gc, image,
				 status_sx[0] + status_sx[1], HEIGHT,
				 status_sx[0] + status_sx[1], HEIGHT,
				 status_sx[2], STATUS_HEIGHT,
				 False ); }
      }
    }else
#endif
    {
      if( x0 >= 0 ){ XPutImage( display, window, gc, image,
				SCREEN_DX + x0, SCREEN_DY + y0,
				SCREEN_DX + x0, SCREEN_DY + y0,
				x1 - x0, y1 - y0 ); }
      if( now_status ){
	if( st0 ){ XPutImage( display, window, gc, image,
			      0, HEIGHT,
			      0, HEIGHT,
			      status_sx[0], STATUS_HEIGHT ); }
	if( st1 ){ XPutImage( display, window, gc, image,
			      status_sx[0], HEIGHT,
			      status_sx[0], HEIGHT,
			      status_sx[1], STATUS_HEIGHT ); }
	if( st2 ){ XPutImage( display, window, gc, image,
			      status_sx[0] + status_sx[1], HEIGHT,
			      status_sx[0] + status_sx[1], HEIGHT,
			      status_sx[2], STATUS_HEIGHT ); }
      }
    }

    XSync( display, False );

  }else{
    /* フルスクリーン時のは、 now_status に関わらず st0, st1, st2 に応じて
       イメージを転送しないといけないが、DGAなので処理する必要なし */
  }

}




/************************************************************************/
/* ステータスを表示・非表示切り替え					*/
/*	show_status に基づいて切り替える				*/
/*		ウインドウ時は、ウインドウサイズを変更する		*/
/*		全画面時は、処理なしのはず・・・			*/
/*	状態不変なら 0 を、再描画不要なら 1 を、再描画必要なら 2 を返す	*/
/************************************************************************/
int	set_status_window( void )
{
  if( now_status == show_status ){		/* ステータスそのまま	*/
    return 0;
  }


  if( now_fullscreen == FALSE ){

    set_wm_hints( WIDTH, HEIGHT + ((show_status) ?STATUS_HEIGHT :0) );

    XResizeWindow( display, window, 
		   WIDTH, HEIGHT + ((show_status) ?STATUS_HEIGHT :0) );

    now_status = show_status;

    /* Expose イベントが起こるので、明示的な再描画は不要 ? */
    return 1;

  }else{

    /* フルスクリーン時は、ステータス表示有無に関わらず、画面サイズは同じ */
    now_status = show_status;
    return 1;
  }
}



/************************************************************************/
/* HALFサイズ時の色補完の有効・無効を設定				*/
/************************************************************************/
int	set_half_interp( void )
{
  if( now_screen_size == SCREEN_SIZE_HALF &&	/* 現在 HALFサイズで    */
      enable_half_interp  &&			/* フィルタリング可能で	*/
      use_half_interp ){			/* 色補完してありなら   */

    now_half_interp = TRUE;

  }else{
    now_half_interp = FALSE;
  }

  return now_half_interp;
}



/************************************************************************/
/* ウインドウのバーに表示するタイトルを設定				*/
/************************************************************************/
void	set_window_title( const char *title )
{
  XStoreName( display, window, title );
}



/************************************************************************/
/* マウスのグラブ・表示を設定する関数					*/
/*	グローバル変数 grab_mouse 、 hide_mouse に基づき、設定する	*/
/*									*/
/*	ついでなんで、キーリピートなども設定してしまおう。		*/
/************************************************************************/

static	char ms_data[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
static	Pixmap cursor_pix;
static	Cursor cursor_id;
static	XColor cursor_col;

static	void	alloc_pixmap_for_invisible_mouse( void )
{
  cursor_pix       = XCreateBitmapFromData( display, window, ms_data, 8, 8 );
  cursor_col.pixel = black_pixel;
  cursor_id        = XCreatePixmapCursor( display, cursor_pix, cursor_pix,
					  &cursor_col, &cursor_col, 0, 0 );
}
static	void	free_pixmap_for_invisible_mouse( void )
{
  XUndefineCursor( display, window );
  XFreePixmap( display, cursor_pix );
}



int	set_mouse_state( void )
{
  int	repeat;		/* オートリピートの有無	*/
  int	mouse;		/* マウス表示の有無	*/
  int	grab;		/* グラブの有無		*/

  if( get_emu_mode() == EXEC ){

    if( get_focus ) repeat = FALSE;
    else            repeat = TRUE;

    if( now_fullscreen || grab_mouse ){
      mouse  = FALSE;
      grab   = TRUE;
    }else{
      mouse  = (hide_mouse) ? FALSE : TRUE;
      grab   = FALSE;
    }

  }else{

    repeat = TRUE;

    if( now_fullscreen ){	/* DGA中はマウス表示できない */
      mouse  = FALSE;
      grab   = TRUE;
    }else{
      mouse  = TRUE;
      grab   = FALSE;
    }
  }


  if( repeat ) XAutoRepeatOn( display );
  else         XAutoRepeatOff( display );

  if( mouse ) XUndefineCursor( display, window );
  else        XDefineCursor( display, window, cursor_id );

  if( grab ) XGrabPointer( display, window, True,
		   PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
			   GrabModeAsync, GrabModeAsync, window, None, 
			   CurrentTime );
  else       XUngrabPointer( display, CurrentTime );


  if     ( now_fullscreen ) mouse_rel_move = 1;	/* 相対移動量 */
  else if( grab )           mouse_rel_move = -1;/* むりやり相対移動量 */
  else                      mouse_rel_move = 0;	/* 絶対座標 */

  now_grab_mouse = grab;

/*printf( "K=%d M=%d G=%d %d\n",repeat,mouse,grab,mouse_rel_move);*/

  return mouse;
}
