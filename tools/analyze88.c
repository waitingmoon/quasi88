/****************************************************************/
/* Disk Image File Analyzer					*/
/*						Ver 0.5		*/
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
  fprintf( stderr, "Disk Image File Analyzer for Quasi88 Ver 0.4\n");
  fprintf( stderr, "Usage: analyze88 [-option] [filename]\n");
  fprintf( stderr, "    -header      - Display header information\n");
  fprintf( stderr, "    -id          - Display id information\n");
  fprintf( stderr, "    -data        - Display id and data\n");
  fprintf( stderr, "    -track n     - Specify track number n.\n");
  fprintf( stderr, "    -track n m   - Specify track number n-m.\n");
  fprintf( stderr, "    -status      - Display status-error sector\n");
  fprintf( stderr, "    -protect     - Display bad format track\n");
  fprintf( stderr, "    -image n     - Specify image number n.\n");
  fprintf( stderr, "    filename     - Disk Image File filename\n");
  exit(1);
}


unsigned char	buf[8192];
long		track[164];
char		*file_name = NULL;


void	error( int x )
{
  fprintf(stderr,"******** Error! : %s Unexpect EOF ! (error code %d.) ********\n", file_name, x);
  exit(1);
}




int	main( int argc, char *argv[] )
{
  FILE	*fp;
  int	c, i, j, x, size, sec, nr_sec;

  int	disp_header  = FALSE;
  int	disp_id      = FALSE;
  int	disp_data    = FALSE;
  int	disp_status  = FALSE;
  int	disp_protect = FALSE;
  int	track_start  = 0;
  int	track_end    = 163;

  int	image_num = -1;
  long	image_top = 0;
  long	image_size;
  
  int	img;


  for( i=1; i<argc; i++ ){
    if      ( strcmp( argv[i], "-help" )==0 ){			/* -help */

      help();

    }else if( strcmp( argv[i], "-track" )==0 ||			/* -track */
	      strcmp( argv[i], "-t"     )==0 ){

      if( ++i < argc ){
	track_end   =
	track_start = strtol( argv[i], NULL, 0 );
	if( track_start < 0 || track_start > 163 ){
	  fprintf(stderr,"Bad track number.\n");
	  exit(1);
	}
      }else{
	fprintf(stderr,"not specify track number.\n");
	exit(1);
      }

      if( i+1 < argc ){
	char *conv_end;
	int   x = strtol(argv[i+1],&conv_end,0);
	if( *conv_end=='\0' ){
	  i++;
	  track_end   = x;
	  if( track_end < track_start || track_end > 163 ){
	    fprintf(stderr,"Bad track number.\n");
	    exit(1);
	  }
	}
      }

    }else if( strcmp( argv[i], "-id" )==0 ){			/* -id */

      disp_id = TRUE;

    }else if( strcmp( argv[i], "-header" )==0 ){		/* -header */

      disp_header = TRUE;

    }else if( strcmp( argv[i], "-data" )==0 ){			/* -data */

      disp_data = TRUE;

    }else if( strcmp( argv[i], "-status" )==0 ){		/* -status */

      disp_status = TRUE;

    }else if( strcmp( argv[i], "-protect" )==0 ){		/* -protect */

      disp_protect = TRUE;

    }else if( strcmp( argv[i], "-image" )==0 ){			/* -image */

      if( ++i < argc ){
	image_num = strtol( argv[i], NULL, 0 );
	if( image_num < 1 ){
	  fprintf(stderr,"Bad image number.\n");
	  exit(1);
	}
	image_num -= 1;
      }else{
	fprintf(stderr,"not specify image number.\n");
	exit(1);
      }

    }else if( *argv[i]=='-' ){					/* - */
      fprintf(stderr,"Unknown option %s.\n",argv[i]);
      exit(1);

    }else if( file_name==NULL ){				/* file_name */
      file_name = argv[i];

    }else{							/* Another */
      fprintf(stderr,"Bad argument ( dupulicate filename-file ).\n");
      exit(1);
    }
  }




		/* 解析するファイルをオープン  */

  if( file_name ){
    if( !(fp=fopen(file_name,"rb")) ){
      fprintf( stderr, "\n%s: Can't open file %s\n", argv[0], file_name );
      exit(1);
    }
  }else{
    help();
  }




	/**** 解析開始 (イメージが終るまでor指定イメージを表示するまで) ****/

  img = 0;
  while(1){

		/* ヘッダ部をバッファに読み込む  */

    if( fseek( fp,  image_top,  SEEK_SET ) ) error(0);
    if( fread( buf, sizeof(unsigned char), 32, fp )!=32 ) error(0);

    image_size =   (int)buf[28]      + ((int)buf[29]<<8) 
		+ ((int)buf[30]<<16) + ((int)buf[31]<<24) ;

    for( i=0; i<=track_end; i++ ){
      x = 0;
      for( j=0; j<4; j++ ){
	if( (c=getc( fp ))==EOF ) error(1);
	x |= c<<(j*8);
      }
      track[ i ] = x;
    }


		/* 指定イメージに関してのみ、各種表示 */

    if( image_num<0 || img==image_num ){


		/* ヘッダ部分表示 */

      c = buf[17];
      buf[17] = '\0';
      if( !disp_header ){
	printf("% 3d : %-17s  ", img+1, buf );
	printf("%s  ", (buf[26]==0x00)?"RW":((buf[26]==0x10)?"RO":"??"));
	printf("%s  ", (buf[27]==0x00)?"2D "
		      :((buf[27]==0x10)?"2DD":((buf[27]==0x20)?"2HD ":"???")));
	printf("%d\n",image_size);
      }else{
	printf("--- Image No. %d ---\n", img+1 );
	printf("Diskname   %s\n",buf);
	printf("reserved   %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n",
	       c&0xff,  buf[18], buf[19], buf[20],
	       buf[21], buf[22], buf[23], buf[24], buf[25]);
	printf("Protect          %02xH (%s)\n",buf[26],
	       (buf[26]==0x00)?"Writable"
			      :((buf[26]=0x10) ?"WriteProtect" :"??"));
	printf("Type             %02xH (%s)\n",c,
	       (buf[27]==0x00)?"2D"
			      :((buf[27]==0x10)?"2DD"
					       :((buf[27]==0x20)?"2HD":"??")));
	printf("Size       %08XH (%d)\n",image_size,image_size);

	for( i=0; i<(track_end+3)/4; i++ ){
	  for( j=0; j<4; j++ ){
	    printf("%03d:%06X:(%06d) ",i*4+j,track[i*4+j],track[i*4+j]);
	  }
	  printf("\n");
	}
      }


			/* 指定トラックに関してのみ、表示 */

      if( disp_id || disp_data || disp_status || disp_protect ){

	for( i=track_start; i<=track_end; i++ ){

	  if( track[i]==0 ){
	    if( disp_id || disp_data || disp_protect )
	      printf("Track %03d : Unformat\n",i );
	    continue;
	  }else if( track[i] >= image_size ){
	    if( disp_id || disp_data || disp_protect )
	      printf("Track %03d : Bad Allocation ( Unformat )\n",i );
	    continue;
	  }

	  if( fseek( fp, image_top + track[i], SEEK_SET ) ){
	    if( disp_id || disp_data || disp_protect )
	      printf("Track %03d : Bad Allocation ( Unformat )\n",i );
	    continue;
	  }

	  sec = 1;
	  if( disp_id || disp_data )
	    printf("Track %03d :  C  H  R  N   nr_sec   "
		   "Den Del Stu  -- -- -- -- --  size\n",i);


				/* セクタ内の情報を表示 */

	  while(1){
	    if( fread( buf, sizeof(unsigned char), 16, fp )!=16 ) error(2);
	    nr_sec = (int)buf[4]+((int)buf[5]<<8);
	    size   = (int)buf[14]+((int)buf[15]<<8);

	    if( disp_id || disp_data )
	      printf("   sec %02d : %02X %02X %02X %02X  %04X(% 3d)  "
		    "%02X  %02X  %02X  %02X %02X %02X %02X %02X  %04X(% 5d)\n",
		     sec,
		     buf[0], buf[1], buf[2], buf[3], nr_sec, nr_sec,
		     buf[6], buf[7], buf[8],
		     buf[9],buf[10], buf[11], buf[12], buf[13], size, size );
	    else if( disp_status && buf[8] )
	      printf("Track %03d : sec %02d : (C,H,R,N)=(%02X,%02X,%02X,%02X) "
		     "Status %02X\n",
		     i, sec, buf[0], buf[1], buf[2], buf[3], 
		     buf[8] );
	    else if( disp_protect ){
	      int correct_size = 0x80 << buf[3];
	      if( correct_size != size )
	      printf("Track %03d : sec %02d : (C,H,R,N)=(%02X,%02X,%02X,%02X) "
		     "Size = %02XH\n",
		     i, sec, buf[0], buf[1], buf[2], buf[3], 
		     size );
	    }

	    for( j=0; j<size; j++ ){
	      if( (j%16)==0 ) if( disp_data ) printf("\t\t");
	      if( (c=getc( fp ))==EOF ) error(3);
	      if( disp_data ) printf("%02X ",c);
	      if( (j%16)==15 ) if( disp_data ) printf("\n");
	    }

	    if( nr_sec==0 )   break;
	    if( sec==nr_sec ) break;
	    sec ++;
	  }
	}
      }
    }

    img ++;
    image_top += image_size;

    if( fseek( fp, image_top, SEEK_SET ) ) error(4);

    if( (c=getc( fp ))==EOF ) break;
    ungetc( c, fp );

  }

  exit(0);
}

