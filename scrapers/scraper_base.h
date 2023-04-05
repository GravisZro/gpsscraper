#ifndef SCRAPER_BASE_H
#define SCRAPER_BASE_H

#include <unordered_set>
#include <string>
#include <vector>

#include "scraper_types.h"

std::string url_encode(const std::string& source, std::unordered_set<char> exceptions = std::unordered_set<char>()) noexcept;
std::string unescape(const std::string& source) noexcept;

struct pair_data_t;
class ScraperBase
{
public:
  virtual ~ScraperBase(void) = default;

  virtual pair_data_t BuildQuery(const pair_data_t& input) = 0;
  virtual std::vector<pair_data_t> Parse(const pair_data_t& data, const std::string& input) = 0;
};

#endif // SCRAPER_BASE_H
