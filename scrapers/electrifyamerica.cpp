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


pair_data_t ElectrifyAmericaScraper::BuildQuery(const pair_data_t& input) const
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
      data.query.URL = "https://api-prod.electrifyamerica.com/v2/locations/" + *data.query.node_id;
      break;
  }
  return data;
}

std::vector<pair_data_t> ElectrifyAmericaScraper::Parse(const pair_data_t& data, const std::string& input) const
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

std::vector<pair_data_t> ElectrifyAmericaScraper::ParseMapArea([[maybe_unused]] const pair_data_t& data, const std::string& input) const
{
  auto input_str = decompress(input);
  std::vector<pair_data_t> return_data;
  shortjson::node_t root = shortjson::Parse(input_str);

  if(root.type != shortjson::Field::Array)
    throw __LINE__;

  for(const shortjson::node_t& nodeL0 : root.toArray())
  {
    if(nodeL0.type != shortjson::Field::Object)
      throw __LINE__;

    ext::string siteId;
    if(shortjson::FindString(nodeL0, siteId, "siteId"))
    {
      pair_data_t nd;
      nd.query.parser = Parser::BuildQuery | Parser::Station;
      nd.query.node_id = siteId;
      return_data.emplace_back(nd);
    }
  }

  return return_data;
}

std::vector<pair_data_t> ElectrifyAmericaScraper::ParseStation(const pair_data_t& data, const std::string& input) const
{
  shortjson::node_t root = shortjson::Parse(input);

  if(root.type != shortjson::Field::Object)
    throw __LINE__;

  pair_data_t nd;
  nd.query.parser = Parser::Complete;
  nd.query.node_id = data.query.node_id;
  nd.station.network_id = data.station.network_id;
  nd.station.access_public = true;

  std::optional<std::string> val;

  for(const shortjson::node_t& nodeL0 : root.toObject())
  {
    val.reset();
    if(parse_string_node<__LINE__>(nodeL0, "siteId", nd.station.station_id) ||
       parse_string_node<__LINE__>(nodeL0, "name", nd.station.name) ||
       parse_street_node<__LINE__>(nodeL0, "address", nd.station.contact.street_number, nd.station.contact.street_name) ||
       parse_string_node<__LINE__>(nodeL0, "city", nd.station.contact.city) ||
       parse_string_node<__LINE__>(nodeL0, "state", nd.station.contact.state) ||
       parse_string_node<__LINE__>(nodeL0, "country", nd.station.contact.country) ||
       parse_string_node<__LINE__>(nodeL0, "postalCode", nd.station.contact.postal_code) ||
       parse_coords_node<__LINE__>(nodeL0, "coordinates", "latitude", "longitude", nd.station.location) ||
       parse_string_node<__LINE__>(nodeL0, "description", nd.station.description) ||
       false)
    {
    }
    else if(parse_string_node<__LINE__>(nodeL0, "type", val))
    {
      if(val == "PUBLIC")
        nd.station.access_public = true;
      else if(val == "COMMERCIAL")
        nd.station.access_public = false;
      else if (val == "COMING_SOON")
      {
        nd.query.parser = Parser::Discard;
        break; // exit loop
      }
      else
        throw __LINE__;
    }
    else if(nodeL0.identifier == "restricted")
    {
      auto val = safe_bool<__LINE__>(nodeL0);
      if(val && *val)
        throw __LINE__; // never seen
    }
    else if(is_object_if_named<__LINE__>(nodeL0, "openingTimes"))
    {
      for(const shortjson::node_t& nodeL1 : nodeL0.toObject())
      {
        if(is_array_if_named<__LINE__>(nodeL1, "regularHours"))
        {
          for([[maybe_unused]] const shortjson::node_t& nodeL2 : nodeL1.toArray())
          {
            throw __LINE__; // never seen
          }
        }
      }
    }
    else if(is_array_if_named<__LINE__>(nodeL0, "pricing"))
    {
      for(const shortjson::node_t& nodeL1 : nodeL0.toArray())
      {
        if(nodeL1.type != shortjson::Field::Object)
          throw __LINE__;
        price_t price;
        price.currency = Currency::USD;
        for(const shortjson::node_t& nodeL2 : nodeL1.toObject())
        {
          if(parse_float_node<__LINE__>(nodeL2, "time", price.per_unit))
            price.unit = Unit::Minutes;
          else if(parse_float_node<__LINE__>(nodeL2, "energy", price.per_unit))
            price.unit = Unit::KilowattHours;
          else if(double val = 0.0; parse_float_node<__LINE__>(nodeL2, "maxPower", val))
          {
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
    else if(is_array_if_named<__LINE__>(nodeL0, "evses"))
    {
      for(const shortjson::node_t& nodeL1 : nodeL0.toArray())
      {
        if(nodeL1.type != shortjson::Field::Object)
          throw __LINE__;
        port_t port;
        for(const shortjson::node_t& nodeL2 : nodeL1.toObject())
        {
          if(parse_string_node<__LINE__>(nodeL2, "id", port.port_id))
          {
          }
          else if(is_array_if_named<__LINE__>(nodeL2, "connectors"))
          {
            for(const shortjson::node_t& nodeL3 : nodeL2.toArray())
            {
              if(nodeL3.type != shortjson::Field::Object)
                throw __LINE__;
              for(const shortjson::node_t& nodeL4 : nodeL3.toObject())
              {
                val.reset();
                if(parse_float_node<__LINE__>(nodeL4, "voltage", port.power.volt) ||
                   parse_float_node<__LINE__>(nodeL4, "amperage", port.power.amp))
                {
                }
                else if(parse_string_node<__LINE__>(nodeL4, "standard", val))
                {
                  if(val == "IEC_62196_T1_COMBO")
                  {
                    port.power.level = 3;
                    port.power.connector = Connector::CCS1;
                  }
                  else if(val == "CHADEMO")
                  {
                    port.power.level = 3;
                    port.power.connector = Connector::CHAdeMO;
                  }
                  else if(val == "IEC_62196_T1")
                  {
                    port.power.level = 2;
                    port.power.connector = Connector::J1772;
                  }
                  else
                    throw __LINE__;
                }
                else if(parse_string_node<__LINE__>(nodeL4, "statue", val))
                {
                  if(val == "UNKNOWN"){}
                  else if(val == "AVAILABLE")
                    port.state = State::Operational;
                  else if(val == "CHARGING")
                    port.state = State::InUse;
                  else
                    throw __LINE__;
                }
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
