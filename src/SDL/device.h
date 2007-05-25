#ifndef DEVICE_H_INCLUDED
#define DEVICE_H_INCLUDED


#include <SDL.h>


extern	int	SCREEN_DX;		/* ウインドウ左上と、		*/
extern	int	SCREEN_DY;		/* 画面エリア左上とのオフセット	*/

extern	int	status_fg;		/* ステータス前景色		*/
extern	int	status_bg;		/* ステータス背景色		*/



extern	int	use_hwsurface;		/* HW SURFACE を使うかどうか	*/
extern	int	use_doublebuf;		/* ダブルバッファを使うかどうか	*/
extern	int	use_swcursor;		/* メニューでカーソル表示するか	*/

extern	int	mouse_rel_move;		/* マウス相対移動量検知可能か	*/

extern	int	use_cmdkey;		/* Commandキーでメニューへ遷移     */
extern	int	keyboard_type;		/* キーボードの種類                */
extern	char	*file_keyboard;		/* キー設定ファイル名		   */
extern	int	use_joydevice;		/* ジョイスティックデバイスを開く? */



void	sdl_system_init( void );
void	sdl_system_term( void );

int	joy_init( void );

#endif	/* DEVICE_H_INCLUDED */
