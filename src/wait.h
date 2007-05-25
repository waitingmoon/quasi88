#ifndef WAIT_H_INCLUDED
#define WAIT_H_INCLUDED

/************************************************************************/
/* ウエイト調整用変数							*/
/************************************************************************/

extern	int	wait_rate;			/* ウエイト調整 比率    [%]  */
extern	int	wait_by_sleep;			/* ウエイト調整時 sleep する */

extern	long	wait_sleep_min_us;		/* 残り idle時間がこの us以下の
						   場合は、 sleep せずに待つ。
						   (MAX 1秒 = 1,000,000us) */



/************************************************************************/
/* ウエイト調整用関数							*/
/************************************************************************/
int	wait_vsync_init( void );
void	wait_vsync_term( void );

void	wait_vsync_reset( void );
void	wait_vsync( void );

void	wait_menu( void );


#endif	/* WAIT_H_INCLUDED */
