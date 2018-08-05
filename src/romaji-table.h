#define XR__A	RJ(P6,3,0)
#define XR__I	RJ(P2,5,0)
#define XR__U	RJ(P6,4,0)
#define XR__E	RJ(P6,5,0)
#define XR__O	RJ(P6,6,0)
#define XR_KA	RJ(P4,4,0)
#define XR_KI	RJ(P2,7,0)
#define XR_KU	RJ(P3,0,0)
#define XR_KE	RJ(P7,2,0)
#define XR_KO	RJ(P2,2,0)
#define XR_SA	RJ(P5,0,0)
#define XR_SI	RJ(P2,4,0)
#define XR_SU	RJ(P4,2,0)
#define XR_SE	RJ(P4,0,0)
#define XR_SO	RJ(P2,3,0)
#define XR_TA	RJ(P4,1,0)
#define XR_TI	RJ(P2,1,0)
#define XR_TU	RJ(P5,2,0)
#define XR_TE	RJ(P4,7,0)
#define XR_TO	RJ(P4,3,0)
#define XR_NA	RJ(P4,5,0)
#define XR_NI	RJ(P3,1,0)
#define XR_NU	RJ(P6,1,0)
#define XR_NE	RJ(P7,4,0)
#define XR_NO	RJ(P3,3,0)
#define XR_HA	RJ(P2,6,0)
#define XR_HI	RJ(P4,6,0)
#define XR_HU	RJ(P6,2,0)
#define XR_HE	RJ(P5,6,0)
#define XR_HO	RJ(P5,7,0)
#define XR_MA	RJ(P3,2,0)
#define XR_MI	RJ(P3,6,0)
#define XR_MU	RJ(P5,5,0)
#define XR_ME	RJ(P7,6,0)
#define XR_MO	RJ(P3,5,0)
#define XR_YA	RJ(P6,7,0)
#define XR_YU	RJ(P7,0,0)
#define XR_YO	RJ(P7,1,0)
#define XR_RA	RJ(P3,7,0)
#define XR_RI	RJ(P3,4,0)
#define XR_RU	RJ(P7,5,0)
#define XR_RE	RJ(P7,3,0)
#define XR_RO	RJ(P7,7,0)
#define XR_WA	RJ(P6,0,0)
#define XR_WO	RJ(P6,0,1)
#define XR__a	RJ(P6,3,1)
#define XR__i	RJ(P2,5,1)
#define XR__u	RJ(P6,4,1)
#define XR__e	RJ(P6,5,1)
#define XR__o	RJ(P6,6,1)
#define XR_ya	RJ(P6,7,1)
#define XR_yu	RJ(P7,0,1)
#define XR_yo	RJ(P7,1,1)
#define XR_tu	RJ(P5,2,1)
#define	XR__N	RJ(P5,1,0)
#define	XR_sp	RJ(P9,6,0)	/*    */
#define	XR_jj	RJ(P2,0,0)	/* ‵ */
#define	XR_pp	RJ(P5,3,0)	/* ′ */
#define	XR_oo	RJ(P7,6,1)	/* ’ */
#define	XR_rr	RJ(P5,4,0)	/* □ */
#define	XR_aa	RJ(P5,3,1)	/* ＞ */
#define	XR_ee	RJ(P5,5,1)	/* ＝ */
#define	XR_xx	RJ(P7,5,1)	/* ﹝ */
#define	XR_yy	RJ(P7,4,1)	/* ﹜ */


static const romaji_list list_NN = { "", { XR__N, } };
static const romaji_list list_tu = { "", { XR_tu, } };

static const romaji_list list_mark[] =
{
		     
/*  	*/ { " ",    { XR_sp, } },
/* ‵ 	*/ { "@",    { XR_jj, } },
/* ′ 	*/ { "[",    { XR_pp, } },
/* ’ 	*/ { "/",    { XR_oo, } },
/* □ 	*/ { "-",    { XR_rr, } },
/* ＞ 	*/ { "{",    { XR_aa, } },
/* ＝ 	*/ { "}",    { XR_ee, } },
/* ﹝ 	*/ { ".",    { XR_xx, } },
/* ﹜ 	*/ { ",",    { XR_yy, } },

};

static const romaji_list list_msime[] =
{

/* 丐   */ { "A",    { XR__A, } },

/* 壬   */ { "BA",   { XR_HA, XR_jj, } },
/* 太   */ { "BI",   { XR_HI, XR_jj, } },
/* 少   */ { "BU",   { XR_HU, XR_jj, } },
/* 屯   */ { "BE",   { XR_HE, XR_jj, } },
/* 廿   */ { "BO",   { XR_HO, XR_jj, } },

/* 井   */ { "CA",   { XR_KA, } },
/* 仄   */ { "CI",   { XR_SI, } },
/* 仁   */ { "CU",   { XR_KU, } },
/* 六   */ { "CE",   { XR_SE, } },
/* 仇   */ { "CO",   { XR_KO, } },

/* 分   */ { "DA",   { XR_TA, XR_jj, } },
/* 刈   */ { "DI",   { XR_TI, XR_jj, } },
/* 勿   */ { "DU",   { XR_TU, XR_jj, } },
/* 匹   */ { "DE",   { XR_TE, XR_jj, } },
/* 升   */ { "DO",   { XR_TO, XR_jj, } },

/* 尹   */ { "E",    { XR__E, } },

/* 孔丑 */ { "FA",   { XR_HU, XR__a, } },
/* 孔不 */ { "FI",   { XR_HU, XR__i, } },
/* 孔   */ { "FU",   { XR_HU,        } },
/* 孔之 */ { "FE",   { XR_HU, XR__e, } },
/* 孔予 */ { "FO",   { XR_HU, XR__o, } },

/* 互   */ { "GA",   { XR_KA, XR_jj, } },
/* 亢   */ { "GI",   { XR_KI, XR_jj, } },
/* 什   */ { "GU",   { XR_KU, XR_jj, } },
/* 仆   */ { "GE",   { XR_KE, XR_jj, } },
/* 仍   */ { "GO",   { XR_KO, XR_jj, } },

/* 反   */ { "HA",   { XR_HA, } },
/* 夫   */ { "HI",   { XR_HI, } },
/* 孔   */ { "HU",   { XR_HU, } },
/* 尺   */ { "HE",   { XR_HE, } },
/* 幻   */ { "HO",   { XR_HO, } },

/* 中   */ { "I",    { XR__I, } },

/* 元扎 */ { "JA",   { XR_SI, XR_jj, XR_ya, } },
/* 元   */ { "JI",   { XR_SI, XR_jj,        } },
/* 元文 */ { "JU",   { XR_SI, XR_jj, XR_yu, } },
/* 元之 */ { "JE",   { XR_SI, XR_jj, XR__e, } },
/* 元斤 */ { "JO",   { XR_SI, XR_jj, XR_yo, } },

/* 井   */ { "KA",   { XR_KA, } },
/* 五   */ { "KI",   { XR_KI, } },
/* 仁   */ { "KU",   { XR_KU, } },
/* 仃   */ { "KE",   { XR_KE, } },
/* 仇   */ { "KO",   { XR_KO, } },

/* 丑   */ { "LA",   { XR__a, } },
/* 不   */ { "LI",   { XR__i, } },
/* 丰   */ { "LU",   { XR__u, } },
/* 之   */ { "LE",   { XR__e, } },
/* 予   */ { "LO",   { XR__o, } },

/* 引   */ { "MA",   { XR_MA, } },
/* 心   */ { "MI",   { XR_MI, } },
/* 戈   */ { "MU",   { XR_MU, } },
/* 戶   */ { "ME",   { XR_ME, } },
/* 手   */ { "MO",   { XR_MO, } },

/* 卅   */ { "NA",   { XR_NA, } },
/* 卞   */ { "NI",   { XR_NI, } },
/* 厄   */ { "NU",   { XR_NU, } },
/* 友   */ { "NE",   { XR_NE, } },
/* 及   */ { "NO",   { XR_NO, } },

/* 云   */ { "O",    { XR__O, } },

/* 天   */ { "PA",   { XR_HA, XR_pp, } },
/* 夭   */ { "PI",   { XR_HI, XR_pp, } },
/* 尤   */ { "PU",   { XR_HU, XR_pp, } },
/* 巴   */ { "PE",   { XR_HE, XR_pp, } },
/* 弔   */ { "PO",   { XR_HO, XR_pp, } },

/* 仁丑 */ { "QA",   { XR_KU, XR__a, } },
/* 仁不 */ { "QI",   { XR_KU, XR__i, } },
/* 仁   */ { "QU",   { XR_KU,        } },
/* 仁之 */ { "QE",   { XR_KU, XR__e, } },
/* 仁予 */ { "QO",   { XR_KU, XR__o, } },

/* 日   */ { "RA",   { XR_RA, } },
/* 曰   */ { "RI",   { XR_RI, } },
/* 月   */ { "RU",   { XR_RU, } },
/* 木   */ { "RE",   { XR_RE, } },
/* 欠   */ { "RO",   { XR_RO, } },

/* 今   */ { "SA",   { XR_SA, } },
/* 仄   */ { "SI",   { XR_SI, } },
/* 允   */ { "SU",   { XR_SU, } },
/* 六   */ { "SE",   { XR_SE, } },
/* 公   */ { "SO",   { XR_SO, } },

/* 凶   */ { "TA",   { XR_TA, } },
/* 切   */ { "TI",   { XR_TI, } },
/* 勾   */ { "TU",   { XR_TU, } },
/* 化   */ { "TE",   { XR_TE, } },
/* 午   */ { "TO",   { XR_TO, } },

/* 丹   */ { "U",    { XR__U, } },

/* 任丑 */ { "VA",   { XR__U, XR_jj, XR__a, } },
/* 任不 */ { "VI",   { XR__U, XR_jj, XR__i, } },
/* 任   */ { "VU",   { XR__U, XR_jj,        } },
/* 任之 */ { "VE",   { XR__U, XR_jj, XR__e, } },
/* 任予 */ { "VO",   { XR__U, XR_jj, XR__o, } },

/* 歹   */ { "WA",   { XR_WA,        } },
/* 丹不 */ { "WI",   { XR__U, XR__i, } },
/* 丹   */ { "WU",   { XR__U,        } },
/* 丹之 */ { "WE",   { XR__U, XR__e, } },
/* 毛   */ { "WO",   { XR_WO,        } },

/* 丑   */ { "XA",   { XR__a, } },
/* 不   */ { "XI",   { XR__i, } },
/* 丰   */ { "XU",   { XR__u, } },
/* 之   */ { "XE",   { XR__e, } },
/* 予   */ { "XO",   { XR__o, } },

/* 支   */ { "YA",   { XR_YA,        } },
/* 中   */ { "YI",   { XR__I,        } },
/* 斗   */ { "YU",   { XR_YU,        } },
/* 中之 */ { "YE",   { XR__I, XR__e, } },
/* 方   */ { "YO",   { XR_YO,        } },

/* 介   */ { "ZA",   { XR_SA, XR_jj, } },
/* 元   */ { "ZI",   { XR_SI, XR_jj, } },
/* 內   */ { "ZU",   { XR_SU, XR_jj, } },
/* 兮   */ { "ZE",   { XR_SE, XR_jj, } },
/* 冗   */ { "ZO",   { XR_SO, XR_jj, } },

/* 太扎 */ { "BYA",  { XR_HI, XR_jj, XR_ya, } },
/* 太不 */ { "BYI",  { XR_HI, XR_jj, XR__i, } },
/* 太文 */ { "BYU",  { XR_HI, XR_jj, XR_yu, } },
/* 太之 */ { "BYE",  { XR_HI, XR_jj, XR__e, } },
/* 太斤 */ { "BYO",  { XR_HI, XR_jj, XR_yo, } },
		     
/* 切扎 */ { "CYA",  { XR_TI, XR_ya, } },
/* 切不 */ { "CYI",  { XR_TI, XR__i, } },
/* 切文 */ { "CYU",  { XR_TI, XR_yu, } },
/* 切之 */ { "CYE",  { XR_TI, XR__e, } },
/* 切斤 */ { "CYO",  { XR_TI, XR_yo, } },
		     
/* 刈扎 */ { "DYA",  { XR_TI, XR_jj, XR_ya, } },
/* 刈不 */ { "DYI",  { XR_TI, XR_jj, XR__i, } },
/* 刈文 */ { "DYU",  { XR_TI, XR_jj, XR_yu, } },
/* 刈之 */ { "DYE",  { XR_TI, XR_jj, XR__e, } },
/* 刈斤 */ { "DYO",  { XR_TI, XR_jj, XR_yo, } },
		     
/* 孔扎 */ { "FYA",  { XR_HU, XR_ya, } },
/* 孔不 */ { "FYI",  { XR_HU, XR__i, } },
/* 孔文 */ { "FYU",  { XR_HU, XR_yu, } },
/* 孔之 */ { "FYE",  { XR_HU, XR__e, } },
/* 孔斤 */ { "FYO",  { XR_HU, XR_yo, } },
		     
/* 亢扎 */ { "GYA",  { XR_KI, XR_jj, XR_ya, } },
/* 亢不 */ { "GYI",  { XR_KI, XR_jj, XR__i, } },
/* 亢文 */ { "GYU",  { XR_KI, XR_jj, XR_yu, } },
/* 亢之 */ { "GYE",  { XR_KI, XR_jj, XR__e, } },
/* 亢斤 */ { "GYO",  { XR_KI, XR_jj, XR_yo, } },
		     
/* 夫扎 */ { "HYA",  { XR_HI, XR_ya, } },
/* 夫不 */ { "HYI",  { XR_HI, XR__i, } },
/* 夫文 */ { "HYU",  { XR_HI, XR_yu, } },
/* 夫之 */ { "HYE",  { XR_HI, XR__e, } },
/* 夫斤 */ { "HYO",  { XR_HI, XR_yo, } },
		     
/* 元扎 */ { "JYA",  { XR_SI, XR_jj, XR_ya, } },
/* 元不 */ { "JYI",  { XR_SI, XR_jj, XR__i, } },
/* 元文 */ { "JYU",  { XR_SI, XR_jj, XR_yu, } },
/* 元之 */ { "JYE",  { XR_SI, XR_jj, XR__e, } },
/* 元斤 */ { "JYO",  { XR_SI, XR_jj, XR_yo, } },
		     
/* 五扎 */ { "KYA",  { XR_KI, XR_ya, } },
/* 五不 */ { "KYI",  { XR_KI, XR__i, } },
/* 五文 */ { "KYU",  { XR_KI, XR_yu, } },
/* 五之 */ { "KYE",  { XR_KI, XR__e, } },
/* 五斤 */ { "KYO",  { XR_KI, XR_yo, } },
		     
/* 扎   */ { "LYA",  { XR_ya, } },
/* 不   */ { "LYI",  { XR__i, } },
/* 文   */ { "LYU",  { XR_yu, } },
/* 之   */ { "LYE",  { XR__e, } },
/* 斤   */ { "LYO",  { XR_yo, } },
		     
/* 心扎 */ { "MYA",  { XR_MI, XR_ya, } },
/* 心不 */ { "MYI",  { XR_MI, XR__i, } },
/* 心文 */ { "MYU",  { XR_MI, XR_yu, } },
/* 心之 */ { "MYE",  { XR_MI, XR__e, } },
/* 心斤 */ { "MYO",  { XR_MI, XR_yo, } },

/* 卞扎 */ { "NYA",  { XR_NI, XR_ya, } },
/* 卞不 */ { "NYI",  { XR_NI, XR__i, } },
/* 卞文 */ { "NYU",  { XR_NI, XR_yu, } },
/* 卞之 */ { "NYE",  { XR_NI, XR__e, } },
/* 卞斤 */ { "NYO",  { XR_NI, XR_yo, } },
		     
/* 夭扎 */ { "PYA",  { XR_HI, XR_pp, XR_ya, } },
/* 夭不 */ { "PYI",  { XR_HI, XR_pp, XR__i, } },
/* 夭文 */ { "PYU",  { XR_HI, XR_pp, XR_yu, } },
/* 夭之 */ { "PYE",  { XR_HI, XR_pp, XR__e, } },
/* 夭斤 */ { "PYO",  { XR_HI, XR_pp, XR_yo, } },
		     
/* 仁扎 */ { "QYA",  { XR_KU, XR_ya, } },
/* 仁不 */ { "QYI",  { XR_KU, XR__i, } },
/* 仁文 */ { "QYU",  { XR_KU, XR_yu, } },
/* 仁之 */ { "QYE",  { XR_KU, XR__e, } },
/* 仁斤 */ { "QYO",  { XR_KU, XR_yo, } },
		     
/* 曰扎 */ { "RYA",  { XR_RI, XR_ya, } },
/* 曰不 */ { "RYI",  { XR_RI, XR__i, } },
/* 曰文 */ { "RYU",  { XR_RI, XR_yu, } },
/* 曰之 */ { "RYE",  { XR_RI, XR__e, } },
/* 曰斤 */ { "RYO",  { XR_RI, XR_yo, } },

/* 仄扎 */ { "SYA",  { XR_SI, XR_ya, } },
/* 仄不 */ { "SYI",  { XR_SI, XR__i, } },
/* 仄文 */ { "SYU",  { XR_SI, XR_yu, } },
/* 仄之 */ { "SYE",  { XR_SI, XR__e, } },
/* 仄斤 */ { "SYO",  { XR_SI, XR_yo, } },
		     
/* 切扎 */ { "TYA",  { XR_TI, XR_ya, } },
/* 切不 */ { "TYI",  { XR_TI, XR__i, } },
/* 切文 */ { "TYU",  { XR_TI, XR_yu, } },
/* 切之 */ { "TYE",  { XR_TI, XR__e, } },
/* 切斤 */ { "TYO",  { XR_TI, XR_yo, } },
		     
/* 任扎 */ { "VYA",  { XR__U, XR_jj, XR_ya, } },
/* 任不 */ { "VYI",  { XR__U, XR_jj, XR__i, } },
/* 任文 */ { "VYU",  { XR__U, XR_jj, XR_yu, } },
/* 任之 */ { "VYE",  { XR__U, XR_jj, XR__e, } },
/* 任斤 */ { "VYO",  { XR__U, XR_jj, XR_yo, } },
		     
/* 扎   */ { "XYA",  { XR_ya, } },
/* 不   */ { "XYI",  { XR__i, } },
/* 文   */ { "XYU",  { XR_yu, } },
/* 之   */ { "XYE",  { XR__e, } },
/* 斤   */ { "XYO",  { XR_yo, } },

/* 元扎 */ { "ZYA",  { XR_SI, XR_jj, XR_ya, } },
/* 元不 */ { "ZYI",  { XR_SI, XR_jj, XR__i, } },
/* 元文 */ { "ZYU",  { XR_SI, XR_jj, XR_yu, } },
/* 元之 */ { "ZYE",  { XR_SI, XR_jj, XR__e, } },
/* 元斤 */ { "ZYO",  { XR_SI, XR_jj, XR_yo, } },
		     
/* 切扎 */ { "CHA",  { XR_TI, XR_ya, } },
/* 切   */ { "CHI",  { XR_TI,        } },
/* 切文 */ { "CHU",  { XR_TI, XR_yu, } },
/* 切之 */ { "CHE",  { XR_TI, XR__e, } },
/* 切斤 */ { "CHO",  { XR_TI, XR_yo, } },
		     
/* 匹扎 */ { "DHA",  { XR_TE, XR_jj, XR_ya, } },
/* 匹不 */ { "DHI",  { XR_TE, XR_jj, XR__i, } },
/* 匹文 */ { "DHU",  { XR_TE, XR_jj, XR_yu, } },
/* 匹之 */ { "DHE",  { XR_TE, XR_jj, XR__e, } },
/* 匹斤 */ { "DHO",  { XR_TE, XR_jj, XR_yo, } },
		     
/* 仄扎 */ { "SHA",  { XR_SI, XR_ya, } },
/* 仄   */ { "SHI",  { XR_SI,        } },
/* 仄文 */ { "SHU",  { XR_SI, XR_yu, } },
/* 仄之 */ { "SHE",  { XR_SI, XR__e, } },
/* 仄斤 */ { "SHO",  { XR_SI, XR_yo, } },
		     
/* 化扎 */ { "THA",  { XR_TE, XR_ya, } },
/* 化不 */ { "THI",  { XR_TE, XR__i, } },
/* 化文 */ { "THU",  { XR_TE, XR_yu, } },
/* 化之 */ { "THE",  { XR_TE, XR__e, } },
/* 化斤 */ { "THO",  { XR_TE, XR_yo, } },
		     
/* 丹丑 */ { "WHA",  { XR__U, XR_ya, } },
/* 丹不 */ { "WHI",  { XR__U, XR__i, } },
/* 丹   */ { "WHU",  { XR__U,        } },
/* 丹之 */ { "WHE",  { XR__U, XR__e, } },
/* 丹予 */ { "WHO",  { XR__U, XR_yo, } },
		     
/* 勾丑 */ { "TSA",  { XR_TU, XR_ya, } },
/* 勾不 */ { "TSI",  { XR_TU, XR__i, } },
/* 勾   */ { "TSU",  { XR_TU,        } },
/* 勾之 */ { "TSE",  { XR_TU, XR__e, } },
/* 勾予 */ { "TSO",  { XR_TU, XR_yo, } },
		     
/* 勻   */ { "XTU",  { XR_tu, } },
		     
/* 仁丑 */ { "QWA",  { XR_KU, XR__a, } },
/* 仁不 */ { "QWI",  { XR_KU, XR__i, } },
/* 仁丰 */ { "QWU",  { XR_KU, XR__u, } },
/* 仁之 */ { "QWE",  { XR_KU, XR__e, } },
/* 仁予 */ { "QWO",  { XR_KU, XR__o, } },
		     
/* 氏   */ { "NN",   { XR__N, } },
/* 氏   */ { "N'",   { XR__N, } },
		     
};

static const romaji_list list_atok[] =
{

/* 丐   */ { "A",    { XR__A, } },

/* 壬   */ { "BA",   { XR_HA, XR_jj, } },
/* 太   */ { "BI",   { XR_HI, XR_jj, } },
/* 少   */ { "BU",   { XR_HU, XR_jj, } },
/* 屯   */ { "BE",   { XR_HE, XR_jj, } },
/* 廿   */ { "BO",   { XR_HO, XR_jj, } },

/* 分   */ { "DA",   { XR_TA, XR_jj, } },
/* 刈   */ { "DI",   { XR_TI, XR_jj, } },
/* 勿   */ { "DU",   { XR_TU, XR_jj, } },
/* 匹   */ { "DE",   { XR_TE, XR_jj, } },
/* 升   */ { "DO",   { XR_TO, XR_jj, } },

/* 尹   */ { "E",    { XR__E, } },

/* 孔丑 */ { "FA",   { XR_HU, XR__a, } },
/* 孔不 */ { "FI",   { XR_HU, XR__i, } },
/* 孔   */ { "FU",   { XR_HU,        } },
/* 孔之 */ { "FE",   { XR_HU, XR__e, } },
/* 孔予 */ { "FO",   { XR_HU, XR__o, } },

/* 互   */ { "GA",   { XR_KA, XR_jj, } },
/* 亢   */ { "GI",   { XR_KI, XR_jj, } },
/* 什   */ { "GU",   { XR_KU, XR_jj, } },
/* 仆   */ { "GE",   { XR_KE, XR_jj, } },
/* 仍   */ { "GO",   { XR_KO, XR_jj, } },

/* 反   */ { "HA",   { XR_HA, } },
/* 夫   */ { "HI",   { XR_HI, } },
/* 孔   */ { "HU",   { XR_HU, } },
/* 尺   */ { "HE",   { XR_HE, } },
/* 幻   */ { "HO",   { XR_HO, } },

/* 中   */ { "I",    { XR__I, } },

/* 元扎 */ { "JA",   { XR_SI, XR_jj, XR_ya, } },
/* 元   */ { "JI",   { XR_SI, XR_jj,        } },
/* 元文 */ { "JU",   { XR_SI, XR_jj, XR_yu, } },
/* 元之 */ { "JE",   { XR_SI, XR_jj, XR__e, } },
/* 元斤 */ { "JO",   { XR_SI, XR_jj, XR_yo, } },

/* 井   */ { "KA",   { XR_KA, } },
/* 五   */ { "KI",   { XR_KI, } },
/* 仁   */ { "KU",   { XR_KU, } },
/* 仃   */ { "KE",   { XR_KE, } },
/* 仇   */ { "KO",   { XR_KO, } },

/* 丑   */ { "LA",   { XR__a, } },
/* 不   */ { "LI",   { XR__i, } },
/* 丰   */ { "LU",   { XR__u, } },
/* 之   */ { "LE",   { XR__e, } },
/* 予   */ { "LO",   { XR__o, } },

/* 引   */ { "MA",   { XR_MA, } },
/* 心   */ { "MI",   { XR_MI, } },
/* 戈   */ { "MU",   { XR_MU, } },
/* 戶   */ { "ME",   { XR_ME, } },
/* 手   */ { "MO",   { XR_MO, } },

/* 卅   */ { "NA",   { XR_NA, } },
/* 卞   */ { "NI",   { XR_NI, } },
/* 厄   */ { "NU",   { XR_NU, } },
/* 友   */ { "NE",   { XR_NE, } },
/* 及   */ { "NO",   { XR_NO, } },

/* 云   */ { "O",    { XR__O, } },

/* 天   */ { "PA",   { XR_HA, XR_pp, } },
/* 夭   */ { "PI",   { XR_HI, XR_pp, } },
/* 尤   */ { "PU",   { XR_HU, XR_pp, } },
/* 巴   */ { "PE",   { XR_HE, XR_pp, } },
/* 弔   */ { "PO",   { XR_HO, XR_pp, } },

/* 日   */ { "RA",   { XR_RA, } },
/* 曰   */ { "RI",   { XR_RI, } },
/* 月   */ { "RU",   { XR_RU, } },
/* 木   */ { "RE",   { XR_RE, } },
/* 欠   */ { "RO",   { XR_RO, } },

/* 今   */ { "SA",   { XR_SA, } },
/* 仄   */ { "SI",   { XR_SI, } },
/* 允   */ { "SU",   { XR_SU, } },
/* 六   */ { "SE",   { XR_SE, } },
/* 公   */ { "SO",   { XR_SO, } },

/* 凶   */ { "TA",   { XR_TA, } },
/* 切   */ { "TI",   { XR_TI, } },
/* 勾   */ { "TU",   { XR_TU, } },
/* 化   */ { "TE",   { XR_TE, } },
/* 午   */ { "TO",   { XR_TO, } },

/* 丹   */ { "U",    { XR__U, } },

/* 任丑 */ { "VA",   { XR__U, XR_jj, XR__a, } },
/* 任不 */ { "VI",   { XR__U, XR_jj, XR__i, } },
/* 任   */ { "VU",   { XR__U, XR_jj,        } },
/* 任之 */ { "VE",   { XR__U, XR_jj, XR__e, } },
/* 任予 */ { "VO",   { XR__U, XR_jj, XR__o, } },

/* 歹   */ { "WA",   { XR_WA,        } },
/* 丹不 */ { "WI",   { XR__U, XR__i, } },
/* 丹   */ { "WU",   { XR__U,        } },
/* 丹之 */ { "WE",   { XR__U, XR__e, } },
/* 毛   */ { "WO",   { XR_WO,        } },

/* 丑   */ { "XA",   { XR__a, } },
/* 不   */ { "XI",   { XR__i, } },
/* 丰   */ { "XU",   { XR__u, } },
/* 之   */ { "XE",   { XR__e, } },
/* 予   */ { "XO",   { XR__o, } },

/* 支   */ { "YA",   { XR_YA,        } },
/* 中   */ { "YI",   { XR__I,        } },
/* 斗   */ { "YU",   { XR_YU,        } },
/* 中之 */ { "YE",   { XR__I, XR__e, } },
/* 方   */ { "YO",   { XR_YO,        } },

/* 介   */ { "ZA",   { XR_SA, XR_jj, } },
/* 元   */ { "ZI",   { XR_SI, XR_jj, } },
/* 內   */ { "ZU",   { XR_SU, XR_jj, } },
/* 兮   */ { "ZE",   { XR_SE, XR_jj, } },
/* 冗   */ { "ZO",   { XR_SO, XR_jj, } },

/* 太扎 */ { "BYA",  { XR_HI, XR_jj, XR_ya, } },
/* 太不 */ { "BYI",  { XR_HI, XR_jj, XR__i, } },
/* 太文 */ { "BYU",  { XR_HI, XR_jj, XR_yu, } },
/* 太之 */ { "BYE",  { XR_HI, XR_jj, XR__e, } },
/* 太斤 */ { "BYO",  { XR_HI, XR_jj, XR_yo, } },
		     
/* 切扎 */ { "CYA",  { XR_TI, XR_ya, } },
/* 切不 */ { "CYI",  { XR_TI, XR__i, } },
/* 切文 */ { "CYU",  { XR_TI, XR_yu, } },
/* 切之 */ { "CYE",  { XR_TI, XR__e, } },
/* 切斤 */ { "CYO",  { XR_TI, XR_yo, } },
		     
/* 刈扎 */ { "DYA",  { XR_TI, XR_jj, XR_ya, } },
/* 刈不 */ { "DYI",  { XR_TI, XR_jj, XR__i, } },
/* 刈文 */ { "DYU",  { XR_TI, XR_jj, XR_yu, } },
/* 刈之 */ { "DYE",  { XR_TI, XR_jj, XR__e, } },
/* 刈斤 */ { "DYO",  { XR_TI, XR_jj, XR_yo, } },
		     
/* 孔扎 */ { "FYA",  { XR_HU, XR_ya, } },
/* 孔不 */ { "FYI",  { XR_HU, XR__i, } },
/* 孔文 */ { "FYU",  { XR_HU, XR_yu, } },
/* 孔之 */ { "FYE",  { XR_HU, XR__e, } },
/* 孔斤 */ { "FYO",  { XR_HU, XR_ya, } },
		     
/* 亢扎 */ { "GYA",  { XR_KI, XR_jj, XR_ya, } },
/* 亢不 */ { "GYI",  { XR_KI, XR_jj, XR__i, } },
/* 亢文 */ { "GYU",  { XR_KI, XR_jj, XR_yu, } },
/* 亢之 */ { "GYE",  { XR_KI, XR_jj, XR__e, } },
/* 亢斤 */ { "GYO",  { XR_KI, XR_jj, XR_yo, } },
		     
/* 夫扎 */ { "HYA",  { XR_HI, XR_ya, } },
/* 夫不 */ { "HYI",  { XR_HI, XR__i, } },
/* 夫文 */ { "HYU",  { XR_HI, XR_yu, } },
/* 夫之 */ { "HYE",  { XR_HI, XR__e, } },
/* 夫斤 */ { "HYO",  { XR_HI, XR_yo, } },
		     
/* 元扎 */ { "JYA",  { XR_SI, XR_jj, XR_ya, } },
/* 元不 */ { "JYI",  { XR_SI, XR_jj, XR__i, } },
/* 元文 */ { "JYU",  { XR_SI, XR_jj, XR_yu, } },
/* 元之 */ { "JYE",  { XR_SI, XR_jj, XR__e, } },
/* 元斤 */ { "JYO",  { XR_SI, XR_jj, XR_yo, } },

/* 五扎 */ { "KYA",  { XR_KI, XR_ya, } },
/* 五不 */ { "KYI",  { XR_KI, XR__i, } },
/* 五文 */ { "KYU",  { XR_KI, XR_yu, } },
/* 五之 */ { "KYE",  { XR_KI, XR__e, } },
/* 五斤 */ { "KYO",  { XR_KI, XR_yo, } },
		     
/* 扎   */ { "LYA",  { XR_ya, } },
/* 不   */ { "LYI",  { XR__i, } },
/* 文   */ { "LYU",  { XR_yu, } },
/* 之   */ { "LYE",  { XR__e, } },
/* 斤   */ { "LYO",  { XR_yo, } },
		     
/* 心扎 */ { "MYA",  { XR_MI, XR_ya, } },
/* 心不 */ { "MYI",  { XR_MI, XR__i, } },
/* 心文 */ { "MYU",  { XR_MI, XR_yu, } },
/* 心之 */ { "MYE",  { XR_MI, XR__e, } },
/* 心斤 */ { "MYO",  { XR_MI, XR_yo, } },
		     
/* 卞扎 */ { "NYA",  { XR_NI, XR_ya, } },
/* 卞不 */ { "NYI",  { XR_NI, XR__i, } },
/* 卞文 */ { "NYU",  { XR_NI, XR_yu, } },
/* 卞之 */ { "NYE",  { XR_NI, XR__e, } },
/* 卞斤 */ { "NYO",  { XR_NI, XR_yo, } },
		     
/* 夭扎 */ { "PYA",  { XR_HI, XR_pp, XR_ya, } },
/* 夭不 */ { "PYI",  { XR_HI, XR_pp, XR__i, } },
/* 夭文 */ { "PYU",  { XR_HI, XR_pp, XR_yu, } },
/* 夭之 */ { "PYE",  { XR_HI, XR_pp, XR__e, } },
/* 夭斤 */ { "PYO",  { XR_HI, XR_pp, XR_yo, } },
		     
/* 曰扎 */ { "RYA",  { XR_RI, XR_ya, } },
/* 曰不 */ { "RYI",  { XR_RI, XR__i, } },
/* 曰文 */ { "RYU",  { XR_RI, XR_yu, } },
/* 曰之 */ { "RYE",  { XR_RI, XR__e, } },
/* 曰斤 */ { "RYO",  { XR_RI, XR_yo, } },
		     
/* 仄扎 */ { "SYA",  { XR_SI, XR_ya, } },
/* 仄不 */ { "SYI",  { XR_SI, XR__i, } },
/* 仄文 */ { "SYU",  { XR_SI, XR_yu, } },
/* 仄之 */ { "SYE",  { XR_SI, XR__e, } },
/* 仄斤 */ { "SYO",  { XR_SI, XR_yo, } },

/* 切扎 */ { "TYA",  { XR_TI, XR_ya, } },
/* 切不 */ { "TYI",  { XR_TI, XR__i, } },
/* 切文 */ { "TYU",  { XR_TI, XR_yu, } },
/* 切之 */ { "TYE",  { XR_TI, XR__e, } },
/* 切斤 */ { "TYO",  { XR_TI, XR_yo, } },
		     
/* 扎   */ { "XYA",  { XR_ya, } },
/* 不   */ { "XYI",  { XR__i, } },
/* 文   */ { "XYU",  { XR_yu, } },
/* 之   */ { "XYE",  { XR__e, } },
/* 斤   */ { "XYO",  { XR_yo, } },
		     
/* 元扎 */ { "ZYA",  { XR_SI, XR_jj, XR_ya, } },
/* 元不 */ { "ZYI",  { XR_SI, XR_jj, XR__i, } },
/* 元文 */ { "ZYU",  { XR_SI, XR_jj, XR_yu, } },
/* 元之 */ { "ZYE",  { XR_SI, XR_jj, XR__e, } },
/* 元斤 */ { "ZYO",  { XR_SI, XR_jj, XR_yo, } },
		     
/* 切扎 */ { "CHA",  { XR_TI, XR_ya, } },
/* 切   */ { "CHI",  { XR_TI,        } },
/* 切文 */ { "CHU",  { XR_TI, XR_yu, } },
/* 切之 */ { "CHE",  { XR_TI, XR__e, } },
/* 切斤 */ { "CHO",  { XR_TI, XR_yo, } },

/* 匹扎 */ { "DHA",  { XR_TE, XR_jj, XR_ya, } },
/* 匹不 */ { "DHI",  { XR_TE, XR_jj, XR__i, } },
/* 匹文 */ { "DHU",  { XR_TE, XR_jj, XR_yu, } },
/* 匹之 */ { "DHE",  { XR_TE, XR_jj, XR__e, } },
/* 匹斤 */ { "DHO",  { XR_TE, XR_jj, XR_yo, } },
		     
/* 仄扎 */ { "SHA",  { XR_SI, XR_ya, } },
/* 仄   */ { "SHI",  { XR_SI,        } },
/* 仄文 */ { "SHU",  { XR_SI, XR_yu, } },
/* 仄之 */ { "SHE",  { XR_SI, XR__e, } },
/* 仄斤 */ { "SHO",  { XR_SI, XR_yo, } },
		     
/* 化扎 */ { "THA",  { XR_TE, XR_ya, } },
/* 化不 */ { "THI",  { XR_TE, XR__i, } },
/* 化文 */ { "THU",  { XR_TE, XR_yu, } },
/* 化之 */ { "THE",  { XR_TE, XR__e, } },
/* 化斤 */ { "THO",  { XR_TE, XR_yo, } },
		     
/* 勾丑 */ { "TSA",  { XR_TU, XR__a, } },
/* 勾不 */ { "TSI",  { XR_TU, XR__i, } },
/* 勾   */ { "TSU",  { XR_TU,        } },
/* 勾之 */ { "TSE",  { XR_TU, XR__e, } },
/* 勾予 */ { "TSO",  { XR_TU, XR__o, } },
		     
/* 勻   */ { "XTU",  { XR_tu, } },
/* 勻   */ { "XTSU", { XR_tu, } },
		     
/* 氏   */ { "NN",   { XR__N, } },
/* 氏   */ { "N'",   { XR__N, } },

};

static const romaji_list list_egg[] =
{

/* 丐   */ { "A",    { XR__A, } },

/* 壬   */ { "BA",   { XR_HA, XR_jj, } },
/* 太   */ { "BI",   { XR_HI, XR_jj, } },
/* 少   */ { "BU",   { XR_HU, XR_jj, } },
/* 屯   */ { "BE",   { XR_HE, XR_jj, } },
/* 廿   */ { "BO",   { XR_HO, XR_jj, } },

/* 分   */ { "DA",   { XR_TA, XR_jj, } },
/* 刈   */ { "DI",   { XR_TI, XR_jj, } },
/* 勿   */ { "DU",   { XR_TU, XR_jj, } },
/* 匹   */ { "DE",   { XR_TE, XR_jj, } },
/* 升   */ { "DO",   { XR_TO, XR_jj, } },

/* 尹   */ { "E",    { XR__E, } },

/* 孔丑 */ { "FA",   { XR_HU, XR__a, } },
/* 孔不 */ { "FI",   { XR_HU, XR__i, } },
/* 孔   */ { "FU",   { XR_HU,        } },
/* 孔之 */ { "FE",   { XR_HU, XR__e, } },
/* 孔予 */ { "FO",   { XR_HU, XR__o, } },

/* 互   */ { "GA",   { XR_KA, XR_jj, } },
/* 亢   */ { "GI",   { XR_KI, XR_jj, } },
/* 什   */ { "GU",   { XR_KU, XR_jj, } },
/* 仆   */ { "GE",   { XR_KE, XR_jj, } },
/* 仍   */ { "GO",   { XR_KO, XR_jj, } },

/* 反   */ { "HA",   { XR_HA, } },
/* 夫   */ { "HI",   { XR_HI, } },
/* 孔   */ { "HU",   { XR_HU, } },
/* 尺   */ { "HE",   { XR_HE, } },
/* 幻   */ { "HO",   { XR_HO, } },

/* 中   */ { "I",    { XR__I, } },

/* 元扎 */ { "JA",   { XR_SI, XR_jj, XR_ya, } },
/* 元   */ { "JI",   { XR_SI, XR_jj,        } },
/* 元文 */ { "JU",   { XR_SI, XR_jj, XR_yu, } },
/* 元之 */ { "JE",   { XR_SI, XR_jj, XR__e, } },
/* 元斤 */ { "JO",   { XR_SI, XR_jj, XR_yo, } },

/* 井   */ { "KA",   { XR_KA, } },
/* 五   */ { "KI",   { XR_KI, } },
/* 仁   */ { "KU",   { XR_KU, } },
/* 仃   */ { "KE",   { XR_KE, } },
/* 仇   */ { "KO",   { XR_KO, } },

/* 日   */ { "LA",   { XR_RA, } },
/* 曰   */ { "LI",   { XR_RI, } },
/* 月   */ { "LU",   { XR_RU, } },
/* 木   */ { "LE",   { XR_RE, } },
/* 欠   */ { "LO",   { XR_RO, } },

/* 引   */ { "MA",   { XR_MA, } },
/* 心   */ { "MI",   { XR_MI, } },
/* 戈   */ { "MU",   { XR_MU, } },
/* 戶   */ { "ME",   { XR_ME, } },
/* 手   */ { "MO",   { XR_MO, } },

/* 卅   */ { "NA",   { XR_NA, } },
/* 卞   */ { "NI",   { XR_NI, } },
/* 厄   */ { "NU",   { XR_NU, } },
/* 友   */ { "NE",   { XR_NE, } },
/* 及   */ { "NO",   { XR_NO, } },

/* 云   */ { "O",    { XR__O, } },

/* 天   */ { "PA",   { XR_HA, XR_pp, } },
/* 夭   */ { "PI",   { XR_HI, XR_pp, } },
/* 尤   */ { "PU",   { XR_HU, XR_pp, } },
/* 巴   */ { "PE",   { XR_HE, XR_pp, } },
/* 弔   */ { "PO",   { XR_HO, XR_pp, } },

/* 日   */ { "RA",   { XR_RA, } },
/* 曰   */ { "RI",   { XR_RI, } },
/* 月   */ { "RU",   { XR_RU, } },
/* 木   */ { "RE",   { XR_RE, } },
/* 欠   */ { "RO",   { XR_RO, } },

/* 今   */ { "SA",   { XR_SA, } },
/* 仄   */ { "SI",   { XR_SI, } },
/* 允   */ { "SU",   { XR_SU, } },
/* 六   */ { "SE",   { XR_SE, } },
/* 公   */ { "SO",   { XR_SO, } },

/* 凶   */ { "TA",   { XR_TA, } },
/* 切   */ { "TI",   { XR_TI, } },
/* 勾   */ { "TU",   { XR_TU, } },
/* 化   */ { "TE",   { XR_TE, } },
/* 午   */ { "TO",   { XR_TO, } },

/* 丹   */ { "U",    { XR__U, } },

/* 任丑 */ { "VA",   { XR__U, XR_jj, XR__a, } },
/* 任不 */ { "VI",   { XR__U, XR_jj, XR__i, } },
/* 任   */ { "VU",   { XR__U, XR_jj,        } },
/* 任之 */ { "VE",   { XR__U, XR_jj, XR__e, } },
/* 任予 */ { "VO",   { XR__U, XR_jj, XR__o, } },

/* 歹   */ { "WA",   { XR_WA,        } },
/* 毋   */ { "WI",   { XR__I,        } },
/* 丹   */ { "WU",   { XR__U,        } },
/* 比   */ { "WE",   { XR__E,        } },
/* 毛   */ { "WO",   { XR_WO,        } },

/* 丑   */ { "XA",   { XR__a, } },
/* 不   */ { "XI",   { XR__i, } },
/* 丰   */ { "XU",   { XR__u, } },
/* 之   */ { "XE",   { XR__e, } },
/* 予   */ { "XO",   { XR__o, } },

/* 支   */ { "YA",   { XR_YA,        } },
/* 中   */ { "YI",   { XR__I,        } },
/* 斗   */ { "YU",   { XR_YU,        } },
/* 中之 */ { "YE",   { XR__I, XR__e, } },
/* 方   */ { "YO",   { XR_YO,        } },

/* 介   */ { "ZA",   { XR_SA, XR_jj, } },
/* 元   */ { "ZI",   { XR_SI, XR_jj, } },
/* 內   */ { "ZU",   { XR_SU, XR_jj, } },
/* 兮   */ { "ZE",   { XR_SE, XR_jj, } },
/* 冗   */ { "ZO",   { XR_SO, XR_jj, } },

/* 太扎 */ { "BYA",  { XR_HI, XR_jj, XR_ya, } },
/* 太文 */ { "BYU",  { XR_HI, XR_jj, XR_yu, } },
/* 太之 */ { "BYE",  { XR_HI, XR_jj, XR__e, } },
/* 太斤 */ { "BYO",  { XR_HI, XR_jj, XR_yo, } },
		     
/* 刈扎 */ { "DYA",  { XR_TI, XR_jj, XR_ya, } },
/* 匹不 */ { "DYI",  { XR_TE, XR_jj, XR__i, } },
/* 刈文 */ { "DYU",  { XR_TI, XR_jj, XR_yu, } },
/* 刈之 */ { "DYE",  { XR_TI, XR_jj, XR__e, } },
/* 刈斤 */ { "DYO",  { XR_TI, XR_jj, XR_yo, } },
		     
/* 亢扎 */ { "GYA",  { XR_KI, XR_jj, XR_ya, } },
/* 亢文 */ { "GYU",  { XR_KI, XR_jj, XR_yu, } },
/* 亢之 */ { "GYE",  { XR_KI, XR_jj, XR__e, } },
/* 亢斤 */ { "GYO",  { XR_KI, XR_jj, XR_yo, } },
		     
/* 夫扎 */ { "HYA",  { XR_HI, XR_ya, } },
/* 夫文 */ { "HYU",  { XR_HI, XR_yu, } },
/* 夫之 */ { "HYE",  { XR_HI, XR__e, } },
/* 夫斤 */ { "HYO",  { XR_HI, XR_yo, } },
		     
/* 元扎 */ { "JYA",  { XR_SI, XR_jj, XR_ya, } },
/* 元文 */ { "JYU",  { XR_SI, XR_jj, XR_yu, } },
/* 元之 */ { "JYE",  { XR_SI, XR_jj, XR__e, } },
/* 元斤 */ { "JYO",  { XR_SI, XR_jj, XR_yo, } },
		     
/* 五扎 */ { "KYA",  { XR_KI, XR_ya, } },
/* 五文 */ { "KYU",  { XR_KI, XR_yu, } },
/* 五之 */ { "KYE",  { XR_KI, XR__e, } },
/* 五斤 */ { "KYO",  { XR_KI, XR_yo, } },

/* 曰扎 */ { "LYA",  { XR_RI, XR_ya, } },
/* 曰文 */ { "LYU",  { XR_RI, XR_yu, } },
/* 曰之 */ { "LYE",  { XR_RI, XR__e, } },
/* 曰斤 */ { "LYO",  { XR_RI, XR_yo, } },
		     
/* 心扎 */ { "MYA",  { XR_MI, XR_ya, } },
/* 心文 */ { "MYU",  { XR_MI, XR_yu, } },
/* 心之 */ { "MYE",  { XR_MI, XR__e, } },
/* 心斤 */ { "MYO",  { XR_MI, XR_yo, } },
		     
/* 卞扎 */ { "NYA",  { XR_NI, XR_ya, } },
/* 卞文 */ { "NYU",  { XR_NI, XR_yu, } },
/* 卞之 */ { "NYE",  { XR_NI, XR__e, } },
/* 卞斤 */ { "NYO",  { XR_NI, XR_yo, } },
		     
/* 夭扎 */ { "PYA",  { XR_HI, XR_pp, XR_ya, } },
/* 夭文 */ { "PYU",  { XR_HI, XR_pp, XR_yu, } },
/* 夭之 */ { "PYE",  { XR_HI, XR_pp, XR__e, } },
/* 夭斤 */ { "PYO",  { XR_HI, XR_pp, XR_yo, } },
		     
/* 曰扎 */ { "RYA",  { XR_RI, XR_ya, } },
/* 曰文 */ { "RYU",  { XR_RI, XR_yu, } },
/* 曰之 */ { "RYE",  { XR_RI, XR__e, } },
/* 曰斤 */ { "RYO",  { XR_RI, XR_yo, } },

/* 仄扎 */ { "SYA",  { XR_SI, XR_ya, } },
/* 仄文 */ { "SYU",  { XR_SI, XR_yu, } },
/* 仄之 */ { "SYE",  { XR_SI, XR__e, } },
/* 仄斤 */ { "SYO",  { XR_SI, XR_yo, } },
		     
/* 切扎 */ { "TYA",  { XR_TI, XR_ya, } },
/* 化不 */ { "TYI",  { XR_TE, XR__i, } },
/* 切文 */ { "TYU",  { XR_TI, XR_yu, } },
/* 切之 */ { "TYE",  { XR_TI, XR__e, } },
/* 切斤 */ { "TYO",  { XR_TI, XR_yo, } },
		     
/* 扎   */ { "XYA",  { XR_ya, } },
/* 文   */ { "XYU",  { XR_yu, } },
/* 斤   */ { "XYO",  { XR_yo, } },
		     
/* 元扎 */ { "ZYA",  { XR_SI, XR_jj, XR_ya, } },
/* 元文 */ { "ZYU",  { XR_SI, XR_jj, XR_yu, } },
/* 元之 */ { "ZYE",  { XR_SI, XR_jj, XR__e, } },
/* 元斤 */ { "ZYO",  { XR_SI, XR_jj, XR_yo, } },
		     
/* 切扎 */ { "CHA",  { XR_TI, XR_ya, } },
/* 切   */ { "CHI",  { XR_TI,        } },
/* 切文 */ { "CHU",  { XR_TI, XR_yu, } },
/* 切之 */ { "CHE",  { XR_TI, XR__e, } },
/* 切斤 */ { "CHO",  { XR_TI, XR_yo, } },

/* 仄扎 */ { "SHA",  { XR_SI, XR_ya, } },
/* 仄   */ { "SHI",  { XR_SI,        } },
/* 仄文 */ { "SHU",  { XR_SI, XR_yu, } },
/* 仄之 */ { "SHE",  { XR_SI, XR__e, } },
/* 仄斤 */ { "SHO",  { XR_SI, XR_yo, } },
		     
/* 勾丑 */ { "TSA",  { XR_TU, XR__a, } },
/* 勾不 */ { "TSI",  { XR_TU, XR__i, } },
/* 勾   */ { "TSU",  { XR_TU,        } },
/* 勾之 */ { "TSE",  { XR_TU, XR__e, } },
/* 勾予 */ { "TSO",  { XR_TU, XR__o, } },
		     
/* 化不 */ { "XTI",  { XR_TE, XR__i, } },
/* 勻   */ { "XTU",  { XR_tu, } },
/* 勻   */ { "XTSU", { XR_tu, } },
		     
/* 氏   */ { "N'",   { XR__N, } },
		     
};


/*
MS-IME 及 伕□穴儂庍晶

	丐   中   丹   尹   云
B	壬   太   少   屯   廿
C	井   仄   仁   六   仇
D	分   刈   勿   匹   升
F	孔丑 孔不 孔   孔之 孔予
G	互   亢   什   仆   仍
H	反   夫   孔   尺   幻
J	元扎 元   元文 元之 元斤
K	井   五   仁   仃   仇
L	丑   不   丰   之   予
M	引   心   戈   戶   手
N	卅   卞   厄   友   及
P	天   夭   尤   巴   弔
Q	仁丑 仁不 仁   仁之 仁予
R	日   曰   月   木   欠
S	今   仄   允   六   公
T	凶   切   勾   化   午
V	任丑 任不 任   任之 任予
W	歹   丹不 丹   丹之 毛
X	丑   不   丰   之   予
Y	支   中   斗   中之 方
Z	介   元   內   兮   冗

BY	太扎 太不 太文 太之 太斤
CY	切扎 切不 切文 切之 切斤
DY	刈扎 刈不 刈文 刈之 刈斤
FY	孔扎 孔不 孔文 孔之 孔斤
GY	亢扎 亢不 亢文 亢之 亢斤
HY	夫扎 夫不 夫文 夫之 夫斤
JY	元扎 元不 元文 元之 元斤
KY	五扎 五不 五文 五之 五斤
LY	扎   不   文   之   斤
MY	心扎 心不 心文 心之 心斤
NY	卞扎 卞不 卞文 卞之 卞斤
PY	夭扎 夭不 夭文 夭之 夭斤
QY	仁扎 仁不 仁文 仁之 仁斤
RY	曰扎 曰不 曰文 曰之 曰斤
SY	仄扎 仄不 仄文 仄之 仄斤
TY	切扎 切不 切文 切之 切斤
VY	任扎 任不 任文 任之 任斤
WY
XY	扎   不   文   之   斤
ZY	元扎 元不 元文 元之 元斤

CH	切扎 切   切文 切之 切斤
DH	匹扎 匹不 匹文 匹之 匹斤
SH	仄扎 仄   仄文 仄之 仄斤
TH	化扎 化不 化文 化之 化斤
WH	丹丑 丹不 丹   丹之 丹予

TS	勾丑 勾不 勾   勾之 勾予

XK	仰             仳
XT	          勻
XTS
LW	止
QW	仁丑 仁不 仁丰 仁之 仁予
LK	仰             仳

NN	氏
N'	氏


ATOK及伕□穴儂庍晶

	丐   中   丹  尹   云
B	壬   太   少  屯   廿
C	
D	分   刈   勿   匹   升
F	孔丑 孔不 孔   孔之 孔予
G	互   亢   什   仆   仍
H	反   夫   孔   尺   幻
J	元扎 元   元文 元之 元斤
K	井   五   仁   仃   仇
L	丑   不   丰   之   予
M	引   心   戈   戶   手
N	卅   卞   厄   友   及
P	天   夭   尤   巴   弔
Q	
R	日   曰   月   木   欠
S	今   仄   允   六   公
T	凶   切   勾   化   午
V	丹‵丑丹‵不丹‵丹‵之丹‵予
W	歹   丹不 丹   丹之 毛
X	丑   不   丰   之   予
Y	支   中   斗   中之 方
Z	介   元   內   兮   冗

BY	太扎 太不 太文 太之 太斤
CY	切扎 切不 切文 切之 切斤
DY	刈扎 刈不 刈文 刈之 刈斤
FY	孔扎 孔不 孔文 孔之 孔斤
GY	亢扎 亢不 亢文 亢之 亢斤
HY	夫扎 夫不 夫文 夫之 夫斤
JY	元扎 元不 元文 元之 元斤
KY	五扎 五不 五文 五之 五斤
LY	扎   不   文   之   斤
MY	心扎 心不 心文 心之 心斤
NY	卞扎 卞不 卞文 卞之 卞斤
PY	夭扎 夭不 夭文 夭之 夭斤
QY	
RY	曰扎 曰不 曰文 曰之 曰斤
SY	仄扎 仄不 仄文 仄之 仄斤
TY	切扎 切不 切文 切之 切斤
VY	
WY	
XY	扎   不   文   之   斤
ZY	元扎 元不 元文 元之 元斤

CH	切扎 切   切文 切之 切斤
DH	匹扎 匹不 匹文 匹之 匹斤
SH	仄扎 仄   仄文 仄之 仄斤
TH	化扎 化不 化文 化之 化斤
WH	

TS	勾丑 勾不 勾   勾之 勾予

XK	仰             仳
XT	          勻
XTS	          勻
LW	止
QW	
LK	仰             仳

NN	氏
N'	氏


egg 及 伕□穴儂庍晶

	丐   中   丹   尹   云
B	壬   太   少   屯   廿
C	
D	分   刈   勿   匹   升
F	孔丑 孔不 孔   孔之 孔予
G	互   亢   什   仆   仍
H	反   夫   孔   尺   幻
J	元扎 元   元文 元之 元斤
K	井   五   仁   仃   仇
L	日   曰   月   木   欠
M	引   心   戈   戶   手
N	卅   卞   厄   友   及
P	天   夭   尤   巴   弔
Q	
R	日   曰   月   木   欠
S	今   仄   允   六   公
T	凶   切   勾   化   午
V	任丑 任不 任   任之 任予
W	歹   毋   丹   比   毛
X	丑   不   丰   之   予
Y	支   中   斗   中之 方
Z	介   元   內   兮   冗

BY	太扎      太文 太之 太斤
CY	
DY	刈扎 匹不 刈文 刈之 刈斤
FY	
GY	亢扎      亢文 亢之 亢斤
HY	夫扎      夫文 夫之 夫斤
JY	元扎      元文 元之 元斤
KY	五扎      五文 五之 五斤
LY	曰扎      曰文 曰之 曰斤
MY	心扎      心文 心之 心斤
NY	卞扎      卞文 卞之 卞斤
PY	夭扎      夭文 夭之 夭斤
QY	
RY	曰扎      曰文 曰之 曰斤
SY	仄扎      仄文 仄之 仄斤
TY	切扎 化不 切文 切之 切斤
VY	
WY	
XY	扎        文        斤
ZY	元扎      元文 元之 元斤

CH	切扎 切   切文 切之 切斤
DH	
SH	仄扎 仄   仄文 仄之 仄斤
TH	
WH	

TS	勾丑 勾不 勾   勾之 勾予

XK	仰             仳
XT	     化不 勻
XTS	          勻
LW	
QW	
LK	

N	氏
N'	氏
*/
