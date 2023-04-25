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
    safenode_t root = shortjson::Parse(input);

    if(root.type == shortjson::Field::Array)
    {
      ext::string child_ids;
      for(const safenode_t& nodeL0 : root.safeArray())
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
      parent.query.parser = Parser::ReplaceRecord | Parser::MapArea;
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

std::optional<int32_t> get_level(const safenode_t& node)
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

pair_data_t EptixScraper::ParseStationNode(const pair_data_t& data, const safenode_t& root) const
{
  if(root.type != shortjson::Field::Object)
    throw __LINE__;

  pair_data_t nd;
  nd.query.parser = Parser::Discard;
  nd.station.network_id = Network::Circuit_Électrique; // default network
  nd.query.bounds = data.query.bounds;
  nd.station.access_public = true; // public by default

  std::optional<Status> status;
  std::optional<std::string> tmpstr;
  std::optional<bool> tmpbool;
  std::optional<double> tmpdbl;

  for(const safenode_t& nodeL0 : root.safeObject())
  {
    if(nodeL0.type == shortjson::Field::Null)
      continue; // ignore field

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

    if(nodeL0.idString("websiteUrl", nd.station.contact.URL))
    {
    }
    else if(nodeL0.idString("id", nd.station.station_id))
      nd.query.node_id = nd.station.station_id;
    else if(nodeL0.idString("name", nd.station.name))
    {
      if(nd.station.access_public == true)
      {
        tmpstr = lower_case(*nd.station.name);

        if(tmpstr->find("priv"  )   != std::string::npos || // private
           tmpstr->find("only"  )   != std::string::npos || // general exclusion
           tmpstr->find("exec"  )   != std::string::npos || // executive charger
           tmpstr->find("dealer")   != std::string::npos || // car dealership
           tmpstr->find(" staff")   != std::string::npos || // staff charger (space intentional)
           tmpstr->find("employee") != std::string::npos || // employee charger
           tmpstr->find("cosfleet")     != std::string::npos || // fleet chargers
           tmpstr->find("bge fleet")    != std::string::npos ||
           tmpstr->find("gsa fleet")    != std::string::npos ||
           tmpstr->find("slco fleet")   != std::string::npos ||
           tmpstr->find("xcel_fleet")   != std::string::npos ||
           tmpstr->find("chem fleet")   != std::string::npos ||
           tmpstr->find("osmp fleet")   != std::string::npos ||
           tmpstr->find("county fleet") != std::string::npos)
          nd.station.access_public = false;
        else if(tmpstr->find("college") != std::string::npos ||
                tmpstr->find("campus")  != std::string::npos) // college campus
        {
          nd.station.access_public = false;
          optional_append(nd.station.restrictions, "students and staff only");
        }
        else if(tmpstr->find("apartment") != std::string::npos)
        {
          nd.station.access_public.reset();
          optional_append(nd.station.restrictions, "likely private");
        }
        else if(tmpstr->find("garage") != std::string::npos)
        {
          nd.station.access_public.reset();
          optional_append(nd.station.restrictions, "likely paid parking");
        }
      }
    }
    else if(nodeL0.idBool("isPaidParking", tmpbool))
    {
      if(tmpbool == true)
      {
        optional_append(nd.station.restrictions, "paid parking");
        nd.station.access_public = false;
      }
    }
    else if(nodeL0.idObject("stats"))
    {
      std::optional<int32_t> last365, last30;
      for(const safenode_t& nodeL1 : nodeL0.safeObject())
      {
        if(nodeL1.idObject("numberOfSessions"))
        {
          for(const safenode_t& nodeL2 : nodeL1.safeObject())
          {
            if(nodeL2.idInteger("lastTwelveMonths", last365) ||
               nodeL2.idInteger("lastMonth", last30))
            {
            }
            else
              throw __LINE__;
          }
        }
      }
      if(last365 && !last365 && last30 && !last30)
        nd.station.description = "dead";
    }
    else if(nodeL0.idString("status", tmpstr))
    {
      if(tmpstr == "outOfService")
        status = Status::Broken;
      else if(*tmpstr == "available")
        status = Status::Operational;
      else if(*tmpstr == "inUse")
        status = Status::InUse;
      else if(*tmpstr == "unknown" ||
              *tmpstr == "pending")
      {
      }
      else
        status = Status::Operational;
    }
    else if(nodeL0.idObject("message"))
    {
      for(const safenode_t& nodeL1 : nodeL0.safeObject())
      {
        if(nodeL1.idString("text", tmpstr))
          optional_append(nd.station.description, tmpstr);
        else if(nodeL1.idFloat("date", tmpdbl))
        {
          std::stringstream ss;
          std::time_t time = *tmpdbl;
          ss << std::put_time(std::localtime(&time), "%Y-%m-%d %X");
          optional_append(nd.station.description, ss.str());
        }
        else
          throw __LINE__;
      }
    }
    else if(nodeL0.identifier == "info")
    {
      if(nodeL0.idString("Line2", tmpstr))
        std::cout << "station: " << nd.station.station_id << std::endl
                  << nodeL0.identifier << ": " << *tmpstr << std::endl;
    }
    else if(nodeL0.idObject("network"))
    {
      for(const safenode_t& nodeL1 : nodeL0.safeObject())
      {
        if(nodeL1.idString("name", tmpstr))
        {
          if(tmpstr == "ChargePoint")
            nd.station.network_id = Network::ChargePoint;
          else if(tmpstr == "SemaConnect")
            nd.station.network_id = Network::SemaConnect;
          else if(tmpstr == "Shell Recharge")
            nd.station.network_id = Network::Shell_Recharge;
          else if(tmpstr == "Flo")
            nd.station.network_id = Network::Flo;
          else if(tmpstr == "SWTCH")
            nd.station.network_id = Network::SWTCH;
          else if(tmpstr == "Hypercharge")
            nd.station.network_id = Network::Hypercharge;
          else if(tmpstr == "Electric Circuit" ||
                  tmpstr == "Circuit électrique" ||
                  tmpstr == "Circuit Électrique")
            nd.station.network_id = Network::Circuit_Électrique;
          else if(tmpstr == "Réseau Branché")
            nd.station.network_id = Network::eCharge;
          else
            throw __LINE__;
        }
      }
    }
    else if(nodeL0.idObject("address"))
    {
      for(const safenode_t& nodeL1 : nodeL0.safeObject())
      {
        if(nodeL1.idString("city", nd.station.contact.city) ||
           nodeL1.idString("country", nd.station.contact.country) ||
           nodeL1.idString("postalCode", nd.station.contact.postal_code) ||
           nodeL1.idString("state", nd.station.contact.state) ||
           nodeL1.idStreet("street", nd.station.contact.street_number, nd.station.contact.street_name))
        {
        }
        else if(nodeL1.idCoords("location", "lat", "lng", nd.station.location))
        {
          nd.query.bounds.setFocus(nd.station.location);
          nd.query.bounds.zoom(12.5);
        }
        else if(nodeL1.identifier == "street2")
        {
          if(nodeL1.idString("street2", tmpstr))
            std::cout << "street2: " << *tmpstr << std::endl;
        }
        else if(nodeL1.type != shortjson::Field::Null)
          throw __LINE__;
      }
    }
    else if(nodeL0.idArray("stations"))
    {
      for(const safenode_t& nodeL1 : nodeL0.safeArray())
      {
        port_t port;
        port.status = status;
        if(nodeL1.type != shortjson::Field::Object)
          throw __LINE__;
        for(const safenode_t& nodeL2 : nodeL1.safeObject())
        {
          if(nodeL2.idString("id", port.port_id) ||
             nodeL2.idString("name", port.display_name) ||
             nodeL2.idFloat("power", port.power.kw))
          {
          }
          else if(nodeL2.identifier == "type")
            port.power.level = get_level(nodeL2);
          else if(nodeL2.idObject("tariff"))
          {
            for(const safenode_t& nodeL3 : nodeL2.safeObject())
            {
              if(nodeL3.identifier == "desc" &&
                 nodeL3.type == shortjson::Field::Object)
              {
                for(const safenode_t& nodeL4 : nodeL3.safeObject())
                  nodeL4.idString("en", port.price.text);
              }
              else if(nodeL3.idString("desc", port.price.text) ||
                 nodeL3.idFloat("price", port.price.per_unit))
              {
              }
              else if(nodeL3.identifier == "type")
              {
              }
              else if(nodeL3.idString("unit", tmpstr))
              {
                if(tmpstr == "hour")
                  port.price.unit = Unit::Hours;
                else if(tmpstr == "kwh")
                  port.price.unit = Unit::KilowattHours;
                else
                  throw __LINE__;
              }
            }
          }
        }
        nd.station.ports.emplace_back(port);
      }
    }
  }
  return nd;
}
