#ifndef SCRAPER_BASE_H
#define SCRAPER_BASE_H

#include "utilities.h"
#include <unordered_set>
#include <optional>
#include <vector>

std::string url_encode(const std::string& source, std::unordered_set<char> exceptions = std::unordered_set<char>()) noexcept;
std::string unescape(const std::string& source) noexcept;

struct port_t
{
  std::optional<int32_t> port_id;

  std::optional<int32_t> level;
  std::optional<int32_t> connector;
  std::optional<std::string> amp;
  std::optional<std::string> kw;
  std::optional<std::string> volt;

  std::optional<std::string> price_string;
  std::optional<std::string> initialization;

  std::optional<int32_t> network_id;
  std::optional<std::string> display_name;
  std::optional<std::string> network_port_id;
  bool weird;
};

struct station_info_t
{
  int32_t station_id;
  std::optional<double> latitude;
  std::optional<double> longitude;

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
  std::optional<std::string> access_times;
  std::optional<std::string> access_type;

  std::optional<std::string> price_string;
  std::optional<std::string> initialization;
  std::optional<int32_t> network_id;

  std::vector<port_t> ports;

  // discarded info
  std::optional<std::string> details_URL;
  std::optional<std::string> post_data;

  constexpr bool operator < (const station_info_t& other) const noexcept { return station_id < other.station_id; }
};

class ScraperBase
{
public:
  virtual ~ScraperBase(void) = default;

  virtual uint8_t StageCount(void) const noexcept = 0;

  virtual std::string IndexURL(void) const noexcept = 0;
  virtual bool IndexingComplete(void) const noexcept { return true; }

  virtual std::list<station_info_t> ParseIndex(const ext::string& input) = 0;
  virtual std::list<station_info_t> ParseStation(const station_info_t& station_info, const ext::string& input);
  virtual std::list<station_info_t> ParseDownload(const station_info_t& station_info, const ext::string& input) const;
};

#endif // SCRAPER_BASE_H
