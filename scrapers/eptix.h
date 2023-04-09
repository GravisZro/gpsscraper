#ifndef EPTIX_H
#define EPTIX_H

#include "scraper_base.h"

class EptixScraper : public ScraperBase
{
public:
  pair_data_t BuildQuery(const pair_data_t& input);
  std::vector<pair_data_t> Parse(const pair_data_t& data, const std::string& input);
};


#endif // EPTIX_H
