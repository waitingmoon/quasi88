#ifndef	CONFIG_H_INCLUDED
#define	CONFIG_H_INCLUDED


/*----------------------------------------------------------------------*/
/* SDL バージョン固有の定義						*/
/*----------------------------------------------------------------------*/

#include <SDL_main.h>
#include <SDL_byteorder.h>


/* エンディアンネスをチェック */
#if	( SDL_BYTEORDER == SDL_LIL_ENDIAN )
#define	LSB_FIRST
#else
#undef	LSB_FIRST
#endif



/* SDL版 QUASI88 のための識別用 */

#ifndef	QUASI88_SDL
#define	QUASI88_SDL
#endif



/* メニューの タイトル・バージョン表示にて追加で表示する言葉 */

#define	Q_COMMENT	"SDL version"



/* SDL版は 16bpp(bit per pixel) 固定とする */

#ifndef	SUPPORT_16BPP
#define	SUPPORT_16BPP
#endif
#undef	SUPPORT_8BPP
#undef	SUPPORT_32BPP



/*
  MAME/XMAME のサウンドを組み込む場合、
	USE_SOUND
  を定義しておく。

  FMGEN を組み込む場合は、
	USE_FMGEN
  も定義しておく。

  上記は、コンパイル時に以下のようにして定義する。
  gcc  の場合、コンパイラにオプション -DUSE_SOUND   などと指定する。
  VC++ の場合、コンパイラにオプション /D"USE_SOUND" などと指定する。
  MWP  の場合、コンパイラにオプション -d USE_SOUND  などと指定する。
*/




/*
 *	VC++ depend
 */

#ifdef	_MSC_VER

/* VC のインラインキーワード */
#define	INLINE		__inline


/* サウンドドライバ用に、PI(π)とM_PI(π)を定義 …  MSC以外では不要? 必要? */
#ifndef PI
#define PI 3.14159265358979323846
#endif
#ifndef	M_PI
#define	M_PI	PI
#endif

#endif



/*
 *	SC depend
 */

#ifdef	macintosh

/* サウンドドライバ用に、PI(π)とM_PI(π)を定義 …  SCでも必要? */
#ifndef PI
#define PI 3.14159265358979323846
#endif
#ifndef	M_PI
#define	M_PI	PI
#endif

#endif



#endif	/* CONFIG_H_INCLUDED */
