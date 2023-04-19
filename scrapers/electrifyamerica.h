#ifndef ELECTRIFYAMERICA_H
#define ELECTRIFYAMERICA_H

#include "scraper_base.h"

class ElectrifyAmericaScraper : public ScraperBase
{
public:
  void classify([[maybe_unused]] pair_data_t& record) const {}
  std::vector<pair_data_t> Parse(const pair_data_t& data, const std::string& input) const;
  pair_data_t BuildQuery(const pair_data_t& data) const;

private:
  std::vector<pair_data_t> ParseMapArea(const pair_data_t& data, const std::string& input) const;
  std::vector<pair_data_t> ParseStation(const pair_data_t& data, const std::string& input) const;
};

#endif // ELECTRIFYAMERICA_H
