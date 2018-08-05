/****************************************************************/
/* Tiny Z80 Assembler						*/
/*						Ver 0.2		*/
/*					(C) Showzoh Fukunaga	*/
/****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif


/* 登録可能なラベル数 */
#define	MAX_LABEL_NUM	(2048)

/* 宣言可能な ORG の数 */
#define	MAX_ORG_NUM	(8)


int		verbose = TRUE;		/* 冗長モード			*/
int		listing = FALSE;	/* ラベルリストの表示		*/


struct{
  long	start;				/* OGR 指定番地			*/
  long	end;				/* ORG 終了番地			*/
} org_table[ MAX_ORG_NUM+1 ];



char		label_buf[0x10000];	/* ラベルの文字列テーブル	*/
long		label_buf_ptr =1;	/*	その未使用領域の先頭	*/
						
struct{					/* ラベルの管理構造体		*/
  unsigned short ptr;			/*	文字列テーブルのポインタ*/
  unsigned short value;			/*	ラベルの指し示す値	*/
} label_table[ MAX_LABEL_NUM ];



unsigned char	program[0x10000];	/* アセンブル後のコード		*/
long		program_counter;	/* アセンブルしているアドレス	*/
long		program_start;		/* 一番最初のアセンブルアドレス	*/
int		assemble_err;		/* アセンブル時に発生したエラー	*/
int		assemble_end;		/* アセンブ終了フラグ		*/
int		assemble_pass;		/* 現在処理中のパス( 1 or 2 )	*/


char	src_line[256];		/* ソース行。この行を解析する		*/
char	*line;			/* 次に解析する文字位置			*/

struct{
  int	exist;			/* ラベルが存在するか			*/
  char	str[32];		/* ラベルの文字列(: は含まず)		*/
} label;

struct{
  int	exist;			/* オペコードが存在するか		*/
  char	str[6];			/* オペコードの文字列			*/
} opcode;

struct{
  int	exist;			/* オペランドが存在するか		*/
  int   type;			/* オペランドの種類			*/
  int   value_define;		/* オペランドが値の時、定義済みかどうか	*/
  short value;			/* オペランドの値(定義済の時のみ)   	*/
  char  str[256];		/* オペランドが文字列の時の文字列	*/
} oprand;



enum{
  NORMAL_END = 0,		/* 正常終了時の返り値			*/
};




/*----------------------------------------------------------------------------
  ラベル・オペコード・オペランドを得る

  行の先頭から文字をチェック
  行の先頭が '*' なら終了。ラベルなし、オペコードなし、終了

  順に文字を検査
    ; \n \0 は終了。ラベルなし、オペコードなし、終了
    SPC TAB はスキップ
    文字が見つかるとバッファに記憶開始
      ; \n \0 が見つかると行終了。ラベルなし、オペコードあり
      SPC TAB が見つかるとオペコードの終了。ラベルなし、オペコードあり
      : が見つかるとラベル終了。ラベルあり
  ラベルありの場合
      記憶したバッファを、ラベルバッファにコピー
      順に文字を検査
        ; \n \0 は終了。ラベルあり、オペコードなし
        SPC TAB はスキップ
        文字が見つかるとバッファに記憶開始
          ; \n \0 が見つかると、行終了。ラベルあり、オペコードあり
          SPC TAB が見つかると、オペコードの終了。ラベルあり、オペコードあり
          文字が見つかるとバッファに記憶する
  オペコードありの場合
      記憶したバッファを、オペコードバッファにコピー
      順に文字を検査
      ; \n \0 は終了。オペランドなし
      SPC TAB はスキップ
      文字が見つかると、処理終了。オペランドあり


  返り値
       0 … 正常終了
      10 … : だけのラベル(空のラベル)があった

      グローバル変数  label.exist   … ラベルの有無
		      label.str[]   … ラベル文字列
                      opcode.exist  … オペコードの有無
		      opcode.str[]  … オペコード文字列
		      oprand.exist  … オペランドの有無
		      *line	    … 次に解析を始める文字位置
----------------------------------------------------------------------------*/
enum{
  NULL_LABEL = 10,
};
int	get_label_opcode( void )
{
  char	buffer[256];
  char	*buf;
  int	i;


  label.exist  = FALSE;
  opcode.exist = FALSE;
  oprand.exist = FALSE;

  line = &src_line[0];

	/* '*' で始まる行は、全てコメントとみなす */

  if( *line=='*' ){
    return NORMAL_END;
  }


	/* 最初の文字列を検索。区切りは : SPC TAB */
	/*     : で終ればラベルなので、その後再度 */
	/*     文字列を検索し、オペコードを探す。 */
	/*     SPC TAB で終ればオペコード         */

  while(1){						/* 文字列の先頭を検索*/
    if     ( *line==';' || *line=='\n' || *line=='\0' ) return NORMAL_END;
    else if( *line==' ' || *line=='\t' ) line++;
    else                                 break;
  }
  buf = &buffer[0];					/* 文字列発見 */
  while(1){
    if      ( *line==';' || *line=='\n' || *line=='\0' ||
	      *line==' ' || *line=='\t' ){
      opcode.exist = TRUE;
      *buf='\0';
      break;
    }else if( *line==':' ){
      label.exist = TRUE;
      *buf='\0';
      line++;
      break;
    }else{
      *buf++ = *line++;
    }
  }

  if( label.exist ){	/*** 文字列がラベルの時は、オペコードを検索 ***/

    for( i=0; i<31; i++ ) label.str[i] = buffer[i];	/* ラベルを格納 */
    label.str[31] = '\0';
    if( label.str[0]=='\0' ) return NULL_LABEL;

    while(1){						/* 文字列の先頭を検索*/
      if     ( *line==';' || *line=='\n' || *line=='\0' ) return NORMAL_END;
      else if( *line==' ' || *line=='\t' ) line++;
      else                                 break;
    }
    buf = &buffer[0];					/* 文字列発見    */
    while(1){
      if      ( *line==';' || *line=='\n' || *line=='\0' ||
	        *line==' ' || *line=='\t' ){
	opcode.exist = TRUE;
	*buf='\0';
	break;
      }else{
	*buf++ = *line++;
      }
    }
  }

  if( opcode.exist ){	/*** オペコードがある場合は、オペランドを検索 ***/

    for( i=0; i<5; i++ ) opcode.str[i] = buffer[i];	/* オペコードを格納*/
    opcode.str[5] = '\0';

    while(1){						/* 文字列の先頭を検索*/
      if     ( *line==';' || *line=='\n' || *line=='\0' ) return NORMAL_END;
      else if( *line==' ' || *line=='\t' ) line++;
      else                                 break;
    }
    oprand.exist = TRUE;				/* 文字列発見    */
  }

  return NORMAL_END;
}





/*----------------------------------------------------------------------------
  label.str[] をlabel_table に登録する

  返り値
	 0 … 登録完了
	20 … ラベル名のフォーマットエラー
        21 … ラベル名の多重定義
	22 … ラベル数オーバー
	23 … ラベルバッファ不足
      グローバル変数  label_table[] … ラベル管理テーブル
                      label_buf[]   … ラベル文字列バッファ
		      label_buf_ptr … ラベル文字列バッファの終端
----------------------------------------------------------------------------*/
static const char *reserved_keyword[] = {	/* ラベルに使ってはいけない */
  "A","B","C","D","E","F","H","L","I","R","IXH","IXL","IYH","IYL",
  "AF","BC","DE","HL","IX","IY","SP",
  "C","NC","Z","NZ","PE","PO","M","P",
  NULL
};
enum{
  LABEL_BAD = 20,
  LABEL_DUPLICATE,
  LABEL_OVER,
  LABEL_SHORTAGE,
};
int	regist_label( int value )
{
  int	i;

  if( assemble_pass==2 ) return NORMAL_END;

	/* ラベルが無い場合は登録せずに戻る	*/

  if( label.exist==FALSE ){
    return NORMAL_END;
  }


	/* ラベル名チェック			*/
	/*	空の文字列でない		*/
	/*	先頭は、_ か、英字		*/
	/*	2文字目以降は _ か、英数字	*/
	/*	予約語とかちあわない		*/

  if( label.str[0]=='\0' ){				/* ラベルがない      */
    return LABEL_BAD;
  }
  if( label.str[0]!='_' && !isalpha(label.str[0]) ){	/*先頭が _ 英字でない*/
    return LABEL_BAD;
  }
  for( i=1; label.str[i]; i++ ){
    if( label.str[i]!='_' && !isalnum(label.str[i]) ){	/* _ 英数時以外の文字*/
      return LABEL_BAD;
    }
  }
  for( i=0; reserved_keyword[i]; i++ ){			/* 予約語でない */
    if( strcmp( reserved_keyword[i], &label.str[0] )==0 ){
      return LABEL_BAD;
    }
  }

	/* ラベル二重定義チェック	*/

  for( i=0; i<MAX_LABEL_NUM; i++ ){
    if( label_table[i].ptr==0 ) break;
    if( strcmp( &label_buf[ label_table[i].ptr ], &label.str[0] )==0 ){
      return LABEL_DUPLICATE;
    }
  }


	/* ラベル数、サイズオーバーチェック	*/

  if( i>=MAX_LABEL_NUM ){				/* ラベル数おお過ぎ */
    return LABEL_OVER;
  }
  if( label_buf_ptr+strlen(&label.str[0])+1>=0x10000 ){	/* バッファ不足 >64K */
    return LABEL_SHORTAGE;
  }


	/* ラベル登録			*/

  label_table[i].ptr   = label_buf_ptr;
  label_table[i].value = value & 0xffff;
  strcpy( &label_buf[ label_buf_ptr ], &label.str[0] );
  label_buf_ptr += strlen( &label.str[0] ) +1;

  return NORMAL_END;
}

/*-----------------------------------------------------------------------------
  オペランドを分析する

	' で始まっている場合
	  対応する ' を探し、文字定数とし終了

	( で始まっている場合
	  対応する ) を探し、()の中身をバッファに格納
	数字、文字で始まっている場合
	  終端を探し、バッファに格納

	バッファに格納した文字列を分析
	最初の1語をチェック
	  * / で始まっていたらエラー
	  予約語かどうか判定
	  その語に + - が続いているか判定
	  その語に * / が続いているか判定

	予約語の場合
	  ()で囲まれていない時
	    + - * / が続いている → エラー
	    そうでない           → 予約語を返す
	  ()で囲まれている時
	    + - が続いている	 → IX IY 以外はエラー
	    * / が続いている	 → エラー
	    そうでない		 → アドレス表記不可ならエラー
	数字、文字の場合
	  数字を計算。未定義ラベルを含む場合、フラグを立てる

  返り値
    0 … 正常終了(未定義ラベルも含む)
    30 … オペランドが存在しない
    31 … '' の対応がない、中身がない、終端で終らない
    32 … () の対応がない、中身がない、終端で終らない
    33 … 指定レジスタでは間接番地参照ができない
    34 … 指定レジスタではインデックス参照ができない
    35 … レジスタに対し演算しようとした
    36 … 式の評価中にエラーが出た

    oprand.type         … オペランドの種類( enum 参照 )
    oprand.value_define … 数値を伴うペランド (IX+d),(IY+x), n, (n) において
                           数値が確定しているか、未確定か
    oprand.value        … 数値を伴うオペランドの数値
                           未確定の場合はゴミが入る
    oprand.str          … 文字列のオペランド 'xxx' における文字列
*/

enum{
  OPRAND_A,	OPRAND_B,	OPRAND_C,	OPRAND_D,	OPRAND_E,
  OPRAND_F,	OPRAND_H,	OPRAND_L,	OPRAND_I,	OPRAND_R,
  OPRAND_IXH,	OPRAND_IXL,	OPRAND_IYH,	OPRAND_IYL,

  OPRAND_AF,	OPRAND_BC,	OPRAND_DE,	OPRAND_HL,
  OPRAND_IX,	OPRAND_IY,	OPRAND_SP,	OPRAND_AFF,

  OPRAND_NZ,	OPRAND_Z,	OPRAND_NC,
  OPRAND_PE,	OPRAND_PO,	OPRAND_P,	OPRAND_M,	

  OPRAND_xBC,	OPRAND_xDE,	OPRAND_xHL,	OPRAND_xSP,
  OPRAND_xIX,	OPRAND_xIY,	OPRAND_xIXd,	OPRAND_xIYd,
  OPRAND_xC,

  OPRAND_n,	OPRAND_xn,	OPRAND_str,
};

static const struct{
  char	*name;			/* オペランドの名前 */
  int	type;			/* オペランドの種類 */
  int	addr_ok;		/* ()  指定可能か？ */
  int	addr_type;		/* ()  指定時の種類 */
  int	index_ok;		/* (+d)指定可能か？ */
  int	index_type;		/* (+d)指定時の種類 */
} reg_list[]=
{
  { "A",   OPRAND_A,   FALSE, 0,          FALSE, 0,           },
  { "B",   OPRAND_B,   FALSE, 0,          FALSE, 0,           },
  { "C",   OPRAND_C,   TRUE,  OPRAND_xC,  FALSE, 0,           },
  { "D",   OPRAND_D,   FALSE, 0,          FALSE, 0,           },
  { "E",   OPRAND_E,   FALSE, 0,          FALSE, 0,           },
  { "F",   OPRAND_F,   FALSE, 0,          FALSE, 0,           },
  { "H",   OPRAND_H,   FALSE, 0,          FALSE, 0,           },
  { "L",   OPRAND_L,   FALSE, 0,          FALSE, 0,           },
  { "I",   OPRAND_I,   FALSE, 0,          FALSE, 0,           },
  { "R",   OPRAND_R,   FALSE, 0,          FALSE, 0,           },
  { "IXH", OPRAND_IXH, FALSE, 0,          FALSE, 0,           },
  { "IXL", OPRAND_IXL, FALSE, 0,          FALSE, 0,           },
  { "IYH", OPRAND_IYH, FALSE, 0,          FALSE, 0,           },
  { "IYL", OPRAND_IYL, FALSE, 0,          FALSE, 0,           },

  { "NZ",  OPRAND_NZ,  FALSE, 0,          FALSE, 0,           },
  { "Z",   OPRAND_Z,   FALSE, 0,          FALSE, 0,           },
  { "NC",  OPRAND_NC,  FALSE, 0,          FALSE, 0,           },
  { "PE",  OPRAND_PE,  FALSE, 0,          FALSE, 0,           },
  { "PO",  OPRAND_PO,  FALSE, 0,          FALSE, 0,           },
  { "P",   OPRAND_P,   FALSE, 0,          FALSE, 0,           },
  { "M",   OPRAND_M,   FALSE, 0,          FALSE, 0,           },

  { "IX",  OPRAND_IX,  TRUE,  OPRAND_xIX, TRUE,  OPRAND_xIXd, },
  { "IY",  OPRAND_IY,  TRUE,  OPRAND_xIY, TRUE,  OPRAND_xIYd, },

  { "BC",  OPRAND_BC,  TRUE,  OPRAND_xBC, FALSE, 0,           },
  { "DE",  OPRAND_DE,  TRUE,  OPRAND_xDE, FALSE, 0,           },
  { "HL",  OPRAND_HL,  TRUE,  OPRAND_xHL, FALSE, 0,           },
  { "SP",  OPRAND_SP,  TRUE,  OPRAND_xSP, FALSE, 0,           },
  { "AF",  OPRAND_AF,  FALSE, 0,          FALSE, 0,           },
  { "AF'", OPRAND_AFF, FALSE, 0,          FALSE, 0,           },
};
enum{
  NO_OPRAND = 30,
  BAD_STRING,
  BAD_ADDRESS,
  BAD_ADDR_REG,
  BAD_INDEX_REG,
  BAD_REG,
  BAD_CALC,
  BAD_CALC_LABEL,
};
int	get_oprand( void )
{
  int	i;
  int	address_flag, keyword_flag, index_flag, calc_flag;
  char	buffer[256];
  char	*buf, *str;


	/* 次にオペランドがあるかをチェック */

  if( oprand.exist==FALSE ) return NO_OPRAND;



	/* 文字列の先頭を検索(SPC TABは飛ばす) */

  while(1){
    if     ( *line==';' || *line=='\n' || *line=='\0' ) return NO_OPRAND;
    else if( *line==' ' || *line=='\t' ) line++;
    else                                 break;
  }
  buf = &oprand.str[0];


	/* '文字列' の場合 → 文字列 oprand.str[] に格納し、終了 */

  if( *line==0x27 ){
    line++;
    while(1){						/* 文字列取得 */
      if     ( *line=='\n' || *line=='\0' )	return BAD_STRING;
      else if( *line==0x27 && *(line+1)!=0x27 ){line++;  break; }
      else if( *line==0x27 && *(line+1)==0x27 ){*buf++ = *line++;  line++; }
      else					*buf++ = *line++;
    }
    *buf = '\0';

    if( oprand.str[0]=='\0' ) return BAD_STRING;	/* 文字列あり? */

    while(1){						/* 次オペランドあり? */
      if     ( *line==';' || *line=='\n' || *line=='\0' ) break;
      else if( *line==' ' || *line=='\t' ) line++;
      else if( *line==',')                 break;
      else                                 return BAD_STRING;
    }
    oprand.type = OPRAND_str;
    buf = &oprand.str[0];      i = 0;			/* 文字列の最後の */
    while( *buf ){					/* 2Byte を数値に */
      i = ( i<<8 ) | *buf++;
    }
    oprand.value_define = TRUE;
    oprand.value = i & 0xffff;
    if( *line==',' ){
      line ++;
      oprand.exist = TRUE;
      return NORMAL_END;
    }else{
      oprand.exist = FALSE;
      return NORMAL_END;
    }
  }


	/* (文字列) の場合 → 文字列を oprand.str[] に格納し、次へ */

  if( *line=='(' ){
    line++;
    address_flag = TRUE;
    while(1){
      if     ( *line==';' || *line=='\n' || *line=='\0' ) return BAD_ADDRESS;
      else if( *line==')'  ){line++;  break; }
      else                   *buf++ = *line++;
    }
    *buf = '\0';

    if( oprand.str[0]=='\0' ) return BAD_ADDRESS;	/* 文字列あり? */

    while(1){
      if     ( *line==';' || *line=='\n' || *line=='\0' ) break;
      else if( *line==' ' || *line=='\t' ) line++;
      else if( *line==',')                 break;
      else                                 return BAD_ADDRESS;
    }
    if( *line==',' ){
      line++;
      oprand.exist = TRUE;
    }else{
      oprand.exist = FALSE;
    }      
  }

	/* 文字列 の場合 → 文字列を oprand.str[] に格納し、次へ */

  else{						/* 文字列 の場合 */
    address_flag = FALSE;
    while(1){
      if     ( *line==';' || *line=='\n' || *line=='\0' ) break;
      else if( *line==',') break;
      else                 *buf++ = *line++;
    }
    *buf = '\0';

    if( oprand.str[0]=='\0' ) return NO_OPRAND;		/* 文字列あり? */

    if( *line==',' ){
      line ++;
      oprand.exist = TRUE;
    }else{
      oprand.exist = FALSE;
    }      
  }


	/* oprand.str[] の中身を解析する。				*/
	/* 最初の1語を予約語／数字に分類する。区切りは SPC TAB + - * /	*/
	/* 予約語に + - が続けば、インデックスの判定			*/
	/* アドレスの場合は、アドレスの判定				*/
	/* ラベル(数字も含む)の場合は、式を評価し、演算する。		*/
	/* 演算子は + - * /。カッコはなし。優先は、* /、+ - の順	*/
	/* 0 で除算した場合は式評価エラーとなる				*/

  buf = &buffer[0];
  str = &oprand.str[0];

  if( *str=='*' || *str=='/' || *str=='\0' ) return BAD_CALC;

  if( *str=='+' || *str=='-' ){
    *buf++ = *str++;
  }

  while(1){					/* 最初の 1語を get */
    if( *str==' ' || *str=='\t'|| *str=='\0'||
        *str=='+' || *str=='-' || *str=='*' || *str=='/' ){
      break;
    }
    *buf++ = *str++;
  };
  *buf = '\0';


  for( i=0; i<(sizeof(reg_list)/sizeof(reg_list[0])); i++ ){	/* 予約語か? */
    if( strcmp( &buffer[0], reg_list[i].name )==0 ) break;
  }
  if( i<(sizeof(reg_list)/sizeof(reg_list[0])) ) keyword_flag = TRUE;
  else                                           keyword_flag = FALSE;

  index_flag = calc_flag = FALSE;				/* 次演算子? */
  while(1){
    if     ( *str==' ' || *str=='\t') str++;
    else if( *str=='+' || *str=='-' ){index_flag = TRUE; break; }
    else if( *str=='*' || *str=='/' ){calc_flag = TRUE;  break; }
    else if( *str=='\0' )             break;
    else                              return BAD_CALC_LABEL;
  }

  if( keyword_flag ){				/* 予約語の場合		*/
    if( address_flag==FALSE ){
      if( index_flag ){				/*	reg +n	  → ×	*/
        return BAD_REG;
      }else if( calc_flag ){			/*	reg *n	  → ×	*/
        return BAD_REG;
      }else{					/*	reg	  → ○	*/
        oprand.type = reg_list[i].type;
        return NORMAL_END;
      }
    }else{
      if( index_flag ){				/*	(reg +n)  → ○?*/
	if( reg_list[i].index_ok ){
	  oprand.type = reg_list[i].index_type;
	}else{
	  return BAD_INDEX_REG;
	}
      }else if( calc_flag ){			/*	(reg *n)  → ×	*/
        return BAD_REG;
      }else{					/*	(reg)	  → ○?*/
	if( reg_list[i].addr_ok ){
	  oprand.type = reg_list[i].addr_type;
	  return NORMAL_END;
	}else{
	  return BAD_ADDR_REG;
	}
      }
    }
  }else{					/* 数字の場合	*/
    if( address_flag==FALSE ){			/*	xxx ..	*/
      oprand.type = OPRAND_n;
    }else{					/*	(xxx ..)*/
      oprand.type = OPRAND_xn;
    }
    str = &oprand.str[0];
  }


	/* *str 以降を演算する。*str はこの時点で、IX,IY の場合は */
	/* +または- を、ラベルの場合はその先頭を指している。      */

  {
    extern int analyze_err;
    extern int analyze_expression( char **s );

    oprand.value = analyze_expression( &str ) & 0xffff;
    if( analyze_err ) return BAD_CALC;
  }

  return NORMAL_END;
}


/*===========================================================================*/

/* *str 以降〜\0までの式を計算すし、その値を返す。*str は更新される。	*/
/* 正常に式の評価が完了した時、analyze_err = 0 となる。			*/
/* 数字の表記にエラーがある場合は、analyze_err = 1 となる。		*/
/* 0 で 除算をした場合、analyze_err = 2 となる。			*/
/* ラベルが未定義の時は、oprand.value_define=FALSE となるが、		*/
/*			そのラベルを 1 に置き換えて計算は続けられる。	*/
/* ラベルが予約語だった場合、analyze_err = 1 となる。			*/

static	int	analyze_ch;
int		analyze_err;

static	void	analyze_next( char **s )
{
  while(1){
    if     ( **s==' ' || **s=='\t' ) (*s)++;
    else if( **s=='\0' )            { analyze_ch = -1;          break; }
    else                            { analyze_ch = **s; (*s)++; break; }
  }	       
}
static	int	analyze_number( char **s )
{
  int	x, sign = +1;
  extern int get_number_in_str( char **str );

  if     ( analyze_ch=='+' ){ sign = +1;  analyze_next(s); }
  else if( analyze_ch=='-' ){ sign = -1;  analyze_next(s); }
  x = get_number_in_str(s);
  if( x<0 ){ analyze_err = 1; return 1; }
  return sign * x;
}
static	int	analyze_term( char **s )
{
  int	x,y;
  x = analyze_number(s);
  while(1){
    if ( analyze_ch=='*' ){
      analyze_next(s);
      x *= analyze_number(s);
    }else if( analyze_ch=='/' ){
      analyze_next(s);
      y = analyze_number(s); if(y==0){ y=1; analyze_err=2; }
      x /= y;
    }
    else break;                    
  }
  return x;
}
int	analyze_expression( char **s )
{
  int	x;
  analyze_err = 0;
  oprand.value_define = TRUE;

  analyze_next(s);
  x = analyze_term(s);
  while(1){
    if     ( analyze_ch=='+' ){ analyze_next(s);  x += analyze_term(s); }
    else if( analyze_ch=='-' ){ analyze_next(s);  x -= analyze_term(s); }
    else                        break;
  }
  return x;
}


/* *str 以降〜演算子または\0までを数字に変換し、その値を返す。		*/
/* 返される値は 0x0000 〜 0xffff まで。*str は更新される。		*/
/* 数字の表記にエラーがある場合は、-1 が返る				*/
/* ラベルの場合はその値を返すが、未定義の場合は				*/
/*			oprand.value_define=FALSE として、1 を返す	*/

static	unsigned short	char_to_num( int c, int base )
{
  int i;
  for( i=0; i<base; i++){
    if( c == "0123456789ABCDEF"[i] ) return i;
    if( c == "0123456789abcdef"[i] ) return i;
  }
  return -1;
}

int	get_number_in_str( char **str )
{
  char buffer[256];
  char *buf;
  int  i, number_flag, base, get_num;
  unsigned short num;


  if( isdigit(analyze_ch) ) number_flag = TRUE;	    /* 先頭の1文字をみて、  */
  else                      number_flag = FALSE;    /* 数字かラベルかを判定 */

  buf = &buffer[0];
  while(1){
    *buf++ = analyze_ch;

    analyze_next(str);
    if( analyze_ch==-1 ) break;
    if( analyze_ch=='+' || analyze_ch=='-' ||
        analyze_ch=='*' || analyze_ch=='/'  ) break;
  }
  *buf='\0';

  if( buffer[0]=='\0' ) return -2;		/* 空文字列…そんなはずないぞ*/


  if( number_flag ){					/* 数字 */

    if     ( *(buf-1)=='B' || *(buf-1)=='b' ){ *(buf-1)='\0'; base =  2; }
    else if( *(buf-1)=='H' || *(buf-1)=='h' ){ *(buf-1)='\0'; base = 16; }
    else                                     { base = 10 ; }

    buf = &buffer[0];
    num = 0;
    while( *buf!='\0' ){
      if( (get_num = char_to_num( *buf++, base )) < 0 ) return -1;
      num = num*base + get_num;
    }
    return num;

  }else{						/* ラベル */

    if( strcmp( "$", &buffer[0] )==0 ){				/* $ の時 */
      return (unsigned short)program_counter;
    }

    for( i=0; reserved_keyword[i]; i++ ){			/* 予約語 */
      if( strcmp( reserved_keyword[i], &buffer[0] )==0 ){
	return -1;
      }
    }

    for( i=0;  ; i++ ){
      if( i==MAX_LABEL_NUM || label_table[i].ptr==0 ){		/* 未定義 */
	oprand.value_define = FALSE;
	return 1;
      }
      if( strcmp( &buffer[0], &label_buf[ label_table[i].ptr ] )==0 ){
	return label_table[i].value;
      }
    }

  }

}



/*---------------------------------------------------------------------------
  1行をアセンブルする。

  返り値
	0 … 正常終了
	1 … ワーニング。未定義ラベルあり
        2 … エラー。アセンブル不能

	pass-1 では、2 でエラー
	pass-2 では、1、2 でエラーとなる。

      グローバル変数
        assemble_err … 返り値が 2 の時のエラーの種類
	   0 … エラーなし
	   1 … 未定義ラベルあり。
	   2 … ラベル名のエラー(予約語、禁止文字、空)
	   3 … ラベルの多重定義
	   4 … ラベル数のエラー(MAX_LABEL_NUM)
	   5 … ラベルバッファオーバー
	   6 … シンタックスエラー
	   7 … 演算エラー(構造)
	   8 … 演算エラー(0除算)
	   9 … ORG,EQU,DS で未定義ラベルを使った

	program_counter … 次のプログラムカウンタの値
----------------------------------------------------------------------------*/
int	op[2];		/* 第1オペランド、第2オペランド */
int	flag[2];	/* 数字は定義済？		*/
int	val[2];		/* 数字				*/
int	code[2];	/* チェック時に返す値		*/


enum{				/***** オペランドのグループ *****/
  g_NULL,			/*	オペランドなし		*/
  g_A,				/*	A			*/
  g_A___xHL,			/*	A B C D E H L (HL)	*/
  g_ABCDEHL,			/*	A B C D E H L		*/
  g_ABCDE,			/*	A B C D E		*/
  g_ABCDEFHL,			/*	A B C D E H L  F	*/
  g_nn,				/*	定数			*/
  g_xnn,			/*	絶対アドレス		*/
  g_xIXdxIYd,			/*	(IX+d) (IY+d)		*/
  g_xHL,			/*	(HL)			*/
  g_xBC_xDE,			/*	(BC) (DE)		*/
  g_xIX_xIY,			/*	(IX) (IY)		*/
  g_I_R,			/*	I R			*/
  g_IXY_H_L,			/*	IXH IXL IYH IYL		*/
  g_IXH_IXL,			/*	IXH IXL			*/
  g_IYH_IYL,			/*		IYH IYL		*/
  g_AF,				/*	AF			*/
  g_AFF,			/*	AF'			*/
  g_DE,				/*	DE			*/
  g_HL,				/*	HL			*/
  g_IX,				/*	IX			*/
  g_IY,				/*	IY			*/
  g_IX_IY,			/*	IX IY			*/
  g_SP,				/*	SP			*/
  g_xSP,			/*	(SP)			*/
  g_BCDEHLAF,			/*	BC DE HL AF		*/
  g_BCDEHLSP,			/*	BC DE HL SP		*/
  g_BCDEIXSP,			/*	BC DE IX SP		*/
  g_BCDEIYSP,			/*	BC DE IY SP		*/
  g_xC,				/*	(C)			*/
  g_IM,				/*	IM  命令 の オペランド	*/
  g_RST,			/*	RST 命令 の オペランド	*/
  g_JP,				/*	NZ Z NC C PO PE P M	*/
  g_JR,				/*	NZ Z NC C		*/
  g_BIT,			/*	0 1 2 3 4 5 6 7		*/
};  

enum{				/***** コードの出力体系分類 *****/
  CONST,			/*	コード	    [コード]	*/
  INDEX,			/*	インデックス[op1 or op2]*/
  ZZP3Q,			/*	[定数] + (op1<<3 | op2)	*/
  ZZP3_,			/*	[定数] + (op1<<3      )	*/
  ZZQ3_,			/*	[定数] + (op2<<3      )	*/
  ZZ__P,			/*	[定数] + (         op1)	*/
  ZZ__Q,			/*	[定数] + (         op2)	*/
  ZZP4_,			/*	[定数] + (op1<<4      )	*/
  ZZQ4_,			/*	[定数] + (op1<<4      )	*/
  NUM_8,			/*	8bit数値     [op1 or op2]*/
  NUM_H,			/*	16bit数値上位[op1 or op2]*/
  NUM_L,			/*	16bit数値下位[op1 or op2]*/
  OFSET,			/*	IX IYの変位値[op1 or op2]*/
  ADDRS,			/*	JR   の変位置[op1 or op2]*/
};

#define CNS(hex)	((CONST<<8) + (0x##hex&0xff))
#define IDX(hex)	((INDEX<<8) + (0x##hex&0xff))
#define P3Q(hex)	((ZZP3Q<<8) + (0x##hex&0xff))
#define P3o(hex)	((ZZP3_<<8) + (0x##hex&0xff))
#define Q3o(hex)	((ZZQ3_<<8) + (0x##hex&0xff))
#define ooP(hex)	((ZZ__P<<8) + (0x##hex&0xff))
#define ooQ(hex)	((ZZ__Q<<8) + (0x##hex&0xff))
#define P4o(hex)	((ZZP4_<<8) + (0x##hex&0xff))
#define Q4o(hex)	((ZZQ4_<<8) + (0x##hex&0xff))
#define n_8(hex)	((NUM_8<<8) + (0x##hex&0xff))
#define n_H(hex)	((NUM_H<<8) + (0x##hex&0xff))
#define n_L(hex)	((NUM_L<<8) + (0x##hex&0xff))
#define d_8(hex)	((OFSET<<8) + (0x##hex&0xff))
#define e_8(hex)	((ADDRS<<8) + (0x##hex&0xff))

struct{
  char	*mnemonic;			/* オペランド		*/
  int	oprand_num;			/* オペコードの数	*/
  int	oprand[2];			/* オペコードのグループ */
  int	code_num;			/* コードの数		*/
  short	code[4];			/* コードの種類		*/
} instruction[] =
{
{"LD",    2, g_ABCDEHL , g_A___xHL ,   1, P3Q(40), 0      , 0      , 0       },
{"LD",    2, g_xHL     , g_ABCDEHL ,   1, P3Q(40), 0      , 0      , 0       },
{"LD",    2, g_A___xHL , g_nn      ,   2, P3Q(06), n_8( 2), 0      , 0       },
{"LD",    2, g_ABCDEHL , g_xIXdxIYd,   3, IDX( 2), P3Q(40), d_8( 2), 0       },
{"LD",    2, g_xIXdxIYd, g_ABCDEHL ,   3, IDX( 1), P3Q(40), d_8( 1), 0       },
{"LD",    2, g_xIXdxIYd, g_nn      ,   4, IDX( 1), P3Q(06), d_8( 1), n_8( 2) },

{"LD",    2, g_A       , g_xBC_xDE ,   1, Q4o(0a), 0      , 0      , 0       },
{"LD",    2, g_A       , g_xnn     ,   3, Q4o(0a), n_L( 2), n_H( 2), 0       },
{"LD",    2, g_xBC_xDE , g_A       ,   1, P4o(02), 0      , 0      , 0       },
{"LD",    2, g_xnn     , g_A       ,   3, P4o(02), n_L( 2), n_H( 2), 0       },
{"LD",    2, g_A       , g_I_R     ,   2, CNS(ed), Q3o(57), 0      , 0       },
{"LD",    2, g_I_R     , g_A       ,   2, CNS(ed), P3o(47), 0      , 0       },

{"LD",    2, g_ABCDE   , g_IXY_H_L ,   2, IDX( 2), P3Q(40), 0      , 0       },
{"LD",    2, g_IXY_H_L , g_ABCDE   ,   2, IDX( 1), P3Q(40), 0      , 0       },
{"LD",    2, g_IXY_H_L , g_nn      ,   3, IDX( 1), P3Q(06), n_8( 2), 0       },
{"LD",    2, g_IXH_IXL , g_IXH_IXL ,   2, IDX( 1), P3Q(40), 0      , 0       },
{"LD",    2, g_IYH_IYL , g_IYH_IYL ,   2, IDX( 1), P3Q(40), 0      , 0       },

{"LD",    2, g_BCDEHLSP, g_nn      ,   3, P4o(01), n_L( 2), n_H( 2), 0       },
{"LD",    2, g_IX_IY   , g_nn      ,   4, IDX( 1), P4o(01), n_L( 2), n_H( 2) },
{"LD",    2, g_HL      , g_xnn     ,   3, CNS(2a), n_L( 2), n_H( 2), 0       },
{"LD",    2, g_IX_IY   , g_xnn     ,   4, IDX( 1), CNS(2a), n_L( 2), n_H( 2) },
{"LD",    2, g_xnn     , g_HL      ,   3, CNS(22), n_L( 1), n_H( 1), 0       },
{"LD",    2, g_xnn     , g_IX_IY   ,   4, IDX( 2), CNS(22), n_L( 1), n_H( 1) },

{"LD",    2, g_BCDEHLSP, g_xnn     ,   4, CNS(ed), P4o(4b), n_L( 2), n_H( 2) },
{"LD",    2, g_xnn     , g_BCDEHLSP,   4, CNS(ed), Q4o(43), n_L( 1), n_H( 1) },

{"LD",    2, g_SP      , g_HL      ,   1, CNS(f9), 0      , 0      , 0       },
{"LD",    2, g_SP      , g_IX_IY   ,   2, IDX( 2), CNS(f9), 0      , 0       },

{"PUSH",  1, g_BCDEHLAF, 0         ,   1, P4o(c5), 0      , 0      , 0       },
{"PUSH",  1, g_IX_IY   , 0         ,   2, IDX( 1), P4o(c5), 0      , 0       },
{"POP",   1, g_BCDEHLAF, 0         ,   1, P4o(c1), 0      , 0      , 0       },
{"POP",   1, g_IX_IY   , 0         ,   2, IDX( 1), P4o(c1), 0      , 0       },

{"EX",    2, g_DE      , g_HL      ,   1, CNS(eb), 0      , 0      , 0       },
{"EX",    2, g_xSP     , g_HL      ,   1, CNS(e3), 0      , 0      , 0       },
{"EX",    2, g_xSP     , g_IX_IY   ,   2, IDX( 2), CNS(e3), 0      , 0       },
{"EX",    2, g_AF      , g_AFF     ,   1, CNS(08), 0      , 0      , 0       },
{"EXX",   0, 0         , 0         ,   1, CNS(d9), 0      , 0      , 0       },

{"LDI",   0, 0         , 0         ,   2, CNS(ed), CNS(a0), 0      , 0       },
{"LDIR",  0, 0         , 0         ,   2, CNS(ed), CNS(b0), 0      , 0       },
{"LDD",   0, 0         , 0         ,   2, CNS(ed), CNS(a8), 0      , 0       },
{"LDDR",  0, 0         , 0         ,   2, CNS(ed), CNS(b8), 0      , 0       },

{"CPI",   0, 0         , 0         ,   2, CNS(ed), CNS(a1), 0      , 0       },
{"CPIR",  0, 0         , 0         ,   2, CNS(ed), CNS(b1), 0      , 0       },
{"CPD",   0, 0         , 0         ,   2, CNS(ed), CNS(a9), 0      , 0       },
{"CPDR",  0, 0         , 0         ,   2, CNS(ed), CNS(b9), 0      , 0       },

{"ADD",   2, g_A       , g_A___xHL ,   1, ooQ(80), 0      , 0      , 0       },
{"ADD",   2, g_A       , g_xIXdxIYd,   3, IDX( 2), ooQ(80), d_8( 2), 0       },
{"ADD",   2, g_A       , g_nn      ,   2, CNS(c6), n_8( 2), 0      , 0       },
{"ADD",   2, g_A       , g_IXY_H_L ,   2, IDX( 2), ooQ(80), 0      , 0       },

{"ADC",   2, g_A       , g_A___xHL ,   1, ooQ(88), 0      , 0      , 0       },
{"ADC",   2, g_A       , g_xIXdxIYd,   3, IDX( 2), ooQ(88), d_8( 2), 0       },
{"ADC",   2, g_A       , g_nn      ,   2, CNS(ce), n_8( 2), 0      , 0       },
{"ADC",   2, g_A       , g_IXY_H_L ,   2, IDX( 2), ooQ(88), 0      , 0       },

{"SUB",   1, g_A___xHL , 0         ,   1, ooP(90), 0      , 0      , 0       },
{"SUB",   1, g_xIXdxIYd, 0         ,   3, IDX( 1), ooP(90), d_8( 1), 0       },
{"SUB",   1, g_nn      , 0         ,   2, CNS(d6), n_8( 1), 0      , 0       },
{"SUB",   1, g_IXY_H_L , 0         ,   2, IDX( 1), ooP(90), 0      , 0       },

{"SBC",   2, g_A       , g_A___xHL ,   1, ooQ(98), 0      , 0      , 0       },
{"SBC",   2, g_A       , g_xIXdxIYd,   3, IDX( 2), ooQ(98), d_8( 2), 0       },
{"SBC",   2, g_A       , g_nn      ,   2, CNS(de), n_8( 2), 0      , 0       },
{"SBC",   2, g_A       , g_IXY_H_L ,   2, IDX( 2), ooQ(98), 0      , 0       },

{"AND",   1, g_A___xHL , 0         ,   1, ooP(a0), 0      , 0      , 0       },
{"AND",   1, g_xIXdxIYd, 0         ,   3, IDX( 1), ooP(a0), d_8( 1), 0       },
{"AND",   1, g_nn      , 0         ,   2, CNS(e6), n_8( 1), 0      , 0       },
{"AND",   1, g_IXY_H_L , 0         ,   2, IDX( 1), ooP(a0), 0      , 0       },

{"XOR",   1, g_A___xHL , 0         ,   1, ooP(a8), 0      , 0      , 0       },
{"XOR",   1, g_xIXdxIYd, 0         ,   3, IDX( 1), ooP(a8), d_8( 1), 0       },
{"XOR",   1, g_nn      , 0         ,   2, CNS(ee), n_8( 1), 0      , 0       },
{"XOR",   1, g_IXY_H_L , 0         ,   2, IDX( 1), ooP(a8), 0      , 0       },

{"OR",    1, g_A___xHL , 0         ,   1, ooP(b0), 0      , 0      , 0       },
{"OR",    1, g_xIXdxIYd, 0         ,   3, IDX( 1), ooP(b0), d_8( 1), 0       },
{"OR",    1, g_nn      , 0         ,   2, CNS(f6), n_8( 1), 0      , 0       },
{"OR",    1, g_IXY_H_L , 0         ,   2, IDX( 1), ooP(b0), 0      , 0       },

{"CP",    1, g_A___xHL , 0         ,   1, ooP(b8), 0      , 0      , 0       },
{"CP",    1, g_xIXdxIYd, 0         ,   3, IDX( 1), ooP(b8), d_8( 1), 0       },
{"CP",    1, g_nn      , 0         ,   2, CNS(fe), n_8( 1), 0      , 0       },
{"CP",    1, g_IXY_H_L , 0         ,   2, IDX( 1), ooP(b8), 0      , 0       },

{"INC",   1, g_A___xHL , 0         ,   1, P3o(04), 0      , 0      , 0       },
{"INC",   1, g_xIXdxIYd, 0         ,   3, IDX( 1), P3o(04), d_8( 1), 0       },
{"INC",   1, g_IXY_H_L , 0         ,   2, IDX( 1), P3o(04), 0      , 0       },
{"DEC",   1, g_A___xHL , 0         ,   1, P3o(05), 0      , 0      , 0       },
{"DEC",   1, g_xIXdxIYd, 0         ,   3, IDX( 1), P3o(05), d_8( 1), 0       },
{"DEC",   1, g_IXY_H_L , 0         ,   2, IDX( 1), P3o(05), 0      , 0       },

{"ADD",   2, g_HL      , g_BCDEHLSP,   1, Q4o(09), 0      , 0      , 0       },
{"ADD",   2, g_IX      , g_BCDEIXSP,   2, IDX( 1), Q4o(09), 0      , 0       },
{"ADD",   2, g_IY      , g_BCDEIYSP,   2, IDX( 1), Q4o(09), 0      , 0       },

{"ADC",   2, g_HL      , g_BCDEHLSP,   2, CNS(ed), Q4o(4a), 0      , 0       },
{"SBC",   2, g_HL      , g_BCDEHLSP,   2, CNS(ed), Q4o(42), 0      , 0       },

{"INC",   1, g_BCDEHLSP, 0         ,   1, P4o(03), 0      , 0      , 0       },
{"INC",   1, g_IX_IY   , 0         ,   2, IDX( 1), P4o(03), 0      , 0       },
{"DEC",   1, g_BCDEHLSP, 0         ,   1, P4o(0b), 0      , 0      , 0       },
{"DEC",   1, g_IX_IY   , 0         ,   2, IDX( 1), P4o(0b), 0      , 0       },

{"DAA",   0, 0         , 0         ,   1, CNS(27), 0      , 0      , 0       },
{"CPL",   0, 0         , 0         ,   1, CNS(2f), 0      , 0      , 0       },
{"NEG",   0, 0         , 0         ,   2, CNS(ed), CNS(44), 0      , 0       },
{"CCF",   0, 0         , 0         ,   1, CNS(3f), 0      , 0      , 0       },
{"SCF",   0, 0         , 0         ,   1, CNS(37), 0      , 0      , 0       },

{"NOP",   0, 0         , 0         ,   1, CNS(00), 0      , 0      , 0       },
{"HALT",  0, 0         , 0         ,   1, CNS(76), 0      , 0      , 0       },
{"DI",    0, 0         , 0         ,   1, CNS(f3), 0      , 0      , 0       },
{"EI",    0, 0         , 0         ,   1, CNS(fb), 0      , 0      , 0       },
{"IM",    1, g_IM      , 0         ,   2, CNS(ed), P3o(46), 0      , 0       },

{"RLCA",  0, 0         , 0         ,   1, CNS(07), 0      , 0      , 0       },
{"RLA",   0, 0         , 0         ,   1, CNS(17), 0      , 0      , 0       },
{"RRCA",  0, 0         , 0         ,   1, CNS(0f), 0      , 0      , 0       },
{"RRA",   0, 0         , 0         ,   1, CNS(1f), 0      , 0      , 0       },

{"RLC",   1, g_A___xHL , 0         ,   2, CNS(cb), ooP(00), 0      , 0       },
{"RLC",   1, g_xIXdxIYd, 0         ,   4, IDX( 1), CNS(cb), d_8( 1), ooP(00) },
{"RRC",   1, g_A___xHL , 0         ,   2, CNS(cb), ooP(08), 0      , 0       },
{"RRC",   1, g_xIXdxIYd, 0         ,   4, IDX( 1), CNS(cb), d_8( 1), ooP(08) },
{"RL",    1, g_A___xHL , 0         ,   2, CNS(cb), ooP(10), 0      , 0       },
{"RL",    1, g_xIXdxIYd, 0         ,   4, IDX( 1), CNS(cb), d_8( 1), ooP(10) },
{"RR",    1, g_A___xHL , 0         ,   2, CNS(cb), ooP(18), 0      , 0       },
{"RR",    1, g_xIXdxIYd, 0         ,   4, IDX( 1), CNS(cb), d_8( 1), ooP(18) },
{"SLA",   1, g_A___xHL , 0         ,   2, CNS(cb), ooP(20), 0      , 0       },
{"SLA",   1, g_xIXdxIYd, 0         ,   4, IDX( 1), CNS(cb), d_8( 1), ooP(20) },
{"SRA",   1, g_A___xHL , 0         ,   2, CNS(cb), ooP(28), 0      , 0       },
{"SRA",   1, g_xIXdxIYd, 0         ,   4, IDX( 1), CNS(cb), d_8( 1), ooP(28) },
{"SLL",   1, g_A___xHL , 0         ,   2, CNS(cb), ooP(30), 0      , 0       },
{"SLL",   1, g_xIXdxIYd, 0         ,   4, IDX( 1), CNS(cb), d_8( 1), ooP(30) },
{"SRL",   1, g_A___xHL , 0         ,   2, CNS(cb), ooP(38), 0      , 0       },
{"SRL",   1, g_xIXdxIYd, 0         ,   4, IDX( 1), CNS(cb), d_8( 1), ooP(38) },

{"RLD",   0, 0         , 0         ,   2, CNS(ed), CNS(6f), 0      , 0       },
{"RRD",   0, 0         , 0         ,   2, CNS(ed), CNS(67), 0      , 0       },

{"RETI",  0, 0         , 0         ,   2, CNS(ed), P3o(4d), 0      , 0       },
{"RETN",  0, 0         , 0         ,   2, CNS(ed), P3o(45), 0      , 0       },
{"RST",   1, g_RST     , 0         ,   1, P3o(c7), 0      , 0      , 0       },

{"IN",    2, g_A       , g_xnn     ,   2, CNS(db), n_8( 2), 0      , 0       },
{"IN",    2, g_ABCDEFHL, g_xC      ,   2, CNS(ed), P3o(40), 0      , 0       },
{"INI",   0, 0         , 0         ,   2, CNS(ed), CNS(a2), 0      , 0       },
{"INIR",  0, 0         , 0         ,   2, CNS(ed), CNS(b2), 0      , 0       },
{"IND",   0, 0         , 0         ,   2, CNS(ed), CNS(aa), 0      , 0       },
{"INDR",  0, 0         , 0         ,   2, CNS(ed), CNS(ba), 0      , 0       },

{"OUT",   2, g_xnn     , g_A       ,   2, CNS(d3), n_8( 1), 0      , 0       },
{"OUT",   2, g_xC      , g_ABCDEFHL,   2, CNS(ed), Q3o(41), 0      , 0       },
{"OUTI",  0, 0         , 0         ,   2, CNS(ed), CNS(a3), 0      , 0       },
{"OTIR",  0, 0         , 0         ,   2, CNS(ed), CNS(b3), 0      , 0       },
{"OUTD",  0, 0         , 0         ,   2, CNS(ed), CNS(ab), 0      , 0       },
{"OTDR",  0, 0         , 0         ,   2, CNS(ed), CNS(bb), 0      , 0       },

{"BIT",   2, g_BIT     , g_A___xHL ,   2, CNS(cb), P3Q(40), 0      , 0       },
{"BIT",   2, g_BIT     , g_xIXdxIYd,   4, IDX( 2), CNS(cb), d_8( 2),P3Q(40)  },
{"RES",   2, g_BIT     , g_A___xHL ,   2, CNS(cb), P3Q(80), 0      , 0       },
{"RES",   2, g_BIT     , g_xIXdxIYd,   4, IDX( 2), CNS(cb), d_8( 2),P3Q(80)  },
{"SET",   2, g_BIT     , g_A___xHL ,   2, CNS(cb), P3Q(c0), 0      , 0       },
{"SET",   2, g_BIT     , g_xIXdxIYd,   4, IDX( 2), CNS(cb), d_8( 2),P3Q(c0)  },

{"JP",    1, g_nn      , 0         ,   3, CNS(c3), n_L( 1), n_H( 1), 0       },
{"JP",    2, g_JP      , g_nn      ,   3, P3o(c2), n_L( 2), n_H( 2), 0       },
{"CALL",  1, g_nn      , 0         ,   3, CNS(cd), n_L( 1), n_H( 1), 0       },
{"CALL",  2, g_JP      , g_nn      ,   3, P3o(c4), n_L( 2), n_H( 2), 0       },
{"RET",   0, 0         , 0         ,   1, CNS(c9), 0      , 0      , 0       },
{"RET",   1, g_JP      , 0         ,   1, P3o(c0), 0      , 0      , 0       },

{"JR",    1, g_nn      , 0         ,   2, CNS(18), e_8( 1), 0      , 0       },
{"JR",    2, g_JR      , g_nn      ,   2, P3o(20), e_8( 2), 0      , 0       },
{"DJNZ",  1, g_nn      , 0         ,   2, CNS(10), e_8( 1), 0      , 0       },
{"JP",    1, g_xHL     , 0         ,   1, CNS(e9), 0      , 0      , 0       },
{"JP",    1, g_xIX_xIY , 0         ,   2, IDX( 1), CNS(e9), 0      , 0       },
};


int	is_g_NULL( int x )		/*	オペランドなし		*/
{
  fprintf(stderr, "\n! Internal Err A !\n\n");
  return 0;
}
int	is_g_A( int x )			/*	A			*/
{
  if     ( op[x]==OPRAND_A  ) return( code[x] = 0x0080 | 7 );
  else                        return( code[x] = 0          );
}
int	is_g_A___xHL( int x )		/*	A B C D E H L (HL)	*/
{
  if     ( op[x]==OPRAND_B   ) return( code[x] = 0x0080 | 0 );
  else if( op[x]==OPRAND_C   ) return( code[x] = 0x0080 | 1 );
  else if( op[x]==OPRAND_D   ) return( code[x] = 0x0080 | 2 );
  else if( op[x]==OPRAND_E   ) return( code[x] = 0x0080 | 3 );
  else if( op[x]==OPRAND_H   ) return( code[x] = 0x0080 | 4 );
  else if( op[x]==OPRAND_L   ) return( code[x] = 0x0080 | 5 );
  else if( op[x]==OPRAND_xHL ) return( code[x] = 0x0080 | 6 );
  else if( op[x]==OPRAND_A   ) return( code[x] = 0x0080 | 7 );
  else                         return( code[x] = 0          );
}
int	is_g_ABCDEHL( int x )		/*	A B C D E H L		*/
{
  if     ( op[x]==OPRAND_B   ) return( code[x] = 0x0080 | 0 );
  else if( op[x]==OPRAND_C   ) return( code[x] = 0x0080 | 1 );
  else if( op[x]==OPRAND_D   ) return( code[x] = 0x0080 | 2 );
  else if( op[x]==OPRAND_E   ) return( code[x] = 0x0080 | 3 );
  else if( op[x]==OPRAND_H   ) return( code[x] = 0x0080 | 4 );
  else if( op[x]==OPRAND_L   ) return( code[x] = 0x0080 | 5 );
  else if( op[x]==OPRAND_A   ) return( code[x] = 0x0080 | 7 );
  else                         return( code[x] = 0          );
}
int	is_g_ABCDE( int x )		/*	A B C D E		*/
{
  if     ( op[x]==OPRAND_B   ) return( code[x] = 0x0080 | 0 );
  else if( op[x]==OPRAND_C   ) return( code[x] = 0x0080 | 1 );
  else if( op[x]==OPRAND_D   ) return( code[x] = 0x0080 | 2 );
  else if( op[x]==OPRAND_E   ) return( code[x] = 0x0080 | 3 );
  else if( op[x]==OPRAND_A   ) return( code[x] = 0x0080 | 7 );
  else                         return( code[x] = 0          );
}
int	is_g_ABCDEFHL( int x )		/*	A B C D E H L  F	*/
{
  if     ( op[x]==OPRAND_B   ) return( code[x] = 0x0080 | 0 );
  else if( op[x]==OPRAND_C   ) return( code[x] = 0x0080 | 1 );
  else if( op[x]==OPRAND_D   ) return( code[x] = 0x0080 | 2 );
  else if( op[x]==OPRAND_E   ) return( code[x] = 0x0080 | 3 );
  else if( op[x]==OPRAND_H   ) return( code[x] = 0x0080 | 4 );
  else if( op[x]==OPRAND_L   ) return( code[x] = 0x0080 | 5 );
  else if( op[x]==OPRAND_F   ) return( code[x] = 0x0080 | 6 );
  else if( op[x]==OPRAND_A   ) return( code[x] = 0x0080 | 7 );
  else                         return( code[x] = 0          );
}
int	is_g_nn( int x )		/*	定数			*/
{
  if     ( op[x]==OPRAND_n   ) return( code[x] = 0x0080 );
  else if( op[x]==OPRAND_str ) return( code[x] = 0x0080 );
  else                         return( code[x] = 0      );
}
int	is_g_xnn( int x )		/*	絶対アドレス		*/
{
  if     ( op[x]==OPRAND_xn  ) return( code[x] = 0x0080 | 3 );
  else                         return( code[x] = 0          );
}
int	is_g_xIXdxIYd( int x )		/*	(IX+d) (IY+d)		*/
{
  if     ( op[x]==OPRAND_xIXd) return( code[x] = 0xdd00 | 6 );
  else if( op[x]==OPRAND_xIYd) return( code[x] = 0xfd00 | 6 );
  else                         return( code[x] = 0          );
}
int	is_g_xHL( int x )		/*	(HL)			*/
{
  if     ( op[x]==OPRAND_xHL ) return( code[x] = 0x0080 | 6 );
  else                         return( code[x] = 0          );
}
int	is_g_xBC_xDE( int x )		/*	(BC) (DE)		*/
{
  if     ( op[x]==OPRAND_xBC ) return( code[x] = 0x0080 | 0 );
  else if( op[x]==OPRAND_xDE ) return( code[x] = 0x0080 | 1 );
  else                         return( code[x] = 0          );
}
int	is_g_xIX_xIY( int x )		/*	(IX) (IY)		*/
{
  if     ( op[x]==OPRAND_xIX ) return( code[x] = 0xdd00 );
  else if( op[x]==OPRAND_xIY ) return( code[x] = 0xfd00 );
  else                         return( code[x] = 0      );
}
int	is_g_I_R( int x )		/*	I R			*/
{
  if     ( op[x]==OPRAND_I   ) return( code[x] = 0x0080 | 0 );
  else if( op[x]==OPRAND_R   ) return( code[x] = 0x0080 | 1 );
  else                         return( code[x] = 0          );
}

int	is_g_IXY_H_L( int x )		/*	IXH IXL IYH IYL		*/
{
  if     ( op[x]==OPRAND_IXH ) return( code[x] = 0xdd00 | 4 );
  else if( op[x]==OPRAND_IXL ) return( code[x] = 0xdd00 | 5 );
  else if( op[x]==OPRAND_IYH ) return( code[x] = 0xfd00 | 4 );
  else if( op[x]==OPRAND_IYL ) return( code[x] = 0xfd00 | 5 );
  else                         return( code[x] = 0          );
}
int	is_g_IXH_IXL( int x )		/*	IXH IXL			*/
{
  if     ( op[x]==OPRAND_IXH ) return( code[x] = 0xdd00 | 4 );
  else if( op[x]==OPRAND_IXL ) return( code[x] = 0xdd00 | 5 );
  else                         return( code[x] = 0          );
}
int	is_g_IYH_IYL( int x )		/*		IYH IYL		*/
{
  if     ( op[x]==OPRAND_IXH ) return( code[x] = 0xfd00 | 4 );
  else if( op[x]==OPRAND_IXL ) return( code[x] = 0xfd00 | 5 );
  else                         return( code[x] = 0          );
}
int	is_g_AF( int x )		/*	AF			*/
{
  if     ( op[x]==OPRAND_AF  ) return( code[x] = 0x0080 | 3 );
  else                         return( code[x] = 0          );
}
int	is_g_AFF( int x )		/*	AF'			*/
{
  if     ( op[x]==OPRAND_AFF ) return( code[x] = 0x0080 | 3 );
  else                         return( code[x] = 0          );
}
int	is_g_DE( int x )		/*	DE			*/
{
  if     ( op[x]==OPRAND_DE  ) return( code[x] = 0x0080 | 1 );
  else                         return( code[x] = 0          );
}
int	is_g_HL( int x )		/*	HL			*/
{
  if     ( op[x]==OPRAND_HL  ) return( code[x] = 0x0080 | 2 );
  else                         return( code[x] = 0          );
}
int	is_g_IX( int x )		/*	IX			*/
{
  if     ( op[x]==OPRAND_IX  ) return( code[x] = 0xdd00 | 2 );
  else                         return( code[x] = 0          );
}
int	is_g_IY( int x )		/*	IY			*/
{
  if     ( op[x]==OPRAND_IY  ) return( code[x] = 0xfd00 | 2 );
  else                         return( code[x] = 0          );
}
int	is_g_IX_IY( int x )		/*	IX IY			*/
{
  if     ( op[x]==OPRAND_IX  ) return( code[x] = 0xdd00 | 2 );
  else if( op[x]==OPRAND_IY  ) return( code[x] = 0xfd00 | 2 );
  else                         return( code[x] = 0          );
}
int	is_g_SP( int x )		/*	SP			*/
{
  if     ( op[x]==OPRAND_SP  ) return( code[x] = 0x0080 | 3 );
  else                         return( code[x] = 0          );
}
int	is_g_xSP( int x )		/*	(SP)			*/
{
  if     ( op[x]==OPRAND_xSP ) return( code[x] = 0x0080 );
  else                         return( code[x] = 0      );
}
int	is_g_BCDEHLAF( int x )		/*	BC DE HL AF		*/
{
  if     ( op[x]==OPRAND_BC  ) return( code[x] = 0x0080 | 0 );
  else if( op[x]==OPRAND_DE  ) return( code[x] = 0x0080 | 1 );
  else if( op[x]==OPRAND_HL  ) return( code[x] = 0x0080 | 2 );
  else if( op[x]==OPRAND_AF  ) return( code[x] = 0x0080 | 3 );
  else                         return( code[x] = 0          );
}
int	is_g_BCDEHLSP( int x )		/*	BC DE HL SP		*/
{
  if     ( op[x]==OPRAND_BC  ) return( code[x] = 0x0080 | 0 );
  else if( op[x]==OPRAND_DE  ) return( code[x] = 0x0080 | 1 );
  else if( op[x]==OPRAND_HL  ) return( code[x] = 0x0080 | 2 );
  else if( op[x]==OPRAND_SP  ) return( code[x] = 0x0080 | 3 );
  else                         return( code[x] = 0          );
}
int	is_g_BCDEIXSP( int x )		/*	BC DE IX SP		*/
{
  if     ( op[x]==OPRAND_BC  ) return( code[x] = 0x0080 | 0 );
  else if( op[x]==OPRAND_DE  ) return( code[x] = 0x0080 | 1 );
  else if( op[x]==OPRAND_IX  ) return( code[x] = 0xdd00 | 2 );
  else if( op[x]==OPRAND_SP  ) return( code[x] = 0x0080 | 3 );
  else                         return( code[x] = 0          );
}
int	is_g_BCDEIYSP( int x )		/*	BC DE IY SP		*/
{
  if     ( op[x]==OPRAND_BC  ) return( code[x] = 0x0080 | 0 );
  else if( op[x]==OPRAND_DE  ) return( code[x] = 0x0080 | 1 );
  else if( op[x]==OPRAND_IY  ) return( code[x] = 0xfd00 | 2 );
  else if( op[x]==OPRAND_SP  ) return( code[x] = 0x0080 | 3 );
  else                         return( code[x] = 0          );
}
int	is_g_xC( int x )		/*	(C)			*/
{
  if     ( op[x]==OPRAND_xC  ) return( code[x] = 0x0080 );
  else                         return( code[x] = 0      );
}
int	is_g_IM( int x )		/*	IM  命令 の オペランド	*/
{
  if( op[x]==OPRAND_n || op[x]==OPRAND_str ){
    if( flag[x] ){
      switch( val[x] ){
      case 0:			 return( code[x] = 0x0080 | 0 );
      case 1:			 return( code[x] = 0x0080 | 2 );
      case 2:			 return( code[x] = 0x0080 | 3 );
      default:			 return( code[x] = 0          );
      }
    }else			 return( code[x] = 0x0080 );
  }
  else                           return( code[x] = 0          );
}
int	is_g_RST( int x )		/*	RST 命令 の オペランド	*/
{
  if( op[x]==OPRAND_n || op[x]==OPRAND_str ){
    if( flag[x] ){
      switch( val[x] ){
      case 0x00:		 return( code[x] = 0x0080 | 0 );
      case 0x08:		 return( code[x] = 0x0080 | 1 );
      case 0x10:		 return( code[x] = 0x0080 | 2 );
      case 0x18:		 return( code[x] = 0x0080 | 3 );
      case 0x20:		 return( code[x] = 0x0080 | 4 );
      case 0x28:		 return( code[x] = 0x0080 | 5 );
      case 0x30:		 return( code[x] = 0x0080 | 6 );
      case 0x38:		 return( code[x] = 0x0080 | 7 );
      default:			 return( code[x] = 0          );
      }
    }else			 return( code[x] = 0x0080 );
  }
  else                           return( code[x] = 0          );
}
int	is_g_JP( int x )		/*	NZ Z NC C PO PE P M	*/
{
  if     ( op[x]==OPRAND_NZ  ) return( code[x] = 0x0080 | 0 );
  else if( op[x]==OPRAND_Z   ) return( code[x] = 0x0080 | 1 );
  else if( op[x]==OPRAND_NC  ) return( code[x] = 0x0080 | 2 );
  else if( op[x]==OPRAND_C   ) return( code[x] = 0x0080 | 3 );
  else if( op[x]==OPRAND_PO  ) return( code[x] = 0x0080 | 4 );
  else if( op[x]==OPRAND_PE  ) return( code[x] = 0x0080 | 5 );
  else if( op[x]==OPRAND_P   ) return( code[x] = 0x0080 | 6 );
  else if( op[x]==OPRAND_M   ) return( code[x] = 0x0080 | 7 );
  else                         return( code[x] = 0          );
}
int	is_g_JR( int x )		/*	NZ Z NC C		*/
{
  if     ( op[x]==OPRAND_NZ  ) return( code[x] = 0x0080 | 0 );
  else if( op[x]==OPRAND_Z   ) return( code[x] = 0x0080 | 1 );
  else if( op[x]==OPRAND_NC  ) return( code[x] = 0x0080 | 2 );
  else if( op[x]==OPRAND_C   ) return( code[x] = 0x0080 | 3 );
  else                         return( code[x] = 0          );
}
int	is_g_BIT( int x )		/*	0 1 2 3 4 5 6 7		*/
{
  if( op[x]==OPRAND_n || op[x]==OPRAND_str ){
    if( flag[x] ){
      switch( val[x] ){
      case 0:			 return( code[x] = 0x0080 | 0 );
      case 1:			 return( code[x] = 0x0080 | 1 );
      case 2:			 return( code[x] = 0x0080 | 2 );
      case 3:			 return( code[x] = 0x0080 | 3 );
      case 4:			 return( code[x] = 0x0080 | 4 );
      case 5:			 return( code[x] = 0x0080 | 5 );
      case 6:			 return( code[x] = 0x0080 | 6 );
      case 7:			 return( code[x] = 0x0080 | 7 );
      default:			 return( code[x] = 0          );
      }
    }else			 return( code[x] = 0x0080 );
  }
  else                           return( code[x] = 0          );
}


struct{
  int	checknum;
  int	(*job_func)(int x);
} is_oprand[]=
{
  g_NULL,	is_g_NULL,
  g_A,		is_g_A,          
  g_A___xHL,	is_g_A___xHL,    
  g_ABCDEHL,	is_g_ABCDEHL,    
  g_ABCDE,	is_g_ABCDE,      
  g_ABCDEFHL,	is_g_ABCDEFHL,   
  g_nn,		is_g_nn,         
  g_xnn,	is_g_xnn,        
  g_xIXdxIYd,	is_g_xIXdxIYd,   
  g_xHL,	is_g_xHL,        
  g_xBC_xDE,	is_g_xBC_xDE,    
  g_xIX_xIY,	is_g_xIX_xIY,    
  g_I_R,	is_g_I_R,        
  g_IXY_H_L,	is_g_IXY_H_L,    
  g_IXH_IXL,	is_g_IXH_IXL,    
  g_IYH_IYL,	is_g_IYH_IYL,    
  g_AF,		is_g_AF,         
  g_AFF,	is_g_AFF,        
  g_DE,		is_g_DE,         
  g_HL,		is_g_HL,         
  g_IX,		is_g_IX,         
  g_IY,		is_g_IY,         
  g_IX_IY,	is_g_IX_IY,      
  g_SP,		is_g_SP,         
  g_xSP,	is_g_xSP,        
  g_BCDEHLAF,	is_g_BCDEHLAF,   
  g_BCDEHLSP,	is_g_BCDEHLSP,   
  g_BCDEIXSP,	is_g_BCDEIXSP,   
  g_BCDEIYSP,	is_g_BCDEIYSP,   
  g_xC,		is_g_xC,         
  g_IM,		is_g_IM,         
  g_RST,	is_g_RST,        
  g_JP,		is_g_JP,         
  g_JR,		is_g_JR,         
  g_BIT,	is_g_BIT,        
};





enum{
  UNDEF_LABEL = 40,
  OPRAND_OVER,
  SYNTAX,
  DISPLACEMENT,
  NEED_LABEL,
  ORG_OVER,
  UNKNOWN_ERROR,
};

int	command_ORG( void )
{
  int	i;

  if( label.exist ){					/* ラベルは禁止 */
    assemble_err = SYNTAX;
    return 2;
  }

  if( (assemble_err=get_oprand()) ) return 2;		/* オペランドは1個 */
  if( oprand.exist ){
    assemble_err = OPRAND_OVER;
    return 2;
  }

  if( oprand.type!=OPRAND_n ){				/* オペランドは数値 */
    assemble_err = SYNTAX;
    return 2;
  }
  if( oprand.value_define==FALSE ){
    assemble_err = NEED_LABEL;
    return 2;
  }

  for( i=1; i<MAX_ORG_NUM+1; i++ ){			/* 最大個数制限Check */
    if( org_table[i].start==-1 ) break;
  }
  if( i==MAX_ORG_NUM+1 ){
    assemble_err = ORG_OVER;
    return 2;
  }else{
    org_table[i-1].end = program_counter;
    org_table[i].start = oprand.value & 0xffff;
    program_counter    = oprand.value & 0xffff;
  }


  assemble_err = NORMAL_END;
  return 0;
}



int	command_END( void )
{
  if( label.exist ){					/* ラベルは禁止 */
    assemble_err = SYNTAX;
    return 2;
  }

  if( get_oprand()!=NO_OPRAND ){			/* オペランドはなし */
    assemble_err = OPRAND_OVER;
    return 2;
  }

  assemble_end = TRUE;

  assemble_err = NORMAL_END;
  return 0;
}



int	command_EQU( void )
{
  if( label.exist==FALSE ){				/* ラベルが必要 */
    assemble_err = SYNTAX;
    return 2;
  }

  if( (assemble_err=get_oprand()) ) return 2;		/* オペランドは1個 */
  if( oprand.exist ){
    assemble_err = OPRAND_OVER;
    return 2;
  }

  if( oprand.type!=OPRAND_n ){				/* オペランドは数値 */
    assemble_err = SYNTAX;
    return 2;
  }
  if( oprand.value_define==FALSE ){
    assemble_err = NEED_LABEL;
    return 2;
  }
							/* ラベル登録 */
  if( (assemble_err = regist_label( oprand.value )) ) return 2;

  assemble_err = NORMAL_END;
  return 0;
}



int	command_DB( void )
{
  int	undefine_flag = 0;
  char *c;

  if( label.exist ){					/* ラベルあれば登録 */
    if( (assemble_err = regist_label( program_counter )) ) return 2;
  }

  if( (assemble_err=get_oprand()) ) return 2;		/* オペランド1個以上 */

  while(1){						/* オペランド個数分  */

    if( oprand.type==OPRAND_n ){
      program[ program_counter++ ] = oprand.value & 0xff;
      if( !oprand.value_define ) undefine_flag = 1;
    }else if( oprand.type==OPRAND_str ){
      c = &oprand.str[0];
      while( *c ){
	program[ program_counter++ ] = *c++;
      }
    }else{
      assemble_err = SYNTAX;
      return 2;
    }

    if( oprand.exist ){
      if( (assemble_err=get_oprand()) ) return 2;
    }else{
      break;
    }
  }

  if( undefine_flag ){
    assemble_err = UNDEF_LABEL;
    return 1;
  }

  assemble_err = NORMAL_END;
  return 0;
}


int	command_DW( void )
{
  int	undefine_flag = 0;

  if( label.exist ){
    if( (assemble_err = regist_label( program_counter )) ) return 2;
  }

  if( (assemble_err=get_oprand()) ) return 2;

  while(1){

    if( oprand.type==OPRAND_n ){
      program[ program_counter++ ] = oprand.value & 0xff;
      program[ program_counter++ ] = oprand.value >> 8 ;
      if( !oprand.value_define ) undefine_flag = 1;
    }else{
      assemble_err = SYNTAX;
      return 2;
    }

    if( oprand.exist ){
      if( (assemble_err=get_oprand()) ) return 2;
    }else{
      break;
    }
  }

  if( undefine_flag ){
    assemble_err = UNDEF_LABEL;
    return 1;
  }

  assemble_err = NORMAL_END;
  return 0;
}


int	command_DS( void )
{
  int	i;

  if( label.exist ){
    if( (assemble_err = regist_label( program_counter )) ) return 2;
  }

  if( (assemble_err=get_oprand()) ) return 2;
  if( get_oprand()!=NO_OPRAND ){
    assemble_err = OPRAND_OVER;
    return 2;
  }

  if( oprand.type!=OPRAND_n ){
    assemble_err = SYNTAX;
    return 2;
  }
  if( oprand.value_define==FALSE ){
    assemble_err = NEED_LABEL;
    return 2;
  }

  for( i=0; i<oprand.value; i++ ){
    program[ program_counter++ ] = 0;
  }

  assemble_err = NORMAL_END;
  return 0;
}





struct {
  char	*mnemonic;
  int  (*job_func)(void);
} command_list[] =
{
  { "ORG",  command_ORG,  },
  { "END",  command_END,  },
  { "EQU",  command_EQU,  },
  { "DB",   command_DB,   },
  { "DW",   command_DW,   },
  { "DS",   command_DS,   },
};


int	assemble_line( void )
{
  int	i, j;
  int	oprand_num;
  unsigned char c, x;

  if( (assemble_err = get_label_opcode()) ) return 2;

  if( opcode.exist ){				/* オペコードありの場合 */

		/* 疑似命令 */

    for( i=0; i<(sizeof(command_list)/sizeof(command_list[0])); i++ ){
      if( strcmp( opcode.str, command_list[i].mnemonic )==0 ) break;
    }

    if( i<(sizeof(command_list)/sizeof(command_list[0])) ){

			/* 命令別に処理 */

      return (command_list[i].job_func());

    }

		/* 通常命令 */

			
    if( label.exist ){				/* ラベルがあれば登録 */

      if( (assemble_err = regist_label( program_counter )) ) return 2;

    }


    oprand_num = 0;				/* オペランドを Get 0〜2個 */
    while( oprand.exist ){
      if( (assemble_err=get_oprand()) ) return 2;
      op[ oprand_num ]   = oprand.type;
      flag[ oprand_num ] = oprand.value_define;
      val[ oprand_num ]  = oprand.value;
      oprand_num++;
      if( oprand_num >=2 ) break;
    }
    if( oprand_num==2 && oprand.exist ){
      assemble_err = OPRAND_OVER;
      return 2;
    }



    for( i=0; i<(sizeof(instruction)/sizeof(instruction[0])); i++ ){

      int f[2];

      if( strcmp( opcode.str, instruction[i].mnemonic ) ) continue;

      if( oprand_num != instruction[i].oprand_num ) continue;

      switch( oprand_num ){
      case 0:
	f[0] = f[1] = 1;	break;
      case 1:
	f[0] = (is_oprand[ instruction[i].oprand[0] ].job_func)(0);
	f[1] = 1;
	break;
      case 2:
	f[0] = (is_oprand[ instruction[i].oprand[0] ].job_func)(0);
	f[1] = (is_oprand[ instruction[i].oprand[1] ].job_func)(1);
	break;
      default:
	fprintf(stderr, "\n! Internal Err B !\n\n");
	f[0] = f[1] = 0;	break;
      }

      if( f[0]==0 || f[1]==0 ) continue;


      for( j=0; j<instruction[i].code_num; j++ ){

	x = instruction[i].code[j] & 0xff;
	switch( instruction[i].code[j]>>8 &0xff ){

	case CONST:			/*	コード	    [コード]	*/
	  c = x;
	  break;

	case INDEX:			/*	インデックス[op1 or op2]*/
	  c = code[ x-1 ] >> 8;
	  break;

	case ZZP3Q:			/*	[定数] + (op1<<3 | op2)	*/
	  c = x | ((code[0]&0x7)<<3) | (code[1]&0x7);
	  break;

	case ZZP3_:			/*	[定数] + (op1<<3      )	*/
	  c = x | ((code[0]&0x7)<<3);
	  break;

	case ZZQ3_:			/*	[定数] + (op2<<3      )	*/
	  c = x | ((code[1]&0x7)<<3);
	  break;

	case ZZ__P:			/*	[定数] + (         op1)	*/
	  c = x | (code[0]&0x7);
	  break;

	case ZZ__Q:			/*	[定数] + (         op2)	*/
	  c = x | (code[1]&0x7);
	  break;

	case ZZP4_:			/*	[定数] + (op1<<4      )	*/
	  c = x | ((code[0]&0x3)<<4);
	  break;

	case ZZQ4_:			/*	[定数] + (op1<<4      )	*/
	  c = x | ((code[1]&0x3)<<4);
	  break;

	case NUM_8:			/*	8bit数値     [op1 or op2]*/
	  if( flag[ x-1 ]==0 ){
	    assemble_err = UNDEF_LABEL;
	  }
	  c = val[ x-1 ] & 0xff;
	  break;

	case NUM_H:			/*	16bit数値上位[op1 or op2]*/
	  if( flag[ x-1 ]==0 ){
	    assemble_err = UNDEF_LABEL;
	  }
	  c = val[ x-1 ] >> 8;
	  break;

	case NUM_L:			/*	16bit数値下位[op1 or op2]*/
	  if( flag[ x-1 ]==0 ){
	    assemble_err = UNDEF_LABEL;
	  }
	  c = val[ x-1 ] & 0xff;
	  break;

	case OFSET:			/*	IX IYの変位値[op1 or op2]*/
	  if( flag[ x-1 ] ){
	    if( val[ x-1 ] < -128 || 127 < val[ x-1 ]  ){
	      assemble_err = DISPLACEMENT;
	      return 2;
	    }
	  }else{
	    assemble_err = UNDEF_LABEL;
	  }
	  c = val[ x-1 ] & 0xff;
	  break;

	case ADDRS:			/*	JR   の変位置[op1 or op2]*/
	  if( flag[ x-1 ] ){
	    if( (short)(val[ x-1 ])-((short)program_counter+2) < -128 ||
	        127 < (short)(val[ x-1 ])-((short)program_counter+2)  ){
	      assemble_err = DISPLACEMENT;
	      return 2;
	    }
	  }else{
	    assemble_err = UNDEF_LABEL;
	  }
	  c = ((long)val[ x-1 ]-((long)program_counter+2)) & 0xff;
	  break;

	default:
	  fprintf(stderr, "\n! Internal Err C !\n\n");
	  assemble_err = UNKNOWN_ERROR;
	  return 2;
	}

	program[ program_counter + j ] = c;

      }

      program_counter += j;

      if( assemble_err==NORMAL_END ) return 0;
      if( assemble_err==UNDEF_LABEL ) return 1;

      fprintf(stderr, "\n! Internal Err D !\n\n");
    }

		/* そんな命令ない */

    assemble_err = SYNTAX;
    return 2;

  }else{					/* オペコード無しの場合 */

    if( label.exist ){
      if( (assemble_err = regist_label( program_counter )) ) return 2;
    }
    return 0;

  }

  fprintf(stderr, "\n! Internal Err E !\n\n");
  return 2;
}



struct{
  int	err_no;
  char	*err_mes;
} error_table[] =
{
  NORMAL_END,		"NORMAL END",

  NULL_LABEL,		"Detect NULL label",

  LABEL_BAD,		"LABEL Detect bad character",
  LABEL_DUPLICATE,	"LABEL Duplicate labels",
  LABEL_OVER,		"LABEL Over! Number of label",
  LABEL_SHORTAGE,	"LABEL Over! Label memory buffer",

  NO_OPRAND,		"Can't detect more OPRAND",
  BAD_STRING,		"String format is bad",
  BAD_ADDRESS,		"Address format is bad",
  BAD_ADDR_REG,		"This REG can't use for addressing",
  BAD_INDEX_REG,	"This REG can't use for index-addressing",
  BAD_REG,		"REG can't caliculate",
  BAD_CALC,		"Calcuration expression error",
  BAD_CALC_LABEL,	"Calcuration string error",

  UNDEF_LABEL,		"Label undefined",
  OPRAND_OVER,		"OPRAND is too many",
  SYNTAX,		"Syntax error",
  DISPLACEMENT,		"Displacement error",
  NEED_LABEL,		"This OPCODE need defined label",
  ORG_OVER,		"ORG Over! Number of ORG",
  UNKNOWN_ERROR,	"!! UNKNOWN ERROR !!?",
};


void	printf_error_mes( int lines, int num )
{
  int	i;

  fprintf(stderr,"Error in %d ",lines );

  for( i=0; i<(sizeof(error_table)/sizeof(error_table[0])); i++ ){
    if( error_table[i].err_no == num ) break;
  }
  if( i<(sizeof(error_table)/sizeof(error_table[0])) ){

    fprintf(stderr," [ %s(%d) ]\n",error_table[i].err_mes,num);

  }else{

    fprintf(stderr," [ ?????(%d) ]\n",num);
    fprintf(stderr, "\n! Internal Err C !\n\n");

  }

  return;
}




void	help( void )
{
  fprintf( stderr, "Tiny Z80 Assembler Ver 0.2 by fukuchan\n" );
  fprintf( stderr, "Usage: zasm88 [-org n] [-nv] [-l] source_file output_file\n" );
  fprintf( stderr, "    -org n          - Set assemble start address n\n");
  fprintf( stderr, "    -nv             - Not verbose output\n" );
  fprintf( stderr, "    -l              - Label list output (stdout)\n");
  fprintf( stderr, "    source_file     - Source-File\n");
  fprintf( stderr, "    output_file     - Output-File\n");
  exit(1);
}


int	main( int argc, char *argv[] )
{
  FILE	*fp_in, *fp_out;
  int	i, j, err_f = 0;
  char	*source_name = NULL;
  char  *output_name = NULL;
  long	org_address = 0;


	/* 初期化 */

  for( i=0; i<0x1000; i++ ) program[i]=0;

  label_buf_ptr = 1;
  for( i=0; i<MAX_LABEL_NUM; i++ ) label_table[i].ptr = 0;








	/* Usase 表示 */

  if( argc==1 ) help();

  for( i=1; i<argc; i++ ){
    if      ( strcmp( argv[i], "-help" )==0 ){			/* -help */
      help();

    }else if( strcmp( argv[i], "-org" )==0 ){			/* -org */
      if( ++i>=argc ){
	fprintf(stderr,"Bad argument ( -org ).\n");
	exit(1);
      }
      org_address = strtol(argv[i],(char**)NULL,0) & 0xffff;

    }else if( strcmp( argv[i], "-nv" )==0 ){			/* -nv */
      fprintf(stderr,"Sorry, -nv option is not supported now.\n");
      verbose = FALSE;

    }else if( strcmp( argv[i], "-l" )==0 ){			/* -l */
      listing = TRUE;

    }else if( *argv[i]=='-' ){					/* - */
      fprintf(stderr,"Unknown option.\n");
      exit(1);

    }else if( source_name==NULL ){				/* source_f */
      source_name = argv[i];

    }else if( output_name==NULL ){				/* output_f */
      output_name = argv[i];

    }else{							/* Another */
      fprintf(stderr,"Bad argument ( dupulicate file ).\n");
      exit(1);
    }
  }

  if( source_name==NULL ){
    fprintf( stderr, "Bad argument ( source-file not exist ).\n");
    exit(1);
  }
  if( output_name==NULL ){
    fprintf( stderr, "Bad argument ( output-file not exist ).\n");
    exit(1);
  }




	/* ソース ファイルオープン */

  if( !(fp_in=fopen(source_name,"r")) ){
    fprintf( stderr, "\n%s: Can't open file %s\n", argv[0], source_name );
    exit(1);
  }

	/* 出力 ファイルオープン */

  if( !(fp_out=fopen(output_name,"wb")) ){
    fprintf( stderr, "\n%s: Can't open file %s\n", argv[0], output_name );
    exit(1);
  }




	/* PASS 1 */

  if( verbose ) fprintf(stderr,"*** pass 1 ***\n");

  assemble_pass = 1;
  assemble_end = FALSE;

  for( i=1; i<MAX_ORG_NUM+1; i++ ) org_table[i].start = -1;
  org_table[0].start = org_address;
  program_counter    = org_address;


  for( i=1; fgets( src_line, 255, fp_in ); i++ ){

    src_line[255] = '\0';
    if( assemble_line() >= 2 ){
      printf_error_mes( i, assemble_err );
      err_f = 1;
    }
    if( assemble_end ) break;

  }

  for( i=1; i<MAX_ORG_NUM+1; i++ ) if( org_table[i].start==-1 ) break;
  org_table[i-1].end = program_counter;
  

  if( err_f ) goto STOP;


	/* PASS 2 */

  if( verbose ) fprintf(stderr,"*** pass 2 ***\n");

  assemble_pass = 2;
  assemble_end = FALSE;

  for( i=1; i<MAX_ORG_NUM+1; i++ ) org_table[i].start = -1;
  org_table[0].start = org_address;
  program_counter    = org_address;


  fseek( fp_in, 0, SEEK_SET );
  for( i=1; fgets( src_line, 255, fp_in ); i++ ){

    src_line[255] = '\0';
    if( assemble_line() >= 1 ){
      printf_error_mes( i, assemble_err );
      err_f = 1;
    }
    if( assemble_end ) break;

  }

  for( i=1; i<MAX_ORG_NUM+1; i++ ) if( org_table[i].start==-1 ) break;
  org_table[i-1].end = program_counter;



  if( err_f ) goto STOP;


	/* バイナリ出力 */

  for( i=0; i<MAX_ORG_NUM+1; i++ ){
    if( org_table[i].start==-1 ) break;
    for( j=org_table[i].start; j<org_table[i].end; j++ ){
      fputc( program[j], fp_out );
    }
    if( verbose ){
      if( org_table[i].start!=org_table[i].end ){
	fprintf(stderr,"Start %04XH  End %04XH (Size=%d)\n",
		(unsigned)(org_table[i].start)&0xffff,
		(unsigned)(org_table[i].end-1)&0xffff,
		(int)(org_table[i].end - org_table[i].start) );
      }
    }
  }

	/* 出力アドレス情報 */



 STOP:;

	/* ラベルリスト出力 */

  if( listing ){
    char *name;
    unsigned short num;

    for( i=0; i< MAX_LABEL_NUM; i++ ){
      if( label_table[i].ptr==0 ) break;

      name = &label_buf[label_table[i].ptr];
      num  = label_table[i].value;

      printf("%s",name);
      if     ( strlen(name)<8 ) printf("\t\t");
      else if( strlen(name)<16) printf("\t");

      printf("%04XH (%d)\n",num,num);
    }
    printf("%d label(s) defined\n",i);
  }


	/* 終了 */

  fclose(fp_out);

  exit(0);
}
