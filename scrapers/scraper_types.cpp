#include "scraper_types.h"

#include <iostream>
#include <algorithm>
#include <cassert>

#include "utilities.h"


//constexpr std::string_view dbfile = "conflicts.db";

std::ostream& operator << (std::ostream &out, const std::pair<int32_t, int32_t>& value) noexcept
{
  out << "{ " << value.first << ", " << value.second << " }";
  return out;
}

template<typename T>
void incorporate_optional(std::optional<T>& a, const std::optional<T>& b)
{
  if(!a && b)
    a = b;
  else if(a && b && *a != *b)
    std::cout << "mismatched values: \"" << *a << "\" vs \"" << *b << "\"" << std::endl;
}

template<>
void incorporate_optional(std::optional<std::string>& a, const std::optional<std::string>& b)
{
  if(!a || (a && a->empty())) // 'a' not set or 'a' empty
    a = b;
  else if(a && b && a != b) // both are set and mismatch
  {
    if(!std::any_of(std::begin(*a), std::end(*a), [](const char c) { return std::islower(c); } )) // if no lowercase chars (all caps alpha characters)
      a = b;
    else if(a->find("The EVgo") == 0 ||
            a->find("Removed") != std::string::npos ||
            false)
    {
      ext::string first = *a, second = *b;
      while(first.replace("\n", "\\n"));
      while(second.replace("\n", "\\n"));
      std::cout << "prefered: \"" << first << "\" over \"" << second << "\"" << std::endl;
    }
    else if(a->find("Puissance") != std::string::npos ||
            a->find("heure") != std::string::npos ||
            a->find("Énergie") == 0 ||
            a->find("null") == 0 ||
            a->find("LUNDI") == 0 ||
            a->find("Temps") == 0 ||
            a->find("Stationnement") == 0 ||
            a->find("Tarification") == 0 ||
            a->find("stationnement") != std::string::npos ||

            a->find("Boulevard") == 0 ||
            a->find("boulevard") == 0 ||
            a->find("Boul.") == 0 ||
            a->find("boul.") == 0 ||
            a->find("boul ") == 0 ||
            a->find("Boul ") == 0 ||

            a->find("avenue") == 0 ||
            a->find("Avenue") == 0 ||
            a->find("Av.") == 0 ||
            a->find("av.") == 0 ||

            (a->find("NW") != std::string::npos && b->find("Northwest") != std::string::npos) ||
            (a->find("NE") != std::string::npos && b->find("Northeast") != std::string::npos) ||
            (a->find("SW") != std::string::npos && b->find("Southwest") != std::string::npos) ||
            (a->find("SE") != std::string::npos && b->find("Southeast") != std::string::npos) ||

            (a->find("Hwy"  ) != std::string::npos && b->find("Highway"   ) != std::string::npos) ||
            (a->find("Rd"   ) != std::string::npos && b->find("Road"      ) != std::string::npos) ||
            (a->find("Ln"   ) != std::string::npos && b->find("Lane"      ) != std::string::npos) ||
            (a->find("Pkwy" ) != std::string::npos && b->find("Parkway"   ) != std::string::npos) ||
            (a->find("Blvd" ) != std::string::npos && b->find("Boulevard" ) != std::string::npos) ||

            (a->find("Dr"   ) != std::string::npos && a->find("Drive" ) == std::string::npos && b->find("Drive" ) != std::string::npos) ||
            (a->find("St"   ) != std::string::npos && a->find("Street") == std::string::npos && b->find("Street") != std::string::npos) ||
            (a->find("Ave"  ) != std::string::npos && a->find("Avenue") == std::string::npos && b->find("Avenue") != std::string::npos) ||
            (a->find("Cir"  ) != std::string::npos && a->find("Circle") == std::string::npos && b->find("Circle") != std::string::npos) ||

            (a->find("E") != std::string::npos && a->find("East"  ) == std::string::npos && b->find("East"  ) != std::string::npos) ||
            (a->find("W") != std::string::npos && a->find("West"  ) == std::string::npos && b->find("West"  ) != std::string::npos) ||
            (a->find("N") != std::string::npos && a->find("North" ) == std::string::npos && b->find("North" ) != std::string::npos) ||
            (a->find("S") != std::string::npos && a->find("South" ) == std::string::npos && b->find("South" ) != std::string::npos) ||
            false)
    {
      ext::string first = *a, second = *b;
      while(first.replace("\n", "\\n"));
      while(second.replace("\n", "\\n"));
      std::cout << "prefered: \"" << second << "\" over \"" << first << "\"" << std::endl;
      a = b;
    }
    else if (a->at(0) == ' ' ||
             std::islower(a->at(0)) ||
             (a->find('|') != std::string::npos && b->find('|') == std::string::npos) ||
             (a->find('-') != std::string::npos && a->find(' ') == std::string::npos) ||
             false)
    {
      ext::string first = *a, second = *b;
      while(first.replace("\n", "\\n"));
      while(second.replace("\n", "\\n"));
      std::cout << "ambiguous preference: \"" << second << "\" over \"" << first << "\"" << std::endl;
      a = b;
    }
    else
    {
      ext::string first = *a, second = *b;
      while(first.replace("\n", "\\n"));
      while(second.replace("\n", "\\n"));
      std::cout << "mismatched strings: \"" << first << "\" vs \"" << second << "\"" << std::endl;
    }
  }
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
  std::string out = raw_string;
  if(out.empty())
    for(int daynum = 0; const auto& dayofweek : week)
    {
      if(daynum++)
        out.push_back(',');
      if(dayofweek)
        out += std::to_string(dayofweek->first) + "|" +
               std::to_string(dayofweek->second);
    }
  return out;
}

schedule_t& schedule_t::operator =(const std::optional<std::string>& input)
{
  raw_string.clear();
  for(auto& day : week)
    day.reset();

  if(input)
  {
    auto days_list = ext::to_list(input, ',');
    if(days_list.size() != 7 ||
       std::any_of(std::begin(*input), std::end(*input), [](const char c) { return std::isalpha(c); } ))
      raw_string = *input;
    else
    {
      for(int daynum = 0; const auto& dayofweek : days_list)
      {
        size_t splitter = dayofweek.find_first_of('|');
        if(splitter != std::string::npos)
          week[daynum].emplace(ext::from_string<int32_t>(dayofweek.substr(0, splitter)),
                               ext::from_string<int32_t>(dayofweek.substr(splitter + 1)));
        ++daynum;
      }
    }
  }

  return *this;
}

schedule_t::operator bool(void) const
  { return std::any_of(std::begin(week), std::end(week), [](const std::optional<hours_t>& i) { return bool(i); }); }

bool schedule_t::operator ==(const schedule_t& o) const
  { return week == o.week; }

void schedule_t::incorporate(const schedule_t& o)
{
  if(!raw_string.empty() && bool(o))
  {
    raw_string.clear();
    *this = o;
  }
  if(bool(*this) && bool(o) && *this != o)
    std::cout << "mismatched scedules!: \"" << *this << "\" vs \"" << o << "\"" << std::endl;
}

// === station_t ===
template<typename T>
void incorporate_list(std::list<T>&a, const std::list<T>& b)
{
  a.insert(std::end(a),
               std::make_move_iterator(std::begin(b)),
               std::make_move_iterator(std::end(b)));

  a.sort();
  a.erase(std::unique(std::begin(a), std::end(a)), std::end(a));
}

void station_t::incorporate(const station_t& o)
{
  incorporate_list(meta_network_ids, o.meta_network_ids);
  incorporate_list(meta_station_ids, o.meta_station_ids);

  if(network_id && o.network_id && network_id == Network::Unknown)
    network_id = o.network_id;

  incorporate_optional(network_id, o.network_id);
  incorporate_optional(station_id, o.station_id);

  power.incorporate(o.power);
  contact.incorporate(o.contact);
  price.incorporate(o.price);

  incorporate_optional(name, o.name);
  incorporate_optional(description, o.description);

  // custom merge
  if(!access_public && o.access_public)
    access_public = o.access_public;
  else if(access_public && o.access_public &&
          (!*access_public || !*o.access_public))
    access_public = false;
  //incorporate_optional(access_public, o.access_public);

  incorporate_optional(restrictions, o.restrictions);
  incorporate_optional(name, o.name);


  for(const auto& port : o.ports)
  {
    auto loc = std::find(std::begin(ports), std::end(ports), port);
    if(loc != std::end(ports))
      loc->incorporate(port);
    else
      ports.push_back(port);
  }
}


void port_t::incorporate(const port_t& o)
{
  incorporate_optional(network_id, o.network_id);
  incorporate_optional(station_id, o.station_id);
  incorporate_optional(port_id, o.port_id);
  incorporate_optional(status, o.status);

  power.incorporate(o.power);
  contact.incorporate(o.contact);
  price.incorporate(o.price);

  incorporate_optional(display_name, o.display_name);
}

std::ostream& operator << (std::ostream &out, const Network value) noexcept
{
  switch(value)
  {
    case Network::Non_Networked: out << "Non_Networked"; break;
    case Network::Unknown: out << "Unknown"; break;
    case Network::AmpUp: out << "AmpUp"; break;
    case Network::Astria: out << "Astria"; break;
    case Network::Azra: out << "Azra"; break;
    case Network::BC_Hydro_EV: out << "BC_Hydro_EV"; break;
    case Network::Blink: out << "Blink"; break;
    case Network::ChargeLab: out << "ChargeLab"; break;
    case Network::ChargePoint: out << "ChargePoint"; break;
    case Network::Circuit_Électrique: out << "Circuit_Électrique"; break;
    case Network::CityVitae: out << "CityVitae"; break;
    case Network::Coop_Connect: out << "Coop_Connect"; break;
    case Network::eCharge: out << "eCharge"; break;
    case Network::EcoCharge: out << "EcoCharge"; break;
    case Network::Electrify_America: out << "Electrify_America"; break;
    case Network::Electrify_Canada: out << "Electrify_Canada"; break;
    case Network::EV_Link: out << "EV_Link"; break;
    case Network::EV_Range: out << "EV_Range"; break;
    case Network::EVConnect: out << "EVConnect"; break;
    case Network::EVCS: out << "EVCS"; break;
    case Network::EVduty: out << "EVduty"; break;
    case Network::evGateway: out << "evGateway"; break;
    case Network::EVgo: out << "EVgo"; break;
    case Network::EVMatch: out << "EVMatch"; break;
    case Network::EVolve_NY: out << "EVolve_NY"; break;
    case Network::EVSmart: out << "EVSmart"; break;
    case Network::Flash: out << "Flash"; break;
    case Network::Flo: out << "Flo"; break;
    case Network::FPL_Evolution: out << "FPL_Evolution"; break;
    case Network::Francis_Energy: out << "Francis_Energy"; break;
    case Network::GE: out << "GE"; break;
    case Network::Go_Station: out << "Go_Station"; break;
    case Network::Hypercharge: out << "Hypercharge"; break;
    case Network::Ivy: out << "Ivy"; break;
    case Network::Livingston_Energy: out << "Livingston_Energy"; break;
    case Network::myEVroute: out << "myEVroute"; break;
    case Network::NL_Hydro: out << "NL_Hydro"; break;
    case Network::Noodoe_EV: out << "Noodoe_EV"; break;
    case Network::OK2Charge: out << "OK2Charge"; break;
    case Network::On_the_Run: out << "On_the_Run"; break;
    case Network::OpConnect: out << "OpConnect"; break;
    case Network::Petro_Canada: out << "Petro_Canada"; break;
    case Network::PHI: out << "PHI"; break;
    case Network::Powerflex: out << "Powerflex"; break;
    case Network::RED_E: out << "RED_E"; break;
    case Network::Rivian: out << "Rivian"; break;
    case Network::SemaConnect: out << "SemaConnect"; break;
    case Network::Shell_Recharge: out << "Shell_Recharge"; break;
    case Network::Sun_Country_Highway: out << "Sun_Country_Highway"; break;
    case Network::SWTCH: out << "SWTCH"; break;
    case Network::SYNC_EV: out << "SYNC_EV"; break;
    case Network::Tesla: out << "Tesla"; break;
    case Network::Universal: out << "Universal"; break;
    case Network::Voita: out << "Voita"; break;
    case Network::Webasto: out << "Webasto"; break;
    case Network::ZEF_Energy: out << "ZEF_Energy"; break;
    case Network::ChargeHub: out << "ChargeHub"; break;
    case Network::Eptix: out << "Eptix"; break;
  }
  return out;
}

std::ostream& operator << (std::ostream &out, const Status value) noexcept
{
  switch(value)
  {
    case Status::Unknown: out << "Unknown"; break;
    case Status::Operational: out << "Operational"; break;
    case Status::InUse: out << "InUse"; break;
    case Status::InMaintaince: out << "InMaintaince"; break;
    case Status::NonFunctional: out << "NonFunctional"; break;
    case Status::PlannedSite: out << "PlannedSite"; break;
  }
  return out;
}

std::ostream& operator << (std::ostream &out, const Payment value) noexcept
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

std::ostream& operator << (std::ostream &out, const Unit value) noexcept
{
  switch(value)
  {
    case Unit::Unknown:       out << "Unknown"; break;
    case Unit::Free:          out << "Free"; break;
    case Unit::Session:       out << "Session"; break;
    case Unit::Hours:         out << "Hours"; break;
    case Unit::KilowattHours: out << "KilowattHours"; break;
    case Unit::SeeText:       out << "SeeText"; break;
    case Unit::Minutes:       out << "Minutes"; break;
    case Unit::WattHours:     out << "WattHours"; break;
  }
  return out;
}

std::ostream& operator << (std::ostream &out, const Currency value) noexcept
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

std::ostream& operator << (std::ostream &out, const Connector value) noexcept
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


std::ostream& operator << (std::ostream &out, const contact_t& value) noexcept
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

std::ostream& operator << (std::ostream &out, const price_t& value) noexcept
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

std::ostream& operator << (std::ostream &out, const power_t& value) noexcept
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
