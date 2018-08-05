/***********************************************************************
 * グラフィック処理 (システム依存)
 *
 *	詳細は、 graph.h 参照
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

#include "quasi88.h"
#include "graph.h"
#include "device.h"

#include "screen.h"
#include "emu.h"	/* get_emu_mode() */




#define	BIT_OF_DEPTH	16		/* 色は、16bpp で固定		*/
#define	SIZE_OF_DEPTH	2

/************************************************************************/

int	now_screen_size;		/* 現在の、画面サイズ		*/

int	enable_fullscreen = 1;		/* 全画面表示可能かどうか	*/
					/*    1:可 0:不可 -1:全画面のみ	*/
int	now_fullscreen	  = FALSE;	/* 現在、全画面表示中なら真	*/

int	enable_half_interp = TRUE;	/* HALF時、色補間可能かどうか	*/
int	now_half_interp    = FALSE;	/* 現在、色補完中なら真		*/




static	int	screen_bx;		/* ボーダー(枠)サイズ x(ドット)	*/
static	int	screen_by;		/*		      y(ドット)	*/

	int	SCREEN_DX = 0;		/* ウインドウ左上と、		*/
	int	SCREEN_DY = 0;		/* 画面エリア左上とのオフセット	*/



int	now_status = FALSE;		/* 現在、ステータス表示中なら真	*/
int	status_fg = 0x000000;		/* ステータス前景色		*/
int	status_bg = 0xd6d6d6;		/* ステータス背景色		*/


int	mouse_rel_move;			/* マウス相対移動量検知可能か	*/
int	use_hwsurface	= TRUE;		/* HW SURFACE を使うかどうか	*/
int	use_doublebuf	= FALSE;	/* ダブルバッファを使うかどうか	*/
int	use_swcursor	= TRUE;		/* メニューでカーソル表示するか	*/



static	SDL_Surface	*display;
	SDL_Surface	*offscreen;

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

static	int	open_display( int first_time );
/*static void	close_display( void );*/



/************************************************************************/
/* SDL システム初期化							*/
/************************************************************************/
void	sdl_system_init( void )
{
  if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) < 0 ){
    fprintf( stderr, "SDL Error: %s\n",SDL_GetError() );
  }
}




/************************************************************************/
/* SDL システム終了							*/
/************************************************************************/
void	sdl_system_term( void )
{
  SDL_Quit();
}




/************************************************************************/
/* グラフィックシステムの初期化						*/
/************************************************************************/
int	graphic_system_init( void )
{
  int w, h;

  black_pixel   = 0x00000000;
  DEPTH         = BIT_OF_DEPTH;


  /* screen_size, WIDTH, HEIGHT にコマンドラインで指定したウインドウサイズが
     セット済みなので、それをもとにボーダー(枠)のサイズを算出する */

  w = screen_size_tbl[ screen_size ].w;
  h = screen_size_tbl[ screen_size ].h;

  screen_bx = ( ( MAX( WIDTH,  w ) - w ) / 2 ) & ~7;	/* 8の倍数 */
  screen_by = ( ( MAX( HEIGHT, h ) - h ) / 2 ) & ~1;	/* 2の倍数 */

  return open_display( TRUE );
}


#define	SET_STATUS_COLOR( n, r, g, b )				\
		status_pixel[ n ] = (((r)>>3) & 0x1f) << 11 |	\
				    (((g)>>3) & 0x1f) <<  6 |	\
				    (((b)>>3) & 0x1f)



static	int	open_display( int first_time )
{
  char video_driver[16];
  int size;
  Uint32 flags;

  if( verbose_proc ){
    if( first_time ) printf("Initializing Graphic System (SDL)");
    else             printf("Restarting Graphic System (SDL)");
  }


	/* ディスプレイを開く */

  if( ! SDL_WasInit( SDL_INIT_VIDEO ) ){
    if( SDL_InitSubSystem( SDL_INIT_VIDEO ) != 0 ){
      if( verbose_proc ) printf(" ... FAILED\n");
      return 0;
    }
  }
  if( verbose_proc ) printf("\n");

  video_driver[0] = '\0';
  SDL_VideoDriverName( video_driver, sizeof(video_driver) );
  if( verbose_proc ){
    printf( "  VideoDriver ... %s\n",
	    video_driver[0] ? video_driver : "???" );
  }


  /* 全画面専用ドライバの場合、変更可能な画面サイズをここで確定させる */

  if( first_time &&			/* 全画面専用のドライバを以下に列記 */
      (  strcmp( video_driver, "dga" )     == 0
      || strcmp( video_driver, "svgalib" ) == 0
      || strcmp( video_driver, "DSp" )     == 0
      ) ){
    int i, j;
    SDL_Rect **modes;

    if( use_hwsurface ) flags = SDL_HWPALETTE | SDL_HWSURFACE | SDL_FULLSCREEN;
    else                flags = SDL_HWPALETTE | SDL_SWSURFACE | SDL_FULLSCREEN;
    if( use_doublebuf ) flags |= SDL_DOUBLEBUF;

    modes = SDL_ListModes( NULL, flags );

    if      ( modes == (SDL_Rect**) 0 ){	/* 全モード不可 */
      screen_size_max = SCREEN_SIZE_FULL;			/* sigh...*/
    }else if( modes == (SDL_Rect**)-1 ){	/* 全モード可 */
      screen_size_max = SCREEN_SIZE_END - 1;
    }else{					/* 各モードをチェック */
      screen_size_max = -1;
      for( i=0; modes[i]; i++ ){
	for( j=0; j<SCREEN_SIZE_END; j++ ){
	  if( modes[i]->w >= screen_size_tbl[j].w + 0             * 2 &&
	      modes[i]->h >= screen_size_tbl[j].h + STATUS_HEIGHT * 2 ){

	    if( screen_size_max < j ) screen_size_max = j;
	  }
	}
      }
      if( screen_size_max < 0 ){			/* 全サイズ不可 */
	screen_size_max = SCREEN_SIZE_FULL;			/* sigh...*/
      }
    }


    enable_fullscreen = -1;
    use_fullscreen    = TRUE;
    have_mouse_cursor = FALSE;
  }


#if 0
  {
    FILE *fp = stdout; /*fopen("/tmp/sdl.log","w");*/
    SDL_Rect **modes; int i, j;
    if( fp ){
      for( j=0; j<2; j++ ){
	if(j==0) i = SDL_DOUBLEBUF|SDL_HWPALETTE|SDL_HWSURFACE;
	else     i = SDL_DOUBLEBUF|SDL_HWPALETTE|SDL_HWSURFACE|SDL_FULLSCREEN;
	modes = SDL_ListModes(NULL,i);
	fprintf(fp,"%s modes\n",(j==0)?"Window":"Fullscreen");
	if     ( modes==(SDL_Rect**) 0){ fprintf(fp,"  No modes\n");  }
	else if( modes==(SDL_Rect**)-1){ fprintf(fp,"  All modes\n"); }
	else{
	  for(i=0;modes[i];i++){
	    fprintf(fp,"  %2d: %d %d\n",i,modes[i]->w,modes[i]->h);
	  }
	}
      }
    }
  }
#endif

	/* ウインドウを開く */

  if( use_fullscreen ){				/* フルスクリーン表示の場合 */

    if( verbose_proc ) printf( "  Trying full screen mode ... " );

    if( use_hwsurface ) flags = SDL_HWPALETTE | SDL_HWSURFACE | SDL_FULLSCREEN;
    else                flags = SDL_HWPALETTE | SDL_SWSURFACE | SDL_FULLSCREEN;

    if( use_doublebuf ) flags |= SDL_DOUBLEBUF;

    display = NULL;
#if 0
    if( enable_fullscreen < 0 ) size = screen_size;
    else                        size = SCREEN_SIZE_FULL;
#else
    size = screen_size;
#endif
    size = size*2+1;
    for( ; size>=0; size -- ){

      SCREEN_W  = screen_size_tbl[ size/2 ].w;
      SCREEN_H  = screen_size_tbl[ size/2 ].h;
      SCREEN_DX = (size & 1) ? screen_size_tbl[ size/2 ].dw : 0;
      SCREEN_DY = (size & 1) ? screen_size_tbl[ size/2 ].dh : STATUS_HEIGHT;
      WIDTH     = SCREEN_W + SCREEN_DX * 2;
      HEIGHT    = SCREEN_H + SCREEN_DY * 2;

      if( verbose_proc ) printf( "(%dx%d) ... ",WIDTH,HEIGHT );

      display = SDL_SetVideoMode( WIDTH, HEIGHT, DEPTH, flags );
      if( display ) break;
    }
    size = size/2;

    if( verbose_proc ) printf( "%s\n", (display ? "OK" : "FAILED") );

    if( display ){ now_fullscreen = TRUE;   HEIGHT -= STATUS_HEIGHT; }
    else         { use_fullscreen = FALSE; }

    if( ! display && enable_fullscreen < 0 ) return 0;
  }

  if( ! use_fullscreen ){			/* ウインドウ表示の場合 */

    if( verbose_proc ) printf( "  Opening window ... " );

    size      = screen_size;
    SCREEN_W  = screen_size_tbl[ size ].w;
    SCREEN_H  = screen_size_tbl[ size ].h;
    SCREEN_DX = screen_bx;
    SCREEN_DY = screen_by;
    WIDTH     = SCREEN_W + SCREEN_DX * 2;
    HEIGHT    = SCREEN_H + SCREEN_DY * 2;

    if( use_hwsurface ) flags = SDL_HWPALETTE | SDL_HWSURFACE;
    else                flags = SDL_HWPALETTE | SDL_SWSURFACE;

    if( use_doublebuf )
      flags |= SDL_DOUBLEBUF;

    display = SDL_SetVideoMode( WIDTH,
				HEIGHT + ((show_status) ? STATUS_HEIGHT : 0),
				DEPTH, flags );

    if( verbose_proc ) printf( "%s\n", (display ? "OK" : "FAILED") );

    if( display ){ now_fullscreen = FALSE; }
    else         { return 0;               }
  }

  now_screen_size = size;

  if( verbose_proc )
    printf("    VideoMode %dx%d->%dx%dx%d(%d)  %c%c%c%c  R:%x G:%x B:%x\n",
	   WIDTH, HEIGHT,
	   display->w, display->h,
	   display->format->BitsPerPixel, display->format->BytesPerPixel,
	   (display->flags & SDL_SWSURFACE) ? 'S' : '-',
	   (display->flags & SDL_HWSURFACE) ? 'H' : 'S',
	   (display->flags & SDL_DOUBLEBUF) ? 'D' : '-',
	   (display->flags & SDL_FULLSCREEN) ? 'F' : '-',
	 display->format->Rmask,display->format->Gmask,display->format->Bmask);


  /* ウインドウのタイトルを表示 */
  SDL_WM_SetCaption( Q_TITLE " ver " Q_VERSION, Q_TITLE " ver " Q_VERSION );

  /* アイコンを設定するならここで。WIN32の場合、32x32 に限る */
  /* SDL_WM_SetIcon(SDL_Surface *icon, Uint8 *mask); */


  /* スクリーンバッファを確保 */
  if( verbose_proc ) printf( "  Allocating screen buffer ... " );

  offscreen = SDL_CreateRGBSurface( SDL_SWSURFACE,
				    WIDTH,
				    HEIGHT + STATUS_HEIGHT,
				    DEPTH,
				    0, 0, 0, 0 );

  if( verbose_proc ) printf( "%s\n", (offscreen ? "OK" : "FAILED") );

  if( offscreen ){ screen_buf = (char *)offscreen->pixels; }
  else           { return 0;                               }


  /* スクリーンバッファの、描画開始位置を設定	*/
  
  screen_start = &screen_buf[ (WIDTH*SCREEN_DY + SCREEN_DX) * SIZE_OF_DEPTH ];



  /* ステータス用のバッファなどを算出 */
  {
    status_sx[0] = WIDTH / 5;
    status_sx[1] = WIDTH - status_sx[0]*2;
    status_sx[2] = WIDTH / 5;

    status_sy[0] = 
    status_sy[1] = 
    status_sy[2] = STATUS_HEIGHT - 3;

    status_buf = &screen_buf[ WIDTH * HEIGHT * SIZE_OF_DEPTH ];

    status_start[0] = status_buf + 3*(WIDTH * SIZE_OF_DEPTH);	/* 3ライン下 */
    status_start[1] = status_start[0] + ( status_sx[0] * SIZE_OF_DEPTH );
    status_start[2] = status_start[1] + ( status_sx[1] * SIZE_OF_DEPTH );
  }


  /* マウス表示、グラブの設定 (ついでにキーリピートも) */
  set_mouse_state();


  /* ステータス用の色ピクセルを定義 */

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


  /* どこかで初期化せねば */
  if( use_joydevice ){
    joy_init();
  }

  /* HALFサイズ時の色補完有無を設定 */
  set_half_interp();

  now_status = show_status;

  return(1);
}



/************************************************************************/
/* グラフィックシステムの終了						*/
/************************************************************************/
void	graphic_system_term( void )
{
  SDL_WM_GrabInput( SDL_GRAB_OFF );

  SDL_QuitSubSystem( SDL_INIT_VIDEO );
}



/************************************************************************/
/* グラフィックシステムの再初期化					*/
/*	screen_size, use_fullscreen より、新たなウインドウを生成する。	*/
/*	再初期化に失敗したときは、どうしようもないので、強制終了する。	*/
/************************************************************************/
int	graphic_system_restart( void )
{
  if( now_fullscreen && use_fullscreen ){	/* 全画面のままです */

    if( now_screen_size != screen_size
	/* && enable_fullscreen < 0 */ ){

      SDL_QuitSubSystem( SDL_INIT_VIDEO );

    }else
    {
      set_half_interp();
      return 0;
    }

  }else if( now_fullscreen != use_fullscreen ){	/* 全画面←→ウインドウ切替 */

#if 0
    if( SDL_WM_ToggleFullScreen( display ) ) return;
#endif
    SDL_QuitSubSystem( SDL_INIT_VIDEO );		/* 一旦 VIDEO 終了 */

  }else{					/* ウインドウサイズ変更 */

    if( now_screen_size == screen_size ){	/* サイズ同じならこのまま */
      set_half_interp();
      return 0;
    }

    /* VIDEO を終了するとウインドウが破棄→生成されて鬱陶しいのでしない */
    /* SDL_QuitSubSystem( SDL_INIT_VIDEO ); */
  }



  if( ! open_display( FALSE ) ){
    fprintf(stderr,"Sorry : Graphic System Fatal Error !!!\n");

    quasi88_exit();
  }

  return 1;
}



/************************************************************************/
/* パレット設定								*/
/************************************************************************/
void	trans_palette( SYSTEM_PALETTE_T syspal[] )
{
  int     i, j;

	/* パレット値をコピー */

  for( i=0; i<16; i++ ){
    color_pixel[i] = (((syspal[i].red  >>3)&0x1f) <<11)|
		     (((syspal[i].green>>3)&0x1f) << 6)|
		     (((syspal[i].blue >>3)&0x1f));
  }


	/* HALFサイズフィルタリング可能時はフィルタパレット値を計算 */

  if( now_half_interp ){
    SYSTEM_PALETTE_T hpal[16];
    for( i=0; i<16; i++ ){
      hpal[i].red   = syspal[i].red   >> 1;
      hpal[i].green = syspal[i].green >> 1;
      hpal[i].blue  = syspal[i].blue  >> 1;
    }

    for( i=0; i<16; i++ ){
      color_half_pixel[i][i] = color_pixel[i];
    }
    for( i=0; i<16; i++ ){
      for( j=i+1; j<16; j++ ){
	color_half_pixel[i][j]=((((hpal[i].red  +hpal[j].red  )>>3)&0x1f)<<11)|
			       ((((hpal[i].green+hpal[j].green)>>3)&0x1f)<< 6)|
			       ((((hpal[i].blue +hpal[j].blue )>>3)&0x1f));
	color_half_pixel[j][i] = color_half_pixel[i][j];
      }
    }
  }
}




/************************************************************************/
/* 画面表示								*/
/************************************************************************/
/*
 *	ボーダー(枠)、ステータスも含めて全てを表示する
 */
void	put_image_all( void )
{
  SDL_Rect srect, drect;
  int h;

  if( now_fullscreen ) h = HEIGHT + STATUS_HEIGHT;
  else                 h = HEIGHT + ((now_status) ? STATUS_HEIGHT : 0);


  drect.x = srect.x = 0;	srect.w = WIDTH;
  drect.y = srect.y = 0;	srect.h = h;

  if( SDL_BlitSurface( offscreen, &srect, display, &drect ) < 0 ){
    fprintf( stderr, "SDL: Warn: Unsuccessful blitting\n" );
  }
  /* SDL_UpdateRect(display, 0,0,0,0); */
  SDL_Flip(display);
}


/*
 *	VRAMの (x0,y0)-(x1,y1) および 指定されたステータスを表示する
 */
void	put_image( int x0, int y0, int x1, int y1, int st0, int st1, int st2 )
{
  int      i, flag = 0, nr_update = 0;
  SDL_Rect srect[4], drect,  update[4];

  if( x0 >= 0 ){
    if      ( now_screen_size == SCREEN_SIZE_FULL ){
      ;
    }else if( now_screen_size == SCREEN_SIZE_HALF ){
      x0 /= 2;  x1 /= 2;  y0 /= 2;  y1 /= 2;
    }else  /* now_screen_size == SCREEN_SIZE_DOUBLE */ {
      x0 *= 2;  x1 *= 2;  y0 *= 2;  y1 *= 2;
    }

    flag |= 1;
    srect[0].x = SCREEN_DX + x0;	srect[0].w = x1 - x0;
    srect[0].y = SCREEN_DY + y0;	srect[0].h = y1 - y0;
  }
  if( now_status || now_fullscreen ){
    if( st0 ){
      flag |= 2;
      srect[1].x = 0;			srect[1].w = status_sx[0];
      srect[1].y = HEIGHT;		srect[1].h = STATUS_HEIGHT;
    }
    if( st1 ){
      flag |= 4;
      srect[2].x = status_sx[0];	srect[2].w = status_sx[1];
      srect[2].y = HEIGHT;		srect[2].h = STATUS_HEIGHT;
    }
    if( st2 ){
      flag |= 8;
      srect[3].x = status_sx[0] + status_sx[1];	srect[3].w = status_sx[2];
      srect[3].y = HEIGHT;			srect[3].h = STATUS_HEIGHT;
    }
  }

  for( i=0; i<4; i++ ){
    if( flag & (1<<i ) ){
      drect = srect[i];

      if( SDL_BlitSurface( offscreen, &srect[i], display, &drect ) < 0 ){
	fprintf( stderr, "SDL: Warn: Unsuccessful blitting\n" );
      }

      update[ nr_update ++ ] = srect[i];
    }
  }

  if( display->flags & SDL_DOUBLEBUF ){

    SDL_Flip( display );

    for( i=0; i<4; i++ ){
      if( flag & (1<<i ) ){
	drect = srect[i];
	SDL_BlitSurface( offscreen, &srect[i], display, &drect );
      }
    }

  }else{

    SDL_UpdateRects( display, nr_update, update );
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

    /* show_status に応じて、画面サイズを変える */

    if( ! open_display( FALSE ) ){
      fprintf(stderr,"Sorry : Graphic System Fatal Error !!!\n");

      quasi88_exit();
    }
    return 2;

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
  if( now_screen_size == SCREEN_SIZE_HALF &&
      use_half_interp ){

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
  SDL_WM_SetCaption( title, title );
}



/************************************************************************/
/* マウスのグラブ・表示を設定する関数					*/
/*	グローバル変数 grab_mouse 、 hide_mouse に基づき、設定する	*/
/*									*/
/*	ついでなんで、キーリピートなども設定してしまおう。		*/
/************************************************************************/
int	set_mouse_state( void )
{
  int	repeat;		/* オートリピートの有無	*/
  int	mouse;		/* マウス表示の有無	*/
  int	grab;		/* グラブの有無		*/

  if( get_emu_mode() == EXEC ){

    repeat = FALSE;
    if( now_fullscreen || grab_mouse ){
      mouse  = FALSE;
      grab   = TRUE;
    }else{
      mouse  = (hide_mouse) ? FALSE : TRUE;
      grab   = FALSE;
    }

  }else{

    repeat = TRUE;
    mouse  = (now_fullscreen && use_swcursor) ? FALSE : TRUE;
    grab   = (now_fullscreen) ? TRUE : FALSE;
  }


  if( repeat )
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
  else
    SDL_EnableKeyRepeat( 0, 0 );

  if( mouse ) SDL_ShowCursor( SDL_ENABLE );
  else        SDL_ShowCursor( SDL_DISABLE );

  if( grab ) SDL_WM_GrabInput( SDL_GRAB_ON );
  else       SDL_WM_GrabInput( SDL_GRAB_OFF );

  mouse_rel_move = (!mouse && grab) ? TRUE : FALSE;


/*printf( "K=%d M=%d G=%d\n",repeat,mouse,grab);*/

  return mouse;
}
/*
  フルスクリーンの問題点 (Winにて発生)
  ダブルバッファの場合、マウスが正常に表示されない？
  シングルバッファでハードウェアサーフェスの場合、
  マウスをONにした瞬間、マウスの表示すべき位置にゴミが残る？

  ↓
  メニュー画面遷移だけの問題なので、ソフトウェアカーソルでごまかそう
*/
