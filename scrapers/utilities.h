#ifndef UTILITIES_H
#define UTILITIES_H

// containers
#include <set>
#include <list>
#include <stack>
#include <vector>
#include <optional>

// C++
#include <string>
#include <type_traits>

// C
#include <cassert>
#include <climits>

// libraries
#include <shortjson/shortjson.h>

constexpr uint32_t operator "" _length(const char*, const size_t sz) noexcept
  { return sz; }

static_assert("test"_length == 4, "misclaculation");

namespace ext
{
  template<typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  T from_string(const std::string& str, size_t* pos = 0, int base = 10);

  template<typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
  T from_string(const std::string& str, size_t* pos = 0);

  class string : public std::string
  {
  public:
    using std::string::string;
    using std::string::operator=;
    using std::string::substr;
    //using std::string::erase;

    string(const std::string&& other) : std::string(other){}
    string(const std::string& other) : std::string(other){}

    template<int base = 10>
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
    string& list_append(const char deliminator, const std::string& item);

    size_t find_before_or_throw(const std::string& other, size_t pos, int throw_value) const;
    size_t find_after_or_throw(const std::string& other, size_t pos, int throw_value) const;
    size_t find_after(const std::string& other, size_t pos = 0) const noexcept;

    size_t rfind_before_or_throw(const std::string& other, size_t pos, int throw_value) const;
    size_t rfind_after_or_throw(const std::string& other, size_t pos, int throw_value) const;
    size_t rfind_after(const std::string& other, size_t pos = 0) const noexcept;
  };
}

// shortJSON helpers
template<int line_number>
inline std::optional<bool> safe_bool(const shortjson::node_t& node)
{
  if(node.type == shortjson::Field::Boolean)
    return node.toBool();
  if(node.type == shortjson::Field::Null)
    return std::optional<bool>();
  throw line_number;
}

template<int line_number>
inline std::optional<std::string> safe_string(const shortjson::node_t& node)
{
  if(node.type == shortjson::Field::String)
    return node.toString();
  if(node.type == shortjson::Field::Integer)
    return std::to_string(node.toNumber());
  if(node.type == shortjson::Field::Float)
    return std::to_string(node.toFloat());
  if(node.type == shortjson::Field::Null)
    return std::optional<std::string>();
  throw line_number;
}

template<int line_number, typename int_type = int32_t>
inline std::optional<int_type> safe_int(const shortjson::node_t& node)
{
  if(node.type == shortjson::Field::Integer)
    return int_type(node.toNumber());
  if(node.type == shortjson::Field::Null)
    return std::optional<int_type>();
  throw line_number;
}

template<int line_number>
inline double safe_float64(const shortjson::node_t& node)
{
  if(node.type == shortjson::Field::Integer)
    return node.toNumber();
  if(node.type == shortjson::Field::Float)
    return node.toFloat();
  throw line_number;
}

template<int line_number>
inline float safe_float_unit(const shortjson::node_t& node)
{
  if(node.type == shortjson::Field::String)
    return std::stof(node.toString());
  throw line_number;
}

template <class container, std::enable_if_t<std::is_arithmetic_v<typename container::value_type>, bool> = true>
struct serializable : container
{
  using T = typename container::value_type;
  using container::container;
  using container::operator=;

  operator std::vector<uint8_t>(void) const
  {
    std::vector<uint8_t> output;
    for(T element : *this)
      for(size_t i = 0; i < sizeof(T); ++i)
        output.push_back(reinterpret_cast<uint8_t*>(&element)[i]);
    return output;
  }

  serializable<container>& operator =(const std::vector<uint8_t>& input)
  {
    for(auto pos = std::begin(input); pos != std::end(input); std::advance(pos, sizeof(T)))
    {
      T element = *pos;
      for(size_t i = 1; i < sizeof(T); ++i)
        reinterpret_cast<uint8_t*>(&element)[i] = *std::next(pos, i);
      container::push_back(element);
    }
    return *this;
  }

};

// polyfill for is_scoped_enum
#if !defined(__cpp_lib_is_scoped_enum) || __cpp_lib_is_scoped_enum < 202011L
#define __cpp_lib_is_scoped_enum 202011L
namespace std
{
  namespace detail {
  namespace { // avoid ODR-violation
  template<class T>
  auto test_sizable(int) -> decltype(sizeof(T), std::true_type{});
  template<class>
  auto test_sizable(...) -> std::false_type;

  template<class T>
  auto test_nonconvertible_to_int(int)
      -> decltype(static_cast<std::false_type (*)(int)>(nullptr)(std::declval<T>()));
  template<class>
  auto test_nonconvertible_to_int(...) -> std::true_type;

  template<class T>
  constexpr bool is_scoped_enum_impl = std::conjunction_v<
      decltype(test_sizable<T>(0)),
      decltype(test_nonconvertible_to_int<T>(0))
  >;
  }
  } // namespace detail


  template<typename E>
  struct is_scoped_enum : std::bool_constant<detail::is_scoped_enum_impl<E>> {};

  template< class T >
  inline constexpr bool is_scoped_enum_v = is_scoped_enum<T>::value;
}
#endif


// add operations for scoped enumberations
/*
template<typename enum_type, std::enable_if_t<std::is_scoped_enum_v<enum_type>, bool> = true>
constexpr bool operator ==(enum_type a, enum_type b)
  { return std::underlying_type_t<enum_type>(a) == std::underlying_type_t<enum_type>(b); }
*/


template<typename enum_type, std::enable_if_t<std::is_scoped_enum_v<enum_type>, bool> = true>
constexpr std::underlying_type_t<enum_type> operator |=(enum_type a, typename std::underlying_type_t<enum_type> b)
  { return static_cast<typename std::underlying_type_t<enum_type>>(a) | b; }

template<typename enum_type, std::enable_if_t<std::is_scoped_enum_v<enum_type>, bool> = true>
constexpr std::underlying_type_t<enum_type> operator |=(enum_type a, enum_type b)
  { return static_cast<typename std::underlying_type_t<enum_type>>(a) | static_cast<typename std::underlying_type_t<const enum_type>>(b); }

template<typename enum_type, std::enable_if_t<std::is_scoped_enum_v<enum_type>, bool> = true>
constexpr std::underlying_type_t<enum_type> operator |(enum_type a, typename std::underlying_type_t<enum_type> b)
  { return static_cast<typename std::underlying_type_t<enum_type>>(a) | b; }

template<typename enum_type, std::enable_if_t<std::is_scoped_enum_v<enum_type>, bool> = true>
constexpr enum_type operator |(enum_type a, enum_type b)
  { return static_cast<enum_type>(
            static_cast<typename std::underlying_type_t<enum_type>>(a) |
            static_cast<typename std::underlying_type_t<enum_type>>(b)); }


template<typename enum_type, std::enable_if_t<std::is_scoped_enum_v<enum_type>, bool> = true>
constexpr std::underlying_type_t<enum_type> operator &(enum_type a, enum_type b)
  { return static_cast<typename std::underlying_type_t<enum_type>>(a) &
           static_cast<typename std::underlying_type_t<enum_type>>(b); }

#endif // UTILITIES_H
