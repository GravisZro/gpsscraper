#ifndef EVGO_H
#define EVGO_H

#include <scrapers/scraper_base.h>

class EVGoScraper : public ScraperBase
{
public:
  void classify(pair_data_t& record) const;
  pair_data_t BuildQuery(const pair_data_t& data) const;
  std::vector<pair_data_t> Parse(const pair_data_t& data, const std::string& input) const;

private:
  std::vector<pair_data_t> ParseMapArea(const pair_data_t& data, const std::string& input) const;
  std::vector<pair_data_t> ParseStation(const pair_data_t& data, const std::string& input) const;
  std::vector<pair_data_t> ParsePort(const pair_data_t& data, const std::string& input) const;
};

#endif // EVGO_H
