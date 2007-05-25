/*****************************************************************************/
/* ファイル操作に関する処理						     */
/*									     */
/*	仕様の詳細は、ヘッダファイル file-op.h 参照			     */
/*									     */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>	/* _fullpath */
#include <string.h>
#include <sys/stat.h>	/* _stat */
#include <direct.h>	/* _getcwd, _getdrives */
#include <io.h>		/* _findfirst */
#include <errno.h>
#include <mbstring.h>	/* _mbsicmp */

#include "quasi88.h"
#include "initval.h"
#include "file-op.h"


#define	IS_DAME_MOJI( n, m )						\
	( ( (m) == '\\') &&						\
	  (( (0x81 <= (unsigned)(n)) && ((unsigned)(n) <= 0x9f) ) ||	\
	   ( (0xe0 <= (unsigned)(n)) && ((unsigned)(n) <= 0xfc) )  ) )

/*****************************************************************************/

static char *dir_cwd;	/* デフォルトのディレクトリ (カレント)		*/
static char *dir_rom;	/* ROMイメージファイルの検索ディレクトリ	*/
static char *dir_disk;	/* DISKイメージファイルの検索ディレクトリ	*/
static char *dir_tape;	/* TAPEイメージファイルの基準ディレクトリ	*/
static char *dir_snap;	/* 画面スナップショットファイルの保存先		*/
static char *dir_state;	/* サスペンドファイルの保存先			*/
static char *dir_home;	/* 共通設定ファイルを置いてるディレクトリ	*/
static char *dir_ini;	/* 個別設定ファイルを置いてるディレクトリ	*/



/****************************************************************************
 * 各種ディレクトリの取得	( osd_dir_cwd は NULLを返してはだめ ! )
 *****************************************************************************/
const char *osd_dir_cwd ( void ){ return dir_cwd;   }
const char *osd_dir_rom ( void ){ return dir_rom;   }
const char *osd_dir_disk( void ){ return dir_disk;  }
const char *osd_dir_tape( void ){ return dir_tape;  }
const char *osd_dir_snap( void ){ return dir_snap;  }
const char *osd_dir_state(void ){ return dir_state; }
const char *osd_dir_gcfg( void ){ return dir_home;  }
const char *osd_dir_lcfg( void ){ return dir_ini;   }

static int set_new_dir( const char *newdir, char **dir )
{
  char *p;
  p = malloc( strlen( newdir ) + 1 );
  if( p ){
    free( *dir );
    *dir = p;
    strcpy( *dir, newdir );
    return TRUE;
  }
  return FALSE;
}

int osd_set_dir_cwd ( const char *d ){ return set_new_dir( d, &dir_cwd );   }
int osd_set_dir_rom ( const char *d ){ return set_new_dir( d, &dir_rom );   }
int osd_set_dir_disk( const char *d ){ return set_new_dir( d, &dir_disk );  }
int osd_set_dir_tape( const char *d ){ return set_new_dir( d, &dir_tape );  }
int osd_set_dir_snap( const char *d ){ return set_new_dir( d, &dir_snap );  }
int osd_set_dir_state(const char *d ){ return set_new_dir( d, &dir_state ); }
int osd_set_dir_gcfg( const char *d ){ return set_new_dir( d, &dir_home );  }
int osd_set_dir_lcfg( const char *d ){ return set_new_dir( d, &dir_ini );   }







/****************************************************************************
 * ファイル名に使用されている漢字コードを取得
 *		0 … ASCII のみ
 *		1 … 日本語EUC
 *		2 … シフトJIS
 *****************************************************************************/
int	osd_kanji_code( void )
{
  return 2;			/* WIN なのできめうちで SJIS */
}



/****************************************************************************
 * ファイル操作
 *
 * OSD_FILE *osd_fopen( int type, const char *path, const char *mode )
 * int	osd_fclose( OSD_FILE *stream )
 * int	osd_fflush( OSD_FILE *stream )
 * int	osd_fseek( OSD_FILE *stream, long offset, int whence )
 * long	osd_ftell( OSD_FILE *stream )
 * void	osd_rewind( OSD_FILE *stream )
 * size_t osd_fread( void *ptr, size_t size, size_t nobj, OSD_FILE *stream )
 * size_t osd_fwrite(const void *ptr,size_t size,size_t nobj,OSD_FILE *stream)
 * int	osd_fputc( int c, OSD_FILE *stream )
 * int	osd_fgetc( OSD_FILE *stream )
 *****************************************************************************/


/*
 * 全てのファイルに対して排他制御したほうがいいと思うけど、面倒なので、
 * ディスク・テープのイメージに関してのみ、多重にオープンしないようにする。
 * いい方法を知らないので、ファイル名で区別することにしよう。
 *
 * osd_fopen が呼び出されたときに、ファイル名を保持しておき、
 * すでに開いているファイルのファイル名と一致しないかをチェックする。
 * ここで、ディスクイメージファイルの場合は、すでに開いているファイルの
 * ファイルポインタを返し、他の場合はオープン失敗として NULL を返す。
 */


/*
 * ファイル名 f1 と f2 が同じファイルであれば真を返す
 */
static int file_cmp( const char *f1, const char *f2 );

#if 0

/*
 * WinAPI を使う方法。正直、よくわからない。
 */

#include <windows.h>
static int file_cmp( const char *f1, const char *f2 )
{
  HANDLE h1, h2;
  BY_HANDLE_FILE_INFORMATION fi1, fi2;

  if( f1 == NULL || f2 == NULL ) return FALSE;
  if( f1 == f2 ) return TRUE;

  h1 = CreateFile( f1, GENERIC_READ, FILE_SHARE_READ, NULL,
		   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
  if( h1 == INVALID_HANDLE_VALUE ){
    return FALSE;
  }

  h2 = CreateFile( f2, GENERIC_READ, FILE_SHARE_READ, NULL,
		   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
  if( h2 == INVALID_HANDLE_VALUE ){
    CloseHandle(h1); return FALSE; 
  }

  if( !GetFileInformationByHandle(h1, &fi1) ){
    CloseHandle(h1);
    CloseHandle(h2);
    return FALSE;
  }
  if( !GetFileInformationByHandle(h2, &fi2) ){
    CloseHandle(h1);
    CloseHandle(h2);
    return FALSE;
  }

  return ( fi1.dwVolumeSerialNumber == fi2.dwVolumeSerialNumber &&
	   fi1.nFileIndexHigh == fi2.nFileIndexHigh && 
	   fi1.nFileIndexLow  == fi2.nFileIndexLow  ) ? TRUE : FALSE;
}
#elif 0

/*
 * ファイル名を比較する方法。英字の大文字小文字の無視だけ、一応処理してある。
 */

static int file_cmp( const char *f1, const char *f2 )
{
  int is_sjis = FALSE;
  int c;

  if( f1 == NULL || f2 == NULL ) return FALSE;
  if( f1 == f2 ) return TRUE;

  while( (c = (int)*f1) ){

    if( is_sjis ){				/* シフトJISの2バイト目	*/
      if( *f1 != *f2 ) return FALSE;
      is_sjis = FALSE;
    }
    else if( (c >= 0x81 && c <= 0x9f) ||	/* シフトJISの1バイト目 */
	     (c >= 0xe0 && c <= 0xfc) ){
      if( *f1 != *f2 ) return FALSE;
      is_sjis = TRUE;
    }
    else{					/* 英数字半角カナ文字	*/
      if( _strnicmp( f1, f2, 1 ) != 0 ) return FALSE;
    }

    f1 ++;
    f2 ++;
  }

  if( *f2 == '\0' ) return TRUE;
  else              return FALSE;
}
#else

/*
 * _mbsicmp を使ってお手軽に。
 */

static int file_cmp( const char *f1, const char *f2 )
{
  if( f1 == NULL || f2 == NULL ) return FALSE;
  if( f1 == f2 ) return TRUE;

  return ( _mbsicmp( f1, f2 ) == 0 ) ? TRUE : FALSE;
}
#endif







struct OSD_FILE_STRUCT{

  FILE		*fp;			/* !=NULL なら使用中	*/
  int		type;			/* ファイル種別		*/
  char		*path;			/* ファイル名		*/
  char		mode[4];		/* 開いた際の、モード	*/

};

#define	MAX_STREAM	8
static	OSD_FILE	osd_stream[ MAX_STREAM ];



OSD_FILE *osd_fopen( int type, const char *path, const char *mode )
{
  int i;
  OSD_FILE	*st;
  char		*fullname;

  st = NULL;
  for( i=0; i<MAX_STREAM; i++ ){	/* 空きバッファを探す */
    if( osd_stream[i].fp == NULL ){		/* fp が NULL なら空き */
      st = &osd_stream[i];
      break;
    }
  }
  if( st == NULL ) return NULL;			/* 空きがなければ NG */
  st->path = NULL;


  fullname = _fullpath( NULL, path, 0 );	/* ファイル名を取得する */
  if( fullname == NULL ) return NULL;



  switch( type ){

  case FTYPE_DISK:		/* "r+b" , "rb"	*/
  case FTYPE_TAPE_LOAD:		/* "rb" 	*/
  case FTYPE_TAPE_SAVE:		/* "ab"		*/
  case FTYPE_PRN:		/* "ab"		*/
  case FTYPE_COM_LOAD:		/* "rb"		*/
  case FTYPE_COM_SAVE:		/* "ab"		*/

    for( i=0; i<MAX_STREAM; i++ ){	/* ファイルがすでに開いてないか検索 */
      if( osd_stream[i].fp ){
						/* すでに開いているならば   */
	if( file_cmp( osd_stream[i].path, fullname ) ){

	  free( fullname );

	  if( type == FTYPE_DISK                    &&	/* DISKの場合のみ、 */
	      osd_stream[i].type == type            &&	/* 同じモードならば */
	      strcmp( osd_stream[i].mode, mode )==0 ){	/* それを返す       */

	    return &osd_stream[i];

	  }else{					/* DISK以外、ないし */
	    return NULL;				/* 違うモードならNG */
	  }
	}
      }
    }
    st->path = fullname;		/* ファイル名を保持する */
    /* FALLTHROUGH */


  default:
    st->fp = fopen( fullname, mode );	/* ファイルを開く */

    if( st->fp ){

      st->type = type;
      strncpy( st->mode, mode, sizeof(st->mode) );
      return st;

    }else{

      free( fullname );
      return NULL;
    }
  }
}



int	osd_fclose( OSD_FILE *stream )
{
  FILE *fp = stream->fp;

  stream->fp = NULL;
  if( stream->path ){
    free( stream->path );
    stream->path = NULL;
  }

  return fclose( fp );
}



int	osd_fflush( OSD_FILE *stream )
{
  if( stream==NULL ) return fflush( NULL );
  else               return fflush( stream->fp );
}



int	osd_fseek( OSD_FILE *stream, long offset, int whence )
{
  return fseek( stream->fp, offset, whence );
}



long	osd_ftell( OSD_FILE *stream )
{
  return ftell( stream->fp );
}



void	osd_rewind( OSD_FILE *stream )
{
  (void)osd_fseek( stream, 0L, SEEK_SET );
  osd_fflush( stream );
}



size_t	osd_fread( void *ptr, size_t size, size_t nobj, OSD_FILE *stream )
{
  return fread( ptr, size, nobj, stream->fp );
}



size_t	osd_fwrite(const void *ptr, size_t size, size_t nobj, OSD_FILE *stream)
{
  return fwrite( ptr, size, nobj, stream->fp );
}



int	osd_fputc( int c, OSD_FILE *stream )
{
  return fputc( c, stream->fp );
}


int	osd_fgetc( OSD_FILE *stream )
{
  return fgetc( stream->fp );
}



/****************************************************************************
 * ディレクトリ閲覧
 *****************************************************************************/

struct	T_DIR_INFO_STRUCT
{
  int		cur_entry;		/* 上位が取得したエントリ数	*/
  int		nr_entry;		/* エントリの全数		*/
  int		nr_total;		/* エントリの全数 + ドライブ数	*/
  T_DIR_ENTRY	*entry;			/* エントリ情報 (entry[0]〜)	*/
};



/*
 * ディレクトリ内のファイル名のソーティングに使う関数
 */

	/* 大文字小文字を無視してファイル名ソート  */
static int namecmp( const void *p1, const void *p2 )
{
  T_DIR_ENTRY *s1 = (T_DIR_ENTRY *)p1;
  T_DIR_ENTRY *s2 = (T_DIR_ENTRY *)p2;
#if 0
  return _stricmp( s1->name, s2->name );
#else
  return _mbsicmp( s1->name, s2->name );
#endif
}
	/* 大文字小文字を無視してディレクトリ名ソート  */
static int dircmp( const void *p1, const void *p2 )
{
  T_DIR_ENTRY *s1 = (T_DIR_ENTRY *)p1;
  T_DIR_ENTRY *s2 = (T_DIR_ENTRY *)p2;
  if( s1->str[0]=='<' && s2->str[0]!='<' ) return -1;	  /* <〜> はドライブ */
  if( s1->str[0]!='<' && s2->str[0]=='<' ) return +1;	  /* なので高優先    */
#if 0
  return _stricmp( s1->name, s2->name );
#else
  return _mbsicmp( s1->name, s2->name );
#endif
}
	/* ファイルとディレクトリとを分離 */
static int typecmp( const void *p1, const void *p2 )
{
  T_DIR_ENTRY *s1 = (T_DIR_ENTRY *)p1;
  T_DIR_ENTRY *s2 = (T_DIR_ENTRY *)p2;

  if( s1->type == s2->type ) return 0;
  if( s1->type == FILE_STAT_DIR ) return -1;
  else                            return +1;
}



/*---------------------------------------------------------------------------
 * T_DIR_INFO *osd_opendir( const char *filename )
 *	_findfirst(), _findnext(), _findclose() を駆使し、
 *	ディレクトリの全てのエントリの ファイル名と属性をワークにセット
 *	する。ワークは適宜、malloc() で確保するが、確保に失敗した場合は
 *	そこでエントリの取得を打ち切る。
 *	処理後は、このワークをファイル名でソートしておく。また、ディレクトリの
 *	場合は、表示用の名の、前後に '[' と ']' を付加して戻る
 *---------------------------------------------------------------------------*/
T_DIR_INFO	*osd_opendir( const char *filename )
{
  int	i;
  T_DIR_INFO	*dir;

  long dirp;
  struct _finddata_t dp;

  int len;
  char *p;
  char *fname;
  long drv_list    = _getdrives();
  char drv_name[4] = "A:\\";
  char drv_str[5]  = "<A:>";
  int top_dir = FALSE;

				/* T_DIR_INFO ワークを 1個確保 */
  if( (dir = (T_DIR_INFO *)malloc( sizeof(T_DIR_INFO) ))==NULL ){
    return NULL;
  }

  if( filename==NULL || filename[0]=='\0' ) filename = ".";

				/* ルートディレクトリかどうかを判定 */

  if( strcmp( &filename[1], ":" )==0   ||	/* x: や x:\ は root-dir */
      strcmp( &filename[1], ":\\" )==0 ) top_dir = TRUE;

  if( strncmp( filename, "\\\\", 2 )== 0 ){	/* \\ で始まる場合、	*/
    int j=0;					/* 先頭から \ を検索し、*/
    len = strlen( filename ) -1;
    for( i=2; i<len; i++ ) 			/* その数を数える。	*/
      if( filename[i]=='\\' )
	if( ! IS_DAME_MOJI( filename[i-1], filename[i] ) )/* (ダメ文字補正) */
	  j++;
    if( j==1 ) top_dir = TRUE;			/* 1個だけなら root-dir */
  }


				/* ディレクトリ検索名 "filename\\*" をセット */
  len = strlen( filename ) + sizeof( "\\*" );
  if( len >= OSD_MAX_FILENAME ||
      ( ( fname = (char*)malloc( len ) ) == NULL  ) ){
    free( dir );
    return NULL;
  }
  strcpy( fname, filename );
  if( strlen(fname) && fname[ strlen(fname)-1 ] != '\\' )
    strcat( fname, "\\" );
  else if( strlen(fname) >= 2 &&			/* (ダメ文字補正) */
	   IS_DAME_MOJI( fname[strlen(fname)-2], fname[strlen(fname)-1] ) ){
    strcat( fname, "\\" );
  }
  strcat( fname, "*" );


				/* ディレクトリ内のファイル数を数える */
  dir->nr_entry = 0;
  dirp = _findfirst( fname, &dp );
  if( dirp != -1 ){
    do{
      dir->nr_entry ++;
    } while( _findnext( dirp, &dp )==0 );
    _findclose( dirp );
  }


				/* T_DIR_ENTRY ワークを ファイル数分 確保 */
  dir->nr_total = dir->nr_entry + 26;		/* +26 はドライブ名の分 */
  dir->entry = (T_DIR_ENTRY *)malloc( dir->nr_total * sizeof(T_DIR_ENTRY) );
  if( dir->entry==NULL ){
    free( dir );
    free( fname );
    return NULL;
  }
  for( i=0; i<dir->nr_total; i++ ) {
    dir->entry[ i ].name = NULL;
    dir->entry[ i ].str  = NULL;
  }


				/* ファイル数分、処理ループ (情報を格納) */
  dirp = -1;
  for( i=0; i<dir->nr_entry; i++ ) {

    if( i==0 ){				/* ファイル名取得(初回) */
      dirp = _findfirst( fname, &dp );
      if( dirp==-1 ){
	dir->nr_entry = i;			/* 取得に失敗したら、中断  */
	break;
      }
    }else{				/* ファイル名取得(次回以降) */
      if( _findnext( dirp, &dp )!= 0 ){
	dir->nr_entry = i;			/* 取得に失敗したら、中断  */
	break;
      }
    }

					/* ファイルの種類をセット */
    if (dp.attrib & _A_SUBDIR) {
      dir->entry[ i ].type = FILE_STAT_DIR;
    }else{
      dir->entry[ i ].type = FILE_STAT_FILE;
    }

					/* ファイル名バッファ確保 */

    len = strlen(dp.name) + 1;
    p = (char *)malloc(     ( len )   +  ( len + 2 ) );
    if( p==NULL ){	/* ↑ファイル名   ↑表示名   */
      dir->nr_entry = i;
      break;					/* malloc に失敗したら中断 */
    }

					/* ファイル名・表示名セット */
    dir->entry[ i ].name = &p[0];
    dir->entry[ i ].str  = &p[len];

    strcpy( dir->entry[ i ].name, dp.name );

    if( dir->entry[ i ].type == FILE_STAT_DIR ){
      sprintf( dir->entry[ i ].str, "[%s]", dp.name );
    }else{
      sprintf( dir->entry[ i ].str, "%s",   dp.name );
    }

  }


  free( fname );
  if( dirp!=-1 )
    _findclose( dirp );			/* ディレクトリを閉じる */


	/* エントリがない(取得失敗)場合や、ルートディレクトリの場合は、
	   ドライブをエントリに追加してあげよう */

  if( dir->nr_entry==0 || top_dir ){
    for( i=0; i<26; i++ ){
      if( drv_list & (1L<<i) ){

	p = (char *)malloc( sizeof(drv_name) + sizeof(drv_str) );
	if( p ){
	  dir->entry[ dir->nr_entry ].name = &p[0];
	  dir->entry[ dir->nr_entry ].str  = &p[sizeof(drv_name)];

	  strcpy( dir->entry[ dir->nr_entry ].name, drv_name );
	  strcpy( dir->entry[ dir->nr_entry ].str,  drv_str );

	  dir->entry[ dir->nr_entry ].type = FILE_STAT_DIR;
	  dir->nr_entry ++;
	}
      }
      drv_name[0] ++;	/* "x:\\" の x を A→Zに置換していく */
      drv_str[1] ++;	/* "<x:>" の x を A→Zに置換していく */
    }
  }


				/* ファイル名をソート */

					/* まずファイルとディレクトリを分離 */
  qsort( dir->entry, dir->nr_entry, sizeof(T_DIR_ENTRY), typecmp );
  {
    T_DIR_ENTRY *p = dir->entry;
    for( i=0; i<dir->nr_entry; i++, p++ ){
      if( p->type == FILE_STAT_FILE ) break;
    }
    					/* 各々をファイル名でソート */
    qsort( &dir->entry[0], i, sizeof(T_DIR_ENTRY), dircmp );
    qsort( &dir->entry[i], dir->nr_entry-i, sizeof(T_DIR_ENTRY), namecmp );
  }


				/* osd_readdir に備えて */
  dir->cur_entry = 0;
  return dir;
}




/*---------------------------------------------------------------------------
 * T_DIR_ENTRY *osd_readdir( T_DIR_INFO *dirp )
 *	osd_opendir() の時に確保した、エントリ情報ワークへのポインタを
 *	順次、返していく。
 *---------------------------------------------------------------------------*/
T_DIR_ENTRY	*osd_readdir( T_DIR_INFO *dirp )
{
  T_DIR_ENTRY	*ret_value = NULL;

  if( dirp->cur_entry != dirp->nr_entry ){
    ret_value = &dirp->entry[ dirp->cur_entry ];
    dirp->cur_entry ++;
  }
  return	ret_value;
}



/*---------------------------------------------------------------------------
 * void osd_closedir( T_DIR_INFO *dirp )
 *	osd_opendir() 時に確保した全てのメモリを開放する。
 *---------------------------------------------------------------------------*/
void		osd_closedir( T_DIR_INFO *dirp )
{
  int	i;

  for( i=0; i<dirp->nr_entry; i++ ){
    if( dirp->entry[i].name ) free( dirp->entry[i].name );
  }
  free( dirp->entry );
  free( dirp );
}



/****************************************************************************
 * パス名の操作
 *****************************************************************************/

/*---------------------------------------------------------------------------
 * int	osd_path_normalize( const char *path, char resolved_path[], int size )
 *
 *	処理内容:
 *		_fullpath()の結果をそのまま返す。
 *		_fullpath()の結果が NULL なら、pathname をそのまま返す
 *		ディレクトリの場合、入力元の末尾が \ なら結果も \ がつくが、
 *		入力元の末尾が \ でないなら、結果も \ はつかない。
 *---------------------------------------------------------------------------*/
int	osd_path_normalize( const char *path, char resolved_path[], int size )
{
  if( _fullpath( resolved_path, path, size ) != NULL ){
    /* 成功: resolved_path には絶対パス格納済み */
    return TRUE;
  }else{
    /* 失敗:  */
    return FALSE;
  }
}



/*---------------------------------------------------------------------------
 * int	osd_path_split( const char *path, char dir[], char file[],
 *			int size )
 *	処理内容:
 *		path の最後の '\\' より前を dir、後ろを file にセットする
 *---------------------------------------------------------------------------*/
int	osd_path_split( const char *path, char dir[], char file[], int size )
{
  int	pos = strlen( path );

  /* dir, file は十分なサイズを確保しているはずなので、軽くチェック */
  if( pos==0 || size <= pos ){
    dir[0]  = '\0';
    file[0] = '\0';
    strncat( file, path, size-1 );
    fprintf( stderr, "internal overflow %d\n",__LINE__ );
    return 0;
  }


  do{					/* '\\' を末尾から探す 		*/
    if( path[ pos-1 ] == '\\' )
      if( (pos-1) == 0 ||				/* (ダメ文字補正) */
	  ! IS_DAME_MOJI( path[pos-2], path[pos-1] ) )
	break;
    pos --;
  } while( pos );

  if( pos ){				/* '\\' が見つかったら		*/
    strncpy( dir, path, pos );			/*先頭〜'\\'までをコピー*/
    dir[pos] = '\0';				/* '\\' も含まれます	*/
    strcpy( file, &path[pos] );

  }else{				/* '\\' が見つからなかった	*/
    dir[0]  = '\0';				/* ディレクトリは ""	*/
    strcpy( file, path );			/* ファイルは path全て	*/
  }

  return 1;
}



/*---------------------------------------------------------------------------
 * int	osd_path_join( const char *dir, const char *file, char path[],
 *		       int size )
 *	処理内容:
 *		file が \\ で始まっていたら、そのまま path にセット
 *		file が x:\\ の場合も、      そのまま path にセット
 *		そうでなければ、"dir" + "\\" + "file" を path にセット
 *---------------------------------------------------------------------------*/
int	osd_path_join( const char *dir, const char *file, char path[],
		       int size )
{
  int	len = strlen( file );

  if( file[0] == '\\' ||			/* ファイル名が、絶対パス */
      file[1] == ':' ){

    if( (size_t)size <= strlen( file ) ) return FALSE;
    strcpy( path, file );

  }else{					/* ファイル名は、相対パス */

    path[0] = '\0';
    strncat( path, dir, size-1 );

    len = strlen( path );				/* ディレクトリ末尾  */
    if( len  &&  path[ len-1 ] != '\\' ){		/* が'\\' でないなら */
      strncat( path, "\\", size - len -1 );		/* 付加する          */
    }
    else if( len >= 2 &&				/* (ダメ文字補正) */
	     IS_DAME_MOJI( path[ len-2 ], path[ len-1 ] ) ){
      strncat( path, "\\", size - len -1 );
    }

    len = strlen( path );
    strncat( path, file, size - len -1 );

  }

  return TRUE;
}



/****************************************************************************
 * ファイル属性の取得
 ****************************************************************************/
#if 1

int	osd_file_stat( const char *pathname )
{
  struct _stat	sb;

  if( _stat( pathname, &sb ) ){
    return FILE_STAT_NOEXIST;
  }
  if( sb.st_mode & _S_IFDIR ){
    return FILE_STAT_DIR;
  }else{
    return FILE_STAT_FILE;
  }
}

#else
int	osd_file_stat( const char *filename )
{
  char *fname;
  int i;
  long dirp;
  struct _finddata_t dp;

  /*
    ""        ならエラー、
    "\\"      なら(ルート)ディレクトリ
    "x:\\"    なら(ルート)ディレクトリ、
    "x:"      ならディレクトリ
    "\\\\*"   なら(ネットワーク)ディレクトリ
    "\\\\*\*" なら(ネットワーク)ディレクトリ
  */

  i = strlen(filename);
  if( i==0 ) return FILE_STAT_FILE;
  if( strcmp(  filename,    "\\"  )== 0 ) return FILE_STAT_DIR;
  if( strcmp( &filename[1], ":"   )== 0 ) return FILE_STAT_DIR;
  if( strcmp( &filename[1], ":\\" )== 0 ) return FILE_STAT_DIR;

  fname = (char *)malloc( i + 1 );
  if( fname==NULL ) return FILE_STAT_FILE;

  strcpy( fname, filename );		/* 末尾が \\ なら削る */
  if( i >= 2 &&					/* (ダメ文字補正) */
      IS_DAME_MOJI( fname[ i-2 ], fname[ i-1 ] ) ){
    ;
  }else
  if( fname[i-1] == '\\' ) fname[i-1] = '\0';

  if( strncmp( fname, "\\\\", 2 )== 0 ){	/* \\ で始まる場合、	*/
    int j=0;					/* 先頭から \ を検索し、*/
    for( i=2; fname[i]; i++ ) 			/* その数を数える。	*/
      if( fname[i]=='\\' )
	if( ! IS_DAME_MOJI( fame[i-1], fname[i] ) )/* (ダメ文字補正) */
	  j++;
    if( j==1 ){					/* 1個だけならネット	*/
      free( fname );				/* ワークディレクトリと	*/
      return FILE_STAT_DIR;			/* みなそう		*/
    }
  }

  dirp = _findfirst( fname, &dp );
  free( fname );

  if( dirp==-1 ){
    return FILE_STAT_NOEXIST;
  }
  _findclose( dirp );

  if (dp.attrib & _A_SUBDIR) return FILE_STAT_DIR;
  else                       return FILE_STAT_FILE;
}
#endif








/****************************************************************************
 * int	osd_environment( void )
 *
 *	この関数は、起動後に1度だけ呼び出される。
 *	正常終了時は真を、 malloc に失敗したなど異常終了時は偽を返す。
 *
 ****************************************************************************/

static int make_dir( const char *dname );
static int check_dir( const char *dname );

/*
 * 環境変数 *env_dir にセットされたディレクトリを **dir にセットする。
 * 環境変数が未定義なら、カレントディレクトリ + *alt_dir で表される
 * ディレクトリを **dir にセットする。
 * いずれの場合も、 **dir には malloc された領域 (ないし NULL) がセットされる
 */

static void set_dir( char **dir, char *env_dir, char *alt_dir )
{
  char *s;

  s = getenv( env_dir );
  if( s ){

    *dir = _fullpath( NULL, s, 0 );

  }else{

    if( alt_dir ){
      if( dir_home ){

	s = (char*)malloc( strlen(dir_home) + strlen(alt_dir) + 2 );

	if( s ){
	  s[0] = '\0';
	  if( dir_home[0] ){
	    strcat( s, dir_home );
	    strcat( s, "\\" );
	  }
	  strcat( s, alt_dir );

	  *dir = _fullpath( NULL, s, 0 );

	  free( s );
	}else{
	  *dir = NULL;
	}

	if( *dir ){
#if 0
	  if( make_dir( *dir ) ) return;	/* ディレクトリなければ作る */
#else
	  if( check_dir( *dir ) ) return;	/* ディレクトリあれば成功 */
#endif
	  free( *dir );
	}
      }
    }

    *dir = _getcwd( NULL, 0 );
  }
}


int	osd_environment( void )
{
  char *s;


	/* カレントワーキングディレクトリ名 (CWD) を取得する */

  dir_cwd = _getcwd( NULL, 0 );


	/* ホームディレクトリ $(QUASI88_HOME) を取得する */

  s = getenv( "QUASI88_HOME" );
  if( s ){
    dir_home = _fullpath( NULL, s, 0 );
  }else{
    dir_home = _getcwd( NULL, 0 );
  }


  /* いろんなディレクトリを設定する				*/
  /*	第2引数の環境変数が設定してあれば、そのディレクトリ。	*/
  /*	未設定なら、第3引数のディレクトリが dir_home 以下に	*/
  /*	あるかチェックし、あればそれ。なければ dir_cwd		*/


	/* 設定ディレクトリ */

  set_dir( &dir_ini, "QUASI88_INI_DIR", "INI" );


	/* ROMディレクトリ */

  set_dir( &dir_rom, "QUASI88_ROM_DIR", "ROM" );


	/* DISKディレクトリ */

  set_dir( &dir_disk, "QUASI88_DISK_DIR", "DISK" );


	/* TAPEディレクトリ */

  set_dir( &dir_tape, "QUASI88_TAPE_DIR", "TAPE" );


	/* SNAPディレクトリ */

  set_dir( &dir_snap, "QUASI88_SNAP_DIR", "SNAP" );


	/* STATEディレクトリ */

  set_dir( &dir_state, "QUASI88_STATE_DIR", "STATE" );



	/* 各ディレクトリが設定できなければ異常終了 */

  if( ! dir_cwd   || ! dir_home  || ! dir_ini   || ! dir_rom   ||
      ! dir_disk  || ! dir_tape  || ! dir_snap  || ! dir_state )  return FALSE;


  return TRUE;
}



/*
 *	ディレクトリ dname があるかチェック。無ければ作る。
 *		成功したら、真を返す
 */
static int make_dir( const char *dname )
{
  struct _stat sb;

  if( _stat( dname, &sb ) ){

    if( errno == ENOENT ){			/* ディレクトリ存在しない */

      if( _mkdir( dname ) ){
	fprintf( stderr, "error: can't make dir %s\n", dname );
	return FALSE;
      }else{
	printf( "make dir \"%s\"\n", dname );
      }

    }else{					/* その他の異常 */
      return FALSE;
    }

  }else{					/* ディレクトリあった */

    if( ! (sb.st_mode & _S_IFDIR) ){			/* と思ったらファイル*/
      fprintf( stderr, "error: not exist dir %s\n", dname );
      return FALSE;
    }

  }

  return TRUE;
}



/*
 *	ディレクトリ dname があるかチェック。あれば 真
 */
static int check_dir( const char *dname )
{
  struct _stat sb;

  if( _stat( dname, &sb ) ){

    return FALSE;				/* チェック失敗 */

  }else{					/* ディレクトリあった */

    if( ! (sb.st_mode & _S_IFDIR) ){			/* と思ったらファイル*/
      return FALSE;
    }

  }

  return TRUE;
}
