#ifndef _ANYOPTION_H
#define _ANYOPTION_H

#include <string>
#include <vector>

class AnyOption 
{

public: 
    AnyOption();
    ~AnyOption();

    /* 
     * following set methods specifies the  
     * special characters and delimiters 
     * if not set traditional defaults will be used
     */
    void setFileCommentChar( char _comment );    /* '#' in shellscripts */
    void setFileDelimiterChar( char _delimiter );/* ':' in "width : 100" */

    /*
     * prints warning verbose if you set anything wrong 
     */
    void setVerbose();
  
    enum type {
        COMMON_OPT      =  1,
        COMMAND_OPT     = 2,
        FILE_OPT        = 3,
        COMMON_FLAG     =  4,
        COMMAND_FLAG    = 5,
        FILE_FLAG       = 6
    };
        
    void addOption( const std::string option , type t );
    void addOption( char optchar , type t );
    void addOption( const std::string name, const char c, type t);

    void processCommandArgs( int _argc, char **_argv );
    bool processFile( const std::string _filename );
    
    std::string getValue( const std::string );
    std::string getValue( char );
    bool  getFlag( const std::string );
    bool  getFlag( char );

    void printUsage();
    void printAutoUsage();
    void addUsage( const std::string line );
    void printHelp();
    /* print auto usage printing for unknown options or flag */
    void autoUsagePrint(bool flag);

private: 
    
    struct option {
        std::string option_name;
        char option_char;
        type option_type; /* type - common, command, file */
        bool char_set;
        bool name_set;
        
        std::string value;
        bool set;
                
        option(const std::string n, const char c, const type t) 
                : option_name(n), option_char(c), option_type(t), 
                char_set(true), name_set(true), set(false) {}
        
        option(const std::string n, const type t) 
                : option_name(n), option_type(t), 
                char_set(false), name_set(true), set(false) {}
        
        option(const char c, const type t) 
                : option_char(c), option_type(t), 
                char_set(true), name_set(false), set(false) {}
    };   
    std::vector<option> options;
   
    std::vector<std::string> usage; /* usage */ 

    bool verbose;

    char equalsign;
    char comment;
    char delimiter;
    bool once; 
    
    bool autousage;
    
    /* command line methods*/
    char parsePOSIX( char* arg );
    std::string parseGNU( char *arg );
    bool matchChar( const char c );
    AnyOption::option* matchOpt( const std::string opt );
    bool setValue( const std::string option , const std::string value );
    bool setValue( char optchar , char *value);
    void printUnknown(const std::string);
    
    /* file methods */
    void processLine( const std::string );
    void valuePairs( const std::string type, const std::string value ); 
    void justValue( const std::string value );

};

#endif /* ! _ANYOPTION_H */
