#ifndef ELECTRIFYSCRAPER_H
#define ELECTRIFYSCRAPER_H

#include "scraper_base.h"

class ElectrifyScraper : public ScraperBase
{
public:
  std::vector<pair_data_t> Parse(const pair_data_t& data, const std::string& input);
  pair_data_t BuildQuery(const pair_data_t& data);

private:
  std::vector<pair_data_t> ParseMapArea(const pair_data_t& data, const std::string& input);
  std::vector<pair_data_t> ParseStation(const pair_data_t& data, const std::string& input);
};

#endif // ELECTRIFYSCRAPER_H
