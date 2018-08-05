/************************************************************************/
/*									*/
/* 起動直後の引数の処理と、ワークの初期化				*/
/*									*/
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "quasi88.h"
#include "initval.h"
#include "getconf.h"
#include "device.h"
#include "event.h"

#include "pc88main.h"
#include "pc88sub.h"
#include "graph.h"
#include "intr.h"
#include "keyboard.h"
#include "memory.h"
#include "screen.h"
#include "soundbd.h"
#include "fdc.h"

#include "emu.h"
#include "file-op.h"
#include "drive.h"
#include "menu.h"
#include "monitor.h"
#include "snddrv.h"
#include "wait.h"
#include "snapshot.h"
#include "suspend.h"


/*----------------------------------------------------------------------*/

/* -f6 .. -f10 オプションの引数と、機能の対応一覧 */

static const struct {
  int	num;
  char	*str;
} fn_index[] =
{
  { FN_FUNC,        NULL,	   },
  { FN_FRATE_UP,    "FRATE-UP",    },
  { FN_FRATE_DOWN,  "FRATE-DOWN",  },
  { FN_VOLUME_UP,   "VOLUME-UP",   },
  { FN_VOLUME_DOWN, "VOLUME-DOWN", },
  { FN_PAUSE,       "PAUSE",       },
  { FN_RESIZE,      "RESIZE",      },
  { FN_NOWAIT,      "NOWAIT",      },
  { FN_SPEED_UP,    "SPEED-UP",    },
  { FN_SPEED_DOWN,  "SPEED-DOWN",  },
/*{ FN_MOUSE_HIDE,  "MOUSE-HIDE",  },*/	/* 削除 */
  { FN_FULLSCREEN,  "FULLSCREEN",  },
  { FN_FULLSCREEN,  "DGA",         },	/* 互換のため */
  { FN_SNAPSHOT,    "SNAPSHOT",    },
  { FN_IMAGE_NEXT1, "IMAGE-NEXT1", },
  { FN_IMAGE_PREV1, "IMAGE-PREV1", },
  { FN_IMAGE_NEXT2, "IMAGE-NEXT2", },
  { FN_IMAGE_PREV2, "IMAGE-PREV2", },
  { FN_NUMLOCK,     "NUMLOCK",     },
  { FN_RESET,       "RESET",       },
  { FN_KANA,        "KANA",        },
  { FN_ROMAJI,      "ROMAJI",      },
  { FN_CAPS,        "CAPS",        },
  { KEY88_KETTEI,   "KETTEI",      },	/* 互換のため(いずれ削除) */
  { KEY88_HENKAN,   "HENKAN",      },	/* 互換のため(いずれ削除) */
  { KEY88_ZENKAKU,  "ZENKAKU",     },	/* 互換のため(いずれ削除) */
  { KEY88_PC,       "PC",          },	/* 互換のため(いずれ削除) */
  { KEY88_STOP,     "STOP",        },	/* 互換のため(いずれ削除) */
  { KEY88_COPY,     "COPY",        },	/* 互換のため(いずれ削除) */
  { FN_STATUS,      "STATUS",      },
  { FN_MENU,        "MENU",        },
  { FN_MAX_SPEED,   "MAX-SPEED",   },
  { FN_MAX_CLOCK,   "MAX-CLOCK",   },
  { FN_MAX_BOOST,   "MAX-BOOST",   },
};


/*----------------------------------------------------------------------*/

/* ヘルプ表示 */
static	char	*command = NULL;

#include "help.h"


/*----------------------------------------------------------------------*/

/* オプションをいくつかのグループに分けて、優先度を設定する。*/
static	signed char	opt_prioroty[ 256 ];

/*----------------------------------------------------------------------*/

static	char	*boot_file[2];
static	int	boot_image[2];

static	int	load_config = TRUE;

/*----------------------------------------------------------------------*/

static	int	tmp_arg;

static int o_baudrate( char *dummy )
{
  static const int table[] = {
    75, 150, 300, 600, 1200, 2400, 4800, 9600, 19200,
  };
  int i;
  for( i=0; i<COUNTOF(table); i++ ){
    if( tmp_arg == table[i] ){
      baudrate_sw = i;
      return 0;
    }
  }
  return 1;
}

static int o_4mhz( char *dummy ){ cpu_clock_mhz = CONST_4MHZ_CLOCK; return 0; }
static int o_8mhz( char *dummy ){ cpu_clock_mhz = CONST_8MHZ_CLOCK; return 0; }

static int o_width ( char *dummy ){ WIDTH  &= ~7; return 0; }
static int o_height( char *dummy ){ HEIGHT &= ~1; return 0; }

static int o_set_version( char *dummy ){ set_version += '0'; return 0; }

static int o_menukey( char *dummy )
{
  function_f[  6 ] = FN_FRATE_UP;
  function_f[  7 ] = FN_FRATE_DOWN;
  function_f[  8 ] = FN_VOLUME_UP;
  function_f[  9 ] = FN_VOLUME_DOWN;
  function_f[ 10 ] = FN_PAUSE;
  return 0;
}
static int o_kanjikey( char *dummy )
{
  function_f[  6 ] = FN_KANA;
  function_f[  7 ] = KEY88_KETTEI;
  function_f[  8 ] = KEY88_HENKAN;
  function_f[  9 ] = KEY88_ZENKAKU;
  function_f[ 10 ] = FN_ROMAJI;
  return 0;
}
static int o_joyassign( char *dummy )
{
  switch( tmp_arg ){
  case 0:	joy_key_assign[0] = KEY88_INVALID;
		joy_key_assign[1] = KEY88_INVALID;
		joy_key_assign[2] = KEY88_INVALID;
		joy_key_assign[3] = KEY88_INVALID;
		joy_key_assign[4] = KEY88_INVALID;
		joy_key_assign[5] = KEY88_INVALID;
		joy_key_mode = 0;
		return 0;

  case 1:	joy_key_assign[4] = KEY88_x;
		joy_key_assign[5] = KEY88_z;		break;
  case 2:	joy_key_assign[4] = KEY88_SPACE;
		joy_key_assign[5] = KEY88_RETURN;	break;
  case 3:	joy_key_assign[4] = KEY88_SPACE;
		joy_key_assign[5] = KEY88_SHIFT;	break;
  case 4:	joy_key_assign[4] = KEY88_SHIFT;
		joy_key_assign[5] = KEY88_z;		break;
  default:	joy_key_assign[4] = KEY88_INVALID;
		joy_key_assign[5] = KEY88_INVALID;	break;
  }
  joy_key_assign[0] = KEY88_KP_8;
  joy_key_assign[1] = KEY88_KP_2;
  joy_key_assign[2] = KEY88_KP_4;
  joy_key_assign[3] = KEY88_KP_6;
  joy_key_mode = 2;

  return 0;
}

static int o_setfn_common( int key, char *str )
{
  int i, fn = FN_FUNC;

  for( i=1; i < COUNTOF(fn_index); i++ ){
    if( my_strcmp( str, fn_index[i].str ) == 0 ){
      fn = fn_index[i].num;
      break;
    }
  }
  if( fn == FN_FUNC ){
    fn = convert_str2key88( str );
    if( fn < 0 ){
      return 1;
    }
  }
  function_f[ key ] = fn;
  return 0;
}
static int o_setfn_1(  char *str ){ return o_setfn_common(  1, str ); }
static int o_setfn_2(  char *str ){ return o_setfn_common(  2, str ); }
static int o_setfn_3(  char *str ){ return o_setfn_common(  3, str ); }
static int o_setfn_4(  char *str ){ return o_setfn_common(  4, str ); }
static int o_setfn_5(  char *str ){ return o_setfn_common(  5, str ); }
static int o_setfn_6(  char *str ){ return o_setfn_common(  6, str ); }
static int o_setfn_7(  char *str ){ return o_setfn_common(  7, str ); }
static int o_setfn_8(  char *str ){ return o_setfn_common(  8, str ); }
static int o_setfn_9(  char *str ){ return o_setfn_common(  9, str ); }
static int o_setfn_10( char *str ){ return o_setfn_common( 10, str ); }
static int o_setfn_11( char *str ){ return o_setfn_common( 11, str ); }
static int o_setfn_12( char *str ){ return o_setfn_common( 12, str ); }

static int o_setkey_common( int key, char *keysym )
{
  int code = convert_str2key88( keysym );
  if( code < 0 ){
    return 1;
  }else{
    cursor_key_mode = 2;
    cursor_key_assign[ key ] = code;
    return 0;
  }
}
static int o_setkey_up(   char *keysym ){ return o_setkey_common( 0, keysym );}
static int o_setkey_down( char *keysym ){ return o_setkey_common( 1, keysym );}
static int o_setkey_left( char *keysym ){ return o_setkey_common( 2, keysym );}
static int o_setkey_right(char *keysym ){ return o_setkey_common( 3, keysym );}

static int o_setmouse_common( int key, char *keysym )
{
  int code = convert_str2key88( keysym );
  if( code < 0 ){
    return 1;
  }else{
    mouse_key_mode = 2;
    mouse_key_assign[ key ] = code;
    return 0;
  }
}
static int o_setmouse_up(   char *ksym ){ return o_setmouse_common( 0, ksym );}
static int o_setmouse_down( char *ksym ){ return o_setmouse_common( 1, ksym );}
static int o_setmouse_left( char *ksym ){ return o_setmouse_common( 2, ksym );}
static int o_setmouse_right(char *ksym ){ return o_setmouse_common( 3, ksym );}
static int o_setmouse_l(    char *ksym ){ return o_setmouse_common( 4, ksym );}
static int o_setmouse_r(    char *ksym ){ return o_setmouse_common( 5, ksym );}

static int o_setjoy_common( int key, char *keysym )
{
  int code = convert_str2key88( keysym );
  if( code < 0 ){
    return 1;
  }else{
    joy_key_mode = 2;
    joy_key_assign[ key ] = code;
    return 0;
  }
}
static int o_setjoy_up(   char *keysym ){ return o_setjoy_common( 0, keysym );}
static int o_setjoy_down( char *keysym ){ return o_setjoy_common( 1, keysym );}
static int o_setjoy_left( char *keysym ){ return o_setjoy_common( 2, keysym );}
static int o_setjoy_right(char *keysym ){ return o_setjoy_common( 3, keysym );}
static int o_setjoy_a(    char *keysym ){ return o_setjoy_common( 4, keysym );}
static int o_setjoy_b(    char *keysym ){ return o_setjoy_common( 5, keysym );}

static int o_help( char *dummy )
{
  help_mess();
  xmame_show_option();
  exit(0);
  return 0;
}

static int o_menu   ( char *dummy ){ set_emu_mode( MENU );    return 0; }
static int o_monitor( char *dummy ){ set_emu_mode( MONITOR ); return 0; }

static int o_resume( char *dummy )
{
  resume_flag  = TRUE; 
  resume_force = FALSE;
  resume_file  = FALSE;
  strcpy( file_state, "" );
  return 0;
}
static int o_resumefile( char *filename )
{
  if( strlen(filename) >= QUASI88_MAX_FILENAME ){
    fprintf( stderr, "filename %s too long, ignored\n", filename );
    resume_flag  = FALSE;
    resume_force = FALSE;
    resume_file  = FALSE;
  }else{
    resume_flag  = TRUE;
    resume_force = FALSE;
    resume_file  = TRUE;
    strcpy( file_state, filename );
  }
  return 0;
}
static int o_resumeforce( char *filename )
{
  memset( file_state, 0, QUASI88_MAX_FILENAME );

  if( strlen(filename) >= QUASI88_MAX_FILENAME ){
    fprintf( stderr, "filename %s too long, ignored\n", filename );
    resume_flag  = FALSE;
    resume_force = FALSE;
    resume_file  = FALSE;
  }else{
    resume_flag  = TRUE;
    resume_force = TRUE;
    resume_file  = TRUE;
    strcpy( file_state, filename );
  }
  return 0;
}

static int o_romdir( char *dir )
{
  if( osd_set_dir_rom( dir ) == FALSE ){
    fprintf( stderr, "-romdir %s failed, ignored\n", dir );
  }
  return 0;
}
static int o_diskdir( char *dir )
{
  if( osd_set_dir_disk( dir ) == FALSE ){
    fprintf( stderr, "-diskdir %s failed, ignored\n", dir );
  }
  return 0;
}
static int o_tapedir( char *dir )
{
  if( osd_set_dir_tape( dir ) == FALSE ){
    fprintf( stderr, "-tapedir %s failed, ignored\n", dir );
  }
  return 0;
}
static int o_snapdir( char *dir )
{
  if( osd_set_dir_snap( dir ) == FALSE ){
    fprintf( stderr, "-snapdir %s failed, ignored\n", dir );
  }
  return 0;
}
static int o_statedir( char *dir )
{
  if( osd_set_dir_state( dir ) == FALSE ){
    fprintf( stderr, "-statedir %s failed, ignored\n", dir );
  }
  return 0;
}

static int o_tapeload( char *filename )
{
  if( strlen(filename) >= QUASI88_MAX_FILENAME ){
    fprintf( stderr, "filename %s too long, ignored\n", filename );
  }else{
    strncpy( file_tape[CLOAD], filename, QUASI88_MAX_FILENAME );
  }
  return 0;
}
static int o_tapesave( char *filename )
{
  if( strlen(filename) >= QUASI88_MAX_FILENAME ){
    fprintf( stderr, "filename %s too long, ignored\n", filename );
  }else{
    strncpy( file_tape[CSAVE], filename, QUASI88_MAX_FILENAME );
  }
  return 0;
}

static int o_printer( char *filename )
{
  if( strlen(filename) >= QUASI88_MAX_FILENAME ){
    fprintf( stderr, "filename %s too long, ignored\n", filename );
  }else{
    strncpy( file_prn, filename, QUASI88_MAX_FILENAME );
  }
  return 0;
}

static int o_serialin( char *filename )
{
  if( strlen(filename) >= QUASI88_MAX_FILENAME ){
    fprintf( stderr, "filename %s too long, ignored\n", filename );
  }else{
    strncpy( file_sin, filename, QUASI88_MAX_FILENAME );
  }
  return 0;
}
static int o_serialout( char *filename )
{
  if( strlen(filename) >= QUASI88_MAX_FILENAME ){
    fprintf( stderr, "filename %s too long, ignored\n", filename );
  }else{
    strncpy( file_sout, filename, QUASI88_MAX_FILENAME );
  }
  return 0;
}

static	char	*tmp_boot_file;

static int o_diskimage( char *dummy )
{
  boot_file[0]  = boot_file[1]  = NULL;
  boot_image[0] = boot_image[1] = 0;

  if( strlen(tmp_boot_file) >= QUASI88_MAX_FILENAME ){
    fprintf( stderr, "filename %s too long, ignored\n", tmp_boot_file );
  }else{
    boot_file[0]  = tmp_boot_file;
  }
  return 0;
}

/*----------------------------------------------------------------------*/

static const struct {
  int	group;		/* 分類 (競合するオプションには同じ値を割り振る)*/
  char	*name;		/* オプションの名前 (ハイフンなし)		*/
  enum {		/* オプションに続くパラメータの処理は・・・	*/
    X_FIX,			/* 定数:   *var = (int)val1 [定数]	*/
    X_INT,			/* int:    *var = argv [範囲 var1〜val2]*/
    X_DBL,			/* double: *var = argv [範囲 var1〜val2]*/
    X_STR,			/* 文字列: strcpy( var, argv );		*/
    X_NOP,			/* 処理:   パラメータ処理なし		*/
    X_INV			/* 無効:   パラメータも処理後関数も無効	*/
  }	type;
  void	*var;		/* ここで示す変数に値をセットする		*/
  float	val1;		/* 指定可能な最小値 または セットする定数	*/
  float	val2;		/* 指定可能な最大値				*/

  int	(*func)( char *argv );	/* 処理の最後に呼び出す関数		*/
				/* 異常終了時は 0 でない値を返す	*/
}
option_table[] =
{
  /*  1〜30 : PC-8801設定オプション */

  {   1, "n",            X_FIX,  &boot_basic,      BASIC_N  ,             0,0},
  {   1, "v1s",          X_FIX,  &boot_basic,      BASIC_V1S,             0,0},
  {   1, "v1h",          X_FIX,  &boot_basic,      BASIC_V1H,             0,0},
  {   1, "v2",           X_FIX,  &boot_basic,      BASIC_V2 ,             0,0},
  {   2, "4mhz",         X_FIX,  &boot_clock_4mhz, TRUE , 0, o_4mhz,         },
  {   2, "8mhz",         X_FIX,  &boot_clock_4mhz, FALSE, 0, o_8mhz,         },
  {   3, "sd",           X_FIX,  &sound_board,     SOUND_I ,              0,0},
  {   3, "sd2",          X_FIX,  &sound_board,     SOUND_II,              0,0},
  {   4, "mouse",        X_FIX,  &mouse_mode,      1,                     0,0},
  {   4, "nomouse",      X_FIX,  &mouse_mode,      0,                     0,0},
  {   4, "joymouse",     X_FIX,  &mouse_mode,      2,                     0,0},
  {   4, "joystick",     X_FIX,  &mouse_mode,      3,                     0,0},
  {   0, "joykey",       X_INV,                                       0,0,0,0},
  {   5, "pcg",          X_FIX,  &use_pcg,         TRUE ,                 0,0},
  {   5, "nopcg",        X_FIX,  &use_pcg,         FALSE,                 0,0},
  {   6, "dipsw",        X_INT,  &boot_dipsw,      0x0000, 0xffff,          0},
  {   7, "baudrate",     X_INT,  &tmp_arg,         75, 19200, o_baudrate,    },
  {   8, "romboot",      X_FIX,  &boot_from_rom,   TRUE ,                 0,0},
  {   8, "diskboot",     X_FIX,  &boot_from_rom,   FALSE,                 0,0},
  {   9, "extram",       X_INT,  &use_extram,      0, 16,                   0},
  {   9, "noextram",     X_FIX,  &use_extram,      0,                     0,0},
  {  10, "jisho",        X_FIX,  &use_jisho_rom,   TRUE ,                 0,0},
  {  10, "nojisho",      X_FIX,  &use_jisho_rom,   FALSE,                 0,0},
  {  11, "analog",       X_FIX,  &monitor_analog,  TRUE ,                 0,0},
  {  11, "digital",      X_FIX,  &monitor_analog,  FALSE,                 0,0},
  {  12, "24k",          X_FIX,  &monitor_15k,     0x00,                  0,0},
  {  12, "15k",          X_FIX,  &monitor_15k,     0x02,                  0,0},
  {  13, "tapeload",     X_NOP,  &tmp_arg,         0, 0, o_tapeload,         },
  {  14, "tapesave",     X_NOP,  &tmp_arg,         0, 0, o_tapesave,         },
  {  15, "printer",      X_NOP,  &tmp_arg,         0, 0, o_printer           },
  {  16, "serialout",    X_NOP,  &tmp_arg,         0, 0, o_serialout,        },
  {  17, "serialin",     X_NOP,  &tmp_arg,         0, 0, o_serialin,         },
  {  18, "ro",           X_FIX,  &menu_readonly,   TRUE ,                 0,0},
  {  18, "rw",           X_FIX,  &menu_readonly,   FALSE,                 0,0},
  {  19, "ignore_ro",    X_FIX,  &fdc_ignore_readonly,  TRUE,             0,0},

  /*  31〜60 : エミュレーション設定オプション */

  {  31, "cpu",          X_INT,  &cpu_timing,      0, 2,                    0},
  {  32, "cpu1count",    X_INT,  &CPU_1_COUNT,     1, 65536,                0},
  {  33, "cpu2us",       X_INT,  &cpu_slice_us,    1, 1000,                 0},
  {  34, "fdc_wait",     X_FIX,  &fdc_wait,        1,                     0,0},
  {  34, "fdc_nowait",   X_FIX,  &fdc_wait,        0,                     0,0},
  {  35, "clock",        X_DBL,  &cpu_clock_mhz,   0.001, 65536.0,          0},
  {   0, "waitfreq",     X_INV,  &tmp_arg,                              0,0,0},
  {  36, "speed",        X_INT,  &wait_rate,       5, 5000,                 0},
  {  37, "nowait",       X_FIX,  &no_wait,         TRUE,                  0,0},
  {  37, "wait",         X_FIX,  &no_wait,         FALSE,                 0,0},
  {  38, "cmt_intr",     X_FIX,  &cmt_intr,        TRUE ,                 0,0},
  {  38, "cmt_poll",     X_FIX,  &cmt_intr,        FALSE,                 0,0},
  {  39, "cmt_speed",    X_INT,  &cmt_speed,       0, 0xffff,               0},
  {  40, "cmt_wait",     X_FIX,  &cmt_wait,        TRUE ,                 0,0},
  {  40, "cmt_nowait",   X_FIX,  &cmt_wait,        FALSE,                 0,0},
  {  41, "hsbasic",      X_FIX,  &highspeed_mode,  TRUE ,                 0,0},
  {  41, "nohsbasic",    X_FIX,  &highspeed_mode,  FALSE,                 0,0},
  {  42, "mem_wait",     X_FIX,  &memory_wait,     TRUE ,                 0,0},
  {  42, "mem_nowait",   X_FIX,  &memory_wait,     FALSE,                 0,0},
  {  43, "setver",       X_INT,  &set_version,     0, 9, o_set_version,      },
  {  44, "exchange",     X_FIX,  &disk_exchange,   TRUE ,                 0,0},
  {  44, "noexchange",   X_FIX,  &disk_exchange,   FALSE,                 0,0},
  {  45, "boost",        X_INT,  &boost,           1, 100,                  0},

  {  46, "fn_max_speed", X_INT,  &fn_max_speed,    5, 5000,                 0},
  {  47, "fn_max_clock", X_DBL,  &fn_max_clock,    0.001, 65536.0,          0},
  {  48, "fn_max_boost", X_INT,  &fn_max_boost,    1, 100,                  0},

  /*  61〜90 : 画面表示設定オプション */

  {  61, "frameskip",    X_INT,  &frameskip_rate,  1, 65536,                0},
  {  62, "autoskip",     X_FIX,  &use_auto_skip,   TRUE ,                 0,0},
  {  62, "noautoskip",   X_FIX,  &use_auto_skip,   FALSE,                 0,0},
  {  63, "half",         X_FIX,  &screen_size,     SCREEN_SIZE_HALF  ,    0,0},
  {  63, "full",         X_FIX,  &screen_size,     SCREEN_SIZE_FULL  ,    0,0},
#ifdef	SUPPORT_DOUBLE
  {  63, "double",       X_FIX,  &screen_size,     SCREEN_SIZE_DOUBLE,    0,0},
#else
  {  63, "double",       X_INV,                                       0,0,0,0},
#endif
  {  64, "width",        X_INT,  &WIDTH,           1, 65536, o_width ,       },
  {  65, "height",       X_INT,  &HEIGHT,          1, 65536, o_height,       },
  {  66, "interp",       X_FIX,  &use_half_interp, TRUE ,                 0,0},
  {  66, "nointerp",     X_FIX,  &use_half_interp, FALSE,                 0,0},
  {  67, "skipline",     X_FIX,  &use_interlace,   -1,                    0,0},
  {  67, "noskipline",   X_FIX,  &use_interlace,   0,                     0,0},
  {  67, "interlace",    X_FIX,  &use_interlace,   1,                     0,0},
  {  67, "nointerlace",  X_FIX,  &use_interlace,   0,                     0,0},
  {  68, "dga",          X_FIX,  &use_fullscreen,  TRUE ,                 0,0},
  {  68, "fullscreen",   X_FIX,  &use_fullscreen,  TRUE ,                 0,0},
  {  68, "nodga",        X_FIX,  &use_fullscreen,  FALSE,                 0,0},
  {  68, "window",       X_FIX,  &use_fullscreen,  FALSE,                 0,0},
  {  69, "hide_mouse",   X_FIX,  &hide_mouse,      TRUE ,                 0,0},
  {  69, "show_mouse",   X_FIX,  &hide_mouse,      FALSE,                 0,0},
  {  70, "grab_mouse",   X_FIX,  &grab_mouse,      TRUE ,                 0,0},
  {  70, "ungrab_mouse", X_FIX,  &grab_mouse,      FALSE,                 0,0},
  {  71, "status",       X_FIX,  &show_status,     TRUE ,                 0,0},
  {  71, "nostatus",     X_FIX,  &show_status,     FALSE,                 0,0},
  {  72, "status_fg",    X_INT,  &status_fg,       0, 0xffffff,             0},
  {  73, "status_bg",    X_INT,  &status_bg,       0, 0xffffff,             0},

  /*  91〜130: キー設定オプション */

  {  91, "tenkey",       X_FIX,  &tenkey_emu,      TRUE ,                 0,0},
  {  91, "notenkey",     X_FIX,  &tenkey_emu,      FALSE,                 0,0},
  {  92, "cursor",       X_FIX,  &cursor_key_mode, 1,                     0,0},
  {  92, "nocursor",     X_FIX,  &cursor_key_mode, 0,                     0,0},
  {  93, "numlock",      X_FIX,  &numlock_emu,     TRUE ,                 0,0},
  {  93, "nonumlock",    X_FIX,  &numlock_emu,     FALSE,                 0,0},
  {  94, "f1",           X_STR,  NULL,             0, 0, o_setfn_1,          },
  {  95, "f2",           X_STR,  NULL,             0, 0, o_setfn_2,          },
  {  96, "f3",           X_STR,  NULL,             0, 0, o_setfn_3,          },
  {  97, "f4",           X_STR,  NULL,             0, 0, o_setfn_4,          },
  {  98, "f5",           X_STR,  NULL,             0, 0, o_setfn_5,          },
  {  99, "f6",           X_STR,  NULL,             0, 0, o_setfn_6,          },
  { 100, "f7",           X_STR,  NULL,             0, 0, o_setfn_7,          },
  { 101, "f8",           X_STR,  NULL,             0, 0, o_setfn_8,          },
  { 102, "f9",           X_STR,  NULL,             0, 0, o_setfn_9,          },
  { 103, "f10",          X_STR,  NULL,             0, 0, o_setfn_10,         },
  { 104, "f11",          X_STR,  NULL,             0, 0, o_setfn_11,         },
  { 105, "f12",          X_STR,  NULL,             0, 0, o_setfn_12,         },
  { 106, "romaji",       X_INT,  &romaji_type,     0, 2,                    0},
  { 107, "menukey",      X_NOP,  0,                0, 0, o_menukey ,         },
  { 108, "kanjikey",     X_NOP,  0,                0, 0, o_kanjikey,         },
  { 109, "joyswap",      X_FIX,  &joy_swap_button, TRUE,                  0,0},
  { 110, "joyassign",    X_INT,  &tmp_arg,         0, 4, o_joyassign,        },
  { 111, "cursor_up",    X_STR,  NULL,             0, 0, o_setkey_up,        },
  { 112, "cursor_down",  X_STR,  NULL,             0, 0, o_setkey_down,      },
  { 113, "cursor_left",  X_STR,  NULL,             0, 0, o_setkey_left,      },
  { 114, "cursor_right", X_STR,  NULL,             0, 0, o_setkey_right,     },
  { 115, "mouse_up",     X_STR,  NULL,             0, 0, o_setmouse_up,      },
  { 116, "mouse_down",   X_STR,  NULL,             0, 0, o_setmouse_down,    },
  { 117, "mouse_left",   X_STR,  NULL,             0, 0, o_setmouse_left,    },
  { 118, "mouse_right",  X_STR,  NULL,             0, 0, o_setmouse_right,   },
  { 119, "mouse_l",      X_STR,  NULL,             0, 0, o_setmouse_l,       },
  { 120, "mouse_r",      X_STR,  NULL,             0, 0, o_setmouse_r,       },
  { 121, "joy_up",       X_STR,  NULL,             0, 0, o_setjoy_up,        },
  { 122, "joy_down",     X_STR,  NULL,             0, 0, o_setjoy_down,      },
  { 123, "joy_left",     X_STR,  NULL,             0, 0, o_setjoy_left,      },
  { 124, "joy_right",    X_STR,  NULL,             0, 0, o_setjoy_right,     },
  { 125, "joy_a",        X_STR,  NULL,             0, 0, o_setjoy_a,         },
  { 126, "joy_b",        X_STR,  NULL,             0, 0, o_setjoy_b,         },
  { 127, "keyboard",     X_INT,  &keyboard_type,   0, 4,                    0},
  { 128, "keyconf",      X_STR,  &file_keyboard,                        0,0,0},
  { 129, "cmdkey",       X_FIX,  &use_cmdkey,      TRUE,                  0,0},
  { 129, "nocmdkey",     X_FIX,  &use_cmdkey,      FALSE,                 0,0},

  /* 131〜140: メニュー設定オプション */

  { 131, "menu",         X_NOP,  0,                0, 0, o_menu,             },
  { 132, "english",      X_FIX,  &menu_lang,       LANG_ENGLISH,          0,0},
  { 132, "japanese",     X_FIX,  &menu_lang,       LANG_JAPAN  ,          0,0},
  { 133, "sjis",         X_FIX,  &file_coding,     1,                     0,0},
  { 133, "euc",          X_FIX,  &file_coding,     0,                     0,0},
  { 134, "bmp",          X_FIX,  &snapshot_format, SNAPSHOT_FMT_BMP,      0,0},
  { 134, "ppm",          X_FIX,  &snapshot_format, SNAPSHOT_FMT_PPM,      0,0},
  { 134, "raw",          X_FIX,  &snapshot_format, SNAPSHOT_FMT_RAW,      0,0},
  { 135, "swapdrv",      X_FIX,  &menu_swapdrv,    TRUE,                  0,0},
  { 135, "noswapdrv"  ,  X_FIX,  &menu_swapdrv,    FALSE,                 0,0},
  {   0, "button2menu",  X_INV,                                       0,0,0,0},
  {   0, "nobutton2menu",X_INV,                                       0,0,0,0},

  /* 141〜170: システム設定オプション */

  { 141, "romdir",       X_NOP,  &tmp_arg,         0, 0, o_romdir,           },
  { 142, "diskdir",      X_NOP,  &tmp_arg,         0, 0, o_diskdir,          },
  { 143, "tapedir",      X_NOP,  &tmp_arg,         0, 0, o_tapedir,          },
  { 144, "snapdir",      X_NOP,  &tmp_arg,         0, 0, o_snapdir,          },
  { 145, "statedir",     X_NOP,  &tmp_arg,         0, 0, o_statedir,         },
  { 146, "noconfig",     X_FIX,  &load_config,     FALSE,                 0,0},
  { 147, "compatrom",    X_STR,  &file_compatrom,                       0,0,0},
  { 148, "resume",       X_NOP,  0,                0, 0, o_resume,           },
  { 149, "resumefile",   X_NOP,  &tmp_arg,         0, 0, o_resumefile,       },
  { 150, "resumeforce",  X_NOP,  &tmp_arg,         0, 0, o_resumeforce,      },
  { 151, "record",       X_STR,  &file_rec,                             0,0,0},
  { 152, "playback",     X_STR,  &file_pb,                              0,0,0},
  { 153, "use_joy",      X_FIX,  &use_joydevice,   TRUE ,                 0,0},
  { 153, "nouse_joy",    X_FIX,  &use_joydevice,   FALSE,                 0,0},
  { 154, "focus",        X_FIX,  &need_focus,      TRUE ,                 0,0},
  { 154, "nofocus",      X_FIX,  &need_focus,      FALSE,                 0,0},
  { 155, "sleep",        X_FIX,  &wait_by_sleep,   TRUE ,                 0,0},
  { 155, "nosleep",      X_FIX,  &wait_by_sleep,   FALSE,                 0,0},
  { 156, "sleepparm",    X_INT,  &wait_sleep_min_us, 0, 1000000,            0},
  {   0, "logo",         X_INV,                                       0,0,0,0},
  {   0, "nologo",       X_INV,                                       0,0,0,0},

  /* 171〜200: デバッグ用オプション */

  { 171, "help",         X_NOP,  0,                0, 0, o_help,             },
  { 172, "verbose",      X_INT,  &verbose_level,   0x00, 0xff,              0},
  { 173, "timestop",     X_FIX,  &calendar_stop,   TRUE,                  0,0},
  { 174, "vsync",        X_DBL,  &vsync_freq_hz,   10.0, 240.0,             0},
  { 175, "soundclock",   X_DBL,  &sound_clock_mhz, 0.001, 65536.0,          0},
  { 177, "subload",      X_INT,  &sub_load_rate,   0, 65536,                0},
  { 178, "nofont",       X_FIX,  &use_built_in_font,TRUE,                 0,0},
  { 178, "font",         X_FIX,  &use_built_in_font,FALSE,                0,0},
  {   0, "load",         X_INV,  &tmp_arg,                              0,0,0},
  { 179, "diskimage",    X_STR,  &tmp_boot_file,   0, 0, o_diskimage,        },

#ifdef  USE_MONITOR
  { 197, "debug",        X_FIX,  &debug_mode,      TRUE,                  0,0},
  { 198, "monitor",      X_FIX,  &debug_mode,      TRUE, 0, o_monitor,       },
  { 199, "fdcdebug",     X_FIX,  &fdc_debug_mode,  TRUE ,                 0,0},
#else
  {   0, "debug",        X_INV,                                       0,0,0,0},
  {   0, "monitor",      X_INV,                                       0,0,0,0},
  {   0, "fdcdebug",     X_INV,                                       0,0,0,0},
#endif

  /* 201〜255: システム依存オプション */

  {   0, "cmap",         X_INV,  &tmp_arg,                              0,0,0},
  {   0, "shm",          X_INV,                                       0,0,0,0},
  {   0, "noshm",        X_INV,                                       0,0,0,0},
  {   0, "xdnd",         X_INV,                                       0,0,0,0},
  {   0, "noxdnd",       X_INV,                                       0,0,0,0},

  { 201, "hwsurface",    X_FIX,  &use_hwsurface,   TRUE,                  0,0},
  { 201, "swsurface",    X_FIX,  &use_hwsurface,   FALSE,                 0,0},
  { 202, "doublebuf",    X_FIX,  &use_doublebuf,   TRUE,                  0,0},
  { 202, "nodoublebuf",  X_FIX,  &use_doublebuf,   FALSE,                 0,0},
  { 203, "menucursor",   X_FIX,  &use_swcursor,    TRUE,                  0,0},
  { 203, "nomenucursor", X_FIX,  &use_swcursor,    FALSE,                 0,0},

  {   0, "streamspace",  X_INV,  &tmp_arg,                              0,0,0},
  {   0, "ss",           X_INV,  &tmp_arg,                              0,0,0},



#if     ( USE_SOUND != 1 )              /* サウンドなし時のオプション (無視) */

  {   0, "sound",        X_INV,                                       0,0,0,0},
  {   0, "nosound",      X_INV,                                       0,0,0,0},
  {   0, "snd",          X_INV,                                       0,0,0,0},
  {   0, "nosnd",        X_INV,                                       0,0,0,0},
  {   0, "samples",      X_INV,                                       0,0,0,0},
  {   0, "nosamples",    X_INV,                                       0,0,0,0},
  {   0, "sam",          X_INV,                                       0,0,0,0},
  {   0, "nosam",        X_INV,                                       0,0,0,0},
  {   0, "fakesound",    X_INV,  &tmp_arg,                              0,0,0},
  {   0, "fsnd",         X_INV,  &tmp_arg,                              0,0,0},
  {   0, "samplefreq",   X_INV,  &tmp_arg,                              0,0,0},
  {   0, "sf",           X_INV,  &tmp_arg,                              0,0,0},
  {   0, "bufsize",      X_INV,  &tmp_arg,                              0,0,0},
  {   0, "bs",           X_INV,  &tmp_arg,                              0,0,0},
  {   0, "volume",       X_INV,  &tmp_arg,                              0,0,0},
  {   0, "v",            X_INV,  &tmp_arg,                              0,0,0},
  {   0, "audiodevice",  X_INV,  &tmp_arg,                              0,0,0},
  {   0, "ad",           X_INV,  &tmp_arg,                              0,0,0},
  {   0, "mixerdevice",  X_INV,  &tmp_arg,                              0,0,0},
  {   0, "md",           X_INV,  &tmp_arg,                              0,0,0},

  {   0, "fmvol",        X_INV,  &tmp_arg,                              0,0,0},
  {   0, "fv",           X_INV,  &tmp_arg,                              0,0,0},
  {   0, "psgvol",       X_INV,  &tmp_arg,                              0,0,0},
  {   0, "pv",           X_INV,  &tmp_arg,                              0,0,0},
  {   0, "beepvol",      X_INV,  &tmp_arg,                              0,0,0},
  {   0, "bv",           X_INV,  &tmp_arg,                              0,0,0},
  {   0, "rhythmvol",    X_INV,  &tmp_arg,                              0,0,0},
  {   0, "rv",           X_INV,  &tmp_arg,                              0,0,0},
  {   0, "adpcmvol",     X_INV,  &tmp_arg,                              0,0,0},
  {   0, "av",           X_INV,  &tmp_arg,                              0,0,0},
  {   0, "sdlbufsize",   X_INV,  &tmp_arg,                              0,0,0},
  {   0, "sdlbufnum",    X_INV,  &tmp_arg,                              0,0,0},
  {   0, "close",        X_INV,                                       0,0,0,0},
  {   0, "noclose",      X_INV,                                       0,0,0,0},
  {   0, "fmgen",        X_INV,                                       0,0,0,0},
  {   0, "nofmgen",      X_INV,                                       0,0,0,0},
  {   0, "streamspace",  X_INV,  &tmp_arg,                              0,0,0},
  {   0, "ss",           X_INV,  &tmp_arg,                              0,0,0},
				/* ほかにもあるけど、これ以上解析できない… */
#else
#if	( USE_FMGEN != 1 )              /* FMGEN なし時のオプション (無視) */
  {   0, "fmgen",        X_INV,                                       0,0,0,0},
  {   0, "nofmgen",      X_INV,                                       0,0,0,0},
#endif
#endif
};



/*--------------------------------------------------------------------------
 * 2個の連続した引数 opt1, opt2 を処理し、処理した引数の数 (1か2) を返す。
 *  (引数が異常(値が範囲外など)で処理しなかった場合も同じく)
 *
 * 未知の引数の場合、処理せずに 0 を返す。
 * 内部で継続不能な異常が発生した時は、 -1 を返す。
 *--------------------------------------------------------------------------*/

static	int	check_option( char *opt1, char *opt2, int priority )
{
  int	i,  ret_val = 1;
  int	ignore, applied;
  char	*end;


  if( opt1==NULL )     return 0;
  if( opt1[0] != '-' ) return -1;


  /* オプション文字列にに合致するものを探しましょう */

  for( i=0; i<COUNTOF(option_table); i++ ){
    if( strcmp( &opt1[1], option_table[i].name )==0 ) break;
  }

  if( i==COUNTOF(option_table) ){

    /* 見つからなければ、MAME のオプションから探します */

    i = xmame_check_option( opt1, opt2, priority );
    return i;

  }else{

    /* 見つかれば処理。ただし最終な処理は '優先度が高い' か '同じ' 場合のみ */

    if( priority < opt_prioroty[ option_table[i].group ] ){
      ignore = TRUE;
    }else{
      ignore = FALSE;
    }
    applied = FALSE;


    /* オプションのタイプ別に処理します */

    switch( option_table[i].type ){

    case X_FIX:			/* なし:   *var = (int)val1 [定数]           */
      {
	if( ignore == FALSE ){
	  *((int*)option_table[i].var) = (int)option_table[i].val1;
	  applied = TRUE;
	}
      }
      break;

    case X_INT:			/* int:    *var = argv  [範囲 var1〜val2]    */
      {
	int low, high, work;

	if( opt2 ){
	  ret_val ++;
	  if( ignore == FALSE ){
	    low  = (int)option_table[i].val1;
	    high = (int)option_table[i].val2;
	    work = strtol( opt2, &end, 0 );

	    if( *end=='\0' && low <= work && work <= high ){
	      *((int*)option_table[i].var) = work;
	      applied = TRUE;
	    }else{
	      fprintf( stderr, "error: invalid value %s %s\n", opt1, opt2 );
	    }
	  }
	}else{
	  fprintf( stderr, "error: %s requires an argument\n", opt1 );
	}
      }
      break;

    case X_DBL:			/* double: *var = argv  [範囲 var1〜val2]    */
      {
	double low, high, work;

	if( opt2 ){
	  ret_val ++;
	  if( ignore == FALSE ){
	    low  = (double)option_table[i].val1;
	    high = (double)option_table[i].val2;
	    work = strtod( opt2, &end );

	    if( *end=='\0' && low <= work && work <= high ){
	      *((double*)option_table[i].var) = work;
	      applied = TRUE;
	    }else{
	      fprintf( stderr, "error: invalid value %s %s\n", opt1, opt2 );
	    }
	  }
	}else{
	  fprintf( stderr, "error: %s requires an argument\n", opt1 );
	}
      }
      break;

    case X_STR:			/* 文字列: strcpy( var, argv );              */
      {
	char *work;

	if( opt2 ){
	  ret_val ++;
	  if( ignore == FALSE ){
	    if( option_table[i].var ){
	      work = (char*)malloc( strlen(opt2)+1 );
	      if( work == NULL ){
		fprintf( stderr, "error: malloc failed for %s\n", opt1 );
		return -1;
	      }else{
		strcpy( work, opt2 );
		if( *(char **)option_table[i].var ){
		  free( *(char **)option_table[i].var );
		}
		*(char **)option_table[i].var = work;
		applied = TRUE;
	      }
	    }else{
		applied = TRUE;
	    }
	  }
	}else{
	  fprintf( stderr, "error: %s requires an argument\n", opt1 );
	}
      }
      break;

    case X_NOP:			/* 無処理:                                   */
      if( option_table[i].var ){
	if( opt2 ){
	  ret_val ++;
	  if( ignore == FALSE ){
	    applied = TRUE;
	  }
	}else{
	  fprintf( stderr, "error: %s requires an argument\n", opt1 );
	}
      }else{
	if( ignore == FALSE ){
	  applied = TRUE;
	}
      }
      break;

    case X_INV:			/* 無効:                                     */
      if( option_table[i].var && opt2 ){
	ret_val ++;
	fprintf( stderr, "error: invalid option %s %s\n", opt1, opt2 );
      }else{
	fprintf( stderr, "error: invalid option %s\n", opt1 );
      }
      break;

    default:
      break;
    }


    /* 処理後の呼び出し関数があれば、それを呼びます */

    if( option_table[i].func && applied ){
      if( (option_table[i].func)( opt2 ) != 0 ){
	fprintf( stderr, "error: invalid option %s %s\n", opt1, opt2 );
      }
    }


    /* 優先度度を書き換えておわり */

    opt_prioroty[ option_table[i].group ] = priority;

    return ret_val;
  }
}





/*--------------------------------------------------------------------------
 * 起動時のオプションを解析する。
 *	戻り値は、正常時 0、異常時 非 0
 *
 *	-help オプションをつけると、強制的に終了する。
 *--------------------------------------------------------------------------*/
static	int	get_option( int argc, char *argv[], int priority )
{
  int	i;
  char	*end;
  int	img, disk_count;

  disk_count = 0;

	/* 引数を1個づつ順に解析 */

  for( i=1; i<argc; ){

    if( *argv[i] != '-' ){	/* '-' 以外で始まるオプションは、ファイル名 */

      switch( disk_count ){
      case 0:
      case 1:
	if( strlen(argv[i]) >= QUASI88_MAX_FILENAME ){
	  fprintf( stderr, "error: image file name \"%s\" is too long\n",
		   argv[i] );
	  break;
	}

	boot_file[ disk_count ] = argv[i];

	if( i+1 < argc ){				/* 次の引数があれば  */
	  img = strtol( argv[i+1], &end, 0 );		/* 数値に変換。OKなら*/
	  if( *end=='\0' ){				/* それはイメージ番号*/
	    if( img<1 || img>MAX_NR_IMAGE ){
	      fprintf( stderr, "error: invalid image value %d\n", img );
	      img = 1;
	    }
	    boot_image[ disk_count ] = img;

	    if( i+2 < argc ){				/* さらに引数があれば*/
	      img = strtol( argv[i+2], &end, 0 );	/* 数値に変換。OKなら*/
	      if( *end=='\0' ){				/* それもイメージ番号*/
		if( img<1 || img>MAX_NR_IMAGE ){
		  fprintf( stderr, "error: invalid image value %d\n", img );
		  img = 1;
		}
		if( disk_count+1 >= NR_DRIVE ){
		  fprintf( stderr, "error: too many image number\n" );
		  img = 1;
		}
		boot_file[ disk_count+1 ] = argv[i];
		boot_image[ disk_count+1 ] = img;
		disk_count ++;
		i ++;
	      }

	    }
	    i ++;
	  }
	}
	disk_count ++;
	break;

      default:			/* 3個以上ファイル指定した場合、無視 */

	fprintf( stderr, "warning: too many image file\n" );
	break;
      }

      i ++;

    }else{			/* '-' で始まる引数は、オプション */

      int j  = check_option( argv[i], (i+1<argc)? argv[i+1] :NULL, priority );
      if( j < 0 ){			/* 致命的エラーなら、解析失敗 */
	return -1;
      }
      if( j==0 ){			/* 未知のオプションは、スキップ */
	fprintf( stderr, "error: unknown option %s\n", argv[i] );
	j = 1;
      }

      i += j;

    }
  }

  return 0;
}






/*--------------------------------------------------------------------------
 * 環境ファイルのオプションを解析する。
 *	戻り値は、正常時 0、異常時 非 0
 *
 *	-help オプションをつけると、強制的に終了する。
 *--------------------------------------------------------------------------*/

/* 環境ファイル1行あたりの最大文字数 */
#define	MAX_RCFILE_LINE	(256)


static	int	get_config_file( FILE *fp, int priority )
{
  int  result;
  char line[ MAX_RCFILE_LINE ];
  char buffer[ MAX_RCFILE_LINE ], *b;
  char *parm1, *parm2, *parm3, *str;

  int  line_cnt = 0;


		/* 設定ファイルを1行づつ解析 */

  while( fgets( line, MAX_RCFILE_LINE, fp ) ){

    line_cnt ++;
    parm1 = parm2 = parm3 = NULL;
    str = line;
    
	/* パラメータを parm1〜parm3 にセット */

    {                      b = &buffer[0];    str = my_strtok( b, str ); }
    if( str ){ parm1 = b;  b += strlen(b)+1;  str = my_strtok( b, str ); }
    if( str ){ parm2 = b;  b += strlen(b)+1;  str = my_strtok( b, str ); }
    if( str ){ parm3 = b;  }


	/* パラメータがなければ次の行へ、あれば解析処理 */

    if      ( parm1 == NULL ){			/* パラメータなし    */
      ;

    }else if( parm3 ){				/* パラメータ3個以上 */
      fprintf( stderr, "warning: too many argument in line %d\n", line_cnt );

    }else{					/* パラメータ1〜2個  */
      result = check_option( parm1, parm2, priority );

      if( (result==1 && parm2==NULL) || (result==2 && parm2 ) ){
	;
      }else{				/* エラー時は エラー行を表示 */
	fprintf( stderr, "warning: error in line %d\n", line_cnt );
      }
    }

  }

  return 0;
}















/***********************************************************************
 * 引数の処理
 *	エラー発生などで処理を続行できない場合、偽を返す。
 ************************************************************************/

int	config_init( int argc, char *argv[] )
{
  int	step;
  char	*fname;

  command = argv[0];

  boot_file[0]  = boot_file[1]  = NULL;
  boot_image[0] = boot_image[1] = 0;


	/* 起動時のオプションを解析 */

  if( get_option( argc, argv, 2 ) != 0 ) return FALSE;


  verbose_proc	= verbose_level & 0x01;

  if( resume_flag ){
    boot_file[0]  = boot_file[1]  = NULL;
  }


	/* 設定ファイル処理			*/
	/*	step 0 : 共通設定ファイルを解析	*/
	/*	step 1 : 個別設定ファイルを解析	*/

	/* ↑	step 1 の際に、ディスクイメージのファイル名を作成する */

  for( step=0; step<2; step ++ ){

    FILE *fp;
    char *dummy;

    if( step == 0 ){

      if( load_config == FALSE ) continue;

			/* 共通設定ファイルのファイル名 */
      fname = alloc_global_cfgname();
      dummy = "Global Config File";

    }else{

	/* 起動時に指定された、ディスクイメージファイルがあるかチェック */

      int	i, same;	

      if( boot_file[0] == boot_file[1] ) same = TRUE;
      else                               same = FALSE;

      for( i=0; i<2; i++ ){
	if( boot_file[i] ){			/* ファイル指定時あり */
	  fname = alloc_diskname( boot_file[i] );
	  if( fname == NULL ){
	    printf( "\n" );
	    printf( "[[[ %-26s ]]]\n", "Open failed" );
	    printf( "[[[   drive %d: %-15s ]]]\n" "\n", i+1, boot_file[i] );
	  }
	  boot_file[i] = fname;
	}

	if( i==0  &&  same ){			/* DRIVE 1/2 同じファイル時 */
	  boot_file[1] = boot_file[0];
	  break;
	}
      }

      if( load_config == FALSE ) continue;

			/* 個別設定ファイルのファイル名 (ディスクorテープ名) */
      if     ( boot_file[0] )        fname = boot_file[0];
      else if( boot_file[1] )        fname = boot_file[1];
      else if( file_tape[CLOAD][0] ) fname = file_tape[CLOAD];
      else break;

      fname = alloc_local_cfgname( fname );
      dummy = "Local Config File";
    }

    if( fname ) fp = fopen( fname, "r" );
    else        fp = NULL;

    if( verbose_proc ){
      if( fp ){ printf( "\"%s\" read and initialize\n", fname ); }
      else    { printf( "\"%s\" open failed\n", (fname) ? fname : dummy ); }
    }
    if( fname ) free( fname );

    if( fp ){
      int result = get_config_file( fp, 1 );
      fclose( fp );
      if( result != 0 ) return FALSE;
    }

    verbose_proc = verbose_level & 0x01;
  }


	/* ディスクイメージ名をセット */

  memset( file_disk[0], 0, QUASI88_MAX_FILENAME );
  memset( file_disk[1], 0, QUASI88_MAX_FILENAME );
  if( boot_file[0] ) strcpy( file_disk[0], boot_file[0] );
  if( boot_file[1] ) strcpy( file_disk[1], boot_file[1] );
  image_disk[0] = boot_image[0] -1;	/* イメージ番号は 1減ずる(0〜にする) */
  image_disk[1] = boot_image[1] -1;
  readonly_disk[0] = menu_readonly;	/* リードオンリーは、両ドライブ共通 */
  readonly_disk[1] = menu_readonly;



	/* 互換ROM指定時に、ファイルがあるかチェック */

  if( file_compatrom ){
    fname = alloc_romname( file_compatrom );
    if( fname ){
      file_compatrom = fname;
    }
  }



	/* 各種ディレクトリの表示 (デバッグ用) */

  if( verbose_proc ){
    const char *d;
    d = osd_dir_cwd ( );  printf( "cwd  directory = %s\n", d ? d : "(undef)" );
    d = osd_dir_rom ( );  printf( "rom  directory = %s\n", d ? d : "(undef)" );
    d = osd_dir_disk( );  printf( "disk directory = %s\n", d ? d : "(undef)" );
    d = osd_dir_tape( );  printf( "tape directory = %s\n", d ? d : "(undef)" );
    d = osd_dir_snap( );  printf( "snap directory = %s\n", d ? d : "(undef)" );
    d = osd_dir_state();  printf( "stat directory = %s\n", d ? d : "(undef)" );
    d = osd_dir_gcfg( );  printf( "gcfg directory = %s\n", d ? d : "(undef)" );
    d = osd_dir_lcfg( );  printf( "lcfg directory = %s\n", d ? d : "(undef)" );
  }

  return TRUE;
}
