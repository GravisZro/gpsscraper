#ifndef ECHARGE_H
#define ECHARGE_H

#include "scraper_base.h"

class EchargeScraper : public ScraperBase
{
public:
  pair_data_t BuildQuery(const pair_data_t& input) const;
  std::vector<pair_data_t> Parse(const pair_data_t& data, const std::string& input) const;

private:
  std::vector<pair_data_t> ParseMapArea(const std::string& input) const;
  std::vector<pair_data_t> ParseStation(const pair_data_t& data, const std::string& input) const;

};

#endif // ECHARGE_H
