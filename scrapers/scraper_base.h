#ifndef SCRAPER_BASE_H
#define SCRAPER_BASE_H

#include "utilities.h"
#include <unordered_set>
#include <optional>
#include <stack>

std::string url_encode(const std::string& source, std::unordered_set<char> exceptions = std::unordered_set<char>()) noexcept;
std::string unescape(const std::string& source) noexcept;

struct port_t
{
  std::optional<int32_t> port_id;
  std::optional<int32_t> network_port_id;

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


struct progress_info_t
{
  uint8_t steps_remaining;
  std::string details_URL;
  std::string post_data;
  std::list<std::pair<std::string, std::string>> header_fields;
};

struct station_info_t : progress_info_t
{
  station_info_t(void) = default;
  station_info_t(progress_info_t&& other) : progress_info_t (other) {}

  std::optional<int32_t> station_id;
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
  std::optional<int32_t> network_id;

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

  virtual std::stack<station_info_t> Init(void) { return Parse(progress_info_t { .steps_remaining = 0xFF }, ""); }
  virtual std::stack<station_info_t> Parse(const station_info_t& station_info, const ext::string& input) = 0;
};

#endif // SCRAPER_BASE_H
