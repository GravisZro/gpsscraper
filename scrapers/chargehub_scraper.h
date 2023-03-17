#ifndef CHARGEHUB_SCRAPER_H
#define CHARGEHUB_SCRAPER_H

#include <scrapers/scraper_base.h>


enum plugs : uint8_t
{
  Nema515 = 0,
  Nema520,
  Nema1450,
  CHAdeMO,
  J1772,
  J1772Combo,
  CCS,
  CCSorCHAdeMO,
  TeslaPlug,
  Max
};

enum network : uint8_t
{
  Non_Networked = 17,
  Unknown = 0,
  AmpUp = 42,
  Astria = 15,
  Azra = 14,
  BC_Hydro_EV = 27,
  Blink = 1,
  ChargeLab = 31,
  ChargePoint = 2,
  Circuit_Électrique = 4,
  CityVitae = 52,
  Coop_Connect = 47,
  eCharge =23,
  EcoCharge = 37,
  Electrify_America = 26,
  Electrify_Canada = 36,
  EV_Link = 18,
  EV_Range = 54,
  EVConnect = 22,
  EVCS = 43,
  EVduty = 20,
  evGateway = 44,
  EVgo = 8,
  EVMatch = 35,
  EVolve_NY = 38,
  EVSmart = 45,
  Flash = 59,
  Flo = 3,
  FPL_Evolution = 41,
  Francis_Energy = 39,
  GE = 16,
  Go_Station = 48,
  Hypercharge = 49,
  Ivy = 34,
  Livingston_Energy = 46,
  myEVroute = 21,
  NL_Hydro = 53,
  Noodoe_EV = 30,
  OK2Charge = 55,
  On_the_Run = 58,
  OpConnect = 9,
  Petro_Canada = 32,
  PHI = 50,
  Powerflex = 40,
  RED_E = 60,
  Rivian = 51,
  SemaConnect = 5,
  Shell_Recharge = 6,
  Sun_Country_Highway = 11,
  SWTCH = 28,
  SYNC_EV = 33,
  Tesla = 12,
  Universal = 56,
  Voita = 19,
  Webasto = 7,
  ZEF_Energy = 25,
};

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
