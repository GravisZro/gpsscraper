#ifndef DBINTERFACE_H
#define DBINTERFACE_H

#include <string_view>
#include <optional>
#include <list>
#include <simplified/simple_sqlite.h>

#include <scrapers/scraper_types.h>

class DBInterface
{
public:
  DBInterface(std::string_view filename);
  ~DBInterface(void);

  void addMapLocation(const pair_data_t& data);
  void addUniqueString(const std::optional<std::string>& string);
  void addContact (const contact_t& contact);
  void addPrice   (const price_t& price);
  void addPower   (const power_t& power);
  void addPort    (port_t& port); // fills in port.power.power_id and port.price.price_id
  void addStation (station_t& station); // fills in station.ports[].port_id and station.schedule.schedule_id

  std::optional<std::string> identifyMapLocation(const pair_data_t& data);
  std::optional<uint64_t> identifyUniqueString(const std::optional<std::string>& string);
  std::optional<uint64_t> identifyContact(const contact_t& contact);
  std::optional<uint64_t> identifyPrice(const price_t& price);
  std::optional<uint64_t> identifyPower(const power_t& power);

  std::optional<pair_data_t> getMapLocation(Network network_id, const std::string& node_id);
  std::optional<std::string> getUniqueString(const std::optional<uint64_t> string_id);
  contact_t getContact(const std::optional<uint64_t> contact_id);
  price_t getPrice(const std::optional<uint64_t> price_id);
  power_t getPower(const std::optional<uint64_t> power_id);
  port_t getPort(Network network_id, const std::string& port_id);
  station_t getStation(uint64_t contact_id);
  station_t getStation(Network network_id, const std::string& station_id);
  station_t getStation(coords_t location);

private:
  station_t getStation(sql::query&& q);
  sql::db m_db;
};

#endif // DBINTERFACE_H
