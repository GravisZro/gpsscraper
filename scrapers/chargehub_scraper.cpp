#include "chargehub_scraper.h"

// https://apiv2.chargehub.com/api/locationsmap?latmin=11.950390327744657&latmax=69.13019374345745&lonmin=112.08930292841586&lonmax=87.47992792841585&limit=10000000&key=olitest&remove_networks=12&remove_levels=1,2&remove_connectors=0,1,2,3,5,6,7&remove_other=0&above_power=

// https://apiv2.chargehub.com/api/stations/details?station_id=85986&language=en


#include <iostream>

#include <shortjson/shortjson.h>


std::string ChargehubScraper::IndexURL(void) const noexcept
{
  return "https://apiv2.chargehub.com/api/locationsmap?"
         "latmin=-90.0" // + std::to_string(m_latitude) +
         "&latmax=90.0" // + std::to_string(m_latitude + 0.1) +
         "&lonmin=-90.0&lonmax=90.0"
         "&limit=9999&key=olitest&remove_networks=&remove_levels=&remove_connectors=&remove_other=0&above_power=";
}

bool ChargehubScraper::IndexingComplete(void) const noexcept
{
//  m_latitude += 0.1;
//  return m_latitude >= 90.0;
  return true;
}

std::list<station_info_t> ChargehubScraper::ParseIndex(const ext::string &input)
{
  std::list<station_info_t> return_data;
  try
  {
    shortjson::node_t root = shortjson::Parse(input);

    if(root.type == shortjson::Field::Object ||
       root.type == shortjson::Field::Array)
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

      return_data.push_back({ uint32_t(subnode.toNumber()), // station_id
                              0.0,  // latitude
                              0.0,  // longitude
                              "",   // name
                              0,    // street_number
                              "",   // street_name
                              "",   // city
                              "",   // state
                              "",   // country
                              "",    // zipcode
                              "",   // phone_number
                              "",   // times_accessible
                              "",   // price_string
                              "",   // payment_types
                              0,    // CHAdeMO
                              0,    // EV_Plug
                              0,    // J1772_combo
                              "https://apiv2.chargehub.com/api/stations/details?language=en&station_id=" + std::to_string(subnode.toNumber()), // url
                              "",   // post_data
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


inline std::string safe_string(shortjson::node_t& node)
{
  if(node.type != shortjson::Field::String)
    throw 9;
  return node.toString();
}

inline int32_t safe_int32(shortjson::node_t& node)
{
  if(node.type != shortjson::Field::Integer)
    throw 10;
  return node.toNumber();
}

inline double safe_float64(shortjson::node_t& node)
{
  if(node.type == shortjson::Field::Integer)
    return node.toNumber();
  if(node.type == shortjson::Field::Float)
    return node.toFloat();
  throw 11;
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
          if(station.station_id != uint32_t(safe_int32(subnode)))
            throw 4;
        }
        else if(subnode.identifier == "LocName")
          station.name = safe_string(subnode);
        else if(subnode.identifier == "StreetNo")
          station.street_number = uint16_t(safe_int32(subnode));
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
        else if(subnode.identifier == "AccessTime")
          station.times_accessible = safe_string(subnode);
//        else if(subchild_node.identifier == "PriceNumber")
//          station.price_number = safe_float64(subnode);
        else if(subnode.identifier == "PriceString")
          station.price_string = safe_string(subnode);
        else if(subnode.identifier == "PaymentMethods")
          station.payment_types = safe_string(subnode);
        else if(subnode.identifier == "PriceString")
          station.price_string = safe_string(subnode);
        else if(subnode.identifier == "PriceString")
          station.price_string = safe_string(subnode);
        else if(subnode.identifier == "PlugsArray")
        {
          if(subnode.type != shortjson::Field::Object &&
             subnode.type != shortjson::Field::Array)
            throw 5;

          for(auto& sub2node : subnode.toArray())
          {
            if(sub2node.type != shortjson::Field::Object &&
               sub2node.type != shortjson::Field::Array)
              throw 6;

            shortjson::node_t ports;
            if(!shortjson::FindNode(sub2node, ports, "Ports"))
              throw 7;

            std::string name;
            if(!shortjson::FindString(sub2node, name, "Name"))
              throw 8;

            if(name == "CHAdeMO")
              station.CHAdeMO = ports.toArray().size();
            else if(name == "EV Plug (J1772)")
              station.JPlug = ports.toArray().size();
            else if(name == "J1772 Combo")
              station.J1772_combo = ports.toArray().size();
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
