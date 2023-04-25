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
  safenode_t root = shortjson::Parse(input_str);

  if(root.type != shortjson::Field::Array)
    throw __LINE__;

  for(const safenode_t& nodeL0 : root.safeArray())
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
  safenode_t root = shortjson::Parse(input);

  if(root.type != shortjson::Field::Object)
    throw __LINE__;

  pair_data_t nd;
  nd.query.parser = Parser::Complete;
  nd.query.node_id = data.query.node_id;
  nd.station.network_id = data.station.network_id;
  nd.station.access_public = true;

  std::optional<std::string> tmpstr;
  std::optional<bool> tmpbool;

  for(const safenode_t& nodeL0 : root.safeObject())
  {
    if(nodeL0.idString("siteId", nd.station.station_id) ||
       nodeL0.idString("name", nd.station.name) ||
       nodeL0.idStreet("address", nd.station.contact.street_number, nd.station.contact.street_name) ||
       nodeL0.idString("city", nd.station.contact.city) ||
       nodeL0.idString("state", nd.station.contact.state) ||
       nodeL0.idString("country", nd.station.contact.country) ||
       nodeL0.idString("postalCode", nd.station.contact.postal_code) ||
       nodeL0.idCoords("coordinates", "latitude", "longitude", nd.station.location) ||
       nodeL0.idString("description", nd.station.description) ||
       false)
    {
    }
    else if(nodeL0.idString("type", tmpstr))
    {
      if(tmpstr == "PUBLIC")
        nd.station.access_public = true;
      else if(tmpstr == "COMMERCIAL")
        nd.station.access_public = false;
      else if (tmpstr == "COMING_SOON")
      {
        nd.query.parser = Parser::Discard;
        break; // exit loop
      }
      else
        throw __LINE__;
    }
    else if(nodeL0.idBool("restricted", tmpbool))
    {
      if(tmpbool == true)
        throw __LINE__; // never seen
    }
    else if(nodeL0.idObject("openingTimes"))
    {
      for(const safenode_t& nodeL1 : nodeL0.safeObject())
      {
        if(nodeL1.idArray("regularHours"))
        {
          for([[maybe_unused]] const safenode_t& nodeL2 : nodeL1.safeArray())
          {
            throw __LINE__; // never seen
          }
        }
      }
    }
    else if(nodeL0.idArray("pricing"))
    {
      for(const safenode_t& nodeL1 : nodeL0.safeArray())
      {
        if(nodeL1.type != shortjson::Field::Object)
          throw __LINE__;
        price_t price;
        price.currency = Currency::USD;
        for(const safenode_t& nodeL2 : nodeL1.safeObject())
        {
          if(nodeL2.idFloat("time", price.per_unit))
            price.unit = Unit::Minutes;
          else if(nodeL2.idFloat("energy", price.per_unit))
            price.unit = Unit::KilowattHours;
          else if(double val = 0.0; nodeL2.idFloat("maxPower", val))
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
    else if(nodeL0.idArray("evses"))
    {
      for(const safenode_t& nodeL1 : nodeL0.safeArray())
      {
        if(nodeL1.type != shortjson::Field::Object)
          throw __LINE__;
        port_t port;
        for(const safenode_t& nodeL2 : nodeL1.safeObject())
        {
          if(nodeL2.idString("id", port.port_id))
          {
          }
          else if(nodeL2.idArray("connectors"))
          {
            for(const safenode_t& nodeL3 : nodeL2.safeArray())
            {
              if(nodeL3.type != shortjson::Field::Object)
                throw __LINE__;
              for(const safenode_t& nodeL4 : nodeL3.safeObject())
              {
                if(nodeL4.idFloat("voltage", port.power.volt) ||
                   nodeL4.idFloat("amperage", port.power.amp))
                {
                }
                else if(nodeL4.idString("standard", tmpstr))
                {
                  if(tmpstr == "IEC_62196_T1_COMBO")
                  {
                    port.power.level = 3;
                    port.power.connector = Connector::CCS1;
                  }
                  else if(tmpstr == "CHADEMO")
                  {
                    port.power.level = 3;
                    port.power.connector = Connector::CHAdeMO;
                  }
                  else if(tmpstr == "IEC_62196_T1")
                  {
                    port.power.level = 2;
                    port.power.connector = Connector::J1772;
                  }
                  else
                    throw __LINE__;
                }
                else if(nodeL4.idString("statue", tmpstr))
                {
                  if(tmpstr == "UNKNOWN"){}
                  else if(tmpstr == "AVAILABLE")
                    port.status = Status::Operational;
                  else if(tmpstr == "CHARGING")
                    port.status = Status::InUse;
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
