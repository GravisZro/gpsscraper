#ifndef EPTIX_H
#define EPTIX_H

#include "scraper_base.h"
#include "utilities.h"

class EptixScraper : public ScraperBase
{
public:
  void classify(pair_data_t& record) const;
  pair_data_t BuildQuery(const pair_data_t& input) const;
  std::vector<pair_data_t> Parse(const pair_data_t& data, const std::string& input) const;

private:
  pair_data_t ParseStationNode(const pair_data_t& data, const safenode_t& root) const;
};


#endif // EPTIX_H
