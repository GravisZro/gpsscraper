#ifndef CHARGEHUB_SCRAPER_H
#define CHARGEHUB_SCRAPER_H

#include <scrapers/scraper_base.h>

class ChargehubScraper : public ScraperBase
{
public:
  ChargehubScraper(double starting_latitude = 12.5,
                   double stopping_latitude = 55.0) noexcept;

  std::stack<station_info_t> Init(void);
  std::stack<station_info_t> Parse(const station_info_t& station_info, const ext::string& input);

private:
  std::stack<station_info_t> ParseIndex(const ext::string& input);
  std::stack<station_info_t> ParseStation(const station_info_t& station_info, const ext::string& input);

private:
  static constexpr double m_latitude_step = 0.25;
  const double m_start_latitude;
  const double m_end_latitude;
};



#endif // CHARGEHUB_SCRAPER_H
