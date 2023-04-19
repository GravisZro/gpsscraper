#include "eptix.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>

#include <cassert>

#include "utilities.h"

void EptixScraper::classify(pair_data_t& record) const
{
  record.query.parser = Parser::BuildQuery | Parser::Station;
}

pair_data_t EptixScraper::BuildQuery(const pair_data_t& input) const
{
  pair_data_t data = input;

  data.query.header_fields =
  {
    { "Authorization", "Basic OUFHdlg4Mmc1cUt2WWtkNnRuQmpnNTh0Y2o5cXFQaDQ6QTlLdzhoZVlMMmZxVFRWRlRnZnVYdFF1YlVxOU14R1Y=" },
    { "Origin", "https://lecircuitelectrique.com/" },
  };

  if(!data.query.node_id)
    throw __LINE__;

  switch(data.query.parser)
  {
    default: throw std::string(__FILE__).append(": unknown parser: ").append(std::to_string(int(input.query.parser)));

    case Parser::BuildQuery | Parser::Initial:
      data.query.parser = Parser::MapArea;
      data.query.URL = "https://api.eptix.co/public/v1/sites/all?showConstruction=0";
      data.query.bounds = { { 0.0, 45.0 }, { 0.0, 120.0 } };
      break;

    case Parser::BuildQuery | Parser::MapArea:
      data.query.parser = Parser::MapArea;
      data.query.URL = "https://api.eptix.co/public/v1/sites/viewport"
                       "?bLeftLng="   + std::to_string(data.query.bounds.southWest().longitude) +
                       "&bLeftLat="   + std::to_string(data.query.bounds.southWest().latitude ) +
                       "&uRightLng="  + std::to_string(data.query.bounds.northEast().longitude) +
                       "&uRightLat="  + std::to_string(data.query.bounds.northEast().latitude ) +
                       "&lng="        + std::to_string(data.station.location.longitude) +
                       "&lat="        + std::to_string(data.station.location.latitude ) +
                       "&showConstruction=0"
                       "&hidePartnerNetworkIds=0";
      break;

    case Parser::BuildQuery | Parser::Station:
    {
      data.query.parser = Parser::Station;
      data.query.URL = "https://api.eptix.co/public/v1/sites/" + *data.query.node_id + "/viewport";
      break;
    }

    case Parser::BuildQuery | Parser::Port:
      data.query.parser = Parser::Port;
      break;
  }
  return data;
}


std::vector<pair_data_t> EptixScraper::Parse(const pair_data_t& data, const std::string& input) const
{
  std::vector<pair_data_t> return_data;
  try
  {
    shortjson::node_t root = shortjson::Parse(input);

    if(root.type == shortjson::Field::Array)
    {
      ext::string child_ids;
      for(const shortjson::node_t& nodeL0 : root.toArray())
        return_data.emplace_back(ParseStationNode(data, nodeL0));
      for(auto& child : return_data)
      {
        assert(child.query.node_id);
        if(child.query.parser == (Parser::BuildQuery | Parser::MapArea) ||
           child.query.parser == (Parser::BuildQuery | Parser::Station))
          child_ids.list_append(',', *child.query.node_id);
      }

      pair_data_t parent;
      parent = data;
      parent.query.parser = Parser::UpdateRecord | Parser::MapArea;
      if(!child_ids.empty())
        parent.query.child_ids = child_ids;
      if(*data.query.node_id != "root")
        return_data.emplace(std::begin(return_data), parent);
    }
    else if(root.type == shortjson::Field::Object)
      return_data.emplace_back(ParseStationNode(data, root));
    else
      throw __LINE__;
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
  return return_data;
}

std::string lower_case(std::string data)
{
  std::transform(data.begin(), data.end(), data.begin(),
                 [](unsigned char c){ return std::tolower(c); });
  return data;
}

void optional_append(std::optional<std::string>& target, std::optional<std::string> data)
{
  if(data)
  {
    if(target)
      target->append("\n").append(*data);
    else
      target = *data;
  }
}

std::optional<int32_t> get_level(const shortjson::node_t& node)
{
  if(node.type == shortjson::Field::String)
  {
    auto lstr = lower_case(node.toString());
    if(lstr == "level1")
      return 1;
    if(lstr == "level2")
      return 2;
    if(lstr == "level3" ||
       lstr == "brcc")
      return 3;
    throw __LINE__;
  }
  return {};
}

pair_data_t EptixScraper::ParseStationNode(const pair_data_t& data, const shortjson::node_t& root) const
{
  if(root.type != shortjson::Field::Object)
    throw __LINE__;

  pair_data_t nd;
  nd.query.parser = Parser::Discard;
  nd.station.network_id = Network::Circuit_Électrique; // default network
  nd.query.bounds = data.query.bounds;
  nd.station.access_public = true; // public by default

  for(const shortjson::node_t& nodeL0 : root.toObject())
  {
    if(nd.query.parser == Parser::Discard) // if not set
    {
      if(nodeL0.identifier == "stationCount") // initial request
      {
        if(nodeL0.type == shortjson::Field::Integer && nodeL0.toNumber() == 1)
          nd.query.parser = Parser::BuildQuery | Parser::Station;
        else
          nd.query.parser = Parser::BuildQuery | Parser::MapArea;
      }
      else if(nodeL0.identifier == "distance") // map view request
        nd.query.parser = Parser::BuildQuery | Parser::Station;
      else if(nodeL0.identifier == "oldId") // station info request
        nd.query.parser = Parser::Complete;
    }

    if(nodeL0.identifier == "id")
      nd.station.station_id = nd.query.node_id = safe_string<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "name")
    {
      nd.station.name = safe_string<__LINE__>(nodeL0);
      if(nd.station.name &&
         nd.station.access_public &&
         *nd.station.access_public)
      {
        auto val = lower_case(*nd.station.name);

        if(val.find("priv"  )   != std::string::npos || // private
           val.find("only"  )   != std::string::npos || // general exclusion
           val.find("exec"  )   != std::string::npos || // executive charger
           val.find("dealer")   != std::string::npos || // car dealership
           val.find(" staff")   != std::string::npos || // staff charger (space intentional)
           val.find("employee") != std::string::npos || // employee charger
           val.find("cosfleet")     != std::string::npos || // fleet chargers
           val.find("bge fleet")    != std::string::npos ||
           val.find("gsa fleet")    != std::string::npos ||
           val.find("slco fleet")   != std::string::npos ||
           val.find("xcel_fleet")   != std::string::npos ||
           val.find("chem fleet")   != std::string::npos ||
           val.find("osmp fleet")   != std::string::npos ||
           val.find("county fleet") != std::string::npos)
          nd.station.access_public = false;
        else if(val.find("college") != std::string::npos ||
                val.find("campus")  != std::string::npos) // college campus
        {
          nd.station.access_public = false;
          optional_append(nd.station.restrictions, "students and staff only");
        }
        else if(val.find("apartment") != std::string::npos)
        {
          nd.station.access_public.reset();
          optional_append(nd.station.restrictions, "likely private");
        }
        else if(val.find("garage") != std::string::npos)
        {
          nd.station.access_public.reset();
          optional_append(nd.station.restrictions, "likely paid parking");
        }
      }
    }
    else if(nodeL0.identifier == "isPaidParking")
    {
      auto val = safe_bool<__LINE__>(nodeL0);
      if(val && *val)
      {
        optional_append(nd.station.restrictions, "paid parking");
        nd.station.access_public = false;
      }
    }
    else if(nodeL0.identifier == "websiteUrl")
      nd.station.contact.URL = safe_string<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "stats")
    {
      int last365 = -1, last30 = -1;
      if(nodeL0.type != shortjson::Field::Object)
        throw __LINE__;
      for(const shortjson::node_t& nodeL1 : nodeL0.toObject())
      {
        if(nodeL1.identifier == "numberOfSessions")
        {
          if(nodeL1.type != shortjson::Field::Object)
            throw __LINE__;
          for(const shortjson::node_t& nodeL2 : nodeL1.toObject())
          {
            if(nodeL2.identifier == "lastTwelveMonths" && nodeL2.type == shortjson::Field::Integer)
              last365 = nodeL2.toNumber();
            else if(nodeL2.identifier == "lastMonth" && nodeL2.type == shortjson::Field::Integer)
              last30 = nodeL2.toNumber();
            else
              throw __LINE__;
          }
        }
      }
      if(!last365 && !last30)
        nd.station.description = "dead";
    }
    else if(nodeL0.identifier == "status")
    {
      auto val = safe_string<__LINE__>(nodeL0);
      if(val)
      {
        if(val == "outOfService")
          nd.station.functional = false;
        else if(*val == "available" ||
                *val == "inUse")
          nd.station.functional = true;
        else if(*val == "unknown" ||
                *val == "pending")
        {
        }
        else
          nd.station.functional = true;
      }
    }
    else if(nodeL0.identifier == "message" && nodeL0.type != shortjson::Field::Null)
    {
      if(nodeL0.type != shortjson::Field::Object)
        throw __LINE__;
      for(const shortjson::node_t& nodeL1 : nodeL0.toObject())
      {
        if(nodeL1.identifier == "text")
          optional_append(nd.station.description, safe_string<__LINE__>(nodeL1));
        else if(nodeL1.identifier == "date")
        {
          std::stringstream ss;
          std::time_t time = safe_float64<__LINE__>(nodeL1);
          ss << std::put_time(std::localtime(&time), "%Y-%m-%d %X");
          optional_append(nd.station.description, ss.str());
        }
        else
          throw __LINE__;
      }
    }
    else if(nodeL0.identifier == "info")
    {
      if(nodeL0.type != shortjson::Field::Null)
      {
        std::cout << "station: " << nd.station.station_id << std::endl
                  << nodeL0.identifier << ": " << safe_string<__LINE__>(nodeL0) << std::endl;
      }
    }
    else if(nodeL0.identifier == "network")
    {
      if(nodeL0.type != shortjson::Field::Object)
        throw __LINE__;
      for(const shortjson::node_t& nodeL1 : nodeL0.toObject())
      {
        if(nodeL1.identifier == "name")
        {
          if(nodeL1.type != shortjson::Field::String)
            throw __LINE__;
          auto val = nodeL1.toString();
          if(val == "ChargePoint")
            nd.station.network_id = Network::ChargePoint;
          else if(val == "SemaConnect")
            nd.station.network_id = Network::SemaConnect;
          else if(val == "Shell Recharge")
            nd.station.network_id = Network::Shell_Recharge;
          else if(val == "Flo")
            nd.station.network_id = Network::Flo;
          else if(val == "SWTCH")
            nd.station.network_id = Network::SWTCH;
          else if(val == "Hypercharge")
            nd.station.network_id = Network::Hypercharge;
          else if(val == "Electric Circuit" ||
                  val == "Circuit électrique" ||
                  val == "Circuit Électrique")
            nd.station.network_id = Network::Circuit_Électrique;
          else if(val == "Réseau Branché")
            nd.station.network_id = Network::eCharge;
          else
            throw __LINE__;
        }
      }
    }
    else if(nodeL0.identifier == "address")
    {
      if(nodeL0.type != shortjson::Field::Object)
        throw __LINE__;
      for(const shortjson::node_t& nodeL1 : nodeL0.toObject())
      {
        if(nodeL1.identifier == "city")
          nd.station.contact.city = safe_string<__LINE__>(nodeL1);
        else if(nodeL1.identifier == "country")
          nd.station.contact.country = safe_string<__LINE__>(nodeL1);
        else if(nodeL1.identifier == "postalCode")
          nd.station.contact.postal_code = safe_string<__LINE__>(nodeL1);
        else if(nodeL1.identifier == "state")
          nd.station.contact.state = safe_string<__LINE__>(nodeL1);
        else if(nodeL1.identifier == "street")
        {
          if(nodeL1.type != shortjson::Field::String)
            throw __LINE__;
          auto val = nodeL1.toString();
          try
          {
            auto pos = std::find_if(std::begin(val), std::end(val),
                           [](unsigned char c){ return std::isspace(c); });
            nd.station.contact.street_number = ext::from_string<int32_t>(std::string(std::begin(val), pos));
            pos = std::next(pos);
            nd.station.contact.street_name = std::string(pos, std::end(val));
          }
          catch(...)
          {
            nd.station.contact.street_name = val;
          }
        }
        else if(nodeL1.identifier == "location")
        {
          if(nodeL1.type != shortjson::Field::Object)
            throw __LINE__;
          for(const shortjson::node_t& nodeL2 : nodeL1.toObject())
          {
            if(nodeL2.identifier == "lat")
              nd.station.location.latitude = safe_float64<__LINE__>(nodeL2);
            else if(nodeL2.identifier == "lng")
              nd.station.location.longitude = safe_float64<__LINE__>(nodeL2);
            else
              throw __LINE__;
          }
          nd.query.bounds.setFocus(nd.station.location);
          nd.query.bounds.zoom(12.5);
        }
        else if(nodeL1.identifier == "street2")
        {
          if(nodeL1.type != shortjson::Field::Null)
            if(nodeL1.type == shortjson::Field::String &&
               !nodeL1.toString().empty())
              std::cout << "street2: " << nodeL1.toString() << std::endl;
        }
        else
          throw __LINE__;
      }
    }
    else if(nodeL0.identifier == "stations")
    {
      if(nodeL0.type != shortjson::Field::Array)
        throw __LINE__;
      for(const shortjson::node_t& nodeL1 : nodeL0.toArray())
      {
        port_t port;
        if(nodeL1.type != shortjson::Field::Object)
          throw __LINE__;
        for(const shortjson::node_t& nodeL2 : nodeL1.toObject())
        {
          if(nodeL2.identifier == "id")
            port.port_id = safe_string<__LINE__>(nodeL2);
          else if(nodeL2.identifier == "name")
            port.display_name = safe_string<__LINE__>(nodeL2);
          else if(nodeL2.identifier == "type")
            port.power.level = get_level(nodeL2);
          else if(nodeL2.identifier == "power")
            port.power.kw = safe_float64<__LINE__>(nodeL2);
          else if(nodeL2.identifier == "tariff")
          {
            if(nodeL2.type != shortjson::Field::Object)
              throw __LINE__;
            for(const shortjson::node_t& nodeL3 : nodeL2.toObject())
            {
              if(nodeL3.identifier == "unit")
              {
                if(nodeL3.type != shortjson::Field::String)
                  throw __LINE__;
                auto val = nodeL3.toString();
                if(val == "hour")
                  port.price.unit = Unit::Hours;
                else if(val == "kwh")
                  port.price.unit = Unit::KilowattHours;
                else
                  throw __LINE__;
              }
              else if(nodeL3.identifier == "type")
              {
              }
              else if(nodeL3.identifier == "price")
                port.price.per_unit = safe_float64<__LINE__>(nodeL3);
            }
          }
        }
        nd.station.ports.emplace_back(port);
      }
    }
  }
  return nd;
}
