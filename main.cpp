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

std::ostream & operator << (std::ostream &out, const station_info_t& station_info) noexcept
{
  out << station_info.latitude << ','
      << station_info.longitude << ','
      << '"' << station_info.name << '"' << ','
      << station_info.street_number << ','
      << '"' << station_info.street_name << '"' << ','
      << '"' << station_info.city << '"' << ','
      << '"' << station_info.state << '"' << ','
      << '"' << station_info.country << '"' << ','
      << station_info.zipcode << ',';

  if(station_info.phone_number)
    out << '"' << *station_info.phone_number << '"' << ',';
  if(station_info.times_accessible)
    out << '"' << *station_info.times_accessible << '"' << ',';
  if(station_info.price_string)
    out << '"' << *station_info.price_string << '"' << ',';
  if(station_info.payment_types)
    out << '"' << *station_info.payment_types << '"' << ',';

  if(station_info.CHAdeMO)
    out << int(*station_info.CHAdeMO) << ',';
  if(station_info.JPlug)
    out << int(*station_info.JPlug) << ',';
  if(station_info.J1772_combo)
    out << int(*station_info.J1772_combo);

  return out;
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

  if(scraper->StageCount() == stage)
  {
    std::list<station_info_t> rdata;
    sql::db db;
    int key = 0;

    assert(db.open("stations.db"));

    assert(db.execute("CREATE TABLE IF NOT EXISTS 'stations' ("
                        "'key' INTEGER NOT NULL UNIQUE,"
                        "'latitude' REAL NOT NULL,"
                        "'longitude' REAL NOT NULL,"
                        "'name' TEXT NOT NULL,"
                        "'street_number' INTEGER,"
                        "'street_name' TEXT,"
                        "'city' TEXT,"
                        "'state' TEXT,"
                        "'country' TEXT,"
                        "'zipcode' TEXT,"
                        "'phone_number' TEXT,"
                        "'times_accessible' TEXT,"
                        "'price_string' TEXT,"
                        "'payment_types' TEXT,"
                        "'CHAdeMO' INTEGER NOT NULL DEFAULT 0,"
                        "'JPlug' INTEGER NOT NULL DEFAULT 0,"
                        "'J1772_combo' INTEGER NOT NULL DEFAULT 0,"
                        "PRIMARY KEY('key')"
                      ")"));


    for(const auto& station_info : tmp)
    {
      std::string url;
      try
      {
        key = 0;
        sql::query q = std::move(db.build_query("SELECT key FROM stations WHERE key=?1")
                                   .arg(station_info.station_id));
        assert(q.valid() && q.lastError() == SQLITE_OK);
        assert(q.execute());

        if(q.fetchRow())
          q.getField(key);
      }
      catch(std::string& error)
      {
        std::cerr << "sql error: " << error << std::endl;
      }

      if(!key)
      {
        try
        {
          sql::query q = std::move(db.build_query("INSERT OR IGNORE INTO stations (key,latitude,longitude,name,street_number,street_name,city,state,country,zipcode,phone_number,times_accessible,price_string,payment_types,CHAdeMO,JPlug,J1772_combo)"
                                                  " VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13,?14,?15,?16,?17)")
                                     .arg(station_info.station_id)
                                     .arg(station_info.latitude)
                                     .arg(station_info.longitude)
                                     .arg(station_info.name)
                                     .arg(station_info.street_number)
                                     .arg(station_info.street_name)
                                     .arg(station_info.city)
                                     .arg(station_info.state)
                                     .arg(station_info.country)
                                     .arg(station_info.zipcode)
                                     .arg(station_info.phone_number)
                                     .arg(station_info.times_accessible)
                                     .arg(station_info.price_string)
                                     .arg(station_info.payment_types)
                                     .arg(station_info.CHAdeMO)
                                     .arg(station_info.JPlug)
                                     .arg(station_info.J1772_combo));
          assert(q.execute());
          ++insertions;
        }
        catch(std::string& error)
        {
          std::cerr << "sql error: " << error << std::endl;
        }
      }
    }
  }

  if(scraper->StageCount() == stage)
    for(const auto& print_info : tmp)
      std::cout << print_info << std::endl;

  storage.merge(tmp);
}

int main(int argc, char* argv[])
{
#if 1
  std::list<station_info_t> rdata;
  sql::db db;

  assert(db.open("stations.db"));
  try
  {
    sql::query q = db.build_query("SELECT latitude, longitude, name, street_number, street_name, city, state, country, zipcode, price_string, CHAdeMO FROM stations WHERE AND 1=1");//street_number IS NULL");

    q.execute();

    while(q.fetchRow())
    {
      station_info_t data;
      q.getField(data.latitude)
          .getField(data.longitude)
          .getField(data.name)
          .getField(data.street_number)
          .getField(data.street_name)
          .getField(data.city)
          .getField(data.state)
          .getField(data.country)
          .getField(data.zipcode)
          .getField(data.price_string)
          .getField(data.CHAdeMO);


      std::cout << data.longitude << ','
          << data.latitude << ','
          << '"' << data.name << '"' << ','
          << '"';

      std::cout << "Plugs: " << int(*data.CHAdeMO) /*<< std::endl << ';'
                << data.price_string << std::endl << ';';

//      if(data.street_number)
//        std::cout << data.street_number << ' ';

      std::cout << data.street_name << std::endl << ';'
                << data.city << ", "
                << data.state << ", "
                << data.zipcode */
                << '"' << std::endl;
    }
  }
  catch(std::string& error)
  {
    std::cerr << "sql error: " << error << std::endl;
  }

/*
  assert(db.execute("CREATE TABLE IF NOT EXISTS 'stations' ("
                      "'key' INTEGER NOT NULL UNIQUE,"
                      "'latitude' REAL NOT NULL,"
                      "'longitude' REAL NOT NULL,"
                      "'name' TEXT NOT NULL,"
                      "'street_number' INTEGER,"
                      "'street_name' TEXT,"
                      "'city' TEXT,"
                      "'state' TEXT,"
                      "'country' TEXT,"
                      "'zipcode' INTEGER,"
                      "'phone_number' TEXT,"
                      "'times_accessible' TEXT,"
//                        "'price_number' NUMERIC,"
                      "'price_string' TEXT,"
                      "'payment_types' TEXT,"
                      "'CHAdeMO' INTEGER NOT NULL DEFAULT 0,"
                      "'EV_Plug' INTEGER NOT NULL DEFAULT 0,"
                      "'J1772_combo' INTEGER NOT NULL DEFAULT 0,"
                      "PRIMARY KEY('key')"
                    ")"));
*/

#else
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
    { "chargehub", new ChargehubScraper },
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
          std::clog << pair.first << ": station url: " << station_info.url << std::endl;

          output.clear();
          if(!station_info.post_data.empty())
          {
            std::clog << pair.first << ": post data: " << station_info.post_data << std::endl;
            request.setOpt(CURLOPT_POST, 1);
            request.setOpt(CURLOPT_POSTFIELDS, station_info.post_data);
          }
          else
            request.setOpt(CURLOPT_POST, 0);

          if(request.setOpt(CURLOPT_URL, station_info.url) &&
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
          std::clog << pair.first << ": station url: " << station_info.url << std::endl;

          output.clear();
          if(!station_info.post_data.empty())
          {
            std::clog << pair.first << ": post data: " << station_info.post_data << std::endl;
            request.setOpt(CURLOPT_POST, 1);
            request.setOpt(CURLOPT_POSTFIELDS, station_info.post_data);
          }
          else
            request.setOpt(CURLOPT_POST, 0);

          if(request.setOpt(CURLOPT_URL, station_info.url) &&
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
#endif
  return EXIT_SUCCESS;
}
