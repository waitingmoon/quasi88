#define	SCREEN_BUF_INIT( screen_buf_init_function )	\
void	screen_buf_init_function( void )		\
{							\
  unsigned int	i,j;					\
  TYPE *p = (TYPE *)SCREEN_TOP;				\
  for( j = SCREEN_HEIGHT; j; j-- ){			\
    for( i = SCREEN_WIDTH; i; i-- ){			\
      *p++ = BLACK;					\
    }							\
  }							\
}
