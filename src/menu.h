#ifndef MENU_H_INCLUDED
#define MENU_H_INCLUDED

extern	int	menu_lang;			/* メニューの言語           */
extern	int	menu_readonly;			/* ディスク選択ダイアログは */
						/* 初期状態は ReadOnly ?    */
extern	int	menu_swapdrv;			/* ドライブの表示順序       */


extern	int	file_coding;			/* ファイル名の漢字コード   */
extern	int	filename_synchronize;		/* ファイル名を同調させる   */


	/* メニューモード */

void	menu_main( void );



/*----------------------------------------------------------------------
 *
 *----------------------------------------------------------------------*/
int		about_msg_init( int japanese );
const char	*about_msg( void );



/*----------------------------------------------------------------------
 * イベント処理の対処
 *----------------------------------------------------------------------*/
void	q8tk_event_key_on( int code );
void	q8tk_event_key_off( int code );
void	q8tk_event_mouse_on( int code );
void	q8tk_event_mouse_off( int code );
void	q8tk_event_mouse_moved( int x, int y );
void	q8tk_event_quit( void );




#endif	/* MENU_H_INCLUDED */
