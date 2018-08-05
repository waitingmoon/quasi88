/************************************************************************/
/* SDL用 ジョイスティック入力処理					*/
/*									*/
/*	このファイルは、 joystick.c からインクルードされます		*/
/*									*/
/************************************************************************/
#if	defined( JOY_SDL )

#include <stdio.h>
#include <SDL.h>

#include "quasi88.h"
#include "keyboard.h"
#include "joystick.h"
#include "event.h"

int	enable_joystick = FALSE;	/* ジョイスティックの使用可否 */



#define	MAX_BUTTON	2

#define	AXIS_U		0x01
#define	AXIS_D		0x02
#define	AXIS_L		0x04
#define	AXIS_R		0x08



typedef	struct {

  SDL_Joystick	*dev;

  int		axis;
  int		nr_button;
  Uint8         button[ MAX_BUTTON ];

} T_JOY;

static	T_JOY	joy_info;





void	joystick_init( void )
{
  T_JOY	*joy = &joy_info;
  int num, btn;

  enable_joystick = FALSE;

  if( verbose_proc ) printf( "\nInitializing joystick ... " );

  if( ! SDL_WasInit( SDL_INIT_JOYSTICK ) ){
    if( SDL_InitSubSystem( SDL_INIT_JOYSTICK ) ){
      if( verbose_proc ) printf( "FAILED\n" );
      return;
    }
  }

  memset( joy, 0, sizeof(*joy) );
  num = SDL_NumJoysticks();		/* ジョイスティックの数をチェック */

  if( num >= 1 ){			/* ジョイスティックあれば */

    joy->dev = SDL_JoystickOpen( 0 );		/* ジョイスティック オープン */
    if( joy->dev ){

      btn = joy->nr_button = SDL_JoystickNumButtons( joy->dev );

      if( joy->nr_button > MAX_BUTTON ){
	joy->nr_button = MAX_BUTTON;
      }

      enable_joystick = TRUE;

      if( verbose_proc ) printf( "OK (found %d button-joystick)\n", btn );
    }else{
      if( verbose_proc ) printf( "FAILED (can't open joystick)\n" );
    }
  }else{
      if( verbose_proc ) printf( "FAILED (not found joystick)\n" );
  }


  if( enable_joystick ){
    SDL_JoystickEventState( SDL_IGNORE );
  }else{
    SDL_QuitSubSystem( SDL_INIT_JOYSTICK );
  }
  return;
}



void	joystick_term( void )
{
  T_JOY	*joy = &joy_info;

  if( enable_joystick ){

    if( joy->dev ){
      SDL_JoystickClose( joy->dev );
    }

    enable_joystick = FALSE;
  }
}




void	joystick_event( void )
{
  int j;
  T_JOY	*joy = &joy_info;

  int	now = 0, chg;
  Sint16 x, y;
  Uint8 button;

  if( enable_joystick ){

    SDL_JoystickUpdate();

    if( joy->dev ){

      /* Uint8 pad = SDL_JoystickGetHat( joy->dev, 
				      SDL_HAT_UP   | SDL_HAT_RIGHT | 
				      SDL_HAT_DOWN | SDL_HAT_LEFT ); */

      x = SDL_JoystickGetAxis( joy->dev, 0 );
      y = SDL_JoystickGetAxis( joy->dev, 1 );

      if     ( x < -0x4000 ) now |= AXIS_L;
      else if( x >  0x4000 ) now |= AXIS_R;

      if     ( y < -0x4000 ) now |= AXIS_U;
      else if( y >  0x4000 ) now |= AXIS_D;

      chg = joy->axis ^ now;

      if( chg & AXIS_L ) pc88_pad( KEY88_PAD_LEFT,  (now & AXIS_L) );
      if( chg & AXIS_R ) pc88_pad( KEY88_PAD_RIGHT, (now & AXIS_R) );
      if( chg & AXIS_U ) pc88_pad( KEY88_PAD_UP,    (now & AXIS_U) );
      if( chg & AXIS_D ) pc88_pad( KEY88_PAD_DOWN,  (now & AXIS_D) );

      joy->axis = now;

      for( j=0; j<joy->nr_button; j++ ){

	button = SDL_JoystickGetButton( joy->dev, j );

	if( joy->button[j] != button ){
	  pc88_pad( KEY88_PAD_A + j, ( button ) );
	  joy->button[j] = button;
	}

      }

    }
  }
}


#endif
