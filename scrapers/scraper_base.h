#ifndef SCRAPER_BASE_H
#define SCRAPER_BASE_H

#include "utilities.h"
#include <unordered_set>
#include <optional>

std::string url_encode(const std::string& source, std::unordered_set<char> exceptions = std::unordered_set<char>()) noexcept;
std::string unescape(const std::string& source) noexcept;

struct station_info_t
{
  uint32_t station_id;
  double latitude;
  double longitude;

  std::string name;
  std::string street_number;
  std::string street_name;
  std::string city;
  std::string state;
  std::string country;
  std::string zipcode;

  std::optional<std::string> phone_number;
  std::optional<std::string> times_accessible;
  std::optional<std::string> price_string;
  std::optional<std::string> payment_types;

  std::optional<uint8_t> CHAdeMO;
  std::optional<uint8_t> JPlug;
  std::optional<uint8_t> J1772_combo;

  // discarded info
  std::string url;
  std::string post_data;

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
