/*
 * MAME/XMAME の サウンドドライバに必要な宣言のよせあつめ
 */

#ifndef MAME_QUASI88_H_INCLUDED
#define MAME_QUASI88_H_INCLUDED


#undef	EXTERN
#ifdef	MAME_QUASI88_WORK_DEFINE
#define	EXTERN
#else
#define EXTERN extern
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "quasi88.h"
#include "z80.h"
#include "pc88cpu.h"
#include "intr.h"
#include "initval.h"
#include "soundbd.h"



/* src/windows/osd_cpu.h
   src/unix/osd_cpu.h ====================================================== */

typedef signed   char      INT8;
typedef signed   short     INT16;
typedef signed   int       INT32;
typedef unsigned char      UINT8;
typedef unsigned short     UINT16;
typedef unsigned int       UINT32;



/* src/mame.mak
   src/rules.mak =========================================================== */

#define	HAS_YM2203		(1)
#define	HAS_YM2608		(1)
#define	HAS_BEEP88		(1)

#ifdef	USE_FMGEN			/* QUASI88 */
#define	HAS_FMGEN2203		(1)
#define	HAS_FMGEN2608		(1)
#endif



/* src/memory.h ============================================================= */

#ifdef __GNUC__
#if (__GNUC__ < 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ <= 7))
#define UNUSEDARG
#else
#ifdef	__cplusplus
#define UNUSEDARG
#else
#define UNUSEDARG __attribute__((__unused__))
#endif
#endif
#else
#define UNUSEDARG
#endif


/* ----- typedefs for data and offset types ----- */
typedef UINT8			data8_t;
typedef UINT16			data16_t;
typedef UINT32			data32_t;
typedef UINT32			offs_t;

/* ----- typedefs for the various common memory/port handlers ----- */
typedef data8_t			(*read8_handler)  (UNUSEDARG offs_t offset);
typedef void			(*write8_handler) (UNUSEDARG offs_t offset, UNUSEDARG data8_t data);

/* ----- typedefs for the various common memory handlers ----- */
typedef read8_handler	mem_read_handler;
typedef write8_handler	mem_write_handler;

/* ----- typedefs for the various common port handlers ----- */
typedef read8_handler	port_read_handler;
typedef write8_handler	port_write_handler;

/* ----- macros for declaring the various common memory/port handlers ----- */
#define READ_HANDLER(name) 		data8_t  name(UNUSEDARG offs_t offset)
#define WRITE_HANDLER(name) 	void     name(UNUSEDARG offs_t offset, UNUSEDARG data8_t data)
#define READ16_HANDLER(name)	data16_t name(UNUSEDARG offs_t offset, UNUSEDARG data16_t mem_mask)
#define WRITE16_HANDLER(name)	void     name(UNUSEDARG offs_t offset, UNUSEDARG data16_t data, UNUSEDARG data16_t mem_mask)
#define	ACCESSING_MSB	0
#define	ACCESSING_LSB	0

/* ----- 16/32-bit memory accessing ----- */
#define COMBINE_DATA(varptr)		(*(varptr) = (*(varptr) & mem_mask) | (data & ~mem_mask))



/* src/osdepend.h ========================================================== */

/* The Win32 port requires this constant for variable arg routines. */
#ifndef CLIB_DECL
#define CLIB_DECL
#endif

int osd_start_audio_stream(int stereo);
int osd_update_audio_stream(INT16 *buffer);
void osd_stop_audio_stream(void);

void osd_set_mastervolume (int attenuation);
int osd_get_mastervolume (void);

void osd_sound_enable (int enable);

INLINE	void CLIB_DECL logerror(UNUSEDARG const char *text,...){}
/* #define logerror		(void)			*/
/* #define logerror		if(1){}else printf	*/


/* src/fileio.h ============================================================ */

typedef void	mame_file;
#define	mame_fread(file, buffer, length)
#define	mame_fwrite(file, buffer, length)


/* src/timer.h ============================================================= */

typedef void		mame_timer;
#define	TIME_NOW	0
#define	timer_enable( which, enable )			(1)
#define	timer_adjust( which, duration, param, period )
#define	timer_alloc( callback )				(NULL)
#define	timer_set( duration, param, callback )


/* src/cpuintrf.h ========================================================== */

#define	activecpu_get_pc()	0






/* src/common.h ============================================================ */

struct GameSample
{
	int length;
	int smpfreq;
	int resolution;
	signed char data[1];	/* extendable */
};


struct GameSamples
{
	int total;	/* total number of samples */
	struct GameSample *sample[1];	/* extendable */
};


struct GameSamples *readsamples(const char **samplenames,const char *name);
#define freesamples(samps)

#define	auto_malloc(size)		malloc(size)


/* profiler.h */
#define profiler_mark(type)



/* src/driver.h ============================================================ */

#include "sndintrf.h"


#define MAX_SOUND 5	/* MAX_SOUND is the maximum number of sound subsystems */
			/* which can run at the same time. Currently, 5 is enough. */

/* ----- flags for sound_attributes ----- */
#define	SOUND_SUPPORTS_STEREO		0x0001




/* src/mame.h ============================================================== */

/* struct GameOptions options					*/
/*	int	options.samplerate;				*/
/*	int	options.use_samples;				*/
/*	int	options.use_filter;				*/
/*		コマンドラインオプションにて指定可		*/

#define		options_samplerate	sample_rate
#define		options_use_samples	xmame_dummy_0
#define		options_use_filter	FALSE

EXTERN	int	xmame_dummy_0;



/* struct RunningMachine *Machine				*/
/*	UINT32	Machine->drv->sound_attributes;			*/
/*	float	Machine->drv->frames_per_second;		*/
/*		エミュレートターゲット依存			*/
/*	int	Machine->sample_rate;				*/
/*		ユーザ指定可、途中変更不可、サウンドカード依存	*/

/* #define Machine__drv__sound_attributes	0			*/
/* #define Machine__drv__sound_attributes	SOUND_SUPPORTS_STEREO	*/

EXTERN	int	Machine__drv__sound_attributes;

#define	Machine__drv__frames_per_second	((float)(vsync_freq_hz))
#define	Machine__sample_rate		sample_rate



/****************************************************************************
 *	forQUASI88
 *****************************************************************************/

extern	int	fmvol;		/* level of FM     (0-100)[%] */
extern	int	psgvol;		/* level of PSG    (0-100)[%] */
extern	int	beepvol;	/* level of BEEP   (0-100)[%] */
extern	int	rhythmvol;	/* level of RHYTHM (0-100)[%] depend on fmvol */
extern	int	adpcmvol;	/* level of ADPCM  (0-100)[%] depend on fmvol */
extern	int	close_device;	/* close sound device at menu mode */
extern	int	stream_space;	/* space rate of sound stream (0-100)[%] */

EXTERN	int	sample_rate;	/* sample freq [Hz]	     */

typedef struct {

  int	  (*sound_timer_over)(int n,int c);

  data8_t (*sound_in_data)  (UNUSEDARG offs_t offset);
  data8_t (*sound_in_status)(UNUSEDARG offs_t offset);
  void	  (*sound_out_reg)  (UNUSEDARG offs_t offset, UNUSEDARG data8_t data);
  void	  (*sound_out_data) (UNUSEDARG offs_t offset, UNUSEDARG data8_t data);
	  
  data8_t (*sound2_in_data)  (UNUSEDARG offs_t offset);
  data8_t (*sound2_in_status)(UNUSEDARG offs_t offset);
  void	  (*sound2_out_reg)  (UNUSEDARG offs_t offset, UNUSEDARG data8_t data);
  void	  (*sound2_out_data) (UNUSEDARG offs_t offset, UNUSEDARG data8_t data);

  void	  (*beep_out_data)(UNUSEDARG offs_t offset, UNUSEDARG data8_t data);

} T_XMAME_FUNC;

extern	T_XMAME_FUNC	*xmame_func;
extern	T_XMAME_FUNC 	xmame_func_nosound;
EXTERN	T_XMAME_FUNC	xmame_func_sound;

#define	xmame_func_set( x )	do{											\
							  if( x ) xmame_func = &xmame_func_sound;	\
							  else    xmame_func = &xmame_func_nosound;	\
							}while(0)

EXTERN	int				use_fmgen;


#endif		/* MAME_QUASI88_H_INCLUDED */
