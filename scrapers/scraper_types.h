#ifndef SCRAPER_TYPES_H
#define SCRAPER_TYPES_H

#include <ostream>
#include <optional>
#include <string>
#include <vector>
#include <list>
#include <array>

#include <cstdint>
#include <cmath>

enum class Connector : uint8_t
{
  Unknown   = 0x00,
  Nema515   = 0x01,
  Nema520   = 0x02,
  Nema1450  = 0x03,
  CHAdeMO   = 0x04,
  J1772     = 0x08,
  CCS1      = 0x10,
  CCS2      = 0x20,
  Tesla     = 0x40,
  Pair      = 0x80,
};

enum class Status : uint8_t
{
  Unknown = 0,  // status unknown
  Operational,  // availible for use
  InUse,        // currently in use
  InMaintaince, // being repaired
  Broken,       // broken
  PlannedSite,  // doesn't exist yet
};

enum class Network : uint8_t
{
  Non_Networked = 17,
  Unknown = 0,
  AmpUp = 42,
  Astria = 15,
  Azra = 14,
  BC_Hydro_EV = 27,
  Blink = 1,
  ChargeLab = 31,
  ChargePoint = 2,
  Circuit_Électrique = 4,
  CityVitae = 52,
  Coop_Connect = 47,
  eCharge = 23,
  EcoCharge = 37,
  Electrify_America = 26,
  Electrify_Canada = 36,
  EV_Link = 18,
  EV_Range = 54,
  EVConnect = 22,
  EVCS = 43,
  EVduty = 20,
  evGateway = 44,
  EVgo = 8,
  EVMatch = 35,
  EVolve_NY = 38,
  EVSmart = 45,
  Flash = 59,
  Flo = 3,
  FPL_Evolution = 41,
  Francis_Energy = 39,
  GE = 16,
  Go_Station = 48,
  Hypercharge = 49,
  Ivy = 34,
  Livingston_Energy = 46,
  myEVroute = 21,
  NL_Hydro = 53,
  Noodoe_EV = 30,
  OK2Charge = 55,
  On_the_Run = 58,
  OpConnect = 9,
  Petro_Canada = 32,
  PHI = 50,
  Powerflex = 40,
  RED_E = 60,
  Rivian = 51,
  SemaConnect = 5,
  Shell_Recharge = 6,
  Sun_Country_Highway = 11,
  SWTCH = 28,
  SYNC_EV = 33,
  Tesla = 12,
  Universal = 56,
  Voita = 19,
  Webasto = 7,
  ZEF_Energy = 25,

  ChargeHub = 128,
};

enum class Parser : uint8_t
{
  Discard = 0x00,
  Port,
  Station,
  MapArea,
  Initial,
  Complete,
  BuildQuery = 0x10,
  BuildPort,
  BuildStation,
  BuildMapArea,
  BuildInitial,
  ReplaceRecord = 0x20,
  ReplacePort,
  ReplaceStation,
  ReplaceMapArea,
};

enum class Unit : uint8_t
{
  Unknown = 0,
  Free,
  Session,
  Hours,
  KilowattHours,
  SeeText,
  WattHours,
  Minutes,
};

enum class Currency : uint8_t
{
  USD = 0,
  CND,
  Euro,
  Pound,
};

enum class Payment : uint8_t
{
  Undefined = 0x00,
  Free      = 0x01,
  Cash      = 0x02,
  Cheque    = 0x04,
  Credit    = 0x08,
  RFID      = 0x10,
  API       = 0x20,
  Website   = 0x40,
  PhoneCall = 0x80,
};

struct map_bounds_t;

struct coords_t
{
  double latitude;
  double longitude;
};

struct bound_t
{
  constexpr bound_t(double a = 0.0, double b = 0.0) : max(a < b ? b : a), min(a < b ? a : b) {}

  double max;
  double min;

  constexpr operator bool(void) const { return max != 0.0 && min != 0.0; }

  constexpr void shift(double i) { max += i; min += i; }

  constexpr void scale_centered(double s)
  {
    double half_diff = (max - min) / 2;
    double center = min + half_diff;
    min = center - half_diff * s;
    max = center + half_diff * s;
  }

  constexpr double distance(void) const { return std::abs(max - min); }
};

struct map_bounds_t
{
  bound_t latitude;
  bound_t longitude;

  constexpr coords_t northEast(void) const { return { latitude.max, longitude.max }; }
  constexpr coords_t southWest(void) const { return { latitude.min, longitude.min }; }

  constexpr operator bool(void) const { return latitude && longitude; }

  constexpr coords_t getFocus(void) const
  {
    return coords_t
    {
      latitude .min + (latitude .max - latitude .min) / 2,
      longitude.min + (longitude.max - longitude.min) / 2,
    };
  }

  void setFocus(const coords_t& new_focus)
  {
    coords_t old_focus = getFocus();
    latitude.shift(new_focus.latitude - old_focus.latitude);
    longitude.shift(new_focus.longitude - old_focus.longitude);
  }

  void zoom(const double zoom)
  {
    double factor = 1.0 / std::pow(2.0, zoom);
    latitude.scale_centered(factor);
    longitude.scale_centered(factor);
  }

};

struct query_info_t
{
  query_info_t(void) : parser(Parser::Discard) { }
  Parser parser;
  std::string URL;
  std::string post_data;
  std::list<std::pair<std::string, std::string>> header_fields;
  // map fields
  map_bounds_t bounds;
  std::optional<std::string> node_id;
  std::optional<std::string> child_ids;
};

struct power_t
{
  std::optional<uint64_t>   power_id;
  std::optional<int32_t>    level;
  std::optional<Connector>  connector;
  std::optional<double>     amp;
  std::optional<double>     kw;
  std::optional<double>     volt;

  operator bool(void) const;
  bool operator ==(const power_t& o) const;
  void incorporate(const power_t& o);
};

struct price_t
{
  price_t(void) : payment(Payment::Undefined) { }
  std::optional<uint64_t>     price_id; // ignored in functions
  std::optional<std::string>  text;
  Payment                     payment;
  std::optional<Currency>     currency;
  std::optional<double>       minimum;
  std::optional<double>       initial;
  std::optional<Unit>         unit;
  std::optional<double>       per_unit;

  operator bool(void) const;
  bool operator ==(const price_t& o) const;
  void incorporate(const price_t& o);
};

struct contact_t
{
  std::optional<uint64_t>    contact_id; // ignored in functions
  std::optional<uint32_t>    street_number;
  std::optional<std::string> street_name;
  std::optional<std::string> city;
  std::optional<std::string> state;
  std::optional<std::string> country;
  std::optional<std::string> postal_code;
  std::optional<std::string> phone_number; // ignored in operator bool
  std::optional<std::string> URL; // ignored in operator bool

  operator bool(void) const;
  bool operator ==(const contact_t& o) const;
  void incorporate(const contact_t& o);
};

struct schedule_t
{
  using hours_t = std::pair<int32_t, int32_t>;

  std::optional<uint64_t> schedule_id; // ignored in functions
  std::array<std::optional<hours_t>, 7> days;
  std::string raw_string;

  operator std::string(void) const;
  schedule_t& operator =(const std::string& input);

  operator bool(void) const;
  bool operator ==(const schedule_t& o) const;
  void incorporate(const schedule_t& o);
};


struct port_t
{
  std::optional<Network>  network_id;
  std::optional<std::string> station_id;
  std::optional<std::string> port_id;

  std::optional<Status> status;
  power_t power;
  contact_t contact;
  price_t price;
  std::optional<std::string> display_name;

  bool operator ==(const port_t& o) const { return port_id == o.port_id; }
  bool operator <(const port_t& o) const { return port_id < o.port_id; }
};

struct station_t
{
  std::optional<Network>  network_id;
  std::optional<std::string> station_id;

  coords_t location;

  std::optional<std::string> name;
  std::optional<std::string> description;

  std::optional<bool> access_public;
  std::optional<std::string> restrictions;

  power_t power;
  contact_t contact;
  price_t price;
  schedule_t schedule;

  std::list<port_t> ports;

  void incorporate(const station_t& o);
};


struct pair_data_t
{
  query_info_t query;
  station_t station;
};



template<typename T>
std::ostream & operator << (std::ostream &out, const std::optional<T>& value) noexcept
{
  if(!value)
    out << "null,";
  else
    out << *value;
  return out;
}

std::ostream & operator << (std::ostream &out, const Connector value) noexcept;
std::ostream & operator << (std::ostream &out, const Payment value) noexcept;
std::ostream & operator << (std::ostream &out, const Unit value) noexcept;
std::ostream & operator << (std::ostream &out, const Currency value) noexcept;

std::ostream & operator << (std::ostream &out, const contact_t& value) noexcept;
std::ostream & operator << (std::ostream &out, const price_t& value) noexcept;
std::ostream & operator << (std::ostream &out, const power_t& value) noexcept;


#endif // SCRAPER_TYPES_H
