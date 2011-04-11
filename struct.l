%{

#include "precomp.h"  //! Where's the right place for this?

#include <stdlib.h>
#include <string.h>
#include "struct_par.hpp"
//#include "thex.h"

class FlexLexer;
static int keywordSearch(FlexLexer *flex);

#define DIM(a) (sizeof(a) / sizeof((a)[0]))

//#undef YY_INTERACTIVE

%}

letter          [a-zA-Z_]
digit           [0-9]
alphanum        [a-zA-Z_0-9]
hex             [0-9A-Fa-f]
/* added \r so we can read files in binary mode */
/* added \032 (0x1A, EOF character) 10-1-04 */
blank           [ \t\f\r\032]
notblank        [^ \t\f\n\r\032;]
other           .

%x      xCOMMENT

%option noyywrap
%option never-interactive
%option yylineno

%%

"EndOfFile"						return 0;

{letter}{alphanum}*             return keywordSearch(this);
{digit}+                        yylval.ul = atoi(yytext); return tNumber;
0[xX]{hex}+                     return tNumber;

{blank}+                        ;
 /* \n                              yylineno++; */

"/*"                       BEGIN(xCOMMENT);
<xCOMMENT>"*/"             BEGIN(0);
<xCOMMENT>[^*\n]+          ; /* break comments into lines so they won't overflow buffer */
 /* <xCOMMENT>\n               yylineno++; */
<xCOMMENT>"*"              ;

"//".*\n                   yylineno++; /* ignore single-line comment */

{other}                         return yytext[0];

%%

/*
 *      reserved word screener
 */

static const struct
{ const char *name;
  int yylex;
} keywords[] = {
	{ "ABS_POINTER", 	ABS_POINTER },
	{ "BASE",			tBASE },
	{ "BIN", 			BIN },
	{ "CHAR", 			tCHAR },
	{ "DEC", 			DEC },
	{ "DOUBLE", 		tDOUBLE },
	{ "FLOAT", 			tFLOAT },
	{ "HEX", 			HEX },
	{ "INT", 			tINT },
	{ "INT64", 		t__INT64 },
	{ "LONG", 			tLONG },
	{ "OCT", 			OCT },
	{ "REL_POINTER", 	REL_POINTER },
	{ "SHORT", 			tSHORT },
	{ "SIGNED", 		tSIGNED },
	{ "STRUCT", 		tSTRUCT },
	{ "TYPEDEF",		tTYPEDEF },
	{ "UNSIGNED", 		tUNSIGNED },
	//!{ "__INT64", 		t__INT64 },
};


/*
 * This function performs a binary search through the keywords table,
 *  and returns the token value found.
 * NB: not all keywords are opcodes.
 *
 * Originally copied from PCLEX manual p. 46-47
 */
static int keywordSearch(FlexLexer *flex)
{
   register int mid;
   int cc, hi, lo;

   lo = 0;
   hi = DIM(keywords) - 1;
   while (lo <= hi)
   {
      mid = (lo + hi) >> 1;
      if ((cc = wxStricmp(flex->YYText(), keywords[mid].name)) == 0)
         return keywords[mid].yylex;
      if (cc < 0)
         hi = mid - 1;
      else
         lo = mid + 1;
   }

   return tIdentifier;
}
