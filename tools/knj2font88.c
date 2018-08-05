/****************************************************************/
/* Generate FONT.ROM from N88KNJ1.ROM				*/
/*						Ver 0.1		*/
/*					(C) Showzoh Fukunaga	*/
/****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif



void	help( void )
{
  fprintf( stderr, "FONT.ROM translator from KANJI-ROM for Quasi88 Ver 0.1\n");
  fprintf( stderr, "Usage: kanji2font [KANJI-ROM filename] [output filename]\n");
  exit(1);
}



void	error( int err, char *file )
{
  static char *error_str[] = { "Seek", "Read", "Write", };
  fprintf(stderr,"%s error (file:%s)\n",error_str[err],file);
  exit(1);
}


int	main( int argc, char *argv[] )
{
  int	i, nr_file = 0;
  char	*in_name, *out_name;
  FILE	*in_fp, *out_fp;
  unsigned char	buf[8];

  if( argc==1 ) help();


  for( i=1; i<argc; i++ ){

    if      ( strcmp( argv[i], "-help" )==0 ){			/* -help */

      help();

    }else if( *argv[i]=='-' ){					/* - */

      fprintf(stderr,"Unknown option.\n");
      exit(1);

    }else{

      if( nr_file == 0 ){
	in_name = argv[i];
	nr_file ++;
      }else if( nr_file == 1 ){
	out_name = argv[i];
	nr_file ++;
      }else{
	
	fprintf(stderr,"Bad argument ( Too many argument ).\n");
	exit(1);
      }
    }
  }


  if( nr_file!=2 ){
    fprintf(stderr,"Bad argument ( Too short argument ).\n");
    exit(1);
  }



		/* 解析するファイルをオープン  */

  if( !(in_fp=fopen(in_name,"rb")) ){
    fprintf( stderr, "\n%s: Can't open file %s\n", argv[0], in_name );
    exit(1);
  }
  if( !(out_fp=fopen(out_name,"wb")) ){
    fprintf( stderr, "\n%s: Can't open file %s\n", argv[0], out_name );
    exit(1);
  }



  if( fseek( in_fp, (1<<12),  SEEK_SET ) ) error(0,in_name);

  for( i=0; i<0x100; i++ ){
    if( fread(  buf, sizeof(unsigned char), 8, in_fp  )!=8 ) error(1,in_name);
    if( fwrite( buf, sizeof(unsigned char), 8, out_fp )!=8 ) error(2,out_name);
  }

  for( i=0; i<0x100; i++ ){
    if( i & 0x01 ){ buf[0] = buf[1]  = 0xf0; }
    else          { buf[0] = buf[1]  = 0x00; }
    if( i & 0x02 ){ buf[2] = buf[3]  = 0xf0; }
    else          { buf[2] = buf[3]  = 0x00; }
    if( i & 0x04 ){ buf[4] = buf[5]  = 0xf0; }
    else          { buf[4] = buf[5]  = 0x00; }
    if( i & 0x08 ){ buf[6] = buf[7]  = 0xf0; }
    else          { buf[6] = buf[7]  = 0x00; }
    if( i & 0x10 ){ buf[0] = buf[1] |= 0x0f; }
    if( i & 0x20 ){ buf[2] = buf[3] |= 0x0f; }
    if( i & 0x40 ){ buf[4] = buf[5] |= 0x0f; }
    if( i & 0x80 ){ buf[6] = buf[7] |= 0x0f; }
    if( fwrite( buf, sizeof(unsigned char), 8, out_fp )!=8 ) error(2,out_name);
  }

};

