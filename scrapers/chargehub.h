#ifndef CHARGEHUB_H
#define CHARGEHUB_H

#include <scrapers/scraper_base.h>

class ChargehubScraper : public ScraperBase
{
public:
  ChargehubScraper(double starting_latitude = 12.5,
                   double stopping_latitude = 55.0) noexcept;

  pair_data_t BuildQuery(const pair_data_t& input);
  std::vector<pair_data_t> Parse(const pair_data_t& data, const std::string& input);

private:
  std::vector<pair_data_t> ParserInit(void);
  std::vector<pair_data_t> ParseMapArea(const std::string& input);
  std::vector<pair_data_t> ParseStation(const pair_data_t& data, const std::string& input);

private:
  static constexpr double m_latitude_step = 0.25;
  const double m_start_latitude;
  const double m_end_latitude;
};



#endif // CHARGEHUB_H
