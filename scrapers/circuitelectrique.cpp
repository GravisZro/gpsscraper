#include "circuitelectrique.h"

#include <shortcsv/shortcsv.h>

#include <cassert>

pair_data_t CircuitElectriqueScraper::BuildQuery(const pair_data_t& input)
{
  pair_data_t data = input;
  data.station.network_id = Network::Circuit_Électrique;
  data.query.parser = Parser::MapArea;
  data.query.URL = "https://lecircuitelectrique-data.s3.ca-central-1.amazonaws.com/stations/export_sites_en.csv";
  return data;
}
std::vector<pair_data_t> CircuitElectriqueScraper::Parse(const pair_data_t& data, const std::string& input)
{
  assert(data.query.parser == Parser::MapArea);
  auto record_vector = shortcsv::Parse(input);

  enum
  {
    StationId = 0,
    SiteName,
    Address,
    Street,
    Suite,
    City,
    State,
    PostalCode,
    Region,
    Level,
    Latitude,
    Longitude,
    Price,
    PriceUnit,
    LocationType,
    Power,
  };

  for(auto& record : record_vector)
  {
    assert(record.size() == 16);
  }

  return {};
}
