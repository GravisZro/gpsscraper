#include "chargehub_scraper.h"

// https://apiv2.chargehub.com/api/locationsmap?latmin=11.950390327744657&latmax=69.13019374345745&lonmin=112.08930292841586&lonmax=87.47992792841585&limit=10000000&key=olitest&remove_networks=12&remove_levels=1,2&remove_connectors=0,1,2,3,5,6,7&remove_other=0&above_power=

// https://apiv2.chargehub.com/api/stations/details?station_id=85986&language=en


#include <iostream>
#include <algorithm>

#include <shortjson/shortjson.h>


ChargehubScraper::ChargehubScraper(void) noexcept
  : m_latitude(12.0)
{

}

std::string ChargehubScraper::IndexURL(void) const noexcept
{
  return "https://apiv2.chargehub.com/api/locationsmap?"
         "latmin=" + std::to_string(m_latitude) +
         "&latmax=" + std::to_string(m_latitude + 0.25) +
         "&lonmin=-90.0&lonmax=90.0"
         "&limit=2000&key=olitest&remove_networks=&remove_levels=&remove_connectors=&remove_other=0&above_power=";
}

bool ChargehubScraper::IndexingComplete(void) const noexcept
{
  m_latitude += 0.25;
  return m_latitude >= 55.0;
}

std::list<station_info_t> ChargehubScraper::ParseIndex(const ext::string &input)
{
  std::list<station_info_t> return_data;
  try
  {
    shortjson::node_t root = shortjson::Parse(input);

    if(root.type != shortjson::Field::Object &&
       root.type != shortjson::Field::Array)
      throw 2;

    for(shortjson::node_t& child_node : root.toArray())
    {
      if(child_node.type == shortjson::Field::Undefined)
        break;

      if(child_node.type == shortjson::Field::Array)
        throw 3;

      auto& subnode = child_node.toArray().front();
      if(subnode.identifier != "LocID")
        throw 4;
      if(subnode.type != shortjson::Field::Integer)
        throw 5;


      return_data.push_back({
                              .station_id = int32_t(subnode.toNumber()),
                              .details_URL = "https://apiv2.chargehub.com/api/stations/details?language=en&station_id=" + std::to_string(subnode.toNumber())
                            });
    }
  }
  catch(int exit_id)
  {
    std::cerr << "ChargehubScraper::ParseIndex threw: " << exit_id << std::endl;
    std::cerr << "page dump:" << std::endl << input << std::endl;
  }
  return return_data;
}


inline std::optional<std::string> safe_string(shortjson::node_t& node)
{
  if(node.type == shortjson::Field::String)
    return !node.toString().empty() ? node.toString() : std::optional<std::string>();
  if(node.type == shortjson::Field::Null)
    return std::optional<std::string>();
  throw 20;
}

inline std::optional<int32_t> safe_int32(shortjson::node_t& node)
{
  if(node.type == shortjson::Field::Integer)
    return node.toNumber();
  if(node.type == shortjson::Field::Null)
    return std::optional<int32_t>();
  throw 21;
}

inline double safe_float64(shortjson::node_t& node)
{
  if(node.type == shortjson::Field::Integer)
    return node.toNumber();
  if(node.type == shortjson::Field::Float)
    return node.toFloat();
  throw 22;
}

inline void connector_from_string(const std::optional<std::string>& str, port_t& port)
{
  if(str)
  {
    std::string out = *str;
    std::transform(std::begin(out), std::end(out), std::begin(out),
                   [](unsigned char c){ return std::tolower(c); });

    if(out == "nema 5-15")
      port.connector = Nema515;
    else if(out == "nema 5-20")
      port.connector = Nema520;
    else if(out == "nema 14-50")
      port.connector = Nema1450;
    else if(out == "chademo")
      port.connector = CHAdeMO;
    else if(out == "ev plug (j1772)")
      port.connector = J1772;
    else if(out == "j1772 combo")
      port.connector = J1772Combo;
    else if(out == "ccs")
      port.connector = CCS;
    else if(out == "tesla" ||
            out == "tesla supercharger")
      port.connector = TeslaPlug;
    else if(out == "ccs & chademo" || out == "ccs & ccs")
    {
      port.weird = true;
      port.connector = CCSorCHAdeMO;
    }
    else
    {
      port.weird = true;
    }
  }
  else
    port.weird = true;
}

std::list<station_info_t> ChargehubScraper::ParseStation(const station_info_t& station_info, const ext::string& input)
{
  (void)station_info;
  std::list<station_info_t> return_data;

  try
  {
    shortjson::node_t root = shortjson::Parse(input);

    if(root.type != shortjson::Field::Object &&
       root.type != shortjson::Field::Array)
      throw 2;

    for(shortjson::node_t& child_node : root.toArray())
    {
      if(child_node.type == shortjson::Field::Array)
        throw 3;

      station_info_t station = station_info;

      for(shortjson::node_t& subnode : child_node.toArray())
      {

        if(subnode.identifier == "Id")
        {
          if(station.station_id != safe_int32(subnode))
            throw 4;
        }
        else if(subnode.identifier == "LocName")
          station.name = safe_string(subnode);
        else if(subnode.identifier == "LocDesc")
          station.description = safe_string(subnode);
        else if(subnode.identifier == "StreetNo")
          station.street_number = safe_int32(subnode);
        else if(subnode.identifier == "Street")
          station.street_name = safe_string(subnode);
        else if(subnode.identifier == "City")
          station.city = safe_string(subnode);
        else if(subnode.identifier == "prov_state")
          station.state = safe_string(subnode);
        else if(subnode.identifier == "Country")
          station.country = safe_string(subnode);
        else if(subnode.identifier == "Zip")
          station.zipcode = safe_string(subnode);
        else if(subnode.identifier == "Lat")
          station.latitude = safe_float64(subnode);
        else if(subnode.identifier == "Long")
          station.longitude = safe_float64(subnode);
        else if(subnode.identifier == "Phone")
          station.phone_number = safe_string(subnode);
        else if(subnode.identifier == "Web")
          station.URL = safe_string(subnode);
        else if(subnode.identifier == "AccessTime")
          station.access_times = safe_string(subnode);
        else if(subnode.identifier == "AccessType")
          station.access_type = safe_string(subnode);
        else if(subnode.identifier == "NetworkId")
          station.network_id = safe_int32(subnode);
        else if(subnode.identifier == "PriceString")
          station.price_string = safe_string(subnode);
        else if(subnode.identifier == "PaymentMethods")
          station.initialization = safe_string(subnode);
        else if(subnode.identifier == "PlugsArray")
        {
          if(subnode.type != shortjson::Field::Object &&
             subnode.type != shortjson::Field::Array)
            throw 5;

          for(auto& plugnode : subnode.toArray())
          {
            port_t port;
            port.weird = false;
            if(plugnode.type != shortjson::Field::Object &&
               plugnode.type != shortjson::Field::Array)
              throw 6;

            for(auto& plug_element : plugnode.toArray())
            {
              if(plug_element.identifier == "Level")
                port.level = safe_int32(plug_element);
              else if(plug_element.identifier == "Network")
                port.network_id = safe_int32(plug_element);
              else if(plug_element.identifier == "Name")
                connector_from_string(safe_string(plug_element), port);
              else if(plug_element.identifier == "PriceString")
                port.price_string = safe_string(plug_element);
              else if(plug_element.identifier == "PaymentMethods")
                port.initialization = safe_string(plug_element);
              else if(plug_element.identifier == "Amp")
                port.amp = safe_string(plug_element);
              else if(plug_element.identifier == "Kw")
                port.kw = safe_string(plug_element);
              else if(plug_element.identifier == "Volt")
                port.volt = safe_string(plug_element);
            }

            shortjson::node_t portsnode;
            if(!shortjson::FindNode(plugnode, portsnode, "Ports"))
              throw 9;

            for(auto& portnode : portsnode.toArray())
            {
              port_t thisport = port;
              for(shortjson::node_t& port_element : portnode.toArray())
              {
                if(port_element.identifier == "portId")
                  thisport.port_id = safe_int32(port_element);
                else if(port_element.identifier == "netPortId")
                  thisport.network_port_id = safe_string(port_element);
                else if(port_element.identifier == "displayName")
                  thisport.display_name = safe_string(port_element);
              }
              station.ports.push_back(thisport);
            }
          }
        }
      }

      return_data.push_back(station);
    }
  }
  catch(int exit_id)
  {
    std::cerr << "ChargehubScraper::ParseStation threw: " << exit_id << std::endl;
    std::cerr << "page dump:" << std::endl << input << std::endl;
  }
  return return_data;
}
