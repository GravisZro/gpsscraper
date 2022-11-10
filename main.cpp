#include <sys/types.h>
#include <sys/stat.h>

#include <list>
#include <string>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cassert>

#include <simplified/simple_sqlite.h>
#include <simplified/simple_curl.h>

//#include "cookie_calculator.h"

#include <scrapers/utilities.h>

#include <scrapers/scraper_base.h>
#include <scrapers/chargehub_scraper.h>

#include <unistd.h>

constexpr std::string_view dbfile = "stations.db";

//static sql::db db;


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
  std::vector<uint8_t> rval;
  uint32_t d = data;
  rval.push_back((d >> 24) & 0xFF);
  rval.push_back((d >> 16) & 0xFF);
  rval.push_back((d >>  8) & 0xFF);
  rval.push_back(d & 0xFF);
  return rval;
}

void db_init(void)
{
  using namespace std::literals;

  sql::db db;
  assert(db.open(dbfile));
  //assert(db.execute("PRAGMA synchronous = OFF"));
  //assert(db.execute("PRAGMA journal_mode = MEMORY"));

  std::string_view stations_table_desc = R"(
            "station_id" INTEGER NOT NULL UNIQUE,
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

            "network_id" INTEGER DEFAULT NULL,
            "price_free" BOOLEAN DEFAULT NULL,
            "price_string" TEXT DEFAULT NULL,
            "initialization" TEXT DEFAULT NULL,

            "port_ids" BLOB DEFAULT NULL,
            PRIMARY KEY("station_id")
      )";

  std::string_view ports_table_desc = R"(
            "port_id" INTEGER NOT NULL UNIQUE,
            "station_id" INTEGER NOT NULL,

            "level" INTEGER NOT NULL,
            "connector" INTEGER DEFAULT NULL,
            "amp" TEXT DEFAULT NULL,
            "kw" TEXT DEFAULT NULL,
            "volt" TEXT DEFAULT NULL,

            "price_string" TEXT DEFAULT NULL,
            "initialization" TEXT DEFAULT NULL,
            "price_free" BOOLEAN DEFAULT NULL,
            "price_unit" INTEGER DEFAULT NULL,
            "price_initial" FLOAT DEFAULT NULL,
            "price_per_KWh" FLOAT DEFAULT NULL,

            "network_id" INTEGER DEFAULT NULL,
            "display_name" TEXT DEFAULT NULL,
            "network_port_id" TEXT DEFAULT NULL,

            "weird" BOOLEAN DEFAULT FALSE,
            PRIMARY KEY("port_id")
      )";

  std::string_view URLs_table_desc = R"(
           "URL_id" INTEGER NOT NULL DEFAULT 0 UNIQUE,
           "URL" TEXT NOT NULL UNIQUE,
           PRIMARY KEY("URL_id" AUTOINCREMENT)
      )";
/*
  assert(db.execute("CREATE TABLE IF NOT EXISTS RAM.stations ("s.append(stations_table_desc) + ")"));
  assert(db.execute("CREATE TABLE IF NOT EXISTS RAM.ports ("s.append(ports_table_desc) + ")"));
  assert(db.execute("CREATE TABLE IF NOT EXISTS RAM.URLs ("s.append(URLs_table_desc) + ")"));
*/
  assert(db.execute("CREATE TABLE IF NOT EXISTS stations ("s.append(stations_table_desc) + ")"));
  assert(db.execute("CREATE TABLE IF NOT EXISTS ports ("s.append(ports_table_desc) + ")"));
  assert(db.execute("CREATE TABLE IF NOT EXISTS URLs ("s.append(URLs_table_desc) + ")"));

  assert(db.execute(
           R"(
          CREATE TABLE IF NOT EXISTS networks (
            "network_id" INTEGER NOT NULL UNIQUE,
            "name" TEXT NOT NULL,
            PRIMARY KEY("network_id")
          )
          )"));

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

void exec_stage(ScraperBase* scraper, int stage, const station_info_t& station_info, const std::string& page_text, std::list<station_info_t>& storage, uintptr_t& insertions)
{
  std::list<station_info_t> tmp;
  switch (stage)
  {
    case 1:
      tmp = scraper->ParseIndex(page_text);
      break;
    case 2:
      tmp = scraper->ParseStation(station_info, page_text);
      break;
    case 3:
      tmp = scraper->ParseDownload(station_info, page_text);
      break;
  }

  if(stage == 1)
  {
    uint32_t skipped = 0;
    sql::db db;
    assert(db.open(dbfile));

    for(auto pos = std::begin(tmp); pos != std::end(tmp);)
    //for(const auto& station_info : tmp)
    {
      try
      {
        sql::query q = std::move(db.build_query("SELECT station_id FROM stations WHERE station_id=?1")
                                   .arg(pos->station_id));
        assert(q.valid() && q.lastError() == SQLITE_OK);
        assert(q.execute());

        if(q.fetchRow())
        {
          auto target = pos;
          pos = std::next(pos);
          tmp.erase(target);
          skipped++;
        }
        else
          pos = std::next(pos);
      }
      catch(std::string& error)
      {
        std::cerr << "sql error: " << error << std::endl;
      }
    }
    std::clog << "entries added: " << tmp.size() << "  - entries skipped: " << skipped << std::endl;
  }

  if(scraper->StageCount() == stage)
  {
    std::list<station_info_t> rdata;
    sql::db db;
    assert(db.open(dbfile));



    for(const auto& station_info : tmp)
    {
      //std::clog << "chargehub" << " - latitude: " << station_info.latitude<< " - longitude: " << station_info.longitude  << std::endl;
      try
      {
        std::optional<std::vector<uint8_t>> port_ids;
        if(!station_info.ports.empty())
        {
          port_ids.emplace(0);
          for(const port_t& port : station_info.ports)
          {
            sql::query q = std::move(db.build_query("INSERT OR IGNORE INTO ports ("
                                                    "port_id,"
                                                    "station_id,"
                                                    "level,"
                                                    "connector,"
                                                    "amp,"
                                                    "kw,"
                                                    "volt,"
                                                    "price_free,"
                                                    "price_string,"
                                                    "initialization,"
                                                    "network_id,"
                                                    "display_name,"
                                                    "network_port_id,"
                                                    "weird)"
                                                    " VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13,?14)")
                                     .arg(port.port_id)
                                     .arg(station_info.station_id)
                                     .arg(port.level)
                                     .arg(port.connector)
                                     .arg(port.amp, sql::reference)
                                     .arg(port.kw, sql::reference)
                                     .arg(port.volt, sql::reference)
                                     .arg(port.price_free)
                                     .arg(port.price_string, sql::reference)
                                     .arg(port.initialization, sql::reference)
                                     .arg(port.network_id)
                                     .arg(port.display_name, sql::reference)
                                     .arg(port.network_port_id, sql::reference)
                                     .arg(port.weird));
              assert(q.execute());


            auto data = serialize(*port.port_id);
            port_ids->insert(
                  port_ids->end(),
                  std::make_move_iterator(std::begin(data)),
                  std::make_move_iterator(std::end(data))
                );
          }
        }

        std::optional<uint64_t> URL_id;
        if(station_info.URL)
        {
          sql::query q = std::move(db.build_query("INSERT OR IGNORE INTO URLs (URL) VALUES (?1)")
                                   .arg(station_info.URL));
          assert(q.execute());

          q = std::move(db.build_query("SELECT URL_id FROM URLs WHERE URL=?1")
                        .arg(station_info.URL));
          assert(q.execute());
          if(q.fetchRow())
            q.getField(URL_id);
        }

        sql::query q = std::move(db.build_query("INSERT OR IGNORE INTO stations ("
                                                "station_id,"
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
                                                "network_id,"
                                                "price_free,"
                                                "price_string,"
                                                "initialization,"
                                                "port_ids)"
                                                " VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13,?14,?15,?16,?17,?18,?19,?20)")
                                  .arg(station_info.station_id)
                                  .arg(station_info.latitude)
                                  .arg(station_info.longitude)
                                  .arg(station_info.name, sql::reference)
                                  .arg(station_info.description, sql::reference)
                                  .arg(station_info.street_number)
                                  .arg(station_info.street_name, sql::reference)
                                  .arg(station_info.city, sql::reference)
                                  .arg(station_info.state, sql::reference)
                                  .arg(station_info.country, sql::reference)
                                  .arg(station_info.zipcode, sql::reference)
                                  .arg(station_info.phone_number, sql::reference)
                                  .arg(URL_id)
                                  .arg(station_info.access_public)
                                  .arg(station_info.access_restrictions, sql::reference)
                                  .arg(station_info.network_id)
                                  .arg(station_info.price_free)
                                  .arg(station_info.price_string, sql::reference)
                                  .arg(station_info.initialization, sql::reference)
                                  .arg(port_ids));
        assert(q.execute());

        ++insertions;
      }
      catch(std::string& error)
      {
        std::cerr << "sql error: " << error << std::endl;
      }
    }
  }

  storage.merge(tmp);
}

int main(int argc, char* argv[])
{
  curl_global_init(CURL_GLOBAL_DEFAULT);
  atexit(curl_global_cleanup);

  // enable automatic flushing
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  std::clog << std::unitbuf;

  SimpleCurl request;
  std::string output;
  std::list<station_info_t> indexed_data, station_data, download_data;

  std::list<std::pair<std::string, ScraperBase*>> scrapers =
  {
    { "chargehub", new ChargehubScraper /*(12.5, 13.0)*/ },
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

  std::cerr << "scrapers: ";
  for(const auto& pair : scrapers)
    std::cerr << pair.first << ", ";
  std::cerr << std::endl;

//  request.setOpt(CURLOPT_COOKIEFILE, ""); // enable cookies
  request.setOpt(CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64; rv:81.0) Gecko/20100101 Firefox/81.0");
  request.setOpt(CURLOPT_TCP_KEEPALIVE, 1);

  if(scrapers.empty())
  {

  }
  else
  {
    db_init();
    uintptr_t station_count = 0;
    uintptr_t total_insertions = 0;
    uintptr_t insertion_count = 0;

    request.setOpt(CURLOPT_WRITEDATA, &output);
    request.setOpt(CURLOPT_WRITEFUNCTION, curl_to_string);

    for(const auto& pair : scrapers)
    {
      indexed_data.clear();
      station_data.clear();
      download_data.clear();
      station_count = 0;
//      request.setOpt(CURLOPT_COOKIE, ""); // erase all cookies

      ScraperBase* scraper = pair.second;
      std::clog << pair.first << ": scraper active" << std::endl;
      do
      {
        std::clog << pair.first << ": index url: " << scraper->IndexURL() << std::endl;



        output.clear();
        if(request.setOpt(CURLOPT_URL, scraper->IndexURL()) &&
           request.perform())
        {
//          cookie_calculator(request, scraper->IndexURL(), output);
          exec_stage(scraper, 1, station_info_t(), output, indexed_data, insertion_count);
        }
        else
        {
          std::cerr << "scraper: " << pair.first << std::endl
                    << "index request: " << scraper->IndexURL() << std::endl
                    << "error: " << request.getLastError() << std::endl;
          continue;
        }
      } while(!scraper->IndexingComplete());

      if(scraper->StageCount() >= 2) // model url isn't final url
      {

        for(const auto& station_info : indexed_data)
        {
          ++station_count;
//          std::clog << pair.first << ": station URL: " << *station_info.details_URL << std::endl;

          output.clear();
          if(station_info.post_data && !station_info.post_data->empty())
          {

            std::clog << pair.first << ": post data: " << *station_info.post_data << std::endl;
            request.setOpt(CURLOPT_POST, 1);
            request.setOpt(CURLOPT_POSTFIELDS, *station_info.post_data);
          }
          else
            request.setOpt(CURLOPT_POST, 0);

          if(request.setOpt(CURLOPT_URL, *station_info.details_URL) &&
             request.perform())
            exec_stage(scraper, 2, station_info, output, station_data, insertion_count);
          else if(request.getLastError() != CURLE_REMOTE_ACCESS_DENIED)
          {
            std::cerr << "scraper: " << pair.first << std::endl
                      << "station id: " << station_info.station_id << std::endl
                      << "name: " << station_info.name << std::endl
                      << "error: " << request.getLastError() << std::endl;
          }
        }
      }

      if(scraper->StageCount() >= 3) // model url isn't final url
      {
        for(auto& station_info : station_data)
        {

          output.clear();
          if(!station_info.post_data->empty())
          {
//            std::clog << pair.first << ": post data: " << station_info.post_data << std::endl;
            request.setOpt(CURLOPT_POST, 1);
            request.setOpt(CURLOPT_POSTFIELDS, station_info.post_data);
          }
          else
            request.setOpt(CURLOPT_POST, 0);

          if(request.setOpt(CURLOPT_URL, station_info.details_URL) &&
             request.perform())
            exec_stage(scraper, 3, station_info, output, download_data, insertion_count);
        }
      }


      std::clog << pair.first << " stage 1 entries: " << indexed_data.size() << std::endl;
      if(scraper->StageCount() >= 2)
        std::clog << pair.first << " stage 2 entries: " << station_data.size() << std::endl;
      if(scraper->StageCount() >= 3)
        std::clog << pair.first << " stage 3 entries: " << download_data.size() << std::endl;

      std::clog << pair.first << " insertions made: " << insertion_count << std::endl;

      total_insertions += insertion_count;
      insertion_count = 0;
      delete scraper, scraper = nullptr;
    }
    std::clog << "total insertions: " << total_insertions << std::endl;
  }
  return EXIT_SUCCESS;
}
