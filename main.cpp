#include <sys/types.h>
#include <sys/stat.h>

#include <list>
#include <stack>
#include <string>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cassert>

#include <simplified/simple_sqlite.h>
#include <simplified/simple_curl.h>

//#include "cookie_calculator.h"
//#include <scrapers/utilities.h>

#include <scrapers/scraper_base.h>
//#include <scrapers/evgo_scraper.h>
#include <scrapers/chargehub_scraper.h>

#include <unistd.h>

constexpr std::string_view dbfile = "stations.db";


std::size_t curl_to_string(char* data, std::size_t size, std::size_t nmemb, std::string* string)
{
  if (!string)
    return 0;
  string->append(data, size * nmemb);
  return size * nmemb;
}

bool list_sorter(const std::pair<std::string, ScraperBase*>& first,
                 const std::pair<std::string, ScraperBase*>& second)
  { return first.first < second.first; }


template<typename T>
std::ostream & operator << (std::ostream &out, const std::optional<T>& value) noexcept
{
  if(!value)
    out << "null,";
  else
    out << *value;
  return out;
}

std::vector<uint8_t> serialize(int32_t data)
{
  auto a = reinterpret_cast<uint8_t*>(&data);
  return std::vector<uint8_t>({ a[0], a[1], a[2], a[3] });
}

void db_init(sql::db& db, std::string_view filename)
{
  using namespace std::literals;

  assert(db.open(filename));
  assert(db.execute("PRAGMA synchronous = OFF"));
  assert(db.execute("PRAGMA journal_mode = MEMORY"));

  std::string_view stations_table_desc = R"(
    CREATE TABLE IF NOT EXISTS stations (
      "station_id" INTEGER DEFAULT NULL,
      "network_id" INTEGER DEFAULT NULL,
      "network_station_id" INTEGER DEFAULT NULL,
      "latitude" REAL NOT NULL,
      "longitude" REAL NOT NULL,
      "name" TEXT NOT NULL,
      "description" TEXT DEFAULT NULL,
      "street_number" INTEGER,
      "street_name" TEXT,
      "city" TEXT,
      "state" TEXT,
      "country" TEXT,
      "zipcode" TEXT,
      "phone_number" TEXT DEFAULT NULL,
      "URL_id" INTEGER DEFAULT NULL,
      "access_public" BOOLEAN DEFAULT NULL,
      "access_restrictions" TEXT DEFAULT NULL,

      "price_free" BOOLEAN DEFAULT NULL,
      "price_string" TEXT DEFAULT NULL,
      "initialization" TEXT DEFAULT NULL,

      "port_ids" BLOB DEFAULT NULL
    ))";

  std::string_view ports_table_desc = R"(
    CREATE TABLE IF NOT EXISTS ports (
      "station_id" INTEGER DEFAULT NULL,
      "port_id" INTEGER DEFAULT NULL,
      "network_id" INTEGER DEFAULT NULL,
      "network_station_id" INTEGER DEFAULT NULL,
      "network_port_id" TEXT DEFAULT NULL,

      "power_id" INTEGER NOT NULL,

      "price_string" TEXT DEFAULT NULL,
      "initialization" TEXT DEFAULT NULL,
      "price_free" BOOLEAN DEFAULT NULL,
      "price_unit" INTEGER DEFAULT NULL,
      "price_initial" FLOAT DEFAULT NULL,
      "price_per_kWh" FLOAT DEFAULT NULL,

      "display_name" TEXT DEFAULT NULL,
      "weird" BOOLEAN DEFAULT FALSE
    ))";

  std::string_view power_table_desc = R"(
    CREATE TABLE IF NOT EXISTS power (
      "power_id" INTEGER PRIMARY KEY,
      "level" INTEGER NOT NULL,
      "connector" INTEGER DEFAULT NULL,
      "amp" TEXT DEFAULT NULL,
      "kW" TEXT DEFAULT NULL,
      "volt" TEXT DEFAULT NULL
    ))";

  std::string_view URLs_table_desc = R"(
    CREATE TABLE IF NOT EXISTS URLs (
      "URL_id" INTEGER PRIMARY KEY,
      "URL" TEXT NOT NULL UNIQUE
    ))";

  std::string_view networks_table_desc = R"(
    CREATE TABLE IF NOT EXISTS networks (
      "network_id" INTEGER PRIMARY KEY,
      "name" TEXT NOT NULL
    ))";

  assert(db.execute(stations_table_desc));
  assert(db.execute(ports_table_desc));
  assert(db.execute(power_table_desc));
  assert(db.execute(URLs_table_desc));
  assert(db.execute(networks_table_desc));

  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Non-Networked", 17))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Unknown", 0))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("AmpUp", 42))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Astria", 15))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Azra", 14))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("BC Hydro EV", 27))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Blink", 1))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("ChargeLab", 31))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("ChargePoint", 2))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Circuit Électrique", 4))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("CityVitae", 52))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Co-op Connect", 47))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("eCharge", 23))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EcoCharge", 37))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Electrify America", 26))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Electrify Canada", 36))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EV Link", 18))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EV Range", 54))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EVConnect", 22))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EVCS", 43))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EVduty", 20))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("evGateway", 44))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EVgo", 8))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EVMatch", 35))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EVolve NY", 38))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("EVSmart", 45))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Flash", 59))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Flo", 3))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("FPL Evolution", 41))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Francis Energy", 39))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("GE", 16))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Go Station", 48))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Hypercharge", 49))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Ivy", 34))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Livingston Energy", 46))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("myEVroute", 21))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("NL Hydro", 53))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Noodoe EV", 30))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("OK2Charge", 55))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("On the Run", 58))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("OpConnect", 9))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Petro-Canada", 32))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("PHI", 50))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Powerflex", 40))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("RED E", 60))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Rivian", 51))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("SemaConnect", 5))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Shell Recharge", 6))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Sun Country Highway", 11))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("SWTCH", 28))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("SYNC EV", 33))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Tesla", 12))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Universal", 56))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Voita", 19))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("Webasto", 7))"));
  assert(db.execute(R"(INSERT OR IGNORE INTO networks (name, network_id) VALUES ("ZEF Energy", 25))"));
}

bool exists_in_db(sql::db& db, const station_info_t& data)
{
  try
  {
    sql::query q;
    if(data.station_id)
    {
      q = std::move(db.build_query("SELECT station_id FROM stations WHERE station_id=?1")
                               .arg(data.station_id));
    }
    assert(q.valid() && q.lastError() == SQLITE_OK);
    assert(q.execute());

    if(q.fetchRow())
      return true;
  }
  catch(std::string& error)
  {
    std::cerr << "sql error: " << error << std::endl;
  }
  return false;
}

void add_to_db(sql::db& db, station_info_t& data)
{
  std::clog << "chargehub" << " - latitude: " << data.latitude<< " - longitude: " << data.longitude  << std::endl;
  try
  {
    std::optional<std::vector<uint8_t>> port_ids;
    if(!data.ports.empty())
    {
      port_ids.emplace(0);
      while(!data.ports.empty())
      {
        auto port = data.ports.top();
        data.ports.pop();
        std::optional<uint64_t> power_id;
        sql::query q;
        if(port.port_id)
        {
          q = std::move(db.build_query("INSERT OR ABORT INTO power (level, connector, amp, kW, volt) VALUES (?1,?2,?3,?4,?5)")
                                   .arg(port.level)
                                   .arg(port.connector)
                                   .arg(port.amp, sql::reference)
                                   .arg(port.kw, sql::reference)
                                   .arg(port.volt, sql::reference));
          assert(q.execute() || q.lastError() == SQLITE_CONSTRAINT_UNIQUE);

          q = std::move(db.build_query("SELECT power_id FROM power WHERE level=?1 AND connector=?2 AND amp=?3 AND kW=?4 AND volt=?5")
                        .arg(port.level)
                        .arg(port.connector)
                        .arg(port.amp, sql::reference)
                        .arg(port.kw, sql::reference)
                        .arg(port.volt, sql::reference));
          assert(q.execute());
          if(q.fetchRow())
            q.getField(power_id);
        }

        q = std::move(db.build_query("INSERT OR IGNORE INTO ports ("
                                     "station_id,"
                                     "port_id,"
                                     "network_id,"
                                     "network_station_id,"
                                     "network_port_id,"
                                     "power_id,"
                                     "price_free,"
                                     "price_string,"
                                     "initialization,"
                                     "display_name,"
                                     "weird)"
                                     " VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11)")
                      .arg(data.station_id)
                      .arg(port.port_id)
                      .arg(data.network_id)
                      .arg(data.network_station_id)
                      .arg(port.network_port_id, sql::reference)
                      .arg(power_id)
                      .arg(port.price_free)
                      .arg(port.price_string, sql::reference)
                      .arg(port.initialization, sql::reference)
                      .arg(port.display_name, sql::reference)
                      .arg(port.weird));
        assert(q.execute());

        auto data = serialize(*port.port_id);
        port_ids->insert(
              port_ids->end(),
              std::make_move_iterator(std::begin(data)),
              std::make_move_iterator(std::end(data))
            );
        std::clog << "added port:" << int(*port.port_id) << std::endl;
      }
    }

    std::optional<uint64_t> URL_id;
    if(data.URL)
    {
      sql::query q = std::move(db.build_query("INSERT OR IGNORE INTO URLs (URL) VALUES (?1)")
                               .arg(data.URL));
      assert(q.execute());

      q = std::move(db.build_query("SELECT URL_id FROM URLs WHERE URL=?1")
                    .arg(data.URL));
      assert(q.execute());
      if(q.fetchRow())
        q.getField(URL_id);
    }

    if(!data.station_id)
    {
      sql::query q = std::move(db.build_query("SELECT station_id FROM stations WHERE network_id=?1 AND network_station_id=?2")
                               .arg(data.network_id)
                               .arg(data.network_station_id));
      assert(q.execute());
      if(q.fetchRow())
        q.getField(data.station_id);
    }

    sql::query q = std::move(db.build_query("INSERT OR ABORT INTO stations ("
                                            "station_id,"
                                            "network_id,"
                                            "network_station_id,"
                                            "latitude,"
                                            "longitude,"
                                            "name,"
                                            "description,"
                                            "street_number,"
                                            "street_name,"
                                            "city,"
                                            "state,"
                                            "country,"
                                            "zipcode,"
                                            "phone_number,"
                                            "URL_id,"
                                            "access_public,"
                                            "access_restrictions,"
                                            "price_free,"
                                            "price_string,"
                                            "initialization,"
                                            "port_ids)"
                                            " VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13,?14,?15,?16,?17,?18,?19,?20,?21)")
                             .arg(data.station_id)
                             .arg(data.network_id)
                             .arg(data.network_station_id)
                             .arg(data.latitude)
                             .arg(data.longitude)
                             .arg(data.name, sql::reference)
                             .arg(data.description, sql::reference)
                             .arg(data.street_number)
                             .arg(data.street_name, sql::reference)
                             .arg(data.city, sql::reference)
                             .arg(data.state, sql::reference)
                             .arg(data.country, sql::reference)
                             .arg(data.zipcode, sql::reference)
                             .arg(data.phone_number, sql::reference)
                             .arg(URL_id)
                             .arg(data.access_public)
                             .arg(data.access_restrictions, sql::reference)
                             .arg(data.price_free)
                             .arg(data.price_string, sql::reference)
                             .arg(data.initialization, sql::reference)
                             .arg(port_ids));
    assert(q.execute());

    std::clog << "added station:" << int(*data.station_id) << std::endl;
  }
  catch(std::string& error)
  {
    std::cerr << "sql error: " << error << std::endl;
  }
}

static SimpleCurl& static_request(void)
{
  static SimpleCurl request;
  static bool have_init = false;
  if(!have_init)
  {
    have_init = true;
    request.reset();
    request.setOpt(CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64; rv:81.0) Gecko/20100101 Firefox/81.0");
    request.setOpt(CURLOPT_TCP_KEEPALIVE, 1);
    request.setOpt(CURLOPT_WRITEFUNCTION, curl_to_string);
  }
  return request;
}

std::string get_page(const std::string& name, const station_info_t& data)
{
  std::string output;
  auto& request = static_request();
  request.setOpt(CURLOPT_WRITEDATA, &output);

  request.setOpt(CURLOPT_POST, !data.post_data.empty());

  if(!data.post_data.empty())
    request.setOpt(CURLOPT_POSTFIELDS, data.post_data);

  for(const auto& pair : data.header_fields)
    request.setHeaderField(pair.first, pair.second);

  std::cout << "requesting: " << data.details_URL << std::endl;
  if(!data.post_data.empty())
    std::cout << "  with post data: " << data.post_data << std::endl;

  if((!request.setOpt(CURLOPT_URL, data.details_URL) ||
      !request.perform()) &&
     request.getLastError() != CURLE_REMOTE_ACCESS_DENIED)
  {
    std::cerr << "scraper: " << name << std::endl
              << "station id: " << data.station_id << std::endl
              << "name: " << data.name << std::endl
              << "error: " << request.getLastError() << std::endl;
  }
  else
    std::cout << output << std::endl;
  return output;
}


int main(int argc, char* argv[])
{
  curl_global_init(CURL_GLOBAL_DEFAULT);
  atexit(curl_global_cleanup);

  // enable automatic flushing
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  std::clog << std::unitbuf;

  std::string output;
  //std::list<station_info_t> indexed_data, station_data, download_data;

  std::list<std::pair<std::string, ScraperBase*>> scrapers =
  {
//    { "evgo", new EVGoScraper() },
    { "chargehub", new ChargehubScraper ( /* 40.5, 40.75 */ ) },
  };

  scrapers.sort(list_sorter);

  if(argc == 2)
  {
    std::pair<std::string, ScraperBase*> prev;
    for(auto& pair : scrapers)
    {
      if(!prev.first.empty())
      {
        scrapers.remove(prev);
        prev.first.clear();
      }
      if(pair.first != argv[1])
      {
        delete pair.second;
        pair.second = nullptr;
        prev = pair;
      }
    }
    if(!prev.first.empty())
    {
      scrapers.remove(prev);
      prev.first.clear();
    }
  }

  if(scrapers.empty())
  {
    std::cerr << "No scrappers queued!" << std::endl
              << "Exiting." << std::endl;
  }
  else
  {    
    std::cerr << "Scraper queue: ";
    for(const auto& pair : scrapers)
      std::cerr << pair.first << ", ";
    std::cerr << std::endl;

    sql::db db;
    db_init(db, dbfile);
    uintptr_t total_insertions = 0;
    uintptr_t insertion_count = 0;



    for(const auto& pair : scrapers)
    {
      std::stack<station_info_t> data;
      ScraperBase* scraper = pair.second;
      std::clog << pair.first << ": scraper active" << std::endl;

      //static_request().setOpt(CURLOPT_COOKIE, ""); // erase all cookies and enable cookies

      output.clear();
      data = scraper->Init();

      while(!data.empty())
      {
        auto d = data.top(); data.pop();
        output.clear();
        output = get_page(pair.first, d);
        std::stack<station_info_t> new_data = scraper->Parse(d, output);

        while(!new_data.empty())
        {
          auto nd = new_data.top(); new_data.pop();
          if(nd.parser == Parser::Complete)
            add_to_db(db, nd);
          else if(nd.parser != Parser::Station ||
                  !exists_in_db(db, nd))
            data.push(nd);
        }
      }
      std::clog << pair.first << " insertions made: " << insertion_count << std::endl;

      total_insertions += insertion_count;
      insertion_count = 0;
      delete scraper, scraper = nullptr;
    }
    std::clog << "total insertions: " << total_insertions << std::endl;
  }
  return EXIT_SUCCESS;
}
