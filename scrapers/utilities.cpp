#include "utilities.h"

// C++
#include <algorithm>

// C
#include <cctype>

namespace ext
{
  // ext::string class member functions
  string string::arg(const std::string& data) const noexcept
  {
    int argnum = 0;
    if(m_argnum)
      argnum = *m_argnum;
    string rval(*this, argnum);
    std::string placeholder = "%0";
    placeholder.at(1) += argnum;
    while(rval.replace(placeholder, data));
    return rval;
  }

  template<>
  bool string::is_number<10>(void) const noexcept
  {
    return !empty() &&
        std::find_if_not(begin(), end(), [](unsigned char c) { return std::isdigit(c); }) == end();
  }

  template<>
  bool string::is_number<16>(void) const noexcept
  {
    return !empty() &&
        std::find_if_not(begin(), end(), [](unsigned char c) { return std::isxdigit(c); }) == end();
  }


  template<>
  bool string::contains_word(const std::string& target) const noexcept
  {
    for(size_t pos = 0; (pos = find(target, pos)) != std::string::npos; ++pos)
    {
      if((!pos || !std::isalnum(at(pos - 1))) &&
         (pos + target.size() == size() || !std::isalnum(at(pos + target.size()))))
        return true;
    }
    return false;
  }

  template<>
  bool string::contains_word(const std::string_view& target) const noexcept
    { return contains_word(std::string(target)); }

  template<>
  bool string::contains_word(const char* const& target) const noexcept
    { return contains_word(std::string(target)); }


  size_t string::erase(const std::string& target) noexcept
  {
    size_t offset = find(target);
    if(offset != std::string::npos)
      erase(offset, target.size());
    return offset;
  }

  bool string::replace(const std::string& target, const std::string& replacement) noexcept
  {
    size_t offset = erase(target);
    if(offset != std::string::npos)
      insert(offset, replacement);
    return offset != std::string::npos;
  }

  void string::erase_before(std::string target, size_t pos) noexcept
  {
    size_t offset = find(target, pos);
    if(offset != std::string::npos)
      erase(0, offset + target.size());
  }

  void string::erase_at(std::string target, size_t pos) noexcept
  {
    size_t offset = find(target, pos);
    if(offset != std::string::npos)
      erase(offset);
  }

  void string::erase(const std::initializer_list<char>& targets) noexcept
  {
    size_t offset = 0;
    while(offset = first_occurence(targets, offset),
          offset != std::string::npos)
      erase(offset, 1);
  }

  void string::trim_front(const std::initializer_list<char>& targets) noexcept
  {
    while(std::any_of(std::begin(targets), std::end(targets), [this](const char c) { return c == back(); }))
    //while(targets.find(front()) != targets.end())
      erase(0, 1);
  }

  void string::trim_back(const std::initializer_list<char>& targets) noexcept
  {
    while(std::any_of(std::begin(targets), std::end(targets), [this](const char c) { return c == back(); }))
      pop_back();
  }

  void string::trim(std::initializer_list<char> targets) noexcept
  {
    trim_front(targets);
    trim_back(targets);
  }

  void string::trim_whitespace(void) noexcept
  {
    while(std::isspace(back()))
      pop_back();

    while(std::isspace(front()))
      erase(0, 1);
  }



  size_t string::first_occurence(const std::initializer_list<char>& targets, size_t pos) const noexcept
  {
    size_t first = std::string::npos;
    size_t current;
    for(char target : targets)
    {
      current = find(target, pos);
      if(current < first)
        first = current;
    }
    return first;
  }

  size_t string::last_occurence(const std::initializer_list<char>& targets, size_t pos) const noexcept
  {
    size_t last = std::string::npos;
    size_t current;
    for(char target : targets)
    {
      current = rfind(target, pos);
      if(current > last || last == std::string::npos)
        last = current;
    }
    return last;
  }

  std::list<string> string::split_string(const std::initializer_list<char>& targets) const noexcept
  {
    std::list<string> rlist;
    size_t offset = 0, end = 0;

    while(end = first_occurence(targets, offset),
          end != std::string::npos)
    {
      rlist.push_back(slice(offset, end));
      offset = end + 1;
    }

    if(offset = last_occurence(targets),
       offset != std::string::npos)
      rlist.push_back(substr(offset + 1));
    else
      rlist.push_back(*this);

    return rlist;
  }

  string& string::list_append(const char deliminator, const std::string& item)
  {
    if(!empty())
      push_back(deliminator);
    append(item);
    return *this;
  }


  size_t string::find_before_or_throw(const std::string& target, size_t pos, int throw_value) const
  {
    size_t offset = find(target, pos);
    if(offset == std::string::npos)
      throw throw_value;
    return offset;
  }

  size_t string::find_after_or_throw(const std::string& target, size_t pos, int throw_value) const
  {
    size_t offset = find_before_or_throw(target, pos, throw_value);
    offset += target.size();
    return offset;
  }

  size_t string::find_after(const std::string& target, size_t pos) const noexcept
  {
    size_t offset = find(target, pos);
    if(offset != std::string::npos)
      offset += target.size();
    return offset;
  }

  size_t string::rfind_before_or_throw(const std::string& target, size_t pos, int throw_value) const
  {
    size_t offset = rfind(target, pos);
    if(offset == std::string::npos)
      throw throw_value;
    return offset;
  }

  size_t string::rfind_after_or_throw(const std::string& target, size_t pos, int throw_value) const
  {
    size_t offset = rfind_before_or_throw(target, pos, throw_value);
    offset += target.size();
    return offset;
  }

  size_t string::rfind_after(const std::string& target, size_t pos) const noexcept
  {
    size_t offset = rfind(target, pos);
    if(offset != std::string::npos)
      offset += target.size();
    return offset;
  }

  //ext::from_string functions
    // integer
  template<> unsigned char from_string(const std::string& str, size_t* pos, int base)
    { return std::stoi(str, pos, base); }

  template<> unsigned int from_string(const std::string& str, size_t* pos, int base)
    { return (unsigned int)(std::stoi(str, pos, base)); }

  template<> int from_string(const std::string& str, size_t* pos, int base)
    { return std::stoi(str, pos, base); }

  template<> long from_string(const std::string& str, size_t* pos, int base)
    { return std::stol(str, pos, base); }

  template<> long long from_string(const std::string& str, size_t* pos, int base)
    { return std::stoll(str, pos, base); }

  template<> unsigned long from_string(const std::string& str, size_t* pos, int base)
    { return std::stoul(str, pos, base); }

  template<> unsigned long long from_string(const std::string& str, size_t* pos, int base)
    { return std::stoull(str, pos, base); }

    // floating point
  template<> float from_string(const std::string& str, size_t* pos)
    { return std::stof(str, pos); }

  template<> double from_string(const std::string& str, size_t* pos)
    { return std::stod(str, pos); }

  template<> long double from_string(const std::string& str, size_t* pos)
    { return std::stold(str, pos); }


  // ext::to_string functions
  std::string to_string(const std::list<std::string>& s_list, const char deliminator)
  {
    string combined;
    for(auto& e : s_list)
      combined.list_append(deliminator, e);
    return { combined };
  }

  // ext::to_list functions
  std::list<std::string> to_list(const std::string& str, const char deliminator)
  {
    std::list<std::string> s_list;
    for(auto& s : ext::string(str).split_string({ deliminator }))
      s_list.emplace_back(s);
    return s_list;
  }

  std::list<std::string> to_list(const std::optional<std::string>& str, const char deliminator)
  {
    if(!str)
      return {};
    return to_list(*str, deliminator);
  }
}
