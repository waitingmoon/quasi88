#ifndef STATUS_H_INCLUDED
#define STATUS_H_INCLUDED


typedef struct {
  byte		*pixmap;
  int		w, h;
} T_STATUS_INFO;

extern	T_STATUS_INFO	status_info[3];


void	status_init( void );
void	status_reset( int show );
void	status_message( int kind, int frames, const char *msg );

int	status_update( int force );


void	indicate_bootup_logo( void );
void	indicate_stateload_logo( void );


#endif	/* STATUS_H_INCLUDED */
