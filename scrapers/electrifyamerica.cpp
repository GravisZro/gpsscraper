#include "electrifyamerica.h"

// STL
#include <iostream>
#include <algorithm>
#include <vector>
//#include <memory>

// libraries
#include <shortjson/shortjson.h>

// project
#include <tinf/src/tinf.h>
#include "utilities.h"

std::string decompress(const std::string& input)
{
  std::vector<uint8_t> buffer(0x000100000);
  unsigned int size = std::size(buffer);
  assert(tinf_gzip_uncompress(std::data(buffer), &size, std::data(input), std::size(input)) == 0);
  return std::string(reinterpret_cast<const char*>(std::data(buffer)), size);
}


pair_data_t ElectrifyAmericaScraper::BuildQuery(const pair_data_t& input)
{
  pair_data_t data = input;
  data.station.network_id = Network::Electrify_America;

  switch(data.query.parser)
  {
    default: throw std::string(__FILE__).append(": unknown parser: ").append(std::to_string(int(input.query.parser)));

    case Parser::BuildQuery | Parser::Initial:
    case Parser::BuildQuery | Parser::MapArea:
      data.query.parser = Parser::MapArea;
      data.query.URL = "https://api-prod.electrifyamerica.com/v2/locations";
      break;

    case Parser::BuildQuery | Parser::Station:
      assert(data.query.node_id);
      data.query.parser = Parser::Station;
      data.query.URL = "https://api-prod.electrifyamerica.com/v2/locations/" + std::to_string(*data.query.node_id);
      break;
  }
  return data;
}

std::vector<pair_data_t> ElectrifyAmericaScraper::Parse(const pair_data_t& data, const std::string& input)
{
  try
  {
    switch(data.query.parser)
    {
      default: throw std::string(__FILE__).append(": unknown parser: ").append(std::to_string(int(data.query.parser)));
      case Parser::MapArea: return ParseMapArea(data, input);
      case Parser::Station: return ParseStation(data, input);
    }
  }
  catch(int line_number)
  {
    std::cerr << __FILE__ << " threw from line: " << line_number << std::endl;
    std::cerr << "page dump:" << std::endl << input << std::endl;
  }
  catch(const char* msg)
  {
    std::cerr << "JSON parser threw: " << msg << std::endl;
    std::cerr << "page dump:" << std::endl << input << std::endl;
  }
  return {};
}

std::vector<pair_data_t> ElectrifyAmericaScraper::ParseMapArea([[maybe_unused]] const pair_data_t& data, const std::string& input)
{
  auto input_str = decompress(input);
  std::vector<pair_data_t> return_data;
  shortjson::node_t root = shortjson::Parse(input_str);

  if(root.type != shortjson::Field::Array)
    throw __LINE__;

  for(shortjson::node_t& nodeL0 : root.toArray())
  {
    if(nodeL0.type != shortjson::Field::Object)
      throw __LINE__;

    ext::string siteId;
    if(shortjson::FindString(nodeL0, siteId, "siteId"))
    {
      if(!siteId.is_number())
        throw __LINE__;
      pair_data_t nd;
      nd.query.parser = Parser::BuildQuery | Parser::Station;
      nd.query.node_id = ext::from_string<uint64_t>(siteId);
      return_data.emplace_back(nd);
    }
  }

  return return_data;
}

std::vector<pair_data_t> ElectrifyAmericaScraper::ParseStation(const pair_data_t& data, const std::string& input)
{
  shortjson::node_t root = shortjson::Parse(input);

  if(root.type != shortjson::Field::Object)
    throw __LINE__;

  pair_data_t nd;
  nd.query.parser = Parser::Complete;
  nd.query.node_id = data.query.node_id;
  nd.station.network_id = data.station.network_id;
  nd.station.access_public = true;

  for(shortjson::node_t& nodeL0 : root.toObject())
  {
    if(nodeL0.identifier == "siteId")
    {
      if(nodeL0.type != shortjson::Field::String)
        throw __LINE__;
      ext::string val = nodeL0.toString();
      if(!val.is_number())
        throw __LINE__;
      nd.station.station_id = ext::from_string<uint64_t>(val);
    }
    else if(nodeL0.identifier == "name")
      nd.station.name = safe_string<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "address")
    {
      auto val = safe_string<__LINE__>(nodeL0);
      try
      {
        auto pos = std::find_if(std::begin(*val), std::end(*val),
                       [](unsigned char c){ return std::isspace(c); });
        nd.station.contact.street_number = ext::from_string<int32_t>(std::string(std::begin(*val), pos));
        pos = std::next(pos);
        nd.station.contact.street_name = std::string(pos, std::end(*val));
      }
      catch(...)
      {
        nd.station.contact.street_name = *val;
      }
    }
    else if(nodeL0.identifier == "city")
      nd.station.contact.city = safe_string<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "state")
      nd.station.contact.state = safe_string<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "country")
      nd.station.contact.country = safe_string<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "postalCode")
      nd.station.contact.postal_code = safe_string<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "coordinates")
    {
      if(nodeL0.type != shortjson::Field::Object)
        throw __LINE__;
      for(shortjson::node_t& nodeL1 : nodeL0.toObject())
      {
        if(nodeL1.identifier == "latitude")
          nd.station.location.latitude = safe_float64<__LINE__>(nodeL1);
        else if(nodeL1.identifier == "longitude")
          nd.station.location.longitude = safe_float64<__LINE__>(nodeL1);
      }
    }
    else if(nodeL0.identifier == "type")
    {
      if(nodeL0.type != shortjson::Field::String)
        throw __LINE__;
      if(nodeL0.toString() == "PUBLIC" ||
         nodeL0.toString() == "COMMERCIAL")
        nd.station.access_public = true;
      else if(nodeL0.toString() == "PRIVATE")
        nd.station.access_public = false;
      else if (nodeL0.toString() == "COMING_SOON")
      {
        nd.query.parser = Parser::Discard;
        break; // exit loop
      }
      else
        throw __LINE__;
    }
    else if(nodeL0.identifier == "description")
      nd.station.description = safe_string<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "restricted")
    {
      auto val = safe_bool<__LINE__>(nodeL0);
      assert(val && !*val);
    }
    else if(nodeL0.identifier == "openingTimes")
    {
      if(nodeL0.type != shortjson::Field::Object)
        throw __LINE__;

      for(shortjson::node_t& nodeL1 : nodeL0.toObject())
      {
        if(nodeL1.identifier == "regularHours")
        {
          if(nodeL1.type != shortjson::Field::Array)
            throw __LINE__;
          for([[maybe_unused]] shortjson::node_t& nodeL2 : nodeL1.toArray())
          {
            throw __LINE__; // never seen
          }
        }
      }
    }
    else if(nodeL0.identifier == "pricing")
    {
      if(nodeL0.type != shortjson::Field::Array)
        throw __LINE__;
      for(shortjson::node_t& nodeL1 : nodeL0.toArray())
      {
        if(nodeL1.type != shortjson::Field::Object)
          throw __LINE__;
        price_t price;
        price.currency = Currency::USD;
        for(shortjson::node_t& nodeL2 : nodeL1.toObject())
        {
          if(nodeL2.identifier == "time")
          {
            price.unit = Unit::Minutes;
            price.per_unit = safe_float64<__LINE__>(nodeL2);
          }
          else if(nodeL2.identifier == "energy")
          {
            price.unit = Unit::Kilowatts;
            price.per_unit = safe_float64<__LINE__>(nodeL2);
          }

          else if(nodeL2.identifier == "maxPower")
          {
            double val = safe_float64<__LINE__>(nodeL2);
            if(val > 7)
              price.payment = Payment::Credit | Payment::API;
            else
              price.payment = Payment::API;

            for(auto& port : nd.station.ports)
              if(port.power.amp <= val && // if within correct amp range
                 (!port.price.per_unit || price.per_unit < *port.price.per_unit)) // no price OR lower price
                port.price = price;
          }
        }
      }
    }
    else if(nodeL0.identifier == "evses")
    {
      if(nodeL0.type != shortjson::Field::Array)
        throw __LINE__;
      for(shortjson::node_t& nodeL1 : nodeL0.toArray())
      {
        if(nodeL1.type != shortjson::Field::Object)
          throw __LINE__;
        port_t port;
        for(shortjson::node_t& nodeL2 : nodeL1.toObject())
        {
          if(nodeL2.identifier == "id")
          {
            if(nodeL2.type != shortjson::Field::String)
              throw __LINE__;
            if(!nd.station.station_id)
              throw __LINE__;
            ext::string val = nodeL2.toString();
            val.erase("EOL "); // End of Life?
            val.erase(std::to_string(*nd.station.station_id) + '-');
            uint64_t modifier = 0;

            if(val.erase("BT-") != std::string::npos)
              modifier += 100 ;
            if(val.erase("G4-") != std::string::npos)
              modifier += 200;
            if(!val.is_number())
              throw __LINE__;
            port.port_id = ext::from_string<uint64_t>(val) + modifier +
                           (*nd.station.station_id * 1000);
          }
          else if(nodeL2.identifier == "connectors")
          {
            if(nodeL2.type != shortjson::Field::Array)
              throw __LINE__;
            for(shortjson::node_t& nodeL3 : nodeL2.toArray())
            {
              if(nodeL3.type != shortjson::Field::Object)
                throw __LINE__;
              for(shortjson::node_t& nodeL4 : nodeL3.toObject())
              {
                if(nodeL4.identifier == "standard")
                {
                  if(nodeL4.type != shortjson::Field::String)
                    throw __LINE__;
                  if(nodeL4.toString() == "IEC_62196_T1_COMBO")
                  {
                    port.power.level = 3;
                    port.power.connector = Connector::CCS1;
                  }
                  else if(nodeL4.toString() == "CHADEMO")
                  {
                    port.power.level = 3;
                    port.power.connector = Connector::CHAdeMO;
                  }
                  else if(nodeL4.toString() == "IEC_62196_T1")
                  {
                    port.power.level = 2;
                    port.power.connector = Connector::J1772;
                  }
                  else
                    throw __LINE__;
                }
                else if(nodeL4.identifier == "voltage")
                  port.power.volt = safe_float64<__LINE__>(nodeL4);
                else if(nodeL4.identifier == "amperage")
                  port.power.amp = safe_float64<__LINE__>(nodeL4);
              }
            }
          }
        }
        nd.station.ports.emplace_back(port);
      }
    }
  }

  return {{ nd }};
}
