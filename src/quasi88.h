#ifndef QUASI88_H_INCLUDED
#define QUASI88_H_INCLUDED


/*----------------------------------------------------------------------*/
/* システム・環境依存の定義						*/
/*----------------------------------------------------------------------*/
#include "config.h"


/*----------------------------------------------------------------------*/
/* ファイルシステム依存の定義						*/
/*----------------------------------------------------------------------*/
#include "filename.h"


/* QUASI88 が内部で保持可能なパス込みのファイル名バイト数 (NUL含む) */
#define	QUASI88_MAX_FILENAME	(1024)


/*----------------------------------------------------------------------*/
/* バージョン情報							*/
/*----------------------------------------------------------------------*/
#include "version.h"



/*----------------------------------------------------------------------*/
/* 共通定義								*/
/*----------------------------------------------------------------------*/

typedef	unsigned char	Uchar;
typedef	unsigned short	Ushort;
typedef	unsigned int	Uint;
typedef	unsigned long	Ulong;

typedef unsigned char  byte;
typedef unsigned short word;
typedef signed   char  offset;


typedef unsigned char  bit8;
typedef unsigned short bit16;
typedef unsigned int   bit32;


#define	COUNTOF(arr)	(int)(sizeof(arr)/sizeof((arr)[0]))
#define	OFFSETOF(s, m)	((size_t)(&((s *)0)->m))
#define	MAX(a,b)	(((a)>(b))?(a):(b))
#define	MIN(a,b)	(((a)<(b))?(a):(b))
#define	ABS(x)		( ((x) >= 0)? (x) : -(x) )
#define	SGN( x )	( ((x)>0) ? 1 : ( ((x)<0) ? -1 : 0 ) ) 


#ifdef LSB_FIRST			/* リトルエンディアン */

typedef union
{
  struct { byte l,h; }	B;
  word			W;
} pair;

#else					/* ビッグエンデイアン */

typedef union
{
  struct { byte h,l; }	B;
  word			W;
} pair;

#endif



#ifndef TRUE
#define	TRUE	(1)
#endif
#ifndef FALSE
#define	FALSE	(0)
#endif



#ifndef	INLINE
#define	INLINE	static
#endif



/*----------------------------------------------------------------------*/
/* 変数 (verbose_*)、関数						*/
/*----------------------------------------------------------------------*/
extern	int	verbose_level;		/* 冗長レベル			*/
extern	int	verbose_proc;		/* 処理の進行状況の表示		*/
extern	int	verbose_z80;		/* Z80処理エラーを表示		*/
extern	int	verbose_io;		/* 未実装 I/Oアクセスを報告	*/
extern	int	verbose_pio;		/* PIO の不正使用を表示		*/
extern	int	verbose_fdc;		/* FDイメージ異常を報告		*/
extern	int	verbose_wait;		/* ウエイト待ち時の異常を報告	*/
extern	int	verbose_suspend;	/* サスペンド時の異常を報告	*/
extern	int	verbose_snd;		/* サウンドのメッセージ		*/


#define	INIT_POWERON	(0)
#define	INIT_RESET	(1)
#define	INIT_STATELOAD	(2)

int	quasi88( void );
void	quasi88_atexit( void (*function)(void) );
void	quasi88_exit( void );
void	quasi88_reset( void );
int	quasi88_stateload( void );



char	*alloc_diskname( const char *filename );
char	*alloc_romname( const char *filename );
char	*alloc_global_cfgname( void );
char	*alloc_local_cfgname( const char *imagename );
char	*alloc_keyboard_cfgname( void );
char	*alloc_state_filename( int init );
char	*alloc_snapshot_filename( int init );



/*----------------------------------------------------------------------*/
/* その他	(実体は、 quasi88.c  にて定義してある)			*/
/*----------------------------------------------------------------------*/
void	sjis2euc( char *euc_p, const char *sjis_p );
void	euc2sjis( char *sjis_p, const char *euc_p );
int	euclen( const char *euc_p );
int	my_strcmp( const char *s, const char *d );
void	my_strncpy( char *s, const char *ct, unsigned long n );
void	my_strncat( char *s, const char *ct, unsigned long n );
char	*my_strtok( char *dst, char *src );



/*----------------------------------------------------------------------*/
/*	デバッグ用に画面やファイルにログを出力 (処理速度が低下します)	*/
/*----------------------------------------------------------------------*/

/*#define PIO_DISP*/		/* PIO 関係のログを画面に表示     */
/*#define PIO_FILE*/		/*		   ファイルに出力 */

/*#define FDC_DISP*/		/* FDC 関係のログを画面に表示     */
/*#define FDC_FILE*/		/*		   ファイルに出力 */

/*#define MAIN_DISP*/		/* メイン Z80 関係のログを画面に表示 */
/*#define MAIN_FILE*/		/*		   ファイルに出力    */

/*#define SUB_DISP*/		/* サブ Z80 関係のログを画面に表示 */
/*#define SUB_FILE*/		/*		   ファイルに出力  */


#ifndef	USE_MONITOR
#undef PIO_DISP
#undef PIO_FILE
#undef FDC_DISP
#undef FDC_FILE
#undef MAIN_DISP
#undef MAIN_FILE
#undef SUB_DISP
#undef SUB_FILE
#endif


/* ログをファイルに取る場合は、ファイルを開く */
#if defined(PIO_FILE) || defined(FDC_FILE) || defined(MAIN_FILE) || defined(SUB_FILE)
#include <stdio.h>
extern	FILE	*LOG;
#endif


/* ログ出力のマクロ							*/
/*	・可変長引数の関数を呼び出すマクロである。			*/
/*	・マクロそのものを無効にするには、一般には			*/
/*		#define  logxxx   (void)				*/
/*		#define  logxxx   if(1){}else printf			*/
/*	などを使う。前者ではワーニングが出る時があるので後者を採用した。*/


#if	defined( PIO_DISP ) || defined( PIO_FILE )
void	logpio( const char *format, ... );
#else
#define	logpio	if(1){}else printf
#endif

#if	defined( FDC_DISP ) || defined( FDC_FILE )
void	logfdc( const char *format, ... );
#else
#define	logfdc	if(1){}else printf
#endif

#if	defined( MAIN_DISP ) || defined( MAIN_FILE ) || defined( SUB_DISP ) || defined( SUB_FILE )
void	logz80( const char *format, ... );
#else
#define	logz80	if(1){}else printf
#endif



#endif		/* QUASI88_H_INCLUDED */
