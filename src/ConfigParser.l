/* networkd config parser / lex template
 * $Id: ConfigParser.l 2 2007-11-02 15:31:40Z oxff $
 */

%{

#include <string>
#include "ConfigParser.yacc.hpp"

using namespace std;


string g_networkd_cfg_yystringBuffer;



extern "C"
{
int networkd_cfg_yywrap()
{
	return 1;
}
}

%}


%x stringconst multicomment


%%


[a-zA-Z][a-zA-Z0-9_\-]*	{ g_networkd_cfg_yystringBuffer = networkd_cfg_yytext; return CFGT_VALUE_NAME; }


"{"			{ return CFGT_SECTION_OPEN; }
"}"			{ return CFGT_SECTION_CLOSE; }
"["			{ return CFGT_VALUELIST_OPEN; }
","			{ return CFGT_VALUELIST_DELIMIT; }
"]"			{ return CFGT_VALUELIST_CLOSE; }

"="			{ return CFGT_VALUE_ASSIGN; }
";"			{ return CFGT_SEMICOLON; }


\"			{ BEGIN(stringconst); g_networkd_cfg_yystringBuffer.clear(); }
<stringconst>\\\"	{ g_networkd_cfg_yystringBuffer.append("\""); }
<stringconst>\\\\	{ g_networkd_cfg_yystringBuffer.append("\\"); }
<stringconst>\\r	{ g_networkd_cfg_yystringBuffer.append("\r"); }
<stringconst>\\n	{ g_networkd_cfg_yystringBuffer.append("\n"); }
<stringconst>\\t	{ g_networkd_cfg_yystringBuffer.append("\t"); }
<stringconst>\\0	{ g_networkd_cfg_yystringBuffer.append("\0", 1); }
<stringconst>[^\"\\]*	{ g_networkd_cfg_yystringBuffer.append(networkd_cfg_yytext, yyleng); }
<stringconst>\"		{ BEGIN(INITIAL); return CFGT_STRINGCONSTANT; }

[0-9]+			{ g_networkd_cfg_yystringBuffer = networkd_cfg_yytext; return CFGT_NUMBERCONSTANT; }
0x[0-9]+		{ g_networkd_cfg_yystringBuffer = networkd_cfg_yytext; return CFGT_NUMBERCONSTANT; }


"//"[^\n]*		{ }
"#"[^\n]*		{ }

"/*".*			{ BEGIN(multicomment); }
<multicomment>.*"*/"	{ BEGIN(INITIAL); }
<multicomment>.*	{ }


[ \t\r\n]		{ }

'			{ return CFGT_UNEXPECTED; }
