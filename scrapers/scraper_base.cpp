#include "scraper_base.h"

#include <cstdlib>
#include <cctype>

const char hexdigit[] =
{
  '0', '1', '2', '3',
  '4', '5', '6', '7',
  '8', '9', 'A', 'B',
  'C', 'D', 'E', 'F',
};

std::string url_encode(const std::string& source, std::unordered_set<char> exceptions) noexcept
{
  std::string output;
  for(auto& letter : source)
  {
    if(exceptions.find(letter) == exceptions.end())
    {
      if(!isspace(letter) &&
         (isalnum(letter) ||
          letter == '-' ||
          letter == '.' ||
          letter == '_' ||
          letter == '~' ||
          letter == '/'))
        output.push_back(letter);
      else
      {
        output.push_back('%');
        output.push_back(hexdigit[letter >> 4]);
        output.push_back(hexdigit[letter & 0x0F]);
      }
    }
  }
  return output;
}

std::string unescape(const std::string& source) noexcept
{
  std::string dest = source;
  size_t pos = std::string::npos;
  while(pos = dest.find("\\/"),
        pos != std::string::npos)
    dest.erase(pos, 1);
  return dest;
}
