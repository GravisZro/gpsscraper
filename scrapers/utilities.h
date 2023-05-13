#ifndef UTILITIES_H
#define UTILITIES_H

// containers
#include <initializer_list>
#include <list>
#include <stack>
#include <vector>
#include <optional>

// C++
#include <string>
#include <type_traits>
#include <algorithm>
#include <functional>

// C
#include <cassert>
#include <climits>

// libraries
#include <shortjson/shortjson.h>

#include "scraper_types.h"

constexpr uint32_t operator "" _length(const char*, const size_t sz) noexcept
  { return sz; }

static_assert("test"_length == 4, "misclaculation");

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

namespace ext
{
  class string : public std::string
  {
  public:
    using std::string::string;
    using std::string::operator=;
    using std::string::substr;
    //using std::string::erase;

    string(const std::string&& other) : std::string(other) {}
    string(const std::string&  other) : std::string(other) {}

    template<int base = 10>
    bool is_number(void) const noexcept;

// "start with" functions
    template<typename string_type = std::string_view>
    bool starts_with(const string_type& target) const noexcept
      { return find(target) == 0; }

    template<typename string_type = std::string_view>
    bool starts_with(const std::initializer_list<string_type>& ilist) const noexcept
      { return std::any_of(std::begin(ilist), std::end(ilist), [this](const string_type& v) { return starts_with(v); }); }

// "ends with" functions
    template<typename string_type = std::string_view>
    bool ends_with(const string_type& target) const noexcept
      { return find(target) == size() - target.size(); }

    template<typename string_type = std::string_view>
    bool ends_with(const std::initializer_list<string_type>& ilist) const noexcept
      { return std::any_of(std::begin(ilist), std::end(ilist), [this](const string_type& v) { return ends_with(v); }); }

// "contains" functions
    template<typename string_type = std::string_view>
    bool contains(const string_type& target) const noexcept
      { return find(target) != std::string::npos; }

    template<typename string_type = std::string_view>
    bool contains_any(const std::initializer_list<string_type>& ilist) const noexcept
      { return std::any_of(std::begin(ilist), std::end(ilist), [this](const string_type& v) { return contains(v); }); }

    template<typename string_type = std::string_view>
    bool contains_all(const std::initializer_list<string_type>& ilist) const noexcept
      { return std::all_of(std::begin(ilist), std::end(ilist), [this](const string_type& v) { return contains(v); }); }

// "contains word" functions
    template<typename string_type = std::string_view>
    bool contains_word(const string_type& str) const noexcept;

    template<typename string_type = std::string_view>
    bool contains_word_any(const std::initializer_list<string_type>& ilist) const noexcept
      { return std::any_of(std::begin(ilist), std::end(ilist), [this](const string_type& v) { return contains_word(v); }); }

    template<typename string_type = std::string_view>
    bool contains_word_all(const std::initializer_list<string_type>& ilist) const noexcept
      { return std::all_of(std::begin(ilist), std::end(ilist), [this](const string_type& v) { return contains_word(v); }); }


    basic_string& pop_front(void)
      { return std::string::erase(0, 1); }

    basic_string& erase(size_type index, size_type count = std::string::npos)
      { return std::string::erase(index, count); }

    string slice(size_type pos, size_type end) const
      { return substr(pos, end - pos); }

    size_t erase(const std::string& target) noexcept;
    bool replace(const std::string& target, const std::string& replacement) noexcept;
    void erase_before(std::string target, size_t pos = 0) noexcept;
    void erase_at(std::string target, size_t pos = 0) noexcept;
    void erase(const std::initializer_list<char>& targets) noexcept;

    void trim_front(const std::initializer_list<char>& targets) noexcept;
    void trim_back(const std::initializer_list<char>& targets) noexcept;
    void trim(std::initializer_list<char> targets) noexcept;
    void trim_whitespace(void) noexcept;


    size_t last_occurence(const std::initializer_list<char>& targets, size_t pos = std::string::npos) const noexcept;
    size_t first_occurence(const std::initializer_list<char>& targets, size_t pos = std::string::npos) const noexcept;
    std::list<string> split_string(const std::initializer_list<char>& targets) const noexcept;
    string& list_append(const char deliminator, const std::string& item);

    size_t find_before_or_throw(const std::string& target, size_t pos, int throw_value) const;
    size_t find_after_or_throw(const std::string& target, size_t pos, int throw_value) const;
    size_t find_after(const std::string& target, size_t pos = 0) const noexcept;

    size_t rfind_before_or_throw(const std::string& target, size_t pos, int throw_value) const;
    size_t rfind_after_or_throw(const std::string& target, size_t pos, int throw_value) const;
    size_t rfind_after(const std::string& target, size_t pos = 0) const noexcept;

    string arg(const std::string& data) const noexcept;

    template<typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
    string arg(T data, int base = 10) const noexcept { return arg(std::to_string(data, base)); }

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
    string arg(T data) const noexcept { return arg(std::to_string(data)); }

  private:
    string(const std::string&  other, int argnum) : std::string(other), m_argnum(argnum + 1) {}
    string(const std::string&& other, int argnum) : std::string(other), m_argnum(argnum + 1) {}
    std::optional<int> m_argnum;
  };

  //ext::from_string functions

  template<typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  T from_string(const std::string& str, size_t* pos = nullptr, int base = 10);

  template<typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
  T from_string(const std::string& str, size_t* pos = nullptr);

  // ext::to_string functions

  std::string to_string(const std::list<std::string>& s_list, const char deliminator = ',');

  template<typename T>
  std::string to_string(const std::list<T>& t_list, const std::function<std::string(const T&)>& accessor, const char deliminator = ',')
  {
    std::list<std::string> s_list;
    for(auto& e : t_list)
      s_list.emplace_back(accessor(e));
    return to_string(s_list, deliminator);
  }

  template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
  std::string to_string(const std::list<T>& t_list, const char deliminator = ',')
    { return to_string<T>(t_list, [](const T t) { return std::to_string(t); }, deliminator); }

  template<typename T, std::enable_if_t<std::is_scoped_enum_v<T>, bool> = true>
  std::string to_string(const std::list<T>& t_list, const char deliminator = ',')
    { return to_string<T>(t_list, [](const T& e) { return std::to_string(static_cast<typename std::underlying_type_t<const T>>(e)); }, deliminator); }

  template<typename T>
  std::string to_string(const std::initializer_list<T>& i_list, const char deliminator = ',')
    { return to_string<T>(std::list<T>(i_list), deliminator); }

  // ext::to_string functions
  std::list<std::string> to_list(const std::string& str, const char deliminator = ',');

  template<typename T>//, std::enable_if_t<(!std::is_arithmetic_v<T> && !std::is_scoped_enum_v<T>), bool> = true>
  std::list<T> to_list(const std::string& str, const std::function<T(const std::string&)>& placer, const char deliminator = ',')
  {
    std::list<T> t_list;
    for(auto& e : ext::string(str).split_string({ deliminator }))
      t_list.emplace_back(placer(e));
    return t_list;
  }

  template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
  std::list<T> to_list(const std::string& str, const char deliminator = ',')
    { return to_list<T>(str, [](const std::string& s) { return ext::from_string<T>(s); }, deliminator); }

  template<typename T, std::enable_if_t<std::is_scoped_enum_v<T>, bool> = true>
  std::list<T> to_list(const std::string& str, const char deliminator = ',')
    { return to_list<T>(str, [](const std::string& s) { return T(ext::from_string<typename std::underlying_type_t<T>>(s)); }, deliminator); }

  std::list<std::string> to_list(const std::optional<std::string>& str, const char deliminator = ',');

  template<typename T>
  std::list<T> to_list(const std::optional<std::string>& str, const char deliminator = ',')
  {
    if(!str)
      return {};
    return to_list<T>(*str, deliminator);
  }
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
      value = { 0.0, 0.0 }; // reset value
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
    if(this->identifier == identifier)
    {
      if(type != shortjson::Field::Object &&
         type != shortjson::Field::Null)
        throw line_number;
      return true;
    }
    return false;
  }

  template<int line_number>
  bool idArray(const std::string_view& identifier) const
  {
    if(this->identifier == identifier)
    {
      if(type != shortjson::Field::Array &&
         type != shortjson::Field::Null)
        throw line_number;
      return true;
    }
    return false;
  }

  template<int line_number>
  std::vector<safenode_t> safeObject(void) const
  {
    std::vector<safenode_t> vec;
    if(type == shortjson::Field::Object)
    {
      for(auto node : std::get<std::vector<node_t>>(data))
        vec.emplace_back(node);
    }
    else if(type != shortjson::Field::Null)
      throw line_number;
    return vec;
  }

  template<int line_number>
  std::vector<safenode_t> safeArray(void) const
  {
    std::vector<safenode_t> vec;
    if(type == shortjson::Field::Array)
    {
      for(auto node : std::get<std::vector<node_t>>(data))
        vec.emplace_back(node);
    }
    else if(type != shortjson::Field::Null)
      throw line_number;
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


// operators for scoped enumerations
// high level operators
template<typename enum_type, std::enable_if_t<std::is_scoped_enum_v<enum_type>, bool> = true>
constexpr enum_type operator |=(enum_type& a, typename std::underlying_type_t<enum_type> b)
  { return a = a | b; }

template<typename enum_type, std::enable_if_t<std::is_scoped_enum_v<enum_type>, bool> = true>
constexpr enum_type operator |=(enum_type& a, enum_type b)
  { return a = a | b; }

template<typename enum_type, std::enable_if_t<std::is_scoped_enum_v<enum_type>, bool> = true>
constexpr enum_type operator &=(enum_type& a, typename std::underlying_type_t<enum_type> b)
  { return a = a & b; }

template<typename enum_type, std::enable_if_t<std::is_scoped_enum_v<enum_type>, bool> = true>
constexpr enum_type operator &=(enum_type& a, enum_type b)
  { return a = a & b; }

// helper operators
template<typename enum_type, std::enable_if_t<std::is_scoped_enum_v<enum_type>, bool> = true>
constexpr enum_type operator |(enum_type a, typename std::underlying_type_t<enum_type> b)
  { return a | static_cast<enum_type>(b); }

template<typename enum_type, std::enable_if_t<std::is_scoped_enum_v<enum_type>, bool> = true>
constexpr enum_type operator &(enum_type a, typename std::underlying_type_t<enum_type> b)
  { return a & static_cast<enum_type>(b); }

// lowest level operators
template<typename enum_type, std::enable_if_t<std::is_scoped_enum_v<enum_type>, bool> = true>
constexpr enum_type operator |(enum_type a, enum_type b)
  { return static_cast<enum_type>(
            static_cast<typename std::underlying_type_t<enum_type>>(a) |
            static_cast<typename std::underlying_type_t<enum_type>>(b)); }

template<typename enum_type, std::enable_if_t<std::is_scoped_enum_v<enum_type>, bool> = true>
constexpr enum_type operator &(enum_type a, enum_type b)
  { return static_cast<enum_type>(static_cast<typename std::underlying_type_t<enum_type>>(a) &
                                  static_cast<typename std::underlying_type_t<enum_type>>(b)); }

#endif // UTILITIES_H
