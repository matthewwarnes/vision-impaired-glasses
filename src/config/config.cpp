#include "config.h"
#include "anyoption.h"

#include <iostream>

namespace config
{


    //!
    //! This method adds a new option to the config parser, this creates an instance of the
    //! option structure and adds it to the _options vector.
    //!
    //!\param shortname charactor used for command line parsing
    //!\param name long name used for config file and command line parsing
    //!\param help the message printed to the console as part of usage message
    void config::add_option(const char shortname, const std::string name, const std::string help, const bool optional)
    {
        _options.push_back(option(shortname, name, help, optional));
    }

    //!
    //! This method adds a new flag to the config parser, this creates an instance of the
    //! flag structure and adds it to the _flags vector.
    //!
    //!\param shortname charactor used for command line parsing
    //!\param name long name used for config file and command line parsing
    //!\param help the message printed to the console as part of usage message
    void config::add_flag(const char shortname, const std::string name, const std::string help)
    {
        _flags.push_back(flag(shortname, name, help));
    }


    //!
    //! This method actually parses the command line and config file. It firsts does an
    //! initial parse of the command line to determine if an alternate config file should
    //! be used. Once this is determined the chosen config file is parsed followed by the
    //! command line to allow the user to override any of the config file options. If any
    //! of the options can't be found a usage message is printed and the function returns
    //! false
    //!
    //!\param filename the location of the default config file
    //!\param argc the number of command line options
    //!\param argv the command line strings
    //!\param version is the software version to return if v flag is present
    //!\return int signifying whether config parsing was successful (-1 for error)
    int config::parse(const std::string filename, int argc, char **argv, const std::string version)
    {
        AnyOption opt;
        //build up options
        opt.addOption("help", 'h', AnyOption::COMMAND_FLAG);
        //opt.addOption("version", 'v', AnyOption::COMMAND_FLAG);
        opt.addOption("config", 'c', AnyOption::COMMAND_OPT);

        opt.addUsage("Usage: ");
        opt.addUsage("");
        opt.addUsage(" -h --help\t\tprint this usage message");
        opt.addUsage(" -c --config\t\tspecify alternate config file to use");

        for(auto i : _options) {
          if(i._optional)
            opt.addOption(i._name, i._shortname, AnyOption::COMMAND_OPT);
          else
            opt.addOption(i._name, i._shortname, AnyOption::COMMON_OPT);
          opt.addUsage(i._usage);
        }

        for(auto i : _flags) {
            opt.addOption(i._name, i._shortname, AnyOption::COMMON_FLAG);
            opt.addUsage(i._usage);
        }


        //do initial command line parse
        opt.processCommandArgs(argc, argv);
        if(opt.getFlag('h')) {
            opt.printUsage();
            return -1;
        }
        /*if(opt.getFlag('v')) {
            std::cout << "Version: " << version << std::endl;
            return -1;
        }*/

        //parse config file
        if(opt.getValue('c').length() != 0)
            opt.processFile(opt.getValue('c'));
        else
            opt.processFile(filename.c_str());

        //parse command line
        opt.processCommandArgs(argc, argv);


        //parsing should be complete, copy data to local options
        //vector and check they are all present
        for(auto& i : _options)
        {
            std::string val = opt.getValue(i._shortname);
            if(val.length() != 0)
              i._value = val;
            else if(i._optional) {
              i._value = "";
            } else {
              opt.printUsage();
              return -1;
            }
        }

        for(auto& i: _flags)
        {
            i._value = opt.getFlag(i._shortname);
        }

        return 0;
    }

    //!
    //! This method parses the command line arguments and doesn't do anything with config
    //! files. If any of the options can't be found a usage message is printed and the
    //! function returns false
    //!
    //!\param argc the number of command line options
    //!\param argv the command line strings
    //!\param version is the software version to return if v flag is present
    //!\return int signifying whether config parsing was successful (-1 for error)
    int config::parse_args(int argc, char **argv, const std::string version)
    {
        AnyOption opt;
        //build up options
        opt.addOption("help", 'h', AnyOption::COMMAND_FLAG);
        //opt.addOption("version", 'v', AnyOption::COMMAND_FLAG);

        opt.addUsage("Usage: ");
        opt.addUsage("");
        opt.addUsage(" -h --help\t\tprint this usage message");

        for(auto i : _options)
        {
            opt.addOption(i._name, i._shortname, AnyOption::COMMON_OPT);
            opt.addUsage(i._usage);
        }

        for(auto i : _flags)
        {
            opt.addOption(i._name, i._shortname, AnyOption::COMMON_FLAG);
            opt.addUsage(i._usage);
        }

        //do initial command line parse
        opt.processCommandArgs(argc, argv);

        if(opt.getFlag('h'))
        {
            opt.printUsage();
            return -1;
        }
        /*if(opt.getFlag('v'))
        {
            std::cout << "Version: " << version << std::endl;
            return -1;
        }*/

        //parsing should be complete, copy data to local options
        //vector and check they are all present
        for(auto& i : _options)
        {
            std::string val = opt.getValue(i._shortname);
            if(val.length() != 0)
              i._value = val;
            else if(i._optional) {
              i._value = "";
            } else {
              opt.printUsage();
              return -1;
            }
        }

        for(auto& i: _flags)
        {
            i._value = opt.getFlag(i._shortname);
        }

        return 0;
    }


    bool config::get_flag(const std::string name)
    {
        for(auto& i : _flags)
            if(i._name == name)
                return i._value;

        return false;
    }

}
