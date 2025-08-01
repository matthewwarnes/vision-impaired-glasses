#ifndef __STRING_UTILS_H__
#define __STRING_UTILS_H__

#include <string>

inline std::string ltrim(std::string s, const char* t = " \t\n\r\f\v-")
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from right
inline std::string rtrim(std::string s, const char* t = " \t\n\r\f\v-")
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from left & right
inline std::string trim(std::string s, const char* t = " \t\n\r\f\v-")
{
    return ltrim(rtrim(s, t), t);
}

inline std::string lowercase(std::string s)
{
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
  return s;
}

inline std::string trim_and_lowercase(std::string s)
{
  return lowercase(trim(s));
}

#endif
