#ifndef CIRCUITELECTRIQUE_H
#define CIRCUITELECTRIQUE_H

#include "scraper_base.h"

class CircuitElectriqueScraper : public ScraperBase
{
public:
  pair_data_t BuildQuery(const pair_data_t& input);
  std::vector<pair_data_t> Parse(const pair_data_t& data, const std::string& input);
};


#endif // CIRCUITELECTRIQUE_H
