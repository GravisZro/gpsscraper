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
#include <algorithm>

// C
#include <cassert>
#include <climits>

// libraries
#include <shortjson/shortjson.h>

#include "scraper_types.h"

constexpr uint32_t operator "" _length(const char*, const size_t sz) noexcept
  { return sz; }

static_assert("test"_length == 4, "misclaculation");

namespace ext
{
  template<typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  T from_string(const std::string& str, size_t* pos = nullptr, int base = 10);

  template<typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
  T from_string(const std::string& str, size_t* pos = nullptr);

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

// shortJSON helper wrapper struct functions
struct safenode_t : shortjson::node_t
{
  using shortjson::node_t::node_t;
  safenode_t(const shortjson::node_t& other) : shortjson::node_t(other) { }

  template<int line_number>
  bool idString(const std::string_view& identifier, std::optional<std::string>& value) const
  {
    if(this->identifier == identifier)
    {
      value.reset();
      if(type == shortjson::Field::String)
        value = toString();
      else if(type == shortjson::Field::Integer)
        value = std::to_string(toNumber());
      else if(type == shortjson::Field::Float)
        value = std::to_string(toFloat());
      else if(type == shortjson::Field::Null)
        value.reset();
      else
        throw line_number;
      return bool(value);
    }
    return false;
  }

  template<int line_number>
  bool idBool(const std::string_view& identifier, std::optional<bool>& value) const
  {
    if(this->identifier == identifier)
    {
      value.reset();
      if(type == shortjson::Field::Boolean)
        value = toBool();
      else if(type == shortjson::Field::Integer && (toNumber() == 1 || toNumber() == 0))
        value = bool(toNumber());
      else if(type == shortjson::Field::String && toString() == "true")
        value = true;
      else if(type == shortjson::Field::String && toString() == "false")
        value = false;
      else if(type == shortjson::Field::Null ||
              (type == shortjson::Field::String && toString() == "null"))
        value.reset();
      else
        throw line_number;
      return bool(value);
    }
    return false;
  }

  template<int line_number, typename integer_t, std::enable_if_t<std::is_integral_v<integer_t>, bool> = true>
  bool idInteger(const std::string_view& identifier, std::optional<integer_t>& value) const
  {
    if(this->identifier == identifier)
    {
      value.reset();
      if(type == shortjson::Field::Boolean)
        value = toBool() ? 1 : 0;
      else if(type == shortjson::Field::Integer)
        value = toNumber();
      else if(type == shortjson::Field::String)
      {
        try
        {
          value = ext::from_string<integer_t>(toString(), nullptr, 0);
          if(std::to_string(*value) != toString())
            value.reset();
        }
        catch(...) { value.reset(); }
      }
      else if(type == shortjson::Field::Null)
        value.reset();
      else
        throw line_number;
      return bool(value);
    }
    return false;
  }

  template<int line_number, typename floating_t>
  bool idFloat(const std::string_view& identifier, std::optional<floating_t>& value) const
  {
    if(this->identifier == identifier)
    {
      value.reset();
      if(type == shortjson::Field::Float)
        value = toFloat();
      else if(type == shortjson::Field::Integer)
        value = toNumber();
      else if(type == shortjson::Field::String)
      {
        try
        {
          value = ext::from_string<floating_t>(toString(), nullptr);
          if(std::to_string(*value) != toString())
            value.reset();
        }
        catch(...) { value.reset(); }
      }
      else if(type == shortjson::Field::Null)
        value.reset();
      else
        throw line_number;
      return bool(value);
    }
    return false;
  }

  template<int line_number, typename floating_t>
  bool idFloat(const std::string_view& identifier, floating_t& value) const
  {
    std::optional<floating_t> v;
    if(idFloat<line_number, floating_t>(identifier, v))
      value = v.value();
    return bool(v);
  }


  template<int line_number, typename floating_t>
  bool idFloatUnit(const std::string_view& identifier, std::optional<floating_t>& value) const
  {
    if(this->identifier == identifier)
    {
      value.reset();
      if(type == shortjson::Field::String)
      {
        try
        {
          std::size_t pos = 0;
          value = ext::from_string<floating_t>(toString(), &pos);
          if(!pos)
            value.reset();
        }
        catch(...) { value.reset(); }
      }
      else if(type == shortjson::Field::Null)
        value.reset();
      else
        throw line_number;
      return bool(value);
    }
    return false;
  }

  template<int line_number, typename enum_t, std::enable_if_t<std::is_enum_v<enum_t>, bool> = true>
  bool idEnum(const std::string_view& identifier, std::optional<enum_t>& value) const
  {
    using underint_t = typename std::underlying_type_t<enum_t>;
    std::optional<underint_t> v;
    if(idInteger<line_number, std::underlying_type_t<enum_t>>(identifier, v))
      value = enum_t(v.value());
    return bool(v);
  }

  template<int line_number, typename integer_t>
  bool idInteger(const std::string_view& identifier, integer_t& value) const
  {
    std::optional<integer_t> v;
    if(idInteger<line_number, integer_t>(identifier, v))
      value = v.value();
    return bool(v);
  }

  template<int line_number>
  bool idStreet(const std::string_view& identifier,
                     std::optional<uint32_t>& street_number,
                     std::optional<std::string>& street_name) const
  {
    if(this->identifier == identifier)
    {
      street_number.reset();
      street_name.reset();
      if(type != shortjson::Field::String)
        throw line_number;
      auto val = toString();
      try
      {
        auto pos = std::find_if(std::begin(val), std::end(val),
                       [](unsigned char c){ return std::isspace(c); });
        street_number = ext::from_string<int32_t>(std::string(std::begin(val), pos));
        pos = std::next(pos);
        street_name = std::string(pos, std::end(val));
      }
      catch(...)
      {
        street_name = val;
      }
      return bool(street_name);
    }
    return false;
  }

  template<int line_number>
  bool idCoords(const std::string_view& coords_id,
                const std::string_view& latitude_id,
                const std::string_view& longitude_id,
                coords_t& value) const
  {
    if(this->identifier == coords_id)
    {
      if(type != shortjson::Field::Object)
        throw line_number;
      for(const safenode_t& node : safeObject<line_number>())
      {
        node.idFloat<line_number>(latitude_id, value.latitude) ||
        node.idFloat<line_number>(longitude_id, value.longitude);
      }
      return value.latitude && value.longitude;
    }
    return false;
  }

  template<int line_number>
  bool idObject(const std::string_view& identifier) const
  {
    if(this->identifier == identifier &&
       type != shortjson::Field::Object)
      throw line_number;
    return this->identifier == identifier;
  }

  template<int line_number>
  bool idArray(const std::string_view& identifier) const
  {
    if(this->identifier == identifier &&
       type != shortjson::Field::Array)
      throw line_number;
    return this->identifier == identifier;
  }

  template<int line_number>
  std::vector<safenode_t> safeObject(void) const
  {
    if(type != shortjson::Field::Object)
      throw line_number;
    std::vector<safenode_t> vec;
    for(auto node : std::get<std::vector<node_t>>(data))
      vec.emplace_back(node);
    return vec;
  }

  template<int line_number>
  std::vector<safenode_t> safeArray(void) const
  {
    if(type != shortjson::Field::Array)
      throw line_number;
    std::vector<safenode_t> vec;
    for(auto node : std::get<std::vector<node_t>>(data))
      vec.emplace_back(node);
    return vec;
  }

#define idString    idString<__LINE__>
#define idBool      idBool<__LINE__>
#define idInteger   idInteger<__LINE__>
#define idEnum      idEnum<__LINE__>
#define idFloat     idFloat<__LINE__>
#define idFloatUnit idFloatUnit<__LINE__>
#define idStreet    idStreet<__LINE__>
#define idCoords    idCoords<__LINE__>
#define idObject    idObject<__LINE__>
#define idArray     idArray<__LINE__>
#define safeObject  safeObject<__LINE__>
#define safeArray   safeArray<__LINE__>
};


// serializer
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
