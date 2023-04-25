#include "chargehub.h"

// https://apiv2.chargehub.com/api/locationsmap?latmin=11.950390327744657&latmax=69.13019374345745&lonmin=112.08930292841586&lonmax=87.47992792841585&limit=10000000&key=olitest&remove_networks=12&remove_levels=1,2&remove_connectors=0,1,2,3,5,6,7&remove_other=0&above_power=

// https://apiv2.chargehub.com/api/stations/details?station_id=85986&language=en


#include <iostream>
#include <algorithm>

#include <shortjson/shortjson.h>
#include "utilities.h"

void ChargehubScraper::classify(pair_data_t& record) const
{
  record.query.parser = Parser::BuildQuery | Parser::Station;
}

pair_data_t ChargehubScraper::BuildQuery(const pair_data_t& input) const
{
  pair_data_t data;
  switch(input.query.parser)
  {
    default: throw std::string(__FILE__).append(": unknown parser: ").append(std::to_string(int(input.query.parser)));

    case Parser::BuildQuery | Parser::Initial:

      data.query.parser = Parser::Station;
      data.query.URL = "https://apiv2.chargehub.com/api/stations/details?language=en&station_id=36111";
      break;
/*/
      data.query.parser = Parser::Initial;
      data.query.bounds = { { 12.5, 55.0 }, { 90.0, -90.0 } };
      break;
/*/
    case Parser::BuildQuery | Parser::MapArea:
      data.query.parser = Parser::MapArea;
      data.query.URL= "https://apiv2.chargehub.com/api/locationsmap"
                      "?latmin=" + std::to_string(input.query.bounds.latitude.min) +
                      "&latmax=" + std::to_string(input.query.bounds.latitude.max) +
                      "&lonmin=" + std::to_string(input.query.bounds.longitude.min) +
                      "&lonmax=" + std::to_string(input.query.bounds.longitude.max) +
                      "&limit=10&key=olitest&remove_networks=&remove_levels=&remove_connectors=&remove_other=0&above_power=";
      data.query.header_fields = { { "Content-Type", "application/json" }, };
      data.query.bounds = input.query.bounds;
      break;

    case Parser::BuildQuery | Parser::Station:
      data.query.parser = Parser::Station;
      if(!input.query.node_id)
        throw __LINE__;
      data.query.URL = "https://apiv2.chargehub.com/api/stations/details?language=en&station_id=" + *input.query.node_id;
      break;
  }
  return data;
}

std::vector<pair_data_t> ChargehubScraper::Parse(const pair_data_t& data, const std::string& input) const
{
  try
  {
    switch(data.query.parser)
    {
      default: throw std::string(__FILE__).append(": unknown parser: ").append(std::to_string(int(data.query.parser)));
      case Parser::Initial: return ParserInit(data);
      case Parser::MapArea: return ParseMapArea(input);
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
  return std::vector<pair_data_t>();
}

std::vector<pair_data_t> ChargehubScraper::ParserInit(const pair_data_t& data) const
{
  std::vector<pair_data_t> return_data;
  pair_data_t nd;

  nd.query.parser = Parser::BuildQuery | Parser::MapArea;
  nd.query.bounds.longitude = data.query.bounds.longitude;
  nd.station.network_id = Network::ChargeHub;

  constexpr double latitude_step = 0.25;
  for(double latitude = data.query.bounds.latitude.min;
      latitude < data.query.bounds.latitude.max;
      latitude += latitude_step)
  {
    nd.query.bounds.latitude = { latitude + latitude_step, latitude };
    return_data.emplace_back(BuildQuery(nd));
  }
  return return_data;
}

std::vector<pair_data_t> ChargehubScraper::ParseMapArea(const std::string &input) const
{
  std::vector<pair_data_t> return_data;
  safenode_t root = shortjson::Parse(input);

  for(const safenode_t& nodeL0 : root.safeArray())
  {
    pair_data_t nd;
    nd.query.parser = Parser::BuildQuery | Parser::Station;

    for(const safenode_t& nodeL1 : nodeL0.safeObject())
      nodeL1.idString("LocID", nd.query.node_id);

    return_data.emplace_back(nd);
  }
  return return_data;
}

inline void connector_from_string(const std::optional<std::string>& str, port_t& port)
{
  if(str)
  {
    std::string out = *str;
    std::transform(std::begin(out), std::end(out), std::begin(out),
                   [](unsigned char c){ return std::tolower(c); });

    if(out == "nema 5-15")
      port.power.connector = Connector::Nema515;
    else if(out == "nema 5-20")
      port.power.connector = Connector::Nema520;
    else if(out == "nema 14-50")
      port.power.connector = Connector::Nema1450;
    else if(out == "chademo")
      port.power.connector = Connector::CHAdeMO;
    else if(out == "ev plug (j1772)")
      port.power.connector = Connector::J1772;
    else if(out == "j1772 combo")
      port.power.connector = Connector::CCS1;
    else if(out == "ccs")
      port.power.connector = Connector::CCS2;
    else if(out == "tesla" ||
            out == "tesla supercharger")
      port.power.connector = Connector::Tesla;
    else if(out == "ccs & chademo" || out == "ccs & ccs")
    {
      port.weird = true;
      port.power.connector = Connector::CCS2 | Connector::CHAdeMO;
    }
    else
    {
      port.weird = true;
    }
  }
  else
    port.weird = true;
}

std::optional<std::string> get_access_restrictions(std::optional<std::string> str)
{
  if(str == "24 Hours")
    return std::optional<std::string>();
  return str;
}

std::optional<bool> get_access_public(std::optional<std::string> str)
{
  if(str == "Public")
    return true;
  if(str == "Restricted")
    return false;
  if(str == "Unknown")
    return std::optional<bool>();
  throw __LINE__;
}

Payment get_payment_methods(std::optional<std::string> str)
{
  Payment rv = Payment::Undefined;
  if(str)
  {
    if(str->find("Network App") != std::string::npos)
      rv |= Payment::API;
    if(str->find("Network RFID") != std::string::npos)
      rv |= Payment::RFID;
    if(str->find("Online") != std::string::npos)
      rv |= Payment::Website;
    if(str->find("Phone") != std::string::npos)
      rv |= Payment::PhoneCall;
    if(str->find("Cash") != std::string::npos)
      rv |= Payment::Cash;
    if(str->find("Cheque") != std::string::npos)
      rv |= Payment::Cheque;
    if(str->find("Visa") != std::string::npos ||
       str->find("Mastercard") != std::string::npos ||
       str->find("Discover") != std::string::npos ||
       str->find("American Express") != std::string::npos)
      rv |= Payment::Credit;
  }
  return rv;
}

std::vector<pair_data_t> ChargehubScraper::ParseStation(const pair_data_t& data, const std::string& input) const
{
  std::vector<pair_data_t> return_data;
  std::optional<std::string> tmpstr;
  safenode_t root = shortjson::Parse(input);

  for(const safenode_t& nodeL0 : root.safeObject())
  {
    pair_data_t nd = data;
    nd.query.parser = Parser::Complete;

    for(const safenode_t& nodeL1 : nodeL0.safeObject())
    {
      if(nodeL1.idString("Id", nd.station.station_id) ||
         nodeL1.idString("LocName", nd.station.name) ||
         nodeL1.idString("LocDesc", nd.station.description) ||
         nodeL1.idInteger("StreetNo", nd.station.contact.street_number) ||
         nodeL1.idString("Street", nd.station.contact.street_name) ||
         nodeL1.idString("City", nd.station.contact.city) ||
         nodeL1.idString("prov_state", nd.station.contact.state) ||
         nodeL1.idString("Country", nd.station.contact.country) ||
         nodeL1.idString("Zip", nd.station.contact.postal_code) ||
         nodeL1.idFloat("Lat", nd.station.location.latitude) ||
         nodeL1.idFloat("Long", nd.station.location.longitude) ||
         nodeL1.idString("Phone", nd.station.contact.phone_number) ||
         nodeL1.idString("Web", nd.station.contact.URL) ||
         nodeL1.idEnum("NetworkId", nd.station.network_id))
      {

      }
      else if(nodeL1.idString("AccessTime", tmpstr))
        nd.station.restrictions = get_access_restrictions(tmpstr);
      else if(nodeL1.idString("AccessType", tmpstr))
        nd.station.access_public = get_access_public(tmpstr);
      else if(nodeL1.idString("PriceString", nd.station.price.text))
      {
        if(nd.station.price.text && *nd.station.price.text == "Cost: Free")
        {
          nd.station.price.payment |= Payment::Free;
          nd.station.price.text.reset();
        }
      }
      else if(nodeL1.idString("PaymentMethods", tmpstr))
        nd.station.price.payment |= get_payment_methods(tmpstr);
      else if(nodeL1.idArray("PlugsArray"))
      {
        for(auto& nodeL2 : nodeL1.safeArray())
        {
          port_t port;
          port.weird = false;

          for(auto& nodeL3 : nodeL2.safeObject())
          {
            if(nodeL3.idInteger("Level", port.power.level) ||
               nodeL3.idFloatUnit("Amp", port.power.amp) ||
               nodeL3.idFloatUnit("Kw", port.power.kw) ||
               nodeL3.idFloatUnit("Volt", port.power.volt))
            {
            }
            else if(nodeL3.idString("PriceString", port.price.text))
            {
              if(port.price.text && *port.price.text == "Cost: Free")
              {
                port.price.payment |= Payment::Free;
                port.price.text.reset();
              }
            }
            else if(nodeL3.idString("PaymentMethods", tmpstr))
              port.price.payment |= get_payment_methods(tmpstr);
            else if(nodeL3.idArray("Ports"))
            {
              for(auto& nodeL4 : nodeL3.safeArray())
              {
                port_t thisport = port;
                for(const safenode_t& portsL2 : nodeL4.safeObject())
                {
                  if(portsL2.idString("portId", thisport.port_id) ||
                     portsL2.idString("displayName", thisport.display_name))
                  {
                  }
                  else if(portsL2.idString("netPortId", tmpstr))
                    thisport.port_id = tmpstr;
                }
                nd.station.ports.push_back(thisport);
              }
            }
            else if(nodeL3.idString("Name", tmpstr))
            {
              for(auto& thisport : nd.station.ports)
                connector_from_string(tmpstr, thisport);
            }
          }
        }
      }
    }

    return_data.emplace_back(nd);
  }
  return return_data;
}
