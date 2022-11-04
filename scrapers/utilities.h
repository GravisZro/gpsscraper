#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>
#include <set>
#include <list>

constexpr uint32_t operator "" _length(const char*, const size_t sz) noexcept
  { return sz; }

static_assert("test"_length == 4, "misclaculation");

namespace ext
{
  class string : public std::string
  {
  public:
    using std::string::string;
    using std::string::operator=;
    using std::string::substr;
    //using std::string::erase;

    string(const std::string&& other) : std::string(other){}
    string(const std::string& other) : std::string(other){}

    bool is_number(void) const noexcept;

    basic_string& pop_front(void)
      { return std::string::erase(0, 1); }

    basic_string& erase(size_type index, size_type count = std::string::npos)
      { return std::string::erase(index, count); }

    string slice(size_type pos, size_type end) const
      { return substr(pos, end - pos); }

    size_t erase(const std::string& other) noexcept;
    void replace(const std::string& other, const std::string& replacement) noexcept;
    void erase_before(std::string target, size_t pos = 0) noexcept;
    void erase_at(std::string target, size_t pos = 0) noexcept;
    void erase(const std::set<char>& targets) noexcept;

    void trim_front(const std::set<char>& targets) noexcept;
    void trim_back(const std::set<char>& targets) noexcept;
    void trim(std::set<char> targets) noexcept;
    void trim_whitespace(void) noexcept;


    size_t last_occurence(const std::set<char>& targets, size_t pos = std::string::npos) const noexcept;
    size_t first_occurence(const std::set<char>& targets, size_t pos = std::string::npos) const noexcept;
    std::list<std::string> split_string(const std::set<char>& splitters) const noexcept;

    size_t find_before_or_throw(const std::string& other, size_t pos, int throw_value) const;
    size_t find_after_or_throw(const std::string& other, size_t pos, int throw_value) const;
    size_t find_after(const std::string& other, size_t pos = 0) const noexcept;

    size_t rfind_before_or_throw(const std::string& other, size_t pos, int throw_value) const;
    size_t rfind_after_or_throw(const std::string& other, size_t pos, int throw_value) const;
    size_t rfind_after(const std::string& other, size_t pos = 0) const noexcept;
  };

}

#endif // UTILITIES_H
