/*
 * MAME/XMAME の サウンドドライバとのインターフェイス
 */

#ifndef SNDDRV_X11_H_INCLUDED
#define SNDDRV_X11_H_INCLUDED


#undef	EXTERN
#ifdef	SNDDRV_WORK_DEFINE
#define EXTERN
#else
#define EXTERN extern
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "quasi88.h"
#include "snddrv.h"
#include "initval.h"
#include "soundbd.h"


/* src/unix/xmame.h ======================================================== */

#include "sysdep/rc.h"
#include "sysdep/sound_stream.h"

EXTERN struct sound_stream_struct *sound_stream;

#define OSD_OK			(0)
#define OSD_NOT_OK		(1)




/* QUASI88 ***************************************************************** */

#define	sound_enabled			use_sound
int	osd_has_sound_mixer(void);



#endif		/* SNDDRV_X11_H_INCLUDED */
