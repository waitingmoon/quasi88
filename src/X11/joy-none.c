/************************************************************************/
/* ダミー用 ジョイスティック入力処理					*/
/*									*/
/*	このファイルは、 joystick.c からインクルードされます		*/
/*									*/
/************************************************************************/
#if	defined( JOY_NOTHING )


#include <stdio.h>

#include "quasi88.h"
#include "device.h"
#include "keyboard.h"
#include "joystick.h"
#include "event.h"

int	enable_joystick = FALSE;	/* ジョイスティックの使用可否 */



void	joystick_init( void )
{
  if( use_joydevice ){
    if( verbose_proc ) printf( "\nJoystick not supported\n" );
  }
  enable_joystick = FALSE;
}

void	joystick_term( void )
{
}

void	joystick_event( void )
{
}


#endif
