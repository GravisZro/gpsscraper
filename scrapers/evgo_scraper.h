#ifndef EVGO_SCRAPER_H
#define EVGO_SCRAPER_H


#include <scrapers/scraper_base.h>

class EVGoScraper : public ScraperBase
{
public:
  EVGoScraper(void) noexcept = default;

  std::stack<station_info_t> Init(void);
  std::stack<station_info_t> Parse(const station_info_t& station_info, const ext::string& input);

private:
  std::stack<station_info_t> ParseIndexes(const station_info_t& station_info, const ext::string& input);
  std::stack<station_info_t> ParseStation(const station_info_t& station_info, const ext::string& input);
};

#endif // EVGO_SCRAPER_H
