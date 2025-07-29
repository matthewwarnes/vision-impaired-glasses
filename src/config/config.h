#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <sstream>

namespace config
{
  ///structure for defining a program option and actually holding the value
  struct option {
  public:
    /// short name used for "-p 999393"
    char _shortname;
    /// long name used for "--port 99393"
    std::string _name;

    /// is the option optional
    bool _optional;

    /// the current value of the option
    std::string _value;

    /// the usage message printed for this option
    std::string _usage;

    /// constructor for creating a new option
    option(const char shortname, const std::string name , const std::string help, const bool optional = false)
      : _shortname(shortname), _name(name), _optional(optional)
    {
      std::stringstream ss;
      ss << " -" << _shortname << " --" << _name << "\t\t" << help;
      _usage = std::string(ss.str());
    }

  };

  ///structure for defining a program flag and actually holding the value
  struct flag {
  public:
    /// short name
    char _shortname;
    /// long name
    std::string _name;

    /// the current value of the option
    bool _value;

    /// the usage message printed for this option
    std::string _usage;

    /// constructor for creating a new flag
    flag(const char shortname, const std::string name , const std::string help)
    : _shortname(shortname), _name(name), _value(false)
    {
      std::stringstream ss;
      ss << " -" << _shortname << " --" << _name << "\t\t" << help;
      _usage = std::string(ss.str());
    }

  };


  ///class for parsing config file and command line and storing the current config
  class config
  {
  public:
    ///default constructor
    config() {}

    ///add a option to find
    void add_option(const char shortname, const std::string name, const std::string help, const bool optional = false);

    ///add an flag to search for
    void add_flag(const char shortname, const std::string name, const std::string help);

    ///parse config file and command line for options
    int parse(const std::string filename, int argc, char **argv, const std::string version);

    ///parse command line for options
    int parse_args(int argc, char **argv, const std::string version);

    ///retrieve a named value after parsing
    template<typename T>
    T get_value(const std::string name)
    {
      std::string str_val = get_value<std::string>(name);
      std::stringstream ss(str_val);
      T val;
      ss >> val;
      return val;
    }

    ///
    bool get_flag(const std::string name);

  private:
    ///the storage for the class holding the different options and their current values
    std::vector<option> _options;
    ///the storage for the class holding the different flags and their current value
    std::vector<flag> _flags;

  };

  //!
  //! This method is used to retrieve the current value of one of the programs options.
  //! This should only be used after the parse function has been calles
  //!
  //!\param name the name of the option, the same as that given using add_option
  //!\return the value of if option not found NULL
  //!\sa parse, add_option
  template<>
  inline std::string config::get_value<std::string>(const std::string name)
  {
    for(auto& i : _options)
      if(i._name == name)
	     return i._value;

    //didn't find the option asked for
    return "";
  }

}

#endif
