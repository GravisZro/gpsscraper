#ifndef CHARGEHUB_H
#define CHARGEHUB_H

#include <scrapers/scraper_base.h>

class ChargehubScraper : public ScraperBase
{
public:
  void classify(pair_data_t& record) const;
  pair_data_t BuildQuery(const pair_data_t& input) const;
  std::vector<pair_data_t> Parse(const pair_data_t& data, const std::string& input) const;

private:
  std::vector<pair_data_t> ParserInit(const pair_data_t& data) const;
  std::vector<pair_data_t> ParseMapArea(const std::string& input) const;
  std::vector<pair_data_t> ParseStation(const pair_data_t& data, const std::string& input) const;
};



#endif // CHARGEHUB_H
