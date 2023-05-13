#include "dbinterface.h"

// STL
#include <iostream>
#include <algorithm>
#include <functional>
#include <utility>
#include <vector>
#include <cassert>

// project
#include <scrapers/utilities.h>

#ifdef DATABASE_TOLERANT
# define MUST(x) if(x)
#else
# define MUST(x) assert(x);
#endif

#define quoted(x) #x
#define insert_unique_string(name, value) \
  "INSERT OR IGNORE INTO unique_strings (string, string_id) VALUES (" #name ", " #value ")"


std::optional<std::string> to_optional(const std::string& input)
{
  if(input.empty())
    return {};
  return { input };
}

DBInterface::DBInterface(std::string_view filename)
{
  assert(m_db.open(filename));

  const std::list<std::string_view> init_commands =
  {
#if 1
    "DROP TABLE IF EXISTS stations",
    "DROP TABLE IF EXISTS ports",
    "DROP TABLE IF EXISTS contacts",
    "DROP TABLE IF EXISTS price",
    "DROP TABLE IF EXISTS power",
    "DROP TABLE IF EXISTS unique_strings",
#endif
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
      "meta_network_ids"  TEXT      DEFAULT NULL,
      "meta_station_ids"  TEXT      DEFAULT NULL,
      "network_id"        INTEGER   NOT     NULL,
      "station_id"        TEXT      DEFAULT NULL,
      "latitude"          REAL      NOT     NULL CHECK (latitude  >  -90.0 AND latitude  <  90.0),
      "longitude"         REAL      NOT     NULL CHECK (longitude > -180.0 AND longitude < 180.0),
      "name"              TEXT      DEFAULT NULL,
      "description"       TEXT      DEFAULT NULL,
      "access_public"     BOOLEAN   DEFAULT NULL,
      "restrictions"      TEXT      DEFAULT NULL,
      "contact_id"        INTEGER   DEFAULT NULL,
      "schedule_id"       INTEGER   DEFAULT NULL,
      "port_ids"          TEXT      NOT     NULL,
      "conflicts"         BOOLEAN   NOT     NULL,
      "last_update"       TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
      PRIMARY KEY (latitude, longitude)
    ) )",

    R"(
    CREATE TABLE IF NOT EXISTS ports (
      "network_id"    INTEGER   NOT NULL,
      "port_id"       TEXT      NOT NULL,
      "station_id"    TEXT      DEFAULT NULL,
      "power_id"      INTEGER   DEFAULT NULL,
      "price_id"      INTEGER   DEFAULT NULL,
      "status"        INTEGER   DEFAULT NULL,
      "display_name"  TEXT      DEFAULT NULL,
      "last_update"   TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
      PRIMARY KEY (network_id,port_id)
    ) )",

    R"(
    CREATE TABLE IF NOT EXISTS contact (
      "contact_id"    INTEGER NOT NULL PRIMARY KEY,
      "street_number" INTEGER DEFAULT NULL,
      "street_name"   TEXT    DEFAULT NULL,
      "city"          TEXT    DEFAULT NULL,
      "state"         TEXT    DEFAULT NULL,
      "country"       TEXT    DEFAULT NULL,
      "postal_code"   TEXT    DEFAULT NULL,
      "phone_id"      INTEGER DEFAULT NULL,
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
        NEW.phone_id IS phone_id AND
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
        "amp"       REAL    DEFAULT NULL,
        "kW"        REAL    DEFAULT NULL,
        "volt"      REAL    DEFAULT NULL
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
      CREATE TABLE IF NOT EXISTS unique_strings (
        "string_id" INTEGER NOT NULL PRIMARY KEY,
        "string" TEXT NOT NULL UNIQUE
      ) )",

    insert_unique_string("Non-Networked", 17),
    insert_unique_string("Unknown", 0),
    insert_unique_string("AmpUp", 42),
    insert_unique_string("Astria", 15),
    insert_unique_string("Azra", 14),
    insert_unique_string("BC Hydro EV", 27),
    insert_unique_string("Blink", 1),
    insert_unique_string("ChargeLab", 31),
    insert_unique_string("ChargePoint", 2),
    insert_unique_string("Circuit Électrique", 4),
    insert_unique_string("CityVitae", 52),
    insert_unique_string("Co-op Connect", 47),
    insert_unique_string("eCharge", 23),
    insert_unique_string("EcoCharge", 37),
    insert_unique_string("Electrify America", 26),
    insert_unique_string("Electrify Canada", 36),
    insert_unique_string("EV Link", 18),
    insert_unique_string("EV Range", 54),
    insert_unique_string("EVConnect", 22),
    insert_unique_string("EVCS", 43),
    insert_unique_string("EVduty", 20),
    insert_unique_string("evGateway", 44),
    insert_unique_string("EVgo", 8),
    insert_unique_string("EVMatch", 35),
    insert_unique_string("EVolve NY", 38),
    insert_unique_string("EVSmart", 45),
    insert_unique_string("Flash", 59),
    insert_unique_string("Flo", 3),
    insert_unique_string("FPL Evolution", 41),
    insert_unique_string("Francis Energy", 39),
    insert_unique_string("GE", 16),
    insert_unique_string("Go Station", 48),
    insert_unique_string("Hypercharge", 49),
    insert_unique_string("Ivy", 34),
    insert_unique_string("Livingston Energy", 46),
    insert_unique_string("myEVroute", 21),
    insert_unique_string("NL Hydro", 53),
    insert_unique_string("Noodoe EV", 30),
    insert_unique_string("OK2Charge", 55),
    insert_unique_string("On the Run", 58),
    insert_unique_string("OpConnect", 9),
    insert_unique_string("Petro-Canada", 32),
    insert_unique_string("PHI", 50),
    insert_unique_string("Powerflex", 40),
    insert_unique_string("RED E", 60),
    insert_unique_string("Rivian", 51),
    insert_unique_string("SemaConnect", 5),
    insert_unique_string("Shell Recharge", 6),
    insert_unique_string("Sun Country Highway", 11),
    insert_unique_string("SWTCH", 28),
    insert_unique_string("SYNC EV", 33),
    insert_unique_string("Tesla", 12),
    insert_unique_string("Universal", 56),
    insert_unique_string("Voita", 19),
    insert_unique_string("Webasto", 7),
    insert_unique_string("ZEF Energy", 25),
    insert_unique_string("ChargeHub", 100),
    insert_unique_string("Eptix", 101),
    insert_unique_string("0,1440;0,1440;0,1440;0,1440;0,1440;0,1440;0,1440", 110),
  };

  for(const auto& command : init_commands)
  {
    if(!m_db.execute(command))
    {
      std::cerr << "SQL command failed:" << std::endl
                << command << std::endl;
      //m_db.clearError();
      assert(false);
    }
  }
  std::cout << "database initialized" << std::endl;
}

DBInterface::~DBInterface(void)
{
  assert(m_db.close());
}

std::optional<pair_data_t> DBInterface::getMapLocation(Network network_id, const std::string& node_id)
{
  std::optional<pair_data_t> return_data;
  sql::query q = std::move(m_db.build_query("SELECT "
                                              "network_id,"
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
                           .arg(node_id));
  while(!q.execute() && q.lastError() == SQLITE_BUSY);
  while(q.fetchRow())
  {
    pair_data_t nd;
    q.getField(nd.station.network_id)
        .getField(nd.query.bounds.latitude.max)
        .getField(nd.query.bounds.latitude.min)
        .getField(nd.query.bounds.longitude.max)
        .getField(nd.query.bounds.longitude.min)
        .getField(nd.query.node_id)
        .getField(nd.query.child_ids);
    if(!return_data)
      return_data.emplace(std::move(nd));
    else
    {
      if(return_data->query.child_ids && nd.query.child_ids)
        return_data->query.child_ids->append(",").append(*nd.query.child_ids);
      else if(nd.query.child_ids)
        return_data->query.child_ids = nd.query.child_ids;
    }
  }
  return return_data;
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
                                              "latitude_max IS ?3 AND "
                                              "latitude_min IS ?4 AND "
                                              "longitude_max IS ?5 AND "
                                              "longitude_min IS ?6"
//                                              "ABS(latitude_max  - latitude_min ) <= ABS(?3 - ?4) AND "
//                                              "ABS(longitude_max - longitude_min) <= ABS(?5 - ?6)"
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

void DBInterface::addUniqueString(const std::optional<std::string>& ustring)
{
  if(ustring)
  {
    sql::query q = std::move(m_db.build_query("INSERT INTO unique_strings (string) VALUES (?1)").arg(ustring));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_CONSTRAINT_UNIQUE);
  }
}

std::optional<uint64_t> DBInterface::identifyUniqueString(const std::optional<std::string>& ustring)
{
  std::optional<uint64_t> string_id;
  if(ustring)
  {
    sql::query q = std::move(m_db.build_query("SELECT string_id FROM unique_strings WHERE string IS ?1").arg(ustring));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

    MUST(q.fetchRow())
      q.getField(string_id);
  }
  return string_id;
}

std::optional<std::string> DBInterface::getUniqueString(const std::optional<uint64_t> string_id)
{
  std::optional<std::string> ustring;
  if(string_id)
  {
    sql::query q = std::move(m_db.build_query("SELECT string FROM unique_strings WHERE string_id IS ?1").arg(string_id));

    while(!q.execute() && q.lastError() == SQLITE_BUSY);
    assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

    MUST(q.fetchRow())
      q.getField(ustring);
  }
  return ustring;
}

void DBInterface::addContact(const contact_t& contact)
{
  addUniqueString(contact.phone_number);
  addUniqueString(contact.URL);

  std::optional<uint64_t> phone_id = identifyUniqueString(contact.phone_number);
  std::optional<uint64_t> URL_id = identifyUniqueString(contact.URL);

  if(contact)
  {
    sql::query q = std::move(m_db.build_query("INSERT INTO contact ("
                                                "street_number,"
                                                "street_name,"
                                                "city,"
                                                "state,"
                                                "country,"
                                                "postal_code,"
                                                "phone_id,"
                                                "URL_id"
                                              ") VALUES (?1,?2,?3,?4,?5,?6,?7,?8)")
                             .arg(contact.street_number)
                             .arg(contact.street_name)
                             .arg(contact.city)
                             .arg(contact.state)
                             .arg(contact.country)
                             .arg(contact.postal_code)
                             .arg(phone_id)
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
    std::optional<uint64_t> URL_id, phone_id;
    { // scope for sql::query type
      sql::query q = std::move(m_db.build_query("SELECT "
                                                  "street_number,"
                                                  "street_name,"
                                                  "city,"
                                                  "state,"
                                                  "country,"
                                                  "postal_code,"
                                                  "phone_id,"
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
         .getField(phone_id)
         .getField(URL_id);

      }
    }
    contact.phone_number = getUniqueString(phone_id);
    contact.URL = getUniqueString(URL_id);
  }
  return contact;
}

void DBInterface::addPrice(const price_t& price)
{
  if(price)
  {
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

void DBInterface::addPort(port_t& port)
{
  addPower(port.power);
  addPrice(port.price);

  std::optional<uint64_t> power_id = identifyPower(port.power);
  std::optional<uint64_t> price_id = identifyPrice(port.price);

  sql::query q = std::move(m_db.build_query("INSERT INTO ports ("
                                              "network_id,"
                                              "port_id,"
                                              "station_id,"
                                              "power_id,"
                                              "price_id,"
                                              "status,"
                                              "display_name"
                                            ") VALUES (?1,?2,?3,?4,?5,?6,?7)")
                           .arg(port.network_id)
                           .arg(port.port_id)
                           .arg(port.station_id)
                           .arg(power_id)
                           .arg(price_id)
                           .arg(port.status)
                           .arg(port.display_name));

  while(!q.execute() && q.lastError() == SQLITE_BUSY);
  assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_CONSTRAINT_PRIMARYKEY);
}

port_t DBInterface::getPort(Network network_id, const std::string& port_id)
{
  std::optional<uint64_t> power_id, price_id;
  port_t port;
  port.network_id = network_id;
  port.port_id = port_id;
  {
    sql::query q = std::move(m_db.build_query("SELECT "
                                                "station_id,"
                                                "power_id,"
                                                "price_id,"
                                                "status,"
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
       .getField(power_id)
       .getField(price_id)
       .getField(port.status)
       .getField(port.display_name);
  }
  port.power = getPower(power_id);
  port.price = getPrice(price_id);
  return port;
}



void DBInterface::addStation(station_t& station)
{
  try
  {
    std::optional<uint64_t> contact_id, schedule_id;
    bool conflicts = station.incorporate(getStation(station.location));

    assert(!station.ports.empty());

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

      contact_id = identifyContact(contact);
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

    if(!station.schedule.empty())
    {
      addUniqueString(station.schedule);
      schedule_id = identifyUniqueString(station.schedule);
    }

    sql::query q = std::move(m_db.build_query("INSERT OR REPLACE INTO stations ("
                                                "meta_network_ids,"
                                                "meta_station_ids,"
                                                "network_id,"
                                                "station_id,"
                                                "latitude,"
                                                "longitude,"
                                                "name,"
                                                "description,"
                                                "access_public,"
                                                "restrictions,"
                                                "contact_id,"
                                                "schedule_id,"
                                                "port_ids,"
                                                "conflicts"
                                              ") VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13,?14)")
                             .arg(to_optional(ext::to_string<Network>(station.meta_network_ids)))
                             .arg(to_optional(ext::to_string(station.meta_station_ids)))
                             .arg(station.network_id)
                             .arg(station.station_id)
                             .arg(station.location.latitude)
                             .arg(station.location.longitude)
                             .arg(station.name)
                             .arg(station.description)
                             .arg(station.access_public)
                             .arg(station.restrictions)
                             .arg(contact_id)
                             .arg(schedule_id)
                             .arg(to_optional(ext::to_string<port_t>(station.ports, [](const port_t& p) { return *p.port_id; })))
                             .arg(conflicts));

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

  while(!q.execute() && q.lastError() == SQLITE_BUSY);
  assert(q.lastError() == SQLITE_DONE || q.lastError() == SQLITE_ROW);

  if(q.fetchRow())
  {
    std::optional<std::string> meta_network_ids, meta_station_ids;
    std::optional<uint64_t> contact_id, schedule_id;
    std::string port_ids;

    q.getField(meta_network_ids)
     .getField(meta_station_ids)
     .getField(station.network_id)
     .getField(station.station_id)
     .getField(station.location.latitude)
     .getField(station.location.longitude)
     .getField(station.name)
     .getField(station.description)
     .getField(station.access_public)
     .getField(station.restrictions)
     .getField(contact_id)
     .getField(schedule_id)
     .getField(port_ids);

    station.meta_network_ids = ext::to_list<Network>(meta_network_ids);
    station.meta_station_ids = ext::to_list(meta_station_ids);

    station.ports = ext::to_list<port_t>(port_ids,
                                         [&station, this](const std::string& port_id)
                                         { return getPort(*station.network_id, port_id); });

    station.contact = getContact(contact_id);
    station.schedule = getUniqueString(schedule_id);
  }

  return station;
}

station_t DBInterface::getStation(uint64_t contact_id)
{
  try
  {
    return getStation(std::move(m_db.build_query( "SELECT "
                                                    "meta_network_ids,"
                                                    "meta_station_ids,"
                                                    "network_id,"
                                                    "station_id,"
                                                    "latitude,"
                                                    "longitude,"
                                                    "name,"
                                                    "description,"
                                                    "access_public,"
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
                                                    "meta_network_ids,"
                                                    "meta_station_ids,"
                                                    "network_id,"
                                                    "station_id,"
                                                    "latitude,"
                                                    "longitude,"
                                                    "name,"
                                                    "description,"
                                                    "access_public,"
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

station_t DBInterface::getStation(coords_t location)
{
  try
  {
    return getStation(std::move(m_db.build_query( "SELECT "
                                                    "meta_network_ids,"
                                                    "meta_station_ids,"
                                                    "network_id,"
                                                    "station_id,"
                                                    "latitude,"
                                                    "longitude,"
                                                    "name,"
                                                    "description,"
                                                    "access_public,"
                                                    "restrictions,"
                                                    "contact_id,"
                                                    "schedule_id,"
                                                    "port_ids "
                                                  "FROM "
                                                    "stations "
                                                  "WHERE "
                                                    "latitude IS ?1 AND "
                                                    "longitude IS ?2")
                                .arg(location.latitude)
                                .arg(location.longitude)));
  }
  catch(std::string& error)
  {
    std::cerr << "sql error: " << error << std::endl;
  }
  return {};
}
