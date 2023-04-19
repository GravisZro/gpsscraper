#include "dbinterface.h"

// STL
#include <iostream>
#include <algorithm>
#include <vector>
#include <cassert>

// project
#include <scrapers/utilities.h>

#ifdef DATABASE_TOLERANT
# define MUST(x) if(x)
#else
# define MUST(x) assert(x);
#endif

DBInterface::DBInterface(std::string_view filename)
{
  assert(m_db.open(filename));

  const std::list<std::string_view> init_commands =
  {
    "PRAGMA synchronous = OFF",
    "PRAGMA journal_mode = MEMORY",

    R"(
    CREATE TABLE IF NOT EXISTS map_query_cache (
      "network_id"      INTEGER NOT NULL,
      "latitude_max"    REAL NOT NULL CHECK (latitude_max  >  -90.0 AND latitude_max  <  90.0),
      "latitude_min"    REAL NOT NULL CHECK (latitude_min  >  -90.0 AND latitude_min  <  90.0),
      "longitude_max"   REAL NOT NULL CHECK (longitude_max > -180.0 AND longitude_max < 180.0),
      "longitude_min"   REAL NOT NULL CHECK (longitude_min > -180.0 AND longitude_min < 180.0),
      "node_id"         TEXT DEFAULT NULL,
      "child_ids"       TEXT DEFAULT NULL,
      "last_update" TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL
    ) )",

    R"(
      CREATE TRIGGER IF NOT EXISTS before_insert_map_query_cache_no_duplicates
      BEFORE INSERT ON map_query_cache
      WHEN EXISTS (SELECT 1 FROM map_query_cache WHERE
        NEW.network_id IS network_id AND
        NEW.latitude_max IS latitude_max AND
        NEW.latitude_min IS latitude_min AND
        NEW.longitude_max IS longitude_max AND
        NEW.longitude_min IS longitude_min AND
        NEW.node_id IS node_id AND
        NEW.child_ids IS child_ids)
      BEGIN
        SELECT RAISE(ABORT,SQLITE_CONSTRAINT_TRIGGER);
      END
      )",

    R"(
      CREATE TRIGGER IF NOT EXISTS before_insert_map_query_cache_replace
      BEFORE INSERT ON map_query_cache
      WHEN EXISTS
        (SELECT 1 FROM map_query_cache WHERE
          NEW.network_id IS network_id AND
          NEW.node_id IS node_id AND
          (
            (child_ids IS NULL AND NEW.child_ids IS NOT NULL
            ) OR
            (ABS(latitude_max  - latitude_min ) > ABS(NEW.latitude_max - NEW.latitude_min) AND
             ABS(longitude_max - longitude_min) > ABS(NEW.longitude_max - NEW.longitude_min)
            )
          )
        )
      BEGIN
        DELETE FROM map_query_cache
          WHERE
            NEW.network_id IS network_id AND
            NEW.node_id IS node_id;
      END
      )",

    R"(
    CREATE TABLE IF NOT EXISTS stations (
      "network_id"    INTEGER NOT NULL,
      "station_id"    TEXT NOT NULL,
      "latitude"      REAL NOT NULL CHECK (latitude  >  -90.0 AND latitude  <  90.0),
      "longitude"     REAL NOT NULL CHECK (longitude > -180.0 AND longitude < 180.0),
      "name"          TEXT NOT NULL,
      "description"   TEXT DEFAULT NULL,
      "access_public" BOOLEAN DEFAULT NULL,
      "functional"    BOOLEAN DEFAULT NULL,
      "restrictions"  TEXT DEFAULT NULL,
      "contact_id"    INTEGER DEFAULT NULL,
      "schedule_id"   INTEGER DEFAULT NULL,
      "port_ids"      TEXT NOT NULL,
      "last_update"   TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
      PRIMARY KEY (latitude, longitude)
    ) )",

    R"(
    CREATE TABLE IF NOT EXISTS ports (
      "network_id"    INTEGER NOT NULL,
      "port_id"       TEXT NOT NULL,
      "station_id"    TEXT NOT NULL,
      "power_id"      INTEGER DEFAULT NULL,
      "price_id"      INTEGER DEFAULT NULL,
      "display_name"  TEXT DEFAULT NULL,
      "last_update"   TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
      PRIMARY KEY (network_id,port_id)
    ) )",

    R"(
    CREATE TABLE IF NOT EXISTS contact (
      "contact_id"    INTEGER NOT NULL PRIMARY KEY,
      "street_number" INTEGER DEFAULT NULL,
      "street_name"   TEXT DEFAULT NULL,
      "city"          TEXT DEFAULT NULL,
      "state"         TEXT DEFAULT NULL,
      "country"       TEXT DEFAULT NULL,
      "postal_code"   TEXT DEFAULT NULL,
      "phone_number"  TEXT DEFAULT NULL,
      "URL_id"        INTEGER DEFAULT NULL
    ) )",

    R"(
      CREATE TRIGGER IF NOT EXISTS before_insert_contact
      BEFORE INSERT ON contact
      WHEN EXISTS (SELECT 1 FROM contact WHERE
        NEW.street_number IS street_number AND
        NEW.street_name IS street_name AND
        NEW.city IS city AND
        NEW.state IS state AND
        NEW.country IS country AND
        NEW.postal_code IS postal_code AND
        NEW.phone_number IS phone_number AND
        NEW.URL_id IS URL_id)
      BEGIN
        SELECT RAISE(ABORT,SQLITE_CONSTRAINT_TRIGGER);
      END
      )",

    R"(
      CREATE TABLE IF NOT EXISTS price (
        "price_id"  INTEGER NOT NULL PRIMARY KEY,
        "text"      TEXT    DEFAULT NULL,
        "payment"   INTEGER DEFAULT NULL,
        "currency"  INTEGER DEFAULT NULL,
        "minimum"   REAL    DEFAULT NULL,
        "initial"   REAL    DEFAULT NULL,
        "unit"      INTEGER DEFAULT NULL,
        "per_unit"  REAL    DEFAULT NULL
      ) )",

    R"(
      CREATE TRIGGER IF NOT EXISTS before_insert_price
      BEFORE INSERT ON price
      WHEN EXISTS (SELECT 1 FROM price WHERE
        NEW.text IS text AND
        NEW.payment IS payment AND
        NEW.currency IS currency AND
        NEW.minimum IS minimum AND
        NEW.initial IS initial AND
        NEW.unit IS unit AND
        NEW.per_unit IS per_unit)
      BEGIN
        SELECT RAISE(ABORT,SQLITE_CONSTRAINT_TRIGGER);
      END
      )",


    R"(
      CREATE TABLE IF NOT EXISTS power (
        "power_id"  INTEGER NOT NULL PRIMARY KEY,
        "level"     INTEGER NOT NULL,
        "connector" INTEGER DEFAULT NULL,
        "amp"       REAL DEFAULT NULL,
        "kW"        REAL DEFAULT NULL,
        "volt"      REAL DEFAULT NULL
      ) )",

    R"(
      CREATE TRIGGER IF NOT EXISTS before_insert_power
      BEFORE INSERT ON power
      WHEN EXISTS (SELECT 1 FROM power WHERE
        NEW.level IS level AND
        NEW.connector IS connector AND
        NEW.amp IS amp AND
        NEW.kW IS kW AND
        NEW.volt IS volt)
      BEGIN
        SELECT RAISE(ABORT,SQLITE_CONSTRAINT_TRIGGER);
      END
      )",

    R"(
      CREATE TABLE IF NOT EXISTS hours (
        "hours_id" INTEGER NOT NULL PRIMARY KEY,
        "begin" INTEGER NOT NULL,
        "end" INTEGER NOT NULL
      ) )",

    R"(
      CREATE TRIGGER IF NOT EXISTS before_insert_hours
      BEFORE INSERT ON hours
      WHEN EXISTS (SELECT 1 FROM hours WHERE
        NEW.begin IS begin AND
        NEW.end IS end)
      BEGIN
        SELECT RAISE(ABORT,SQLITE_CONSTRAINT_TRIGGER);
      END
      )",

    R"(
      CREATE TABLE IF NOT EXISTS schedule (
        "schedule_id" INTEGER NOT     NULL PRIMARY KEY,
        "sunday"      INTEGER DEFAULT NULL,
        "monday"      INTEGER DEFAULT NULL,
        "tuesday"     INTEGER DEFAULT NULL,
        "wednesday"   INTEGER DEFAULT NULL,
        "thursday"    INTEGER DEFAULT NULL,
        "friday"      INTEGER DEFAULT NULL,
        "saturday"    INTEGER DEFAULT NULL
      ) )",

    R"(
      CREATE TRIGGER IF NOT EXISTS before_insert_schedule
      BEFORE INSERT ON schedule
      WHEN EXISTS (SELECT 1 FROM schedule WHERE
        NEW.sunday IS sunday AND
        NEW.monday IS monday AND
        NEW.tuesday IS tuesday AND
        NEW.wednesday IS wednesday AND
        NEW.thursday IS thursday AND
        NEW.friday IS friday AND
        NEW.saturday IS saturday)
      BEGIN
        SELECT RAISE(ABORT,SQLITE_CONSTRAINT_TRIGGER);
      END
      )",

    R"(
      CREATE TABLE IF NOT EXISTS URLs (
        "URL_id" INTEGER NOT NULL PRIMARY KEY,
        "URL" TEXT NOT NULL UNIQUE
      ) )",

    R"(
      CREATE TABLE IF NOT EXISTS networks (
        "network_id" INTEGER NOT NULL PRIMARY KEY,
        "name" TEXT NOT NULL
      ) )",

    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Non-Networked", 17))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Unknown", 0))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("AmpUp", 42))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Astria", 15))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Azra", 14))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("BC Hydro EV", 27))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Blink", 1))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("ChargeLab", 31))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("ChargePoint", 2))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Circuit Électrique", 4))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("CityVitae", 52))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Co-op Connect", 47))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("eCharge", 23))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EcoCharge", 37))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Electrify America", 26))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Electrify Canada", 36))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EV Link", 18))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EV Range", 54))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EVConnect", 22))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EVCS", 43))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EVduty", 20))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("evGateway", 44))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EVgo", 8))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EVMatch", 35))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EVolve NY", 38))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EVSmart", 45))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Flash", 59))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Flo", 3))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("FPL Evolution", 41))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Francis Energy", 39))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("GE", 16))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Go Station", 48))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Hypercharge", 49))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Ivy", 34))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Livingston Energy", 46))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("myEVroute", 21))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("NL Hydro", 53))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Noodoe EV", 30))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("OK2Charge", 55))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("On the Run", 58))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("OpConnect", 9))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Petro-Canada", 32))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("PHI", 50))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Powerflex", 40))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("RED E", 60))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Rivian", 51))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("SemaConnect", 5))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Shell Recharge", 6))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Sun Country Highway", 11))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("SWTCH", 28))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("SYNC EV", 33))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Tesla", 12))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Universal", 56))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Voita", 19))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Webasto", 7))",
    R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("ZEF Energy", 25))",
  };

  for(const auto& command : init_commands)
  {
    if(!m_db.execute(command))
    {
      std::cerr << "SQL command failed:" << std::endl
                << command << std::endl;
      assert(false);
    }
  }
  std::clog << "database initialized" << std::endl;
}

DBInterface::~DBInterface(void)
{
  assert(m_db.close());
}

std::list<query_info_t> DBInterface::getChildLocations(Network network_id, query_info_t qinfo)
{
  std::list<query_info_t> children;

  if(qinfo.child_ids)
    for(auto& child_id : *qinfo.child_ids)
    {
      sql::query q = std::move(m_db.build_query("SELECT "
                                                  "latitude_max,"
                                                  "latitude_min,"
                                                  "longitude_max,"
                                                  "longitude_min,"
                                                  "node_id,"
                                                  "child_ids "
                                                "FROM "
                                                  "map_query_cache "
                                                "WHERE "
                                                  "network_id IS ?1 AND "
                                                  "node_id IS ?2")
                               .arg(network_id)
                               .arg(child_id));
      while(!q.execute() && q.lastError() == SQLITE_BUSY);

      while(q.fetchRow())
      {
        query_info_t nqi;

        q.getField(nqi.bounds.latitude.max)
            .getField(nqi.bounds.latitude.min)
            .getField(nqi.bounds.longitude.max)
            .getField(nqi.bounds.longitude.min)
            .getField(nqi.node_id)
            .getField(nqi.child_ids);

        children.emplace_back(std::move(nqi));
      }
    }
  return children;
}

std::list<pair_data_t> DBInterface::getMapLocation(const pair_data_t& data)
{
  std::list<pair_data_t> return_data;

  MUST(data.station.network_id && data.query.node_id)
  {
    sql::query q = std::move(m_db.build_query("SELECT "
                                                "child_ids "
                                              "FROM "
                                                "map_query_cache "
                                              "WHERE "
                                                "network_id IS ?1 AND "
                                                "node_id IS ?2")
                             .arg(data.station.network_id)
                             .arg(data.query.node_id));
    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

    MUST(q.fetchRow())
    {
      std::optional<std::string> child_ids;
      q.getField(child_ids);
      if(child_ids)
        for(const auto& child_id : ext::string(*child_ids).split_string({','}))
        {
          pair_data_t nd;
          nd.query.node_id = child_id;
          nd.station.network_id = data.station.network_id;
          return_data.emplace_back(nd);
        }
    }
  }

  return return_data;
}

std::list<query_info_t> DBInterface::recurseChildLocations(Network network_id, query_info_t qinfo, std::set<std::string>& node_ids)
{
  std::list<query_info_t> result;
  for(auto& child : getChildLocations(network_id, qinfo))
  {
    if(!node_ids.contains(*child.node_id))
    {
      node_ids.insert(*child.node_id);

      auto tmp = recurseChildLocations(network_id, child, node_ids);
      if(tmp.empty())
        result.push_back(child);
      else
        result.insert(std::end(result),
                      std::make_move_iterator(std::begin(tmp)),
                      std::make_move_iterator(std::end(tmp)));
    }
  }
  return result;
}

void DBInterface::addMapLocation(const pair_data_t& data)
{
  MUST(data.station.network_id && data.query.node_id)
  {
    sql::query q = std::move(m_db.build_query("INSERT INTO map_query_cache ("
                                                "network_id,"
                                                "latitude_max,"
                                                "latitude_min,"
                                                "longitude_max,"
                                                "longitude_min,"
                                                "node_id,"
                                                "child_ids,"
                                                "last_update"
                                              ") VALUES (?1,?2,?3,?4,?5,?6,?7,CURRENT_TIMESTAMP)")
                             .arg(data.station.network_id)
                             .arg(data.query.bounds.latitude.max)
                             .arg(data.query.bounds.latitude.min)
                             .arg(data.query.bounds.longitude.max)
                             .arg(data.query.bounds.longitude.min)
                             .arg(data.query.node_id)
                             .arg(data.query.child_ids));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_CONSTRAINT_TRIGGER);
  }
}

std::optional<std::string> DBInterface::identifyMapLocation(const pair_data_t& data)
{
  std::optional<std::string> node_id;
  sql::query q = std::move(m_db.build_query("SELECT "
                                              "node_id "
                                            "FROM "
                                              "map_query_cache "
                                            "WHERE "
                                              "network_id IS ?1 AND "
                                              "node_id IS ?2 AND "
                                              "ABS(latitude_max  - latitude_min ) <= ABS(?3 - ?4) AND "
                                              "ABS(longitude_max - longitude_min) <= ABS(?5 - ?6)"
                                            )
                           .arg(data.station.network_id)
                           .arg(data.query.node_id)
                           .arg(data.query.bounds.latitude.max)
                           .arg(data.query.bounds.latitude.min)
                           .arg(data.query.bounds.longitude.max)
                           .arg(data.query.bounds.longitude.min));
  while(!q.execute() && q.lastError() == SQLITE_BUSY);
  if(q.fetchRow())
    q.getField(node_id);
  return node_id;
}

std::list<pair_data_t> DBInterface::getMapLocationCache(const pair_data_t& data)
{
  std::list<pair_data_t> return_data;
  std::set<std::string> node_ids;
  std::list<query_info_t> nodes = recurseChildLocations(*data.station.network_id, data.query, node_ids);
  for(const auto& qinfo : nodes)
    return_data.emplace_back(pair_data_t{ qinfo, {}});
  return return_data;
}


void DBInterface::addURL(const std::optional<std::string>& URL)
{
  if(URL)
  {
    sql::query q = std::move(m_db.build_query("INSERT INTO URLs (URL) VALUES (?1)")
                             .arg(URL));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_CONSTRAINT_PRIMARYKEY);
  }
}

std::optional<uint64_t> DBInterface::identifyURL(const std::optional<std::string>& URL)
{
  std::optional<uint64_t> URL_id;
  if(URL)
  {
    sql::query q = std::move(m_db.build_query("SELECT URL_id FROM URLs WHERE URL IS ?1")
                  .arg(URL));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

    MUST(q.fetchRow())
      q.getField(URL_id);
  }
  return URL_id;
}

std::optional<std::string> DBInterface::getURL(const std::optional<uint64_t> URL_id)
{
  std::optional<std::string> URL;
  if(URL_id)
  {
    sql::query q = std::move(m_db.build_query("SELECT URL FROM URLs WHERE URL_id IS ?1")
                  .arg(URL_id));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

    MUST(q.fetchRow())
      q.getField(URL);
  }
  return URL;
}

void DBInterface::addContact(const contact_t& contact)
{
  addURL(contact.URL);
  std::optional<uint64_t> URL_id = identifyURL(contact.URL);
  if(contact)
  {
    //std::cout << "adding " << contact << std::endl;
    sql::query q = std::move(m_db.build_query("INSERT INTO contact ("
                                                "street_number,"
                                                "street_name,"
                                                "city,"
                                                "state,"
                                                "country,"
                                                "postal_code,"
                                                "phone_number,"
                                                "URL_id"
                                              ") VALUES (?1,?2,?3,?4,?5,?6,?7,?8)")
                             .arg(contact.street_number)
                             .arg(contact.street_name)
                             .arg(contact.city)
                             .arg(contact.state)
                             .arg(contact.country)
                             .arg(contact.postal_code)
                             .arg(contact.phone_number)
                             .arg(URL_id));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_CONSTRAINT_TRIGGER);
  }
}

std::optional<uint64_t> DBInterface::identifyContact(const contact_t& contact)
{
  std::optional<uint64_t> contact_id;
  if(contact)
  {
    sql::query q = std::move(m_db.build_query("SELECT "
                                                "contact_id "
                                              "FROM "
                                                "contact "
                                              "WHERE "
                                                "street_number IS ?1 AND "
                                                "street_name IS ?2 AND "
                                                "city IS ?3 AND "
                                                "state IS ?4 AND "
                                                "country IS ?5 AND "
                                                "postal_code IS ?6")
                             .arg(contact.street_number)
                             .arg(contact.street_name)
                             .arg(contact.city)
                             .arg(contact.state)
                             .arg(contact.country)
                             .arg(contact.postal_code));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

    MUST(q.fetchRow())
      q.getField(contact_id);
  }
  return contact_id;
}

contact_t DBInterface::getContact(const std::optional<uint64_t> contact_id)
{
  contact_t contact;
  if(contact_id)
  {
    std::optional<uint64_t> URL_id;
    contact.contact_id = contact_id;
    { // scope for sql::query type
      sql::query q = std::move(m_db.build_query("SELECT "
                                                  "street_number,"
                                                  "street_name,"
                                                  "city,"
                                                  "state,"
                                                  "country,"
                                                  "postal_code,"
                                                  "phone_number,"
                                                  "URL_id "
                                                "FROM "
                                                  "contact "
                                                "WHERE "
                                                  "contact_id IS ?1")
                               .arg(contact_id));

      while(!q.execute() && q.lastError() == SQLITE_BUSY);
      assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

      MUST(q.fetchRow())
      {

        q.getField(contact.street_number)
         .getField(contact.street_name)
         .getField(contact.city)
         .getField(contact.state)
         .getField(contact.country)
         .getField(contact.postal_code)
         .getField(contact.phone_number)
         .getField(URL_id);

      }
    }
    contact.URL = getURL(URL_id);
  }
  return contact;
}

void DBInterface::addPrice(const price_t& price)
{
  if(price)
  {
    //std::cout << "adding " << price << std::endl;
    sql::query q = std::move(m_db.build_query("INSERT INTO price ("
                                                "text,"
                                                "payment,"
                                                "currency,"
                                                "minimum,"
                                                "initial,"
                                                "unit,"
                                                "per_unit"
                                              ") VALUES (?1,?2,?3,?4,?5,?6,?7)")
                             .arg(price.text)
                             .arg(price.payment)
                             .arg(price.currency)
                             .arg(price.minimum)
                             .arg(price.initial)
                             .arg(price.unit)
                             .arg(price.per_unit));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_CONSTRAINT_TRIGGER);
  }
}

std::optional<uint64_t> DBInterface::identifyPrice(const price_t& price)
{
  std::optional<uint64_t> price_id;
  if(price)
  {
    sql::query q = std::move(m_db.build_query("SELECT "
                                                "price_id "
                                              "FROM "
                                                "price "
                                              "WHERE "
                                                "text IS ?1 AND "
                                                "payment IS ?2 AND "
                                                "currency IS ?3 AND "
                                                "minimum IS ?4 AND "
                                                "initial IS ?5 AND "
                                                "unit IS ?6 AND "
                                                "per_unit IS ?7")
                             .arg(price.text)
                             .arg(price.payment)
                             .arg(price.currency)
                             .arg(price.minimum)
                             .arg(price.initial)
                             .arg(price.unit)
                             .arg(price.per_unit));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

    MUST(q.fetchRow())
      q.getField(price_id);
  }
  return price_id;
}

price_t DBInterface::getPrice(const std::optional<uint64_t> price_id)
{
  price_t price;
  if(price_id)
  {
    price.price_id = price_id;
    sql::query q = std::move(m_db.build_query("SELECT "
                                                "text,"
                                                "payment,"
                                                "currency,"
                                                "minimum,"
                                                "initial,"
                                                "unit,"
                                                "per_unit "
                                              "FROM "
                                                "price "
                                              "WHERE "
                                                "price_id IS ?1")
                             .arg(price_id));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

    MUST(q.fetchRow())
      q.getField(price.text)
       .getField(price.payment)
       .getField(price.currency)
       .getField(price.minimum)
       .getField(price.initial)
       .getField(price.unit)
       .getField(price.per_unit);
  }
  return price;
}


void DBInterface::addPower(const power_t& power)
{
  if(power)
  {
    //std::cout << "adding " << power << std::endl;
    sql::query q = std::move(m_db.build_query("INSERT INTO power ("
                                                "level,"
                                                "connector,"
                                                "amp,"
                                                "kW,"
                                                "volt"
                                              ") VALUES (?1,?2,?3,?4,?5)")
                             .arg(power.level)
                             .arg(power.connector)
                             .arg(power.amp)
                             .arg(power.kw)
                             .arg(power.volt));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_CONSTRAINT_TRIGGER);
  }
}


std::optional<uint64_t> DBInterface::identifyPower(const power_t& power)
{
  std::optional<uint64_t> power_id;
  if(power)
  {
    sql::query q = std::move(m_db.build_query("SELECT "
                                                "power_id "
                                              "FROM "
                                                "power "
                                              "WHERE "
                                                "level IS ?1 AND "
                                                "connector IS ?2 AND "
                                                "amp IS ?3 AND "
                                                "kW IS ?4 AND "
                                                "volt IS ?5")
                             .arg(power.level)
                             .arg(power.connector)
                             .arg(power.amp)
                             .arg(power.kw)
                             .arg(power.volt));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

    MUST(q.fetchRow())
      q.getField(power_id);
  }
  return power_id;
}

power_t DBInterface::getPower(const std::optional<uint64_t> power_id)
{
  power_t power;
  if(power_id)
  {
    power.power_id = power_id;
    sql::query q = std::move(m_db.build_query("SELECT "
                                                "level,"
                                                "connector,"
                                                "amp,"
                                                "kW,"
                                                "volt "
                                              "FROM "
                                                "power "
                                              "WHERE "
                                                "power_id IS ?1")
                             .arg(power_id));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

    MUST(q.fetchRow())
      q.getField(power.level)
       .getField(power.connector)
       .getField(power.amp)
       .getField(power.kw)
       .getField(power.volt);
  }
  return power;
}


void DBInterface::addHours(const std::optional<schedule_t::hours_t>& hours)
{
  if(hours)
  {
    sql::query q = std::move(m_db.build_query("INSERT INTO hours ("
                                                "begin,"
                                                "end"
                                              ") VALUES (?1,?2)")
                             .arg(hours->first)
                             .arg(hours->second));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_CONSTRAINT_TRIGGER);
  }
}

std::optional<uint64_t> DBInterface::identifyHours(const std::optional<schedule_t::hours_t>& hours)
{
  std::optional<uint64_t> hours_id;
  if(hours)
  {
    sql::query q = std::move(m_db.build_query("SELECT "
                                                "hours_id "
                                              "FROM "
                                                "hours "
                                              "WHERE "
                                                "begin IS ?1 AND "
                                                "end IS ?2")
                             .arg(hours->first)
                             .arg(hours->second));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

    MUST(q.fetchRow())
      q.getField(hours_id);
  }
  return hours_id;
}

std::optional<schedule_t::hours_t> DBInterface::getHours(const std::optional<uint64_t> hours_id)
{
  schedule_t::hours_t hours;
  if(hours_id)
  {
    sql::query q = std::move(m_db.build_query("SELECT "
                                                "begin,"
                                                "end "
                                              "FROM "
                                                "hours "
                                              "WHERE "
                                                "hours_id IS ?1")
                             .arg(hours_id));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

    MUST(q.fetchRow())
      q.getField(hours.first)
       .getField(hours.second);
  }
  return hours;
}


void DBInterface::addSchedule(const schedule_t& schedule)
{
  if(schedule)
  {
    std::vector<std::optional<uint64_t>> hours_ids;
    for(auto& hours : schedule.days)
    {
      addHours(hours);
      hours_ids.push_back(identifyHours(hours));
    }

    sql::query q = std::move(m_db.build_query("INSERT INTO schedule ("
                                                "sunday,"
                                                "monday,"
                                                "tuesday,"
                                                "wednesday,"
                                                "thursday,"
                                                "friday,"
                                                "saturday"
                                              ") VALUES (?1,?2,?3,?4,?5,?6,?7)")
                             .arg(hours_ids[0])
                             .arg(hours_ids[1])
                             .arg(hours_ids[2])
                             .arg(hours_ids[3])
                             .arg(hours_ids[4])
                             .arg(hours_ids[5])
                             .arg(hours_ids[6]));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_CONSTRAINT_TRIGGER);
  }
}

std::optional<uint64_t> DBInterface::identifySchedule(const schedule_t& schedule)
{
  std::optional<uint64_t> schedule_id;
  if(schedule)
  {
    std::vector<std::optional<uint64_t>> hours_ids;
    for(auto& hours : schedule.days)
      hours_ids.push_back(identifyHours(hours));
    sql::query q = std::move(m_db.build_query("SELECT "
                                                "schedule_id "
                                              "FROM "
                                                "schedule "
                                              "WHERE "
                                                "sunday IS ?1 AND "
                                                "monday IS ?2 AND "
                                                "tuesday IS ?3 AND "
                                                "wednesday IS ?4 AND "
                                                "thursday IS ?5 AND "
                                                "friday IS ?6 AND "
                                                "saturday IS ?7")
                             .arg(hours_ids[0])
                             .arg(hours_ids[1])
                             .arg(hours_ids[2])
                             .arg(hours_ids[3])
                             .arg(hours_ids[4])
                             .arg(hours_ids[5])
                             .arg(hours_ids[6]));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

    MUST(q.fetchRow())
      q.getField(schedule_id);
  }
  return schedule_id;
}

schedule_t DBInterface::getSchedule(const std::optional<uint64_t> schedule_id)
{
  schedule_t schedule;
  std::array<std::optional<uint64_t>, 7> hours_ids;
  if(schedule_id)
  {
    schedule.schedule_id = schedule_id;
    sql::query q = std::move(m_db.build_query("SELECT "
                                                "sunday,"
                                                "monday,"
                                                "tuesday,"
                                                "wednesday,"
                                                "thursday,"
                                                "friday,"
                                                "saturday "
                                              "FROM "
                                                "schedule "
                                              "WHERE "
                                                "schedule_id IS ?1")
                             .arg(schedule_id));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

    MUST(q.fetchRow())
      q.getField(hours_ids[0])
       .getField(hours_ids[1])
       .getField(hours_ids[2])
       .getField(hours_ids[3])
       .getField(hours_ids[4])
       .getField(hours_ids[5])
       .getField(hours_ids[6]);
  }

  for(size_t i = 0; i < hours_ids.size(); ++i)
    schedule.days[i] = getHours(hours_ids[i]);

  return schedule;
}

// from https://stackoverflow.com/a/12883767/454237
template<int digit>
constexpr double precision(double x)
{
  if (x == 0.f)
    return x;
  int ex = std::floor(std::log10(std::abs(x))) - digit + 1;
  double div = std::pow(10, ex);
  return std::floor(x / div + 0.5) * div;
}


void DBInterface::addPort(port_t& port)
{
  addPower(port.power);
  port.power.power_id = identifyPower(port.power);

  addPrice(port.price);
  port.price.price_id = identifyPrice(port.price);

  sql::query q = std::move(m_db.build_query("INSERT INTO ports ("
                                              "network_id,"
                                              "port_id,"
                                              "station_id,"
                                              "power_id,"
                                              "price_id,"
                                              "display_name"
                                            ") VALUES (?1,?2,?3,?4,?5,?6)")
                           .arg(port.network_id)
                           .arg(port.port_id)
                           .arg(port.station_id)
                           .arg(port.power.power_id)
                           .arg(port.price.price_id)
                           .arg(port.display_name));

  while(!q.execute() && q.lastError() == SQLITE_BUSY);
  assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_CONSTRAINT_PRIMARYKEY);
}

port_t DBInterface::getPort(Network network_id, const std::string& port_id)
{
  port_t port;
  port.network_id = network_id;
  port.port_id = port_id;
  {
    sql::query q = std::move(m_db.build_query("SELECT "
                                                "station_id,"
                                                "power_id,"
                                                "price_id,"
                                                "display_name "
                                              "FROM "
                                                "ports "
                                              "WHERE "
                                                "network_id IS ?1 AND "
                                                "port_id IS ?2")
                             .arg(port.network_id)
                             .arg(port.port_id));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

    MUST(q.fetchRow())
      q.getField(port.station_id)
       .getField(port.power.power_id)
       .getField(port.price.price_id)
       .getField(port.display_name);
  }
  port.power = getPower(port.power.power_id);
  port.price = getPrice(port.price.price_id);
  return port;
}



void DBInterface::addStation(station_t& station)
{
  try
  {
    station_t existing = getStation(*station.network_id, *station.station_id);

    assert(!station.ports.empty());
    ext::string port_ids;
    for(auto& port : station.ports)
      port_ids.list_append(',', *port.port_id);

    { // sync contact structs
      contact_t contact;
      for(auto& port : station.ports)
      {
        if(port.contact)
        {
          if(!contact)
            contact = port.contact;
          else
            assert(contact == port.contact);
        }
      }
      if(!contact && station.contact)
        contact = station.contact;
      if(contact)
        addContact(contact);
      contact.contact_id = identifyContact(contact);
      station.contact = contact;
    }

    if(station.price) // sync price structs
    {
      for(auto& port : station.ports)
        if(!port.price)
          port.price = station.price;
    }

    for(auto& port : station.ports) // add ports to database
    {
      port.station_id = station.station_id; // sync station and network ids
      port.network_id = station.network_id;
      addPort(port);
    }

    addSchedule(station.schedule);
    station.schedule.schedule_id = identifySchedule(station.schedule);

    sql::query q = std::move(m_db.build_query("INSERT INTO stations ("
                                                "network_id,"
                                                "station_id,"
                                                "latitude,"
                                                "longitude,"
                                                "name,"
                                                "description,"
                                                "access_public,"
                                                "functional,"
                                                "restrictions,"
                                                "contact_id,"
                                                "schedule_id,"
                                                "port_ids"
                                              ") VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12)")
                             .arg(station.network_id)
                             .arg(station.station_id)
                             .arg(station.location.latitude)
                             .arg(station.location.longitude)
                             .arg(station.name)
                             .arg(station.description)
                             .arg(station.access_public)
                             .arg(station.functional)
                             .arg(station.restrictions)
                             .arg(station.contact.contact_id)
                             .arg(station.schedule.schedule_id)
                             .arg(static_cast<std::string&>(port_ids)));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_CONSTRAINT_PRIMARYKEY);
  }
  catch(std::string& error)
  {
    std::cerr << "sql error: " << error << std::endl;
  }
}

station_t DBInterface::getStation(sql::query&& q)
{
  station_t station;
  ext::string port_ids;

  while(!q.execute() && q.lastError() == SQLITE_BUSY);
  assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

  if(q.fetchRow())
  {
    q.getField(station.network_id)
     .getField(station.station_id)
     .getField(station.location.latitude)
     .getField(station.location.longitude)
     .getField(station.name)
     .getField(station.description)
     .getField(station.access_public)
     .getField(station.functional)
     .getField(station.restrictions)
     .getField(station.contact.contact_id)
     .getField(station.schedule.schedule_id)
     .getField(static_cast<std::string&>(port_ids));

    for(const auto& port_id : port_ids.split_string({','}))
      station.ports.emplace_back(getPort(*station.network_id, port_id));

    station.contact = getContact(station.contact.contact_id);
    station.schedule = getSchedule(station.schedule.schedule_id);
  }

  return station;
}

station_t DBInterface::getStation(uint64_t contact_id)
{
  try
  {
    return getStation(std::move(m_db.build_query( "SELECT "
                                                    "network_id,"
                                                    "station_id,"
                                                    "latitude,"
                                                    "longitude,"
                                                    "name,"
                                                    "description,"
                                                    "access_public,"
                                                    "functional,"
                                                    "restrictions,"
                                                    "contact_id,"
                                                    "schedule_id,"
                                                    "port_ids "
                                                  "FROM "
                                                    "stations "
                                                  "WHERE "
                                                    "contact_id IS ?1")
                             .arg(contact_id)));
  }
  catch(std::string& error)
  {
    std::cerr << "sql error: " << error << std::endl;
  }
  return {};
}

station_t DBInterface::getStation(Network network_id, const std::string& station_id)
{
  try
  {
    return getStation(std::move(m_db.build_query( "SELECT "
                                                    "network_id,"
                                                    "station_id,"
                                                    "latitude,"
                                                    "longitude,"
                                                    "name,"
                                                    "description,"
                                                    "access_public,"
                                                    "functional,"
                                                    "restrictions,"
                                                    "contact_id,"
                                                    "schedule_id,"
                                                    "port_ids "
                                                  "FROM "
                                                    "stations "
                                                  "WHERE "
                                                    "network_id IS ?1 AND "
                                                    "station_id IS ?2")
                             .arg(network_id)
                             .arg(station_id)));
  }
  catch(std::string& error)
  {
    std::cerr << "sql error: " << error << std::endl;
  }
  return {};
}

station_t DBInterface::getStation(double latitude, double longitude)
{
  try
  {
    return getStation(std::move(m_db.build_query( "SELECT "
                                                    "network_id,"
                                                    "station_id,"
                                                    "latitude,"
                                                    "longitude,"
                                                    "name,"
                                                    "description,"
                                                    "access_public,"
                                                    "functional,"
                                                    "restrictions,"
                                                    "contact_id,"
                                                    "schedule_id,"
                                                    "port_ids "
                                                  "FROM "
                                                    "stations "
                                                  "WHERE "
                                                    "latitude IS ?1 AND "
                                                    "longitude IS ?2")
                             .arg(latitude)
                             .arg(longitude)));
  }
  catch(std::string& error)
  {
    std::cerr << "sql error: " << error << std::endl;
  }
  return {};
}
