/* networkd config parser / yacc template
 * $Id: ConfigParser.y 2 2007-11-02 15:31:40Z oxff $
 */
 
%{

#include <string>
#include <stdio.h>
#include <libnetworkd/Configuration.hpp>

using namespace std;
using namespace libnetworkd;


void networkd_cfg_yyerror(const char * errorMessage)
{
}

int networkd_cfg_yylex();


Configuration::ConfigurationNode * g_networkd_cfg_yyrootSection;
Configuration::ConfigurationNode * g_networkd_cfg_yycurrentSection;
Configuration::ConfigurationNode * g_networkd_cfg_yycurrentValue;


extern string g_networkd_cfg_yystringBuffer;
extern FILE * networkd_cfg_yyin;

%}


%token CFGT_SECTION_OPEN CFGT_SECTION_CLOSE
%token CFGT_VALUELIST_OPEN CFGT_VALUELIST_DELIMIT CFGT_VALUELIST_CLOSE
%token CFGT_VALUE_ASSIGN CFGT_SEMICOLON
%token CFGT_VALUE_NAME CFGT_STRINGCONSTANT CFGT_NUMBERCONSTANT
%token CFGT_UNEXPECTED

%start body

%%

body		:
		{
			g_networkd_cfg_yyrootSection = 0;
		};
		| bodyopening sectioncontent CFGT_SECTION_CLOSE
		;
		
bodyopening	: CFGT_SECTION_OPEN
		{
			g_networkd_cfg_yyrootSection = new Configuration::ConfigurationNode;
			g_networkd_cfg_yycurrentSection = g_networkd_cfg_yyrootSection;
			
			g_networkd_cfg_yyrootSection->nodeType = CFGNT_SECTION;
			g_networkd_cfg_yyrootSection->nodeName = "$root$";
			g_networkd_cfg_yyrootSection->backlink = g_networkd_cfg_yyrootSection;
		};
	
section		: sectionname CFGT_SECTION_OPEN sectioncontent CFGT_SECTION_CLOSE
		{
			g_networkd_cfg_yycurrentSection->backlink->data.children.push_back(g_networkd_cfg_yycurrentSection);
			g_networkd_cfg_yycurrentSection = g_networkd_cfg_yycurrentSection->backlink;
		};
		
sectionname	: CFGT_VALUE_NAME
		{
			Configuration::ConfigurationNode * backlink = g_networkd_cfg_yycurrentSection;
			
			g_networkd_cfg_yycurrentSection = new Configuration::ConfigurationNode;
			g_networkd_cfg_yycurrentSection->nodeType = CFGNT_SECTION;
			g_networkd_cfg_yycurrentSection->nodeName = g_networkd_cfg_yystringBuffer;
			g_networkd_cfg_yycurrentSection->backlink = backlink;
		};
	
sectioncontent	:
		| assignment sectioncontent
		| section sectioncontent
		;
		
assignment	: assignname CFGT_VALUE_ASSIGN assignval CFGT_SEMICOLON
		{				
			g_networkd_cfg_yycurrentSection->data.children.push_back(g_networkd_cfg_yycurrentValue);
		};

assignname	: CFGT_VALUE_NAME
		{
			g_networkd_cfg_yycurrentValue = new Configuration::ConfigurationNode;
			
			g_networkd_cfg_yycurrentValue->nodeName = g_networkd_cfg_yystringBuffer;
			g_networkd_cfg_yycurrentValue->nodeType = CFGNT_VALUE;
		};
		
assignval	: CFGT_STRINGCONSTANT
		{
			if(g_networkd_cfg_yycurrentValue->nodeType == CFGNT_VALUE)
				g_networkd_cfg_yycurrentValue->data.value = g_networkd_cfg_yystringBuffer;
			else
				g_networkd_cfg_yycurrentValue->data.valueList.push_back(g_networkd_cfg_yystringBuffer);
		};
		| CFGT_NUMBERCONSTANT
		{
			if(g_networkd_cfg_yycurrentValue->nodeType == CFGNT_VALUE)
				g_networkd_cfg_yycurrentValue->data.value = g_networkd_cfg_yystringBuffer;
			else
				g_networkd_cfg_yycurrentValue->data.valueList.push_back(g_networkd_cfg_yystringBuffer);
		};
		| assignliststart assignlist CFGT_VALUELIST_CLOSE
		;
		
assignliststart : CFGT_VALUELIST_OPEN
		{
			g_networkd_cfg_yycurrentValue->nodeType = CFGNT_LIST;
		};

assignlist	: assignval
		| assignval CFGT_VALUELIST_DELIMIT assignlist
		;

%%

/*

int indent = 0;

void printNode(ConfigurationNode * node)
{
	for(int i = 0; i < indent; ++i)
		putc('\t', stdout);
		
	if(node->nodeType == CFGNT_VALUE)
	{
		printf("String Node: \"%s\" -> \"%s\"\n", node->nodeName.c_str(), node->data.value.c_str());
	}
	else if(node->nodeType == CFGNT_LIST)
	{
		printf("List Node: \"%s\" -> (%u)\n", node->nodeName.c_str(), node->data.valueList.size());
		
		for(list<string>::iterator i = node->data.valueList.begin(); i != node->data.valueList.end(); ++i)
		{
			for(int j = 0; j <= indent; ++j)
				putc('\t', stdout);
				
			printf("\"%s\"\n", i->c_str());
		}
	}
	else if(node->nodeType == CFGNT_SECTION)
	{
		printf(">>> %s (%u)\n", node->nodeName.c_str(), node->data.children.size());
		
		++indent;
		
		for(list<ConfigurationNode *>::iterator i = node->data.children.begin(); i != node->data.children.end(); ++i)
			printNode(* i);
			
		printf("\n");			
		--indent;
	}
	else
		printf("Unknown node type!\n");
}

int main()
{
	if(networkd_cfg_yyparse() != 0)
		return -1;
		
	printNode(g_networkd_cfg_yyrootSection);
	return 0;
}

*/

void Configuration::parseFile(const char * inputFile)
{
	networkd_cfg_yyin = fopen(inputFile, "rt");
	
	if(!networkd_cfg_yyin)
		throw "Could not open configuration file for reading!";
		
	if(networkd_cfg_yyparse())
	{
		fclose(networkd_cfg_yyin);
		
		throw "Configuration syntax error!";
	}
	
	fclose(networkd_cfg_yyin);
	
	if(!(m_rootNode = g_networkd_cfg_yyrootSection))
		throw "Empty configuration!";
}
