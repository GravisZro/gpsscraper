#ifndef DBINTERFACE_H
#define DBINTERFACE_H

#include <string_view>
#include <optional>
#include <list>
#include <set>
#include <simplified/simple_sqlite.h>

#include <scrapers/scraper_types.h>

class DBInterface
{
public:
  DBInterface(std::string_view filename);
  ~DBInterface(void);

  void addMapLocation(const pair_data_t& data);
  void addURL     (const std::optional<std::string>& URL);
  void addContact (const contact_t& contact);
  void addPrice   (const price_t& price);
  void addPower   (const power_t& power);
  void addHours   (const std::optional<schedule_t::hours_t>& hours);
  void addSchedule(const schedule_t& schedule);
  void addPort    (port_t& port); // fills in port.power.power_id and port.price.price_id
  void addStation (station_t& station); // fills in station.ports[].port_id and station.schedule.schedule_id

  //void updateMapLocation(const pair_data_t& data);

  std::optional<uint64_t> identifyMapLocation(const pair_data_t& data);
  std::optional<uint64_t> identifyURL(const std::optional<std::string>& URL);
  std::optional<uint64_t> identifyContact(const contact_t& contact);
  std::optional<uint64_t> identifyPrice(const price_t& price);
  std::optional<uint64_t> identifyPower(const power_t& power);
  std::optional<uint64_t> identifyHours   (const std::optional<schedule_t::hours_t>& hours);
  std::optional<uint64_t> identifySchedule(const schedule_t& schedule);

  std::list<pair_data_t> getMapLocationCache(const pair_data_t& data);
  std::optional<std::string> getURL(const std::optional<uint64_t> URL_id);
  contact_t getContact(const std::optional<uint64_t> contact_id);
  price_t getPrice(const std::optional<uint64_t> price_id);
  power_t getPower(const std::optional<uint64_t> power_id);
  std::optional<schedule_t::hours_t> getHours(const std::optional<uint64_t> hours_id);
  schedule_t getSchedule(const std::optional<uint64_t> schedule_id);
  port_t getPort(Network network_id, uint64_t port_id);
  station_t getStation(uint64_t contact_id);
  station_t getStation(Network network_id, uint64_t station_id);
  station_t getStation(double latitude, double longitude);

private:
  std::list<query_info_t> getChildLocations(Network network_id, query_info_t qinfo);
  std::list<query_info_t> recurseChildLocations(Network network_id, query_info_t qinfo, std::set<uint64_t>& node_ids);
  station_t getStation(sql::query&& q);
  sql::db m_db;
};

#endif // DBINTERFACE_H
