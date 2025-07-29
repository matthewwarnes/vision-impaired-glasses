#include "anyoption.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdlib.h>
#include <string.h>

AnyOption::AnyOption()
{
    verbose = false;
    autousage = false;
    equalsign = '=';
    comment       = '#';
    delimiter     = '=';
    once = true;
}

AnyOption::~AnyOption()
{
}

void AnyOption::setFileCommentChar( char _comment )
{
    comment = _comment;
}

void AnyOption::setFileDelimiterChar( char _delimiter )
{
    delimiter = _delimiter ;
}

void AnyOption::setVerbose()
{
    verbose = true ;
}

void AnyOption::autoUsagePrint(bool _autousage)
{
    autousage = _autousage;
}

void AnyOption::addOption( const std::string opt, type t )
{
    options.push_back(option(opt, t));
}

void AnyOption::addOption( char opt, type t )
{
    options.push_back(option(opt, t));
}

void AnyOption::addOption( const std::string name, const char c, type t)
{
    options.push_back(option(name, c, t));
}

void AnyOption::printUnknown(const std::string s)
{
    if(verbose)
        std::cout << "Unknown command argument option : " << s << std::endl;
    printAutoUsage();
}

void AnyOption::processCommandArgs(int argc, char **argv)
{
    for( int i = 1 ; i < argc ; i++ ) /* ignore first argv */
    {
	if(  argv[i][0] == '-' && argv[i][1] == '-' )
        { /* long GNU option */
	    std::string match = parseGNU( argv[i]+2 ); /* skip -- */
	    if( match.length() > 0 && i < argc-1 ) /* found match */
		setValue( match , std::string(argv[++i]) );
	}
        else if(  argv[i][0] ==  '-' )
        { /* POSIX char */
            char ch =  parsePOSIX( argv[i]+1 );/* skip - */
            if( ch != '0' && i < argc-1 ) /* matching char */
                setValue( ch ,  argv[++i] );
	}
        else
        { /* not option but an argument keep index */
            printUnknown(std::string(argv[i]));
	}
    }
}

char AnyOption::parsePOSIX( char* arg )
{
    for( unsigned int i = 0 ; i < strlen(arg) ; i++ ){
	char ch = arg[i] ;
	if( matchChar(ch) )
        { /* keep matching flags till an option */
	    if( i == strlen(arg)-1 )
            { /*if last char argv[++i] is the value */
		return ch;
	    }
            else
            {/* else the rest of arg is the value */
		i++; /* skip any '=' and ' ' */
		while( arg[i] == ' '
		       || arg[i] == equalsign )
		    i++;
		setValue( ch , arg+i );
		return '0';
	    }
	}
    }
    printUnknown(std::string(arg));
    return '0';
}

std::string AnyOption::parseGNU( char *arg )
{
    int split_at = 0;
    /* if has a '=' sign get value */
    for( unsigned int i = 0 ; i < strlen(arg) ; i++ ){
	if(arg[i] ==  equalsign ){
	    split_at = i ; /* store index */
	    i = strlen(arg); /* get out of loop */
	}
    }
    if( split_at > 0 ){ /* it is an option value pair */
        std::string opt(arg, split_at);
        std::string val(arg+split_at+1);

        setValue(opt, val);
        return "";

    }else{ /* regular options with no '=' sign  */
        option* opt = matchOpt(std::string(arg));
        if(opt == NULL) return "";
	return opt->option_name;
    }
    return "";
}


AnyOption::option* AnyOption::matchOpt( const std::string opt )
{
    for( auto& i: options)
    {
        if(i.name_set)
        {
            if( i.option_name == opt )
            {
                if( i.option_type ==  COMMON_OPT ||
                    i.option_type ==  COMMAND_OPT )
                { /* found option return index */
                    return &i;
                }
                else if( i.option_type == COMMON_FLAG ||
                          i.option_type == COMMAND_FLAG )
                { /* found flag, set it */
                    i.set = true;
                    return NULL;
                }
            }
        }
    }
    printUnknown(opt);
    return  NULL;
}

bool AnyOption::matchChar( const char c )
{
    for( auto& i: options)
    {
        if(i.char_set)
        {
            if( i.option_char == c )
            { /* found match */
                if(i.option_type == COMMON_OPT ||
                    i.option_type == COMMAND_OPT )
                { /* an option store and stop scanning */
                    return true;
                }
                else if( i.option_type == COMMON_FLAG ||
		      i.option_type == COMMAND_FLAG )
                { /* a flag store and keep scanning */
                    i.set = true;
                    return false;
                }
            }
        }
    }
    char p[2] = {c, 0};
    printUnknown(std::string(p));
    return false;
}

bool AnyOption::setValue( const std::string name , const std::string value )
{
    option* opt = matchOpt(name);
    if(opt == NULL) return false;
    opt->value = value;
    opt->set = true;
    return true;
}

bool AnyOption::setValue( char option , char *value )
{
    for(auto& i: options)
        if(i.char_set)
            if( i.option_char == option )
            { /* found match */
                if(i.option_type == COMMON_OPT ||
                   i.option_type == COMMAND_OPT )
                {
                    i.value = std::string(value);
                    i.set = true;
                    return true;
                }
            }
    return false;
}

bool AnyOption::processFile( const std::string filename )
{
    std::ifstream is;
    is.open ( filename , std::ifstream::in );
    if( ! is.good() ){
	is.close();
	return false;
    }

    std::string line;
    while(std::getline(is, line))
        processLine(line);

    is.close();
    return true;
}

void AnyOption::processLine(const std::string line)
{
    std::string s(line);
    //strip whitespace
    s.erase(std::remove_if( s.begin(), s.end(),
     [](char c){ return (c =='\r' || c =='\t' || c == ' ' || c == '\n');}), s.end() );

    //is first character a comment or length is now zero
     if(s.length() == 0 || s.at(0) == comment) return;

    //does it contain a delimiter
    switch(std::count(s.begin(), s.end(), delimiter))
    {
        case 0:
            //not a pair (flag)
            justValue(s);
            break;
        case 1:
            //value-pair (option)
            size_t delim_pos = s.find(delimiter);
            if(delim_pos == std::string::npos) return;
            std::string option(s, 0, delim_pos);
            std::string value(s, delim_pos+1);
            valuePairs(option, value);
            break;
    }
}

void AnyOption::valuePairs( const std::string type, const std::string value )
{
    if(type.length() == 1)
    {
        for(auto& i: options)
            if(i.char_set)
                if(i.option_char == type[0])
                    if( i.option_type == COMMON_OPT ||
                    i.option_type == FILE_OPT )
                    {
                        i.set = true;
                        i.value = value;
                        return;
                    }
    }

    for(auto& i: options)
    {
        if(i.name_set)
            if(i.option_name == type)
                if( i.option_type == COMMON_OPT ||
                    i.option_type == FILE_OPT )
                {
                    i.set = true;
                    i.value = value;
                    return;
                }
    }

    if(verbose)
    {
        std::cout << "Unknown option in resourcefile : " << type << std::endl;
    }
}

void AnyOption::justValue(const std::string type)
{
    if(type.length() == 1)
    {
        for(auto& i: options)
            if(i.char_set)
                if(i.option_char == type[0])
                    if( i.option_type == COMMON_FLAG ||
                    i.option_type == FILE_FLAG )
                    {
                        i.set = true;
                        return;
                    }
    }

    for(auto& i: options)
    {
        if(i.name_set)
            if(i.option_name == type)
                if( i.option_type == COMMON_FLAG ||
                    i.option_type == FILE_FLAG )
                {
                    i.set = true;
                    return;
                }
    }

    if(verbose)
    {
        std::cout << "Unknown option in resourcefile : " << type << std::endl;
    }
}

std::string AnyOption::getValue( const std::string option )
{
    for(auto& i: options)
    {
        if(i.name_set && (i.option_name == option))
        {
            if((i.option_type == COMMON_OPT) ||
                (i.option_type == COMMAND_OPT) ||
                (i.option_type == FILE_OPT))
            {
                if(i.set)
                    return i.value;
                else
                    return "";
            }
        }
    }
    return "";
}

std::string AnyOption::getValue( char option )
{
    for(auto& i: options)
    {
        if(i.char_set && (i.option_char == option))
        {
            if((i.option_type == COMMON_OPT) ||
                (i.option_type == COMMAND_OPT) ||
                (i.option_type == FILE_OPT))
            {
                if(i.set)
                    return i.value;
                else
                    return "";
            }
        }
    }
    return "";
}

bool AnyOption::getFlag( const std::string option )
{
    for(auto& i: options)
    {
        if(i.name_set && (i.option_name == option))
        {
            if((i.option_type == COMMON_FLAG) ||
                (i.option_type == COMMAND_FLAG) ||
                (i.option_type == FILE_FLAG))
            return i.set;
        }

    }
    return false;
}

bool AnyOption::getFlag( char option )
{
    for(auto& i: options)
    {
        if(i.char_set && (i.option_char == option))
        {
            if((i.option_type == COMMON_FLAG) ||
                (i.option_type == COMMAND_FLAG) ||
                (i.option_type == FILE_FLAG))
            return i.set;
        }
    }
    return false;
}

void AnyOption::printAutoUsage()
{
    if( autousage ) printUsage();
}

void AnyOption::printUsage()
{
    if( once ) {
	once = false ;
	std::cout << std::endl ;
	for( auto& i : usage)
	    std::cout << i << std::endl;
	std::cout << std::endl ;
    }
}

void AnyOption::addUsage( const std::string line )
{
  usage.push_back(line);
}
