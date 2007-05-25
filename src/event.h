#ifndef	EVENT_H_INCLUDED
#define	EVENT_H_INCLUDED


/***********************************************************************
 * イベント処理 (システム依存)
 ************************************************************************/

/******************************************************************************
 *
 * void event_handle_init( void )
 *	QUASI88 起動時に呼び出される。
 *	システム固有のキーバインディング処理などは、ここでやっておく。
 *
 * void event_handle( void )
 *	約1/60秒毎 (エミュレータの VSYNC毎) に呼び出される。
 *	ここで、全てのイベント処理を行い、エミュレータへの入力を更新する。
 *
 *	o キー押下／解放時
 *	  押されたキーに応じて、以下の関数を呼び出す。
 *		押下時: pc88_key_press( KEY88_XXX, TRUE );
 *		開放時: pc88_key_press( KEY88_XXX, FALSE );
 *
 *	  この時の、押されたキーと KEY88_XXX の対応は任意 (機種依存) だが、
 *	  ここで対応をしていない (割り当てていない) KEY88_XXX については、
 *	  QUASI88 で入力不可となる。
 *
 *	  特に、KEY_SPACE 〜 KEY88_TILDE を割り当てていなかった場合、メニュー
 *	  モードで これらに該当する ASCII文字が入力が出来なくなるので注意。
 *
 *	o マウスボタン押下／解放時
 *	  押されたボタンに応じて、以下の関数を呼び出す。
 *		押下時: pc88_mouse_press( KEY88_MOUSE_XXX, TRUE );
 *		開放時: pc88_mouse_press( KEY88_MOUSE_XXX, FALSE );
 *
 *	o ジョイスティック押下／解放時
 *	  押されたボタンに応じて、以下の関数を呼び出す。
 *		押下時: pc88_pad_press( KEY88_PAD_XXX, TRUE );
 *		開放時: pc88_pad_press( KEY88_PAD_XXX, FALSE );
 *
 *	o マウス移動時 (絶対座標 / 相対座標)
 *	  移動先 x,y (ないし移動量 dx,xy) に応じて、以下の関数を呼び出す。
 *		移動先: pc88_mouse_moved_abs(  x,  y );
 *		移動量: pc88_mouse_moved_rel( dx, dy );
 *
 *	  絶対座標の場合、画面サイズを 640x400 とした時の値をセットすること。
 *	  つまり、ボーダー部は引いておく (値の範囲は、640x400を超えても可)
 *
 *	o フォーカスあり／なし時
 *	  ウインドウがフォーカスを失った時にポーズモードに遷移させたいなら、
 *	  以下の関数を呼び出す。遷移させたくないなら、呼び出す必要なし。
 *			pc88_focus_in() / pc88_focus_out()
 *
 *	o 強制終了時
 *	  ウインドウが強制的に閉じられた場合、以下の関数を呼び出す。
 *			pc88_quit()
 *
 * int  numlock_on( void )
 * void numlock_off( void )
 *	ソフトウェア NumLock を有効にする／無効にする際に、呼び出される。
 *	システムのキー配列にに合わせて、キーバインディングを変更すればよい。
 *	変更したくない・できないならば、 numlock_on() の戻り値を 偽 にする。
 *
 * void	init_mouse_position( int *x, int *y )
 *	現在のマウスの絶対座標をセットする。
 *	絶対座標の概念がない場合は、任意の値をセットしてもよい。
 *	セットする座標値は、画面サイズを 640x400 とした時の値をセットすること。
 *	つまり、ボーダー部は引いておく (値の範囲は、640x400を超えても可)
 *	この関数は、モードの切り替わり時に呼び出される。
 *
 * void	event_init( void )
 *	エミュレートモード／メニューモード／ポーズモード／モニターモードの
 *	開始時に呼び出される。
 *	エミュレートモード以外ではグラブを解除するとか、マウスの表示・非表示を
 *	切り替えるとか………
 *
 *****************************************************************************/


		/**************************************/
		/*   システム依存部別に用意する関数   */
		/**************************************/

/*----------------------------------------------------------------------
 * event_handle() は 1/60毎に呼び出される
 *----------------------------------------------------------------------*/
void	event_handle_init( void );
void	event_handle( void );


/*----------------------------------------------------------------------
 * ソフトウェア NumLock 有効/無効時の処理
 *----------------------------------------------------------------------*/
int	numlock_on( void );
void	numlock_off( void );


/*----------------------------------------------------------------------
 * 現在のマウスの絶対座標を返す。
 *----------------------------------------------------------------------*/
void	init_mouse_position( int *x, int *y );


/*----------------------------------------------------------------------
 * 各モードの開始時のイベント処理再初期化
 *----------------------------------------------------------------------*/
void	event_init( void );


/*----------------------------------------------------------------------
 * その他
 *----------------------------------------------------------------------*/
const	char	*get_keysym_menu( void );




		/******************************************/
		/* 上記の関数内部から呼び出してほしい関数 */
		/******************************************/

/*----------------------------------------------------------------------
 * イベント処理の対処
 *----------------------------------------------------------------------*/
void	pc88_key  ( int code, int on_flag );
void	pc88_mouse( int code, int on_flag );
void	pc88_pad  ( int code, int on_flag );
void	pc88_mouse_move ( int x, int y, int abs_flag );

void	pc88_focus_in( void );
void	pc88_focus_out( void );
void	pc88_quit( void );

#define	pc88_key_pressed( code )	pc88_key  ( code, TRUE  )
#define	pc88_key_released( code )	pc88_key  ( code, FALSE )
#define	pc88_mouse_pressed( code )	pc88_mouse( code, TRUE  )
#define	pc88_mouse_released( code )	pc88_mouse( code, FALSE )
#define	pc88_pad_pressed( code )	pc88_pad  ( code, TRUE  )
#define	pc88_pad_released( code )	pc88_pad  ( code, FALSE )
#define	pc88_mouse_moved_abs( x, y )	pc88_mouse_move( x, y, TRUE )
#define	pc88_mouse_moved_rel( x, y )	pc88_mouse_move( x, y, FALSE )



/* 以下は、実装実験中。呼び出しに条件があるでの注意 */

void	quasi88_change_screen( void );
void	quasi88_snapshot( void );
void	quasi88_status( void );
void	quasi88_menu( void );
void	quasi88_pause( void );
void	quasi88_framerate_up( void );
void	quasi88_framerate_down( void );
void	quasi88_volume_up( void );
void	quasi88_volume_down( void );
void	quasi88_wait_up( void );
void	quasi88_wait_down( void );
void	quasi88_wait_none( void );
void	quasi88_max_speed( int new_speed );
void	quasi88_max_clock( double new_clock );
void	quasi88_max_boost( int new_boost );
void	quasi88_drv1_image_empty( void );
void	quasi88_drv2_image_empty( void );
void	quasi88_drv1_image_next( void );
void	quasi88_drv1_image_prev( void );
void	quasi88_drv2_image_next( void );
void	quasi88_drv2_image_prev( void );

int	quasi88_disk_insert_and_reset( const char *filename, int ro );
int	quasi88_disk_insert_all( const char *filename, int ro );
int	quasi88_disk_insert( int drv, const char *filename, int image, int ro);
int	quasi88_disk_insert_A_to_B( int src_drv, int dst_drv, int dst_img );
void	quasi88_disk_eject_all( void );
void	quasi88_disk_eject( int drv );
int	quasi88_load_tape_insert( const char *filename );
int	quasi88_load_tape_rewind( void );
void	quasi88_load_tape_eject( void );
int	quasi88_save_tape_insert( const char *filename );
void	quasi88_save_tape_eject( void );

int	quasi88_serial_in_connect( const char *filename );
void	quasi88_serial_in_remove( void );
int	quasi88_serial_out_connect( const char *filename );
void	quasi88_serial_out_remove( void );
int	quasi88_printer_connect( const char *filename );
void	quasi88_printer_remove( void );



#endif	/* EVENT_H_INCLUDED */
