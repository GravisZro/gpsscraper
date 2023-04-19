#include "utilities.h"

// C++
#include <algorithm>

// C
#include <cctype>

namespace ext
{

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


  template<> float from_string(const std::string& str, size_t* pos)
    { return std::stof(str, pos); }

  template<> double from_string(const std::string& str, size_t* pos)
    { return std::stod(str, pos); }

  template<> long double from_string(const std::string& str, size_t* pos)
    { return std::stold(str, pos); }

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

  size_t string::erase(const std::string& other) noexcept
  {
    size_t offset = find(other);
    if(offset != std::string::npos)
      erase(offset, other.size());
    return offset;
  }

  void string::replace(const std::string& other, const std::string& replacement) noexcept
  {
    size_t offset = erase(other);
    if(offset != std::string::npos)
      insert(offset, replacement);
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

  void string::erase(const std::set<char>& targets) noexcept
  {
    size_t offset = 0;
    while(offset = first_occurence(targets, offset),
          offset != std::string::npos)
      erase(offset, 1);
  }

  void string::trim_front(const std::set<char>& targets) noexcept
  {
    while(targets.find(front()) != targets.end())
      erase(0, 1);
  }

  void string::trim_back(const std::set<char>& targets) noexcept
  {
    while(targets.find(back()) != targets.end())
      pop_back();
  }

  void string::trim(std::set<char> targets) noexcept
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



  size_t string::first_occurence(const std::set<char>& targets, size_t pos) const noexcept
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

  size_t string::last_occurence(const std::set<char>& targets, size_t pos) const noexcept
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

  std::list<std::string> string::split_string(const std::set<char>& splitters) const noexcept
  {
    std::list<std::string> rlist;
    size_t offset = 0, end = 0;

    while(end = first_occurence(splitters, offset),
          end != std::string::npos)
    {
      rlist.push_back(slice(offset, end));
      offset = end + 1;
    }

    if(offset = last_occurence(splitters),
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


  size_t string::find_before_or_throw(const std::string& other, size_t pos, int throw_value) const
  {
    size_t offset = find(other, pos);
    if(offset == std::string::npos)
      throw throw_value;
    return offset;
  }

  size_t string::find_after_or_throw(const std::string& other, size_t pos, int throw_value) const
  {
    size_t offset = find_before_or_throw(other, pos, throw_value);
    offset += other.size();
    return offset;
  }

  size_t string::find_after(const std::string& other, size_t pos) const noexcept
  {
    size_t offset = find(other, pos);
    if(offset != std::string::npos)
      offset += other.size();
    return offset;
  }

  size_t string::rfind_before_or_throw(const std::string& other, size_t pos, int throw_value) const
  {
    size_t offset = rfind(other, pos);
    if(offset == std::string::npos)
      throw throw_value;
    return offset;
  }

  size_t string::rfind_after_or_throw(const std::string& other, size_t pos, int throw_value) const
  {
    size_t offset = rfind_before_or_throw(other, pos, throw_value);
    offset += other.size();
    return offset;
  }

  size_t string::rfind_after(const std::string& other, size_t pos) const noexcept
  {
    size_t offset = rfind(other, pos);
    if(offset != std::string::npos)
      offset += other.size();
    return offset;
  }
}

