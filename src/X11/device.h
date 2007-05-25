#ifndef DEVICE_H_INCLUDED
#define DEVICE_H_INCLUDED


#include <X11/Xlib.h>
#include <X11/Xutil.h>


extern	Display	*display;
extern	Window	window;
extern	Atom	atom_WM_close_type;
extern	Atom	atom_WM_close_data;



extern	int	SCREEN_DX;		/* ウインドウ左上と、		*/
extern	int	SCREEN_DY;		/* 画面エリア左上とのオフセット	*/

extern	int	status_fg;		/* ステータス前景色		*/
extern	int	status_bg;		/* ステータス背景色		*/


extern	int	colormap_type;		/* カラーマップのタイプ	0/1/2	*/
#ifdef MITSHM
extern	int	use_SHM;		/* MIT-SHM を使用するかどうか	*/
#endif

extern	int	mouse_rel_move;		/* マウス相対移動量検知させるか	*/
extern	int	get_focus;		/* 現在、フォーカスありかどうか	*/

extern	int	keyboard_type;		/* キーボードの種類                */
extern	char	*file_keyboard;		/* キー設定ファイル名		   */
extern	int	use_xdnd;		/* XDnD に対応するかどうか	   */
extern	int	use_joydevice;		/* ジョイスティックデバイスを開く? */


void	x11_system_init( void );
void	x11_system_term( void );


void	xdnd_initialize( void );
void	xdnd_start( void );
void	xdnd_receive_drag( XClientMessageEvent *E );
void	xdnd_receive_drop( XSelectionEvent *E );


#endif	/* DEVICE_H_INCLUDED */
