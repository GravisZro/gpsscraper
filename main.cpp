#include <sys/types.h>
#include <sys/stat.h>

#include <list>
#include <stack>
#include <string>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <cctype>
#include <cassert>
#include <cmath>

#include <simplified/simple_sqlite.h>
#include <simplified/simple_curl.h>

//#include "cookie_calculator.h"

#include <scrapers/utilities.h>
#include <scrapers/scraper_base.h>
#include <scrapers/circuitelectrique.h>
#include <scrapers/evgo.h>
#include <scrapers/electrifyamerica.h>
#include <scrapers/chargehub.h>

#include <unistd.h>

#include "dbinterface.h"

using namespace std::string_literals;
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
    request.setOpt(CURLOPT_FOLLOWLOCATION, 1);
  }
  return request;
}

std::string get_page(const std::string& name, const pair_data_t& data)
{
  std::string output;
  auto& request = static_request();
  request.setOpt(CURLOPT_WRITEDATA, &output);

  request.setOpt(CURLOPT_POST, !data.query.post_data.empty());

  if(!data.query.post_data.empty())
    request.setOpt(CURLOPT_POSTFIELDS, data.query.post_data);

  request.clearHeaderFields();
  for(const auto& pair : data.query.header_fields)
    request.setHeaderField(pair.first, pair.second);

  std::cout << "requesting: " << data.query.URL << std::endl;
  if(!data.query.post_data.empty())
    std::cout << "  with post data: " << data.query.post_data << std::endl;

  if((!request.setOpt(CURLOPT_URL, data.query.URL) ||
      !request.perform()) &&
     request.getLastError() != CURLE_REMOTE_ACCESS_DENIED)
  {
    std::cerr << "scraper: " << name << std::endl
              << "station id: " << data.station.station_id << std::endl
              << "name: " << data.station.name << std::endl
              << "error: " << request.getLastError() << std::endl;
  }
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

  std::list<std::pair<std::string, ScraperBase*>> scrapers =
  {
    { "circuit_electrique", new CircuitElectriqueScraper() },
    //{ "evgo", new EVGoScraper() },
    //{ "electrify_america", new ElectrifyScraper() },
    //{ "chargehub", new ChargehubScraper ( /* 40.5, 40.75 */ ) },
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

    DBInterface db(dbfile);

    uintptr_t total_insertions = 0;
    uintptr_t insertion_count = 0;

    try
    {
      for(const auto& pair : scrapers)
      {
        insertion_count = 0;
        std::vector<pair_data_t> data;
        ScraperBase* scraper = pair.second;
        std::clog << pair.first << ": scraper active" << std::endl;

        //static_request().setOpt(CURLOPT_COOKIE, ""); // erase all cookies and enable cookies
        {
          pair_data_t nd;
          nd.query.parser = Parser::BuildQuery | Parser::Initial;
          nd.query.node_id = 0;
          data.emplace_back(nd);
        }

        std::unordered_set<uint64_t> station_nodes, port_nodes; // used to avoid duplicate requests
        while(!data.empty())
        {
          auto pos = data.back();
          data.pop_back();

          if(static_cast<typename std::underlying_type_t<Parser>>(pos.query.parser) &
             static_cast<typename std::underlying_type_t<Parser>>(Parser::BuildQuery))
          {
            pos = scraper->BuildQuery(pos);
          }
          std::string result;
          for(int i = 0; result.empty(); ++i)
          {
            if(i)
              std::clog << "retry #" << i << std::endl;
            result = get_page(pair.first, pos);
          }
          std::vector<pair_data_t> new_data = scraper->Parse(pos, result);
          std::clog << "result count: " << new_data.size() << std::endl;

          for(auto& nd : new_data)
          {
            switch(nd.query.parser)
            {
              case Parser::Discard:
                break;
              case Parser::Complete:
                db.addStation(nd.station), ++insertion_count;
                break;

              case Parser::UpdateRecord | Parser::MapArea:
                db.addMapLocation(nd);
                break;

              case Parser::BuildQuery | Parser::MapArea:
                if(!db.identifyMapLocation(nd))
                {
                  db.addMapLocation(nd);
                  data.emplace_back(nd);
                }
                break;

              case Parser::BuildQuery | Parser::Station:
                if(auto id = *nd.query.node_id; !station_nodes.contains(id))
                  station_nodes.insert(id),data.emplace_back(nd);
                break;

              case Parser::BuildQuery | Parser::Port:
                if(auto id = *nd.query.node_id; !port_nodes.contains(id))
                  port_nodes.insert(id),data.emplace_back(nd);
                break;

              default:
                data.emplace_back(nd);
            }
          }
        }
        std::clog << pair.first << " insertions made: " << insertion_count << std::endl;

        total_insertions += insertion_count;
        delete scraper, scraper = nullptr;
      }
      std::clog << "total insertions: " << total_insertions << std::endl;
    }
    catch(std::string& error) // parser or SQL failure
    {
      std::clog << "An irrecoverable error has occurred. Parsing halted." << std::endl;
      std::clog << "{last scraper]:" << " insertions made: " << insertion_count << std::endl;
      std::clog << "total insertions: " << total_insertions << std::endl;
      std::cerr << "ERROR: " << error << std::endl;
    }
    catch(std::logic_error& error)
    {
      std::cerr << "ERROR: " << error.what() << std::endl;
    }
  }

  return EXIT_SUCCESS;
}
