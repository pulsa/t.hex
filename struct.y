%{
#include "precomp.h"
#include "thex.h"
#include "struct.h"

#include <FlexLexer.h>
#define YYLEX_PARAM flex
#define FLEX ((yyFlexLexer*)(flex))
#define yytext (FLEX->YYText())

//#define yylex(flex) (((yyFlexLexer*)(flex))->yylex())

int yylex(void *flex)
{
	int token = (((yyFlexLexer*)(flex))->yylex());
	//PRINTF("yylex %3d\n", token);
	PRINTF(_T("yylex line %3d: %3d %hs\n"), FLEX->lineno(), token, FLEX->YYText());
	return token;
}

#define YYERROR_VERBOSE

void yyerror(const char *msg)
{
	//wxMessageBox(msg, "yyerror");
	PRINTF(_T("%s\n"), msg);
}

static bool AddUserType();
static bool HandleDefinedType(wxString type);

//#pragma warning(disable: 4018) // ignore signed/unsigned mismatch

static StructInfo tstruct;
static VarInfo tvar;

%}

%union
{
  unsigned long  ul;
  unsigned short us;
  bool           b;
  char           *s;
}


%token <ul> tNumber                   /* a number found by lex                  */
%token tInvalidNumber            /* an invalid number found by lex         */
%token tIdentifier               /* identifier found by lex                */

%token tSTRUCT
%token tSIGNED
%token tUNSIGNED
%token tCHAR
%token tSHORT
%token tINT
%token tLONG
%token t__INT64
%token tFLOAT
%token tDOUBLE
%token ABS_POINTER
%token REL_POINTER
%token HEX
%token DEC
%token OCT
%token BIN
%token tTYPEDEF
%token tBASE

%type <ul> sign int_type float_type base_value

/* set operator precedence to be C-like
 * see PCYACC manual, page 82 for details
 * or O'Reilly book, page 62 */
/*%left SHL SHR
%left AND OR
%left '+' '-'
%left '*' '/'
%left UMINUS*/

%start File                     /* where to start the grammar          */
%%
File                    :
                          Sections
                          {

                          //errorout:
                          //  if(g_errct || g_cancelled)
                          //    return 1;         // abort!!
                          }
                        ;

/*
 * There was a rather nasty bug here, where the parser would stop and act like
 *   it had finished successfully, if it found _anything_ besides a comment
 *   before the first #VECTOR line.
 * Solution:  define Sections as one or more, not zero or more.
 * AEB 7-1-2003
 * Better solution:  let the parser keep running.  Only return on error or EOF.
 * maybe not.  This is confusing.  Let's just require at least one Section.
 */
Sections                : /* empty */
                        | Section Sections
                        ;

Section                 : tSTRUCT tIdentifier 
						  {
							tstruct.Init(wxString(yytext, wxConvLibc));
							tvar.Init();
						  }
						'{' declaration_list '}' ';'
						  {
						    pStructVec->push_back(tstruct);
						  }
						| tTYPEDEF { tvar.Init(); }
						  any_type tIdentifier
						  {
						    VarInfo newtype;
						    newtype.Init(wxString(yytext, wxConvLibc));
						    newtype.type = tvar.type;
						    newtype.bSigned = tvar.bSigned;
						    newtype.size = tvar.size;
						    tvar = newtype;
						    AddUserType();
						    tvar.Init();
						  }
						';'
                        ;

declaration_list        : /* empty */
						| declaration declaration_list
						;

/*declaration				: sign int_type name_decl array_bounds options ';'
						  {
							tvar.bSigned = !!$1;
							tvar.type = 0;
							tvar.size = $2;
							if (tstruct.HasVar(tvar.name))
							{
								PRINTF("Line %d: %s is already present in structure %s\n", FLEX->lineno(), tvar.name.c_str(), tstruct.name.c_str());
								YYERROR;
							}
							tstruct.Add(tvar);
							tvar.Init();
						  }
						| float_type name_decl array_bounds options ';'
						  {
							tvar.type = 1;
							tvar.size = $1;
							tstruct.Add(tvar);
							tvar.Init();
						  }
						;*/
declaration				: any_type name_decl array_bounds options ';'
						  {
							if (tstruct.HasVar(tvar.name))
							{
								PRINTF(_T("Line %d: %s is already present in structure %s\n"), FLEX->lineno(), tvar.name.c_str(), tstruct.name.c_str());
								YYERROR;
							}
							tstruct.Add(tvar);
							tvar.Init();
						  }
						;

name_decl				: tIdentifier
						  {
						    wxString tname = wxString(yytext, wxConvLibc);
							if (tstruct.HasVar(tname))
							{
								//YYERROR("Name already defined");
								//goto error;
								YYERROR;
							}
							tvar.name = tname;
						  }
						;
						
array_bounds			: /* empty */			{ tvar.IsArray = false; }
						| '[' array_count ']'   { tvar.IsArray = true; }
						
array_count				: /* empty */	{ tvar.count = -1; }
						| tNumber		{ tvar.count = $1; }
						| tIdentifier	{ tvar.count =  0; }

sign					: /* empty */ { $$ = true; }
						| tSIGNED { $$ = true; }
						| tUNSIGNED { $$ = false; }
						;

int_type				: tCHAR		{ $$ = 1; }
						| tSHORT	{ $$ = 2; }
						| tINT		{ $$ = 4; }
						| tLONG		{ $$ = 4; }
						| t__INT64	{ $$ = 8; }
						;

float_type				: tFLOAT	{ $$ = 4; }
						| tDOUBLE	{ $$ = 8; }
						;

any_type				: sign int_type { tvar.type = 0; tvar.bSigned = !!$1; tvar.size = $2; }
						| float_type { tvar.type = 1; tvar.size = $1; }
						| tIdentifier { HandleDefinedType(wxString(yytext, wxConvLibc)); }
						;

options					: /* empty */
						| '(' option_list ')'
						;

option_list				: /* empty */
						| option option_list
						;

option					: pointer_option
						| base_option
						;

pointer_option			: ABS_POINTER { tvar.pointerType = VarInfo::PT_ABSOLUTE; }
						| REL_POINTER tNumber { tvar.pointerType = VarInfo::PT_RELATIVE; tvar.pointerBase = $2; }
						;

base_value				: HEX { $$ = 16; }
						| DEC { $$ = 10; }
						| OCT { $$ = 8; }
						| BIN { $$ = 2; }
						;

base_option				: tBASE tNumber {
							if ($2 < 2 || $2 > 36)
							{
								PRINTF(_T("Line %d: Expected number base between 2 and 36, got %d\n"), FLEX->lineno(), $2);
								YYERROR;								
							}
							tvar.base = $2;
						  } /* //! todo: clean up syntax */
						| tBASE base_value { tvar.base = $2; }
						| base_value { tvar.base = $1; }
						;

%%


std::vector<VarInfo> typedefs;
static int LookupTypedef(wxString type)
{
	for (size_t n = 0; n < typedefs.size(); n++)
	{
		if (typedefs[n].name == type)
			return n;
	}
	return -1;
}

bool AddUserType()
{
	int n = LookupTypedef(tvar.name);
	if (n >= 0)
	{
		PRINTF(_T("Data type %s already defined.\n"), tvar.name.c_str());
		return false;
	}
	
	typedefs.push_back(tvar);
	return true;
}

bool HandleDefinedType(wxString type)
{
	int n = LookupTypedef(type);
	if (n < 0)
	{
		PRINTF(_T("Data type %s not defined.\n"), type.c_str());
		return false;
	}
	tvar = typedefs[n];
	return true;
}
