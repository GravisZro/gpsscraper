#include "chargehub_scraper.h"

// https://apiv2.chargehub.com/api/locationsmap?latmin=11.950390327744657&latmax=69.13019374345745&lonmin=112.08930292841586&lonmax=87.47992792841585&limit=10000000&key=olitest&remove_networks=12&remove_levels=1,2&remove_connectors=0,1,2,3,5,6,7&remove_other=0&above_power=

// https://apiv2.chargehub.com/api/stations/details?station_id=85986&language=en


#include <iostream>
#include <algorithm>

#include <shortjson/shortjson.h>
#include "utilities.h"

ChargehubScraper::ChargehubScraper(double starting_latitude,
                                   double stopping_latitude) noexcept
  : m_start_latitude(starting_latitude),
    m_end_latitude(stopping_latitude)
{
}

pair_data_t ChargehubScraper::BuildQuery(const pair_data_t& input)
{
  pair_data_t data;
  switch(input.query.parser)
  {
    default: throw std::string(__FILE__).append(": unknown parser: ").append(std::to_string(int(input.query.parser)));

    case Parser::BuildQuery | Parser::Initial:
      data.query.parser = Parser::Initial;
      break;

    case Parser::BuildQuery | Parser::MapArea:
    {
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
    }
    case Parser::BuildQuery | Parser::Station:
      data.query.parser = Parser::Station;
      if(!input.station.station_id)
        throw __LINE__;
      data.query.URL = "https://apiv2.chargehub.com/api/stations/details?language=en&station_id=" + std::to_string(*input.station.station_id);
      break;
  }
  return data;
}

std::vector<pair_data_t> ChargehubScraper::Parse(const pair_data_t& data, const std::string& input)
{
  try
  {
    switch(data.query.parser)
    {
      default: throw std::string(__FILE__).append(": unknown parser: ").append(std::to_string(int(data.query.parser)));
      case Parser::Initial: return ParserInit();
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

std::vector<pair_data_t> ChargehubScraper::ParserInit(void)
{
  std::vector<pair_data_t> return_data;
  query_info_t query;
  query.parser = Parser::BuildQuery | Parser::MapArea;
  for(double latitude = m_start_latitude;
      latitude < m_end_latitude;
      latitude += m_latitude_step)
  {
    query.bounds = { { latitude + m_latitude_step , latitude }, { 90.0, -90.0 } };
    return_data.emplace_back(BuildQuery({ query, {}}));
  }
  return return_data;
}

std::vector<pair_data_t> ChargehubScraper::ParseMapArea(const std::string &input)
{
  std::vector<pair_data_t> return_data;
  shortjson::node_t root = shortjson::Parse(input);

  if(root.type != shortjson::Field::Object &&
     root.type != shortjson::Field::Array)
    throw __LINE__;

  for(shortjson::node_t& nodeL0 : root.toArray())
  {
    if(nodeL0.type == shortjson::Field::Undefined)
      break;

    if(nodeL0.type == shortjson::Field::Array)
      throw __LINE__;

    auto& nodeL1 = nodeL0.toArray().front();
    if(nodeL1.identifier != "LocID")
      throw __LINE__;
    if(nodeL1.type != shortjson::Field::Integer)
      throw __LINE__;

    std::cout << "station id: " << int(nodeL1.toNumber()) << std::endl;

    pair_data_t nd;
    nd.query.parser = Parser::BuildQuery | Parser::Station;
    nd.station.station_id = nodeL1.toNumber();
    return_data.emplace_back(BuildQuery(nd));
  }
  return return_data;
}

/*
inline std::optional<std::string> validate_string(const std::string& str)
{
  auto pos = std::find_if_not(std::begin(str), std::end(str),
                 [](unsigned char c){ return std::isspace(c); });
  return pos == std::end(str) ? std::optional<std::string>() : str;
}
*/


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

std::vector<pair_data_t> ChargehubScraper::ParseStation(const pair_data_t& data, const std::string& input)
{
  std::vector<pair_data_t> return_data;
  shortjson::node_t root = shortjson::Parse(input);

  if(root.type != shortjson::Field::Object &&
     root.type != shortjson::Field::Array)
    throw __LINE__;

  for(shortjson::node_t& nodeL0 : root.toArray())
  {
    if(nodeL0.type != shortjson::Field::Array &&
       nodeL0.type != shortjson::Field::Object)
      throw __LINE__;

    pair_data_t nd = data;
    nd.query.parser = Parser::Complete;

    for(shortjson::node_t& nodeL1 : nodeL0.toObject())
    {

      if(nodeL1.identifier == "Id")
      {
        if(nd.station.station_id != safe_int<__LINE__>(nodeL1))
          throw __LINE__;
      }
      else if(nodeL1.identifier == "LocName")
        nd.station.name = safe_string<__LINE__>(nodeL1);
      else if(nodeL1.identifier == "LocDesc")
        nd.station.description = safe_string<__LINE__>(nodeL1);
      else if(nodeL1.identifier == "StreetNo")
        nd.station.contact.street_number = safe_int<__LINE__>(nodeL1);
      else if(nodeL1.identifier == "Street")
        nd.station.contact.street_name = safe_string<__LINE__>(nodeL1);
      else if(nodeL1.identifier == "City")
        nd.station.contact.city = safe_string<__LINE__>(nodeL1);
      else if(nodeL1.identifier == "prov_state")
        nd.station.contact.state = safe_string<__LINE__>(nodeL1);
      else if(nodeL1.identifier == "Country")
        nd.station.contact.country = safe_string<__LINE__>(nodeL1);
      else if(nodeL1.identifier == "Zip")
        nd.station.contact.postal_code = safe_string<__LINE__>(nodeL1);
      else if(nodeL1.identifier == "Lat")
        nd.station.location.latitude = safe_float64<__LINE__>(nodeL1);
      else if(nodeL1.identifier == "Long")
        nd.station.location.longitude = safe_float64<__LINE__>(nodeL1);
      else if(nodeL1.identifier == "Phone")
        nd.station.contact.phone_number = safe_string<__LINE__>(nodeL1);
      else if(nodeL1.identifier == "Web")
        nd.station.contact.URL = safe_string<__LINE__>(nodeL1);
      else if(nodeL1.identifier == "AccessTime")
        nd.station.restrictions = get_access_restrictions(safe_string<__LINE__>(nodeL1));
      else if(nodeL1.identifier == "AccessType")
        nd.station.access_public = get_access_public(safe_string<__LINE__>(nodeL1));
      else if(nodeL1.identifier == "NetworkId")
        nd.station.network_id = safe_int<__LINE__, Network>(nodeL1);
      else if(nodeL1.identifier == "PriceString")
      {
        nd.station.price.text = safe_string<__LINE__>(nodeL1);
        if(nd.station.price.text &&
           *nd.station.price.text == "Cost: Free")
        {
          nd.station.price.payment |= Payment::Free;
          nd.station.price.text.reset();
        }
      }
      else if(nodeL1.identifier == "PaymentMethods")
        nd.station.price.payment |= get_payment_methods(safe_string<__LINE__>(nodeL1));
      else if(nodeL1.identifier == "PlugsArray")
      {
        if(nodeL1.type != shortjson::Field::Object &&
           nodeL1.type != shortjson::Field::Array)
          throw __LINE__;

        for(auto& nodeL2 : nodeL1.toArray())
        {
          port_t port;
          port.weird = false;
          if(nodeL2.type != shortjson::Field::Object &&
             nodeL2.type != shortjson::Field::Array)
            throw __LINE__;

          for(auto& nodeL3 : nodeL2.toArray())
          {
            if(nodeL3.identifier == "Level")
              port.power.level = safe_int<__LINE__>(nodeL3);
            else if(nodeL3.identifier == "Network")
            {
              if(nd.station.network_id != safe_int<__LINE__,Network>(nodeL3))
                 throw __LINE__;
            }
            else if(nodeL3.identifier == "Name")
              connector_from_string(safe_string<__LINE__>(nodeL3), port);
            else if(nodeL3.identifier == "PriceString")
            {
              port.price.text = safe_string<__LINE__>(nodeL3);
              if(port.price.text &&
                 *port.price.text == "Cost: Free")
              {
                port.price.payment |= Payment::Free;
                port.price.text.reset();
              }
            }
            else if(nodeL3.identifier == "PaymentMethods")
              port.price.payment |= get_payment_methods(safe_string<__LINE__>(nodeL3));
            else if(nodeL3.identifier == "Amp")
              port.power.amp = safe_float_unit<__LINE__>(nodeL3);
            else if(nodeL3.identifier == "Kw")
              port.power.kw = safe_float_unit<__LINE__>(nodeL3);
            else if(nodeL3.identifier == "Volt")
              port.power.volt = safe_float_unit<__LINE__>(nodeL3);
          }

          shortjson::node_t portsL0;
          if(!shortjson::FindNode(nodeL2, portsL0, "Ports"))
            throw __LINE__;

          for(auto& portsL1 : portsL0.toArray())
          {
            port_t thisport = port;
            for(shortjson::node_t& portsL2 : portsL1.toArray())
            {
              if(portsL2.identifier == "portId")
                thisport.port_id = safe_int<__LINE__>(portsL2);
              else if(portsL2.identifier == "netPortId")
                thisport.port_id = safe_int<__LINE__>(portsL2);
              else if(portsL2.identifier == "displayName")
                thisport.display_name = safe_string<__LINE__>(portsL2);
            }
            nd.station.ports.push_back(thisport);
          }
        }
      }
    }

    return_data.emplace_back(nd);
  }
  return return_data;
}
