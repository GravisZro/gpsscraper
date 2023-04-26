#include "scraper_types.h"

#include <iostream>
#include <algorithm>
#include <cassert>

#include "utilities.h"


template<typename T>
void incorporate_optional(std::optional<T>& a, const std::optional<T>& b)
{
  if(!a && b)
    a = b;
  else if(a && b)
    assert(*a == *b);
}

template<>
void incorporate_optional(std::optional<std::string>& a, const std::optional<std::string>& b)
{
  if(!a || (a && a->empty())) // 'a' not set or 'a' empty
    a = b;
  else if(a && b) // both are set
    if(a != b)
      std::cout << "mismatched strings!: \"" << *a << "\" vs \"" << *b << "\"" << std::endl;
}


// === power_t ===
power_t::operator bool(void) const
{
  return
      level ||
      connector ||
      amp ||
      kw ||
      volt;
}

bool power_t::operator ==(const power_t& o) const
{
  return
      level == o.level &&
      connector == o.connector &&
      amp == o.amp &&
      kw == o.kw &&
      volt == o.volt;
}

void power_t::incorporate(const power_t& o)
{
  incorporate_optional(level, o.level);
  incorporate_optional(connector, o.connector);
  incorporate_optional(amp, o.amp);
  incorporate_optional(kw, o.kw);
  incorporate_optional(volt, o.volt);
}


// === price_t ===
price_t::operator bool(void) const
{
  return
      text ||
      payment != Payment::Undefined ||
      currency ||
      minimum ||
      initial ||
      unit ||
      per_unit;
}

bool price_t::operator ==(const price_t& o) const
{
  return
      text == o.text &&
      currency == o.currency &&
      payment == o.payment &&
      minimum == o.minimum &&
      initial == o.initial &&
      unit == o.unit &&
      per_unit == o.per_unit;
}

void price_t::incorporate(const price_t& o)
{
  incorporate_optional(text, o.text);
  payment |= o.payment;
  incorporate_optional(currency, o.currency);
  incorporate_optional(minimum, o.minimum);
  incorporate_optional(initial, o.initial);
  incorporate_optional(unit, o.unit);
  incorporate_optional(per_unit, o.per_unit);
}


// === contact_t ===
contact_t::operator bool(void) const
{
  return
      street_number ||
      street_name ||
      city ||
      state ||
      country ||
      postal_code;
}

bool contact_t::operator ==(const contact_t& o) const
{
  return
      street_number == o.street_number &&
      street_name == o.street_name &&
      city == o.city &&
      state == o.state &&
      country == o.country &&
      postal_code == o.postal_code &&
      phone_number == o.phone_number &&
      URL == o.URL;
}

void contact_t::incorporate(const contact_t& o)
{
  incorporate_optional(street_number, o.street_number);
  incorporate_optional(street_name, o.street_name);
  incorporate_optional(city, o.city);
  incorporate_optional(state, o.state);
  incorporate_optional(country, o.country);
  incorporate_optional(postal_code, o.postal_code);
  incorporate_optional(phone_number, o.phone_number);
  incorporate_optional(URL, o.URL);
}

// === operation_schedule_t ===

schedule_t::operator std::string(void) const
{
  std::string out;
  for(const auto& dayofweek : days)
  {
    if(!out.empty())
      out.push_back(';');
    if(dayofweek)
      out += std::to_string(dayofweek->first) + "," +
             std::to_string(dayofweek->second);
  }
  return out;
}

schedule_t& schedule_t::operator =(const std::string& input)
{
  int daynum = 0;
  for(const auto& dayofweek : ext::string(input).split_string({';'}))
  {
    auto comma = dayofweek.find_first_of(',');
    days[daynum].reset();
    if(comma == std::string::npos)
      days[daynum].emplace(ext::from_string<int32_t>(dayofweek.substr(0, comma - 1)),
                           ext::from_string<int32_t>(dayofweek.substr(comma + 1)));
  }
  return *this;
}

schedule_t::operator bool(void) const
  { return std::any_of(std::begin(days), std::end(days), [](const std::optional<hours_t>& i) { return bool(i); }); }

bool schedule_t::operator ==(const schedule_t& o) const
  { return days == o.days; }

void schedule_t::incorporate(const schedule_t& o)
{
  for(std::size_t i = 0; i < days.size(); ++i)
    incorporate_optional(days[i], o.days[i]);
}

// === station_t ===
void station_t::incorporate(const station_t& o)
{
  power.incorporate(o.power);
  contact.incorporate(o.contact);
  price.incorporate(o.price);

  incorporate_optional(name, o.name);
  incorporate_optional(description, o.description);
  incorporate_optional(access_public, o.access_public);
  incorporate_optional(restrictions, o.restrictions);
  incorporate_optional(name, o.name);

  ports.insert(std::end(ports),
               std::make_move_iterator(std::begin(o.ports)),
               std::make_move_iterator(std::end(o.ports)));

  ports.sort();
  ports.erase(std::unique(std::begin(ports), std::end(ports)), std::end(ports));
}


std::ostream & operator << (std::ostream &out, const Payment value) noexcept
{
  if(value == Payment::Undefined)
    out << "null";
  else
  {
    auto v = static_cast<typename std::underlying_type_t<const Payment>>(value);
    if(v & static_cast<typename std::underlying_type_t<Payment>>(Payment::Free))
      out << "Free ";
    if(v & static_cast<typename std::underlying_type_t<Payment>>(Payment::Cheque))
      out << "Cheque ";
    if(v & static_cast<typename std::underlying_type_t<Payment>>(Payment::Credit))
      out << "Credit ";
    if(v & static_cast<typename std::underlying_type_t<Payment>>(Payment::RFID))
      out << "RFID ";
    if(v & static_cast<typename std::underlying_type_t<Payment>>(Payment::API))
      out << "API ";
    if(v & static_cast<typename std::underlying_type_t<Payment>>(Payment::Website))
      out << "Website ";
    if(v & static_cast<typename std::underlying_type_t<Payment>>(Payment::PhoneCall))
      out << "PhoneCall ";
  }
  return out;
}

std::ostream & operator << (std::ostream &out, const Unit value) noexcept
{
  switch(value)
  {
    case Unit::WattHours: out << "WattHours"; break;
    case Unit::KilowattHours: out << "KilowattHours"; break;
    case Unit::Minutes: out << "Minutes"; break;
    case Unit::Hours: out << "Hours"; break;
  }
  return out;
}

std::ostream & operator << (std::ostream &out, const Currency value) noexcept
{
  switch(value)
  {
    case Currency::USD: out << "USD"; break;
    case Currency::CND: out << "CND"; break;
    case Currency::Euro: out << "Euro"; break;
    case Currency::Pound: out << "Pound"; break;
  }
  return out;
}

std::ostream & operator << (std::ostream &out, const Connector value) noexcept
{
  if(value == Connector::Unknown)
    out << "null";
  else
  {
    auto v = static_cast<typename std::underlying_type_t<const Connector>>(value);
    if(v & static_cast<typename std::underlying_type_t<Connector>>(Connector::Nema515))
      out << "Nema515 ";
    if(v & static_cast<typename std::underlying_type_t<Connector>>(Connector::Nema520))
      out << "Nema520 ";
    if(v & static_cast<typename std::underlying_type_t<Connector>>(Connector::Nema1450))
      out << "Nema1450 ";
    if(v & static_cast<typename std::underlying_type_t<Connector>>(Connector::CHAdeMO))
      out << "CHAdeMO ";
    if(v & static_cast<typename std::underlying_type_t<Connector>>(Connector::J1772))
      out << "J1772 ";
    if(v & static_cast<typename std::underlying_type_t<Connector>>(Connector::CCS1))
      out << "CCS1 ";
    if(v & static_cast<typename std::underlying_type_t<Connector>>(Connector::CCS2))
      out << "CCS2 ";
    if(v & static_cast<typename std::underlying_type_t<Connector>>(Connector::Tesla))
      out << "Tesla ";
  }
  return out;
}


std::ostream & operator << (std::ostream &out, const contact_t& value) noexcept
{
  if(!value)
    out << "Contact: null" << std::endl;
  else
    out << "Contact:" << std::endl
        << "contact_id:    " << value.contact_id << std::endl
        << "street_number: " << value.street_number << std::endl
        << "street_name:   " << value.street_name << std::endl
        << "city:          " << value.city << std::endl
        << "state:         " << value.state << std::endl
        << "country:       " << value.country << std::endl
        << "postal_code:   " << value.postal_code << std::endl
        << "phone_number:  " << value.phone_number << std::endl
        << "URL:           " << value.URL << std::endl;
  return out;
}

std::ostream & operator << (std::ostream &out, const price_t& value) noexcept
{
  if(!value)
    out << "Price: null" << std::endl;
  else
    out << "Price:" << std::endl
        << "price_id: " << value.price_id << std::endl
        << "text:     " << value.text << std::endl
        << "payment:  " << value.payment << std::endl
        << "currency: " << value.currency << std::endl
        << "minimum:  " << value.minimum << std::endl
        << "initial:  " << value.initial << std::endl
        << "unit:     " << value.unit << std::endl
        << "per_unit: " << value.per_unit << std::endl;
  return out;
}

std::ostream & operator << (std::ostream &out, const power_t& value) noexcept
{
  if(!value)
    out << "Power: null" << std::endl;
  else
    out << "Power:" << std::endl
        << "power_id:   " << value.power_id << std::endl
        << "level:      " << value.level << std::endl
        << "connector:  " << value.connector << std::endl
        << "amp:        " << value.amp << std::endl
        << "kw:         " << value.kw << std::endl
        << "volt:       " << value.volt << std::endl;
  return out;
}
