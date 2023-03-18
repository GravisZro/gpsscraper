#ifndef SCRAPER_BASE_H
#define SCRAPER_BASE_H

#include "utilities.h"
#include <unordered_set>
#include <optional>
#include <stack>

std::string url_encode(const std::string& source, std::unordered_set<char> exceptions = std::unordered_set<char>()) noexcept;
std::string unescape(const std::string& source) noexcept;

enum plugs : uint8_t
{
  Nema515 = 0,
  Nema520,
  Nema1450,
  CHAdeMO,
  J1772,
  J1772Combo,
  CCS,
  CCSorCHAdeMO,
  TeslaPlug,
  Max
};

enum class network : uint8_t
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
};

struct port_t
{
  std::optional<int32_t> port_id;
  std::optional<std::string> network_port_id;

  std::optional<int32_t> level;
  std::optional<int32_t> connector;
  std::optional<std::string> amp;
  std::optional<std::string> kw;
  std::optional<std::string> volt;

  std::optional<bool> price_free;
  std::optional<std::string> price_string;
  std::optional<std::string> initialization;

  std::optional<std::string> display_name;
  bool weird;
};


enum class Parser : uint8_t
{
  Complete = 0x00,
  Station,
  Index,
  Initial = 0xFF
};

struct progress_info_t
{
  Parser parser;
  std::string details_URL;
  std::string post_data;
  std::list<std::pair<std::string, std::string>> header_fields;
};

struct station_info_t : progress_info_t
{
  station_info_t(void) = default;
  station_info_t(progress_info_t&& other) : progress_info_t (other) {}

  std::optional<int32_t> station_id;
  std::optional<network> network_id;
  std::optional<int32_t> network_station_id;

  double latitude;
  double longitude;

  std::optional<std::string> name;
  std::optional<std::string> description;
  std::optional<uint32_t> street_number;
  std::optional<std::string> street_name;
  std::optional<std::string> city;
  std::optional<std::string> state;
  std::optional<std::string> country;
  std::optional<std::string> zipcode;

  std::optional<std::string> phone_number;
  std::optional<std::string> URL;
  //std::optional<std::string> access_times;
  //std::optional<std::string> access_type;
  std::optional<bool> access_public;
  std::optional<std::string> access_restrictions;

  std::optional<bool> price_free;
  std::optional<std::string> price_string;
  std::optional<std::string> initialization;

  std::stack<port_t> ports;

  // discarded info

  constexpr bool operator  < (const station_info_t& other) const noexcept { return latitude  < other.latitude &&
                                                                                   longitude < other.longitude; }
  constexpr bool operator == (const station_info_t& other) const noexcept { return station_id == other.station_id; }
};


class ScraperBase
{
public:
  virtual ~ScraperBase(void) = default;

  virtual std::stack<station_info_t> Init(void) = 0;
  virtual std::stack<station_info_t> Parse(const station_info_t& station_info, const ext::string& input) = 0;
};

#endif // SCRAPER_BASE_H
