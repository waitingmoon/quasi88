#ifdef SYSDEP_DSP_SDL

/* Sysdep SDL sound dsp driver

   Copyright 2001 Jack Burton aka Stefano Ceccherini
   <burton666@freemail.it>
   
   This file and the acompanying files in this directory are free software;
   you can redistribute them and/or modify them under the terms of the GNU
   Library General Public License as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   These files are distributed in the hope that they will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with these files; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

/* Thank goes to Caz Jones <turok2@currantbun.com>, who fixed this plugin 
   and made it finally sound good.
*/
/* Changelog
Version 0.1, January 2002
-initial release, based on various xmame dsp plugins and Yoshi's code


*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "SDL.h"
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_dsp_priv.h"
#include "sysdep/plugin_manager.h"

#ifdef __BEOS__
#define BUFFERSIZE 1470 * 4 /* in my experience, BeOS likes buffers to be 4x */
#else
#define BUFFERSIZE 1024
#endif

#if 1		/* forQUASI88 */
int	SDL_BUFFERSIZE = BUFFERSIZE;
int	SDL_BUFFERNUM  = 4;
struct rc_option sdl_dsp_opts[] = {
/* name, shortname, type, dest, */
/* deflt, min, max, func, help */

{ "sdlbufsize", NULL, rc_int, &SDL_BUFFERSIZE,
  "1024", 512, 8192, NULL, "buffer size of SDL sound (power of 2)"},

{ "sdlbufnum",  NULL, rc_int, &SDL_BUFFERNUM,
  "4",      1,   16, NULL, "number of SDL sound buffer"},

{NULL,NULL,rc_end,NULL,NULL,0,0,NULL,NULL}
};
#endif		/* forQUASI88 */

/* private variables */
static struct {
	Uint8 *data;
    int amountRemain;
    int amountWrite;
    int amountRead;
    int tmp;
    Uint32 soundlen;
    int sound_n_pos;
    int sound_w_pos;
    int sound_r_pos;
} sample; 

/* callback function prototype */
static void sdl_fill_sound(void *unused, Uint8 *stream, int len);

/* public methods prototypes */
static void *sdl_dsp_create(const void *flags);
static void sdl_dsp_destroy(struct sysdep_dsp_struct *dsp);
static int sdl_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
   int count);


/* public variables */
const struct plugin_struct sysdep_dsp_sdl = {
   "sdl",
   "sysdep_dsp",
   "Simple Direct Library DSP plugin",
#if 1		/* forQUASI88 */
   sdl_dsp_opts,
#else		/* forQUASI88 */
   NULL, /* no options */
#endif		/* forQUASI88 */
   NULL, /* no init */
   NULL, /* no exit */
   sdl_dsp_create,
   3     /* high priority */
};

/* public methods (static but exported through the sysdep_dsp or plugin
   struct) */
static void *sdl_dsp_create(const void *flags)
{
   struct sdl_dsp_priv_data *priv = NULL;
   struct sysdep_dsp_struct *dsp = NULL;
   const struct sysdep_dsp_create_params *params = flags;
   const char *device = params->device;
   SDL_AudioSpec *audiospec;
   
   /* allocate the dsp struct */
   if (!(dsp = calloc(1, sizeof(struct sysdep_dsp_struct))))
   {
      fprintf(stderr,
         "error malloc failed for struct sysdep_dsp_struct\n");
      return NULL;
   }
   
   
   if (!(audiospec = calloc(1, sizeof(SDL_AudioSpec))))
   {
   		fprintf(stderr, "error malloc failed for SDL_AudioSpec\n");
   		sdl_dsp_destroy(dsp);
   		return NULL;
   }

   
#if 0		/* forQUASI88 */
   if (!(sample.data = calloc(BUFFERSIZE, sizeof(Uint8))))
#else		/* forQUASI88 */
   if (!(sample.data = calloc(SDL_BUFFERSIZE*SDL_BUFFERNUM, sizeof(Uint16))))
#endif		/* forQUASI88 */
   {
   		fprintf(stderr, "error malloc failed for data\n");
   		sdl_dsp_destroy(dsp);
   		return NULL;
   }
   
   /* fill in the functions and some data */
   dsp->_priv = priv;
   dsp->write = sdl_dsp_write;
   dsp->destroy = sdl_dsp_destroy;
   dsp->hw_info.type = params->type;
   dsp->hw_info.samplerate = params->samplerate;
    
    
   /* set the number of bits */
   audiospec->format = (dsp->hw_info.type & SYSDEP_DSP_16BIT)?
   							AUDIO_S16SYS : AUDIO_S8;
   
   /* set the number of channels */
   audiospec->channels = (dsp->hw_info.type & SYSDEP_DSP_STEREO)? 2:1;
         
   /* set the samplerate */
   audiospec->freq = dsp->hw_info.samplerate;
   
   /* set samples size */
#if 0		/* forQUASI88 */
   audiospec->samples = BUFFERSIZE;
#else		/* forQUASI88 */
   audiospec->samples = SDL_BUFFERSIZE;
#endif		/* forQUASI88 */
   
   /* set callback funcion */
   audiospec->callback = sdl_fill_sound;
   
   audiospec->userdata = NULL;
   
#if 0		/* forQUASI88 */
   /* Open audio device */
   if(SDL_WasInit(SDL_INIT_VIDEO)!=0)   /* If sdl video system is already */
      SDL_InitSubSystem(SDL_INIT_AUDIO);/* initialized, we just initialize */
   else									/* the audio subsystem */
   	  SDL_Init(SDL_INIT_AUDIO);   		/* else we MUST use "SDL_Init" */
   										/* (untested) */
#else		/* forQUASI88 */
   if( ! SDL_WasInit( SDL_INIT_AUDIO ) ) SDL_InitSubSystem( SDL_INIT_AUDIO );
#endif		/* forQUASI88 */

   if (SDL_OpenAudio(audiospec, NULL) != 0) { 
   		fprintf(stderr, "failed opening audio device\n");
   		return NULL;
   }
   SDL_PauseAudio(0);
   
   fprintf(stderr, "info: audiodevice %s set to %dbit linear %s %dHz\n",
      device, (dsp->hw_info.type & SYSDEP_DSP_16BIT)? 16:8,
      (dsp->hw_info.type & SYSDEP_DSP_STEREO)? "stereo":"mono",
      dsp->hw_info.samplerate);
      
#if 1		/* forquasi88 */
   SDL_Delay( 500 );		/* Really Need? */
#endif		/* forquasi88 */
   return dsp;
}

static void sdl_dsp_destroy(struct sysdep_dsp_struct *dsp)
{ 
   SDL_CloseAudio();
    
   free(dsp);
}
   

static int sdl_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
   int count)
{
	/* sound_n_pos = normal position
	   sound_r_pos = read position
	   and so on.					*/
	int result = 0;
	Uint8 *src;
	SDL_LockAudio();
	
#if 0		/* forQUASI88 */
	sample.amountRemain = BUFFERSIZE - sample.sound_n_pos;
#else		/* forQUASI88 */
	sample.amountRemain = SDL_BUFFERSIZE*sizeof(Uint16)*SDL_BUFFERNUM - sample.sound_n_pos;
#endif		/* forQUASI88 */
	sample.amountWrite = (dsp->hw_info.type & SYSDEP_DSP_STEREO)? count * 4 : count * 2;
	
	if(sample.amountRemain <= 0) {
		SDL_UnlockAudio();
		return(result);
	}
	
	if(sample.amountRemain < sample.amountWrite) sample.amountWrite = sample.amountRemain;
		result = (int)sample.amountWrite;
		sample.sound_n_pos += sample.amountWrite;
		
		src = (Uint8 *)data;
#if 0		/* forQUASI88 */
		sample.tmp = BUFFERSIZE - sample.sound_w_pos;
#else		/* forQUASI88 */
		sample.tmp = SDL_BUFFERSIZE*sizeof(Uint16)*SDL_BUFFERNUM - sample.sound_w_pos;
#endif		/* forQUASI88 */
		
		if(sample.tmp < sample.amountWrite){
			memcpy(sample.data + sample.sound_w_pos, src, sample.tmp);
			sample.amountWrite -= sample.tmp;
			src += sample.tmp;
			memcpy(sample.data, src, sample.amountWrite);			
			sample.sound_w_pos = sample.amountWrite;
		}
		else{
			memcpy( sample.data + sample.sound_w_pos, src, sample.amountWrite);
			sample.sound_w_pos += sample.amountWrite;
		}
		SDL_UnlockAudio();
		
	return	count;
}

/* Private method */
static void sdl_fill_sound(void *unused, Uint8 *stream, int len) 
{
	int result;
	Uint8 *dst;
#if 0		/* forQUASI88 */
	SDL_LockAudio();
#endif		/* forQUASI88 */
	sample.amountRead = len;
#if 0		/* forQUASI88 */
	if(sample.sound_n_pos <= 0)
		SDL_UnlockAudio();
#endif		/* forQUASI88 */
		
		if(sample.sound_n_pos<sample.amountRead) sample.amountRead = sample.sound_n_pos;
		result = (int)sample.amountRead;
		sample.sound_n_pos -= sample.amountRead;
		
		dst = (Uint8*)stream;
		
#if 0		/* forQUASI88 */
		sample.tmp = BUFFERSIZE - sample.sound_r_pos;
#else		/* forQUASI88 */
		sample.tmp = SDL_BUFFERSIZE*sizeof(Uint16)*SDL_BUFFERNUM - sample.sound_r_pos;
#endif		/* forQUASI88 */
		if(sample.tmp<sample.amountRead){
			memcpy( dst, sample.data + sample.sound_r_pos, sample.tmp);
			sample.amountRead -= sample.tmp;
			dst += sample.tmp;
			memcpy( dst, sample.data, sample.amountRead);	
			sample.sound_r_pos = sample.amountRead;
		}
		else{
			memcpy( dst, sample.data + sample.sound_r_pos, sample.amountRead);
			sample.sound_r_pos += sample.amountRead;
		}
#if 0		/* forQUASI88 */
	SDL_UnlockAudio();
#endif		/* forQUASI88 */

}

#endif /* ifdef SYSDEP_DSP_SDL */
