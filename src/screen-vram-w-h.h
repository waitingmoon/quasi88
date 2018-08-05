#undef		LINE200
#undef		LINE400
#undef		COLUMNS
#undef		COLUMN_SKIP
#undef		ROWS
#undef		CHARA_LINES

/*======================================================================
  COLUMNS	80固定
  COLUMN_SKIP	80桁なら +1、 40桁なら +2
  ROWS		25 ないし 20
  CHARA_LINES	200ラインなら 8 ないし 10、 400ラインなら 16 ないし 20
  ======================================================================*/

#if	defined( COLOR ) || defined( MONO ) || defined( UNDISP )

#define		LINE200

#elif	defined( HIRESO )

#define		LINE400

#else
#error
#endif
/*----------------------------------------------------------------------*/
#if	(TEXT_WIDTH == 80)

#define		COLUMNS		(80)
#define		COLUMN_SKIP	(1)

#elif	(TEXT_WIDTH == 40)

#define		COLUMNS		(80)
#define		COLUMN_SKIP	(2)

#else
#error
#endif
/*----------------------------------------------------------------------*/
#if	(TEXT_HEIGHT == 25)

#define		ROWS		(25)

#elif	(TEXT_HEIGHT == 20)

#define		ROWS		(20)

#else
#error
#endif
/*----------------------------------------------------------------------*/
#if	defined( LINE200 )

#define		CHARA_LINES	(200/ROWS)

#elif	defined( LINE400 )

#define		CHARA_LINES	(400/ROWS)

#else
#error
#endif
/*----------------------------------------------------------------------*/
