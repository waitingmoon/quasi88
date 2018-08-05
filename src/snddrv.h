#ifndef SNDDRV_H_INCLUDED
#define SNDDRV_H_INCLUDED


enum {
  XMAME_MIXER_PSG,
  XMAME_MIXER_FM,
  XMAME_MIXER_BEEP,
  XMAME_MIXER_RHYTHM,
  XMAME_MIXER_ADPCM,
  XMAME_MIXER_FMPSG,
  XMAME_MIXER_END
};

#define	VOL_MAX		(0)
#define	FMVOL_MAX	(100)
#define	PSGVOL_MAX	(100)
#define	BEEPVOL_MAX	(100)
#define	RHYTHMVOL_MAX	(200)
#define	ADPCMVOL_MAX	(200)
#define	FMPSGVOL_MAX	(100)

#define	VOL_MIN		(-32)
#define	FMVOL_MIN	(0)
#define	PSGVOL_MIN	(0)
#define	BEEPVOL_MIN	(0)
#define	RHYTHMVOL_MIN	(0)
#define	ADPCMVOL_MIN	(0)
#define	FMPSGVOL_MIN	(0)



#ifdef	USE_SOUND

extern	int	use_sound;		/* 0:not use / 1 use sound	*/
extern	int	use_fmgen;		/* 0:not use / 1 use Fmgen	*/

int	xmame_system_init( void );
void	xmame_system_term( void );

int	xmame_check_option( char *arg1, char *arg2, int priority );
void	xmame_show_option( void );

int	xmame_sound_start( void );
void	xmame_sound_update( void );
void	xmame_sound_stop( void );
void	xmame_sound_suspend( void );
void	xmame_sound_resume( void );
void	xmame_sound_reset( void );
int	xmame_sound_is_enable( void );
int	xmame_volume_is_variable( void );
void	xmame_update_video_and_audio( void );

byte	xmame_sound_in_data( void );
byte	xmame_sound_in_status( void );
void	xmame_sound_out_reg( byte data );
void	xmame_sound_out_data( byte data );

byte	xmame_sound2_in_data( void );
byte	xmame_sound2_in_status( void );
void	xmame_sound2_out_reg( byte data );
void	xmame_sound2_out_data( byte data );

void	xmame_beep_out_data( byte data );

void	xmame_sound_timer_over( int timer );

int	xmame_get_sound_volume( void );
void	xmame_set_sound_volume( int vol );

int	xmame_get_mixer_volume( int ch );
void	xmame_set_mixer_volume( int ch, int level );


#else


#define	use_sound			(FALSE)

#define	xmame_system_init()		(TRUE)
#define	xmame_system_term()

#define	xmame_check_option( a1, a2, p )	(0)
#define	xmame_show_option()

#define	xmame_sound_start()		(TRUE)
#define	xmame_sound_update()
#define	xmame_sound_stop()
#define	xmame_sound_suspend()
#define	xmame_sound_resume()
#define	xmame_sound_reset()
#define	xmame_sound_is_enable()		(FALSE)
#define	xmame_volume_is_variable()	(FALSE)
#define	xmame_update_video_and_audio()

#define	xmame_sound_in_data()		(0xff)
#define	xmame_sound_in_status()		(0x00)
#define	xmame_sound_out_reg( d )
#define	xmame_sound_out_data( d )
#define	xmame_sound2_in_data()		(0xff)
#define	xmame_sound2_in_status()	(0xff)
#define	xmame_sound2_out_reg( d )
#define	xmame_sound2_out_data( d )
#define	xmame_beep_out_data( d )

#define	xmame_sound_timer_over( t )

#define	xmame_get_sound_volume()	(0)
#define	xmame_set_sound_volume( v )

#define	xmame_get_mixer_volume( c )	(0)
#define	xmame_set_mixer_volume( c, l )


#endif


#endif	/* SNDDRV_H_INCLUDED */
