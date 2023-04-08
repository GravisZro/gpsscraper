#include "scraper_types.h"

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
  if(a && (!a && a->empty()))
    a = b;
  else if(a && b && !a->empty() && !b->empty())
    assert(a == b);
  else
    a.reset();
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
schedule_t::operator bool(void) const
  { return days[0] || days[1] || days[2] || days[3] || days[4] || days[5] || days[6]; }

bool schedule_t::operator ==(const schedule_t& o) const
{
  return
      days[0] == o.days[0] &&
      days[1] == o.days[1] &&
      days[2] == o.days[2] &&
      days[3] == o.days[3] &&
      days[4] == o.days[4] &&
      days[5] == o.days[5] &&
      days[6] == o.days[6];
}

void schedule_t::incorporate(const schedule_t& o)
{
  incorporate_optional(days[0], o.days[0]);
  incorporate_optional(days[1], o.days[1]);
  incorporate_optional(days[2], o.days[2]);
  incorporate_optional(days[3], o.days[3]);
  incorporate_optional(days[4], o.days[4]);
  incorporate_optional(days[5], o.days[5]);
  incorporate_optional(days[6], o.days[6]);
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
