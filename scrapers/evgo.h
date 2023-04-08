#ifndef EVGO_H
#define EVGO_H

#include <scrapers/scraper_base.h>

class EVGoScraper : public ScraperBase
{
public:
  EVGoScraper(void) noexcept = default;

  std::vector<pair_data_t> Parse(const pair_data_t& data, const std::string& input);
  pair_data_t BuildQuery(const pair_data_t& data);

private:
  std::vector<pair_data_t> ParseMapArea(const pair_data_t& data, const std::string& input);
  std::vector<pair_data_t> ParseStation(const pair_data_t& data, const std::string& input);
  std::vector<pair_data_t> ParsePort(const pair_data_t& data, const std::string& input);
};

#endif // EVGO_H
