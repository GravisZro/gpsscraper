#ifndef CHARGEHUB_SCRAPER_H
#define CHARGEHUB_SCRAPER_H

#include <scrapers/scraper_base.h>

class ChargehubScraper : public ScraperBase
{
public:
  ChargehubScraper(void) noexcept = default;

  uint8_t StageCount(void) const noexcept { return 2; }

  std::string IndexURL(void) const noexcept;
  bool IndexingComplete(void) const noexcept;

  std::list<station_info_t> ParseIndex(const ext::string& input);
  std::list<station_info_t> ParseStation(const station_info_t& station_info, const ext::string& input);

private:
  mutable double m_latitude;
};



#endif // CHARGEHUB_SCRAPER_H
