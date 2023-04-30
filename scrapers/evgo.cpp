#include "evgo.h"

#include <iostream>
#include <algorithm>
#include <iomanip>

#include <shortjson/shortjson.h>
#include "utilities.h"

 // helpers

void append_section(std::optional<std::string>& data, std::string value)
{
  if(data)
    data->append("\n").append(value);
  else
    data = value;
}
/*
constexpr double upper_bound(double origin, double distance)
  { return distance > 0.0 ? origin + distance : origin; }

constexpr double lower_bound(double origin, double distance)
  { return distance < 0.0 ? origin + distance : origin; }

constexpr bool inside_bound(double value, double origin, double distance)
{
  return value < upper_bound(origin, distance) &&
         value > lower_bound(origin, distance);
}
*/

void EVGoScraper::classify(pair_data_t& record) const
{

  if(record.query.child_ids)
    record.query.parser = Parser::BuildQuery | Parser::MapArea;
  else
    record.query.parser = Parser::BuildQuery | Parser::Station;
}

pair_data_t EVGoScraper::BuildQuery(const pair_data_t& input) const
{
  pair_data_t data = input;
  data.station.network_id = Network::EVgo;

  if(!data.query.node_id)
    throw __LINE__;

  switch(data.query.parser)
  {
    default: throw std::string(__FILE__).append(": unknown parser: ").append(std::to_string(int(input.query.parser)));

    case Parser::BuildQuery | Parser::Initial:
#if 1
      data.query.parser = Parser::BuildQuery | Parser::MapArea;
      data.query.bounds = { { 60.0, 15.0 }, { -130.0, -11.0 } };
#elif 1
      // test data
      data.query.parser = Parser::BuildQuery | Parser::Station;
      data.query.node_id = 261;
#else
      // test data
      data.query.parser = Parser::BuildQuery | Parser::Port;
      data.query.node_id = 465;
#endif
      return BuildQuery(data);

    case Parser::BuildQuery | Parser::MapArea:
    {
      data.query.parser = Parser::MapArea;
      data.query.URL = "https://account.evgo.com/stationFacade/findSitesInBounds";
      auto post_data =
          ext::string(
            R"(
            {
              "filterByIsManaged": true,
              "filterByBounds":
              {
                "northEastLat": %0,
                "northEastLng": %1,
                "southWestLat": %2,
                "southWestLng": %3
              }
            })")
          .arg(input.query.bounds.northEast().latitude)
          .arg(input.query.bounds.northEast().longitude)
          .arg(input.query.bounds.southWest().latitude)
          .arg(input.query.bounds.southWest().longitude);
      post_data.erase(std::set<char>{'\n',' '});
      data.query.post_data = post_data;
      data.query.header_fields = { { "Content-Type", "application/json" } };
      break;
    }

    case Parser::BuildQuery | Parser::Station:
    {
      data.query.parser = Parser::Station;
      data.query.URL = "https://account.evgo.com/stationFacade/findStationsBySiteId";
      ext::string post_data =
          R"(
          {
            "filterByIsManaged": true,
            "filterBySiteId": %1
          })";
      post_data.erase(std::set<char>{'\n',' '});
      post_data.replace("%1", *data.query.node_id);
      data.query.post_data = post_data;
      data.query.header_fields = { { "Content-Type", "application/json" } };
      break;
    }

    case Parser::BuildQuery | Parser::Port:
      data.query.parser = Parser::Port;
      data.query.URL = "https://account.evgo.com/stationFacade/findStationById?stationId=" + *data.query.node_id;
      break;
  }
  return data;
}

std::vector<pair_data_t> EVGoScraper::Parse(const pair_data_t& data, const std::string& input) const
{
  try
  {
    switch(data.query.parser)
    {
      default: throw std::string(__FILE__).append(": unknown parser: ").append(std::to_string(int(data.query.parser)));
      case Parser::MapArea: return ParseMapArea(data, input);
      case Parser::Station: return ParseStation(data, input);
      case Parser::Port:    return ParsePort   (data, input);
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

inline safenode_t response_parse(const std::string& input)
{
  shortjson::node_t root = shortjson::Parse(input);

  if(root.type != shortjson::Field::Object)
    throw __LINE__;

  auto reply = root.toObject();

  auto pos = std::begin(reply);
  if(pos->identifier != "errors" ||
     pos->type != shortjson::Field::Array ||
     !pos->toArray().empty())
    throw __LINE__;

  pos = std::next(pos);
  if(pos->identifier != "success" ||
     pos->type != shortjson::Field::Boolean ||
     !pos->toBool())
    throw __LINE__;

  pos = std::next(pos);

  if(pos->identifier != "data")
    throw __LINE__;

  if(pos->type == shortjson::Field::Array)
  {
    auto newpos = std::begin(pos->toArray());
    if(newpos->type == shortjson::Field::String &&
       newpos->toString() == "java.util.ArrayList")
    {
      pos = std::next(newpos);
      if(pos->type != shortjson::Field::Array)
        throw __LINE__;
    }
  }
  else if(pos->type != shortjson::Field::Object)
    throw __LINE__;

  return *pos;
}

std::vector<pair_data_t> EVGoScraper::ParseMapArea(const pair_data_t& data, const std::string& input) const
{
  std::vector<pair_data_t> return_data;
  ext::string child_ids;
  std::optional<std::string> tmpstr;
  auto response = response_parse(input);

  for(const safenode_t& nodeL0 : response.safeArray())
  {
    std::optional<uint64_t> quantity;
    std::optional<double> latitude, longitude;
    std::optional<std::string> id;

    if(nodeL0.type != shortjson::Field::Object)
      throw __LINE__;

    for(const safenode_t& nodeL1 : nodeL0.safeObject())
    {
      if(nodeL1.idString("@class", tmpstr))
      {
        if(tmpstr != "com.driivz.appserver.bl.dto.imp.SiteMapDataDtoImp")
          throw __LINE__;
      }
      else if(nodeL1.idInteger("q", quantity) ||
              nodeL1.idString("id", id) ||
              nodeL1.idFloat("latitude", latitude) ||
              nodeL1.idFloat("longitude", longitude))
      {
      }
    }

    if(id && latitude && longitude)
    {
      pair_data_t nd;
      nd.station.network_id = Network::EVgo;
      if(id == data.query.node_id) // just zoomed in
      {
        nd.query.node_id = data.query.node_id;
        nd.query.child_ids = data.query.child_ids;
      }
      else
      {
        nd.query.node_id = id;
        child_ids.list_append(',', *id);
      }
      nd.query.bounds = data.query.bounds; // copy bounding box
      nd.query.bounds.setFocus({ *latitude, *longitude }); // move box to new location
      nd.query.bounds.zoom(1); // shrink box to new size
      nd.query.parser = Parser::BuildQuery | (quantity ? Parser::MapArea : Parser::Station);
      return_data.emplace_back(nd);
    }
    else
      throw __LINE__;
  }

  if(!child_ids.empty())
  {
    pair_data_t nd = data;
    nd.query.parser = Parser::ReplaceRecord | Parser::MapArea;
    if(!child_ids.empty())
      nd.query.child_ids = child_ids;
    return_data.emplace(std::begin(return_data), nd); // insert to front
  }

  return return_data;
}


std::vector<pair_data_t> EVGoScraper::ParseStation([[maybe_unused]] const pair_data_t& data, const std::string& input) const
{
  std::vector<pair_data_t> return_data;
  std::optional<std::string> tmpstr;
  auto response = response_parse(input);

  for(const safenode_t& nodeL0 : response.safeArray())
  {
    if(nodeL0.type != shortjson::Field::Object)
      throw __LINE__;

    for(const safenode_t& nodeL1 : nodeL0.safeObject())
    {
      if(nodeL1.idString("@class", tmpstr))
      {
        if(tmpstr != "com.driivz.stationserver.bl.dto.imp.StationDtoImp")
          throw __LINE__;
      }
      else if(nodeL1.idString("id", tmpstr))
      {
        pair_data_t nd;
        nd.query.parser = Parser::BuildQuery | Parser::Port;
        nd.query.node_id = tmpstr;
        return_data.emplace_back(nd);
      }
    }
  }

  return return_data;
}

std::vector<pair_data_t> EVGoScraper::ParsePort(const pair_data_t& data, const std::string& input) const
{
  pair_data_t nd;
  auto response = response_parse(input);

  nd.station.network_id = data.station.network_id;
  nd.station.access_public = true;

  std::optional<std::string> port_name, tmpstr;
  std::optional<bool> tmpbool;
  bool credit_card_ok = false;
  for(const safenode_t& nodeL0 : response.safeObject())
  {
    if(nodeL0.idString("siteId", nd.station.station_id) ||
       nodeL0.idString("addressCity", nd.station.contact.city) ||
       nodeL0.idString("addressCountryIso2Code", nd.station.contact.country) ||
       nodeL0.idString("addressZipCode", nd.station.contact.postal_code) ||
       nodeL0.idString("addressUsaStateCode", nd.station.contact.state) ||
       nodeL0.idStreet("addressAddress1", nd.station.contact.street_number, nd.station.contact.street_name) ||
       nodeL0.idFloat("latitude", nd.station.location.latitude) ||
       nodeL0.idFloat("longitude", nd.station.location.longitude) ||
       nodeL0.idString("caption", port_name) ||
       nodeL0.idString("siteDisplayName", nd.station.name) ||
       nodeL0.idString("notesForDriver", nd.station.description))
    {
    }
    else if(nodeL0.idString("@class", tmpstr))
    {
      if(tmpstr != "com.driivz.stationserver.bl.dto.imp.StationDtoImp")
        throw __LINE__;
    }
    else if(nodeL0.idString("stationModelName", tmpstr))
    {
      std::transform(std::begin(*tmpstr), std::end(*tmpstr), std::begin(*tmpstr),
                     [](unsigned char c){ return c == 'K' ? 'k' : c; });

      ext::string core(*tmpstr);
      core = core.substr(0, std::min(core.find('|'), core.find_after("kW")));

      if(core == "ABB Terra 53 50kW" ||
         core == "ABB Terra 54HV 50kW" ||
         core == "ABB Terra HP 175kW" ||
         core == "ABB Terra HP 350kW" ||
         core == "BTC Fatboy 208V 50kW" ||
         core == "BTC Fatboy 480V 50kW" ||
         core == "BTC HP 100kW" ||
         core == "BTC HP 150kW" ||
         core == "BTC HP 175kW" ||
         core == "BTC HP 200kW" ||
         core == "BTC Slimline 480V 50kW" ||
         core == "DCQC 44kW" ||
         core == "Delta City 100 kW" ||
         core == "Delta City 208V 50kW" ||
         core == "Delta HP 200 kW" ||
         core == "Delta HP 350 kW" ||
         core == "Efacec 50kW" ||
         core == "Signet HP 350kW")
        credit_card_ok = true;
      else if(core == "Aero EVSE-CS L2" ||
              core == "BTC EVC-30 L2" ||
              core == "LiteOn IC3 L2")
        credit_card_ok = false;
      else
        std::cout << "station model: " << *tmpstr << std::endl;
    }
    else if(nodeL0.idArray("stationSockets"))
    {
      for(const safenode_t& nodeL1 : nodeL0.safeArray())
      {
        port_t port;
        if(credit_card_ok)
          port.price.payment |= Payment::Credit;
        for(const safenode_t& nodeL2 : nodeL1.safeObject())
        {
          if(nodeL2.idString("id", port.port_id) ||
             nodeL2.idFloat("stationModelSocketMaximumPower", port.power.kw))
          {
          }
          else if(nodeL2.identifier == "inMaintenance" && nodeL2.type == shortjson::Field::Boolean)
            port.status = nodeL2.toBool() ? Status::Broken : Status::Operational;
          else if(nodeL2.idString("stationModelSocketSocketTypeId", tmpstr))
          {
            if(tmpstr == "TYPE_1_J1772_YAZAKI")
              port.power.connector = Connector::J1772;
            else if(tmpstr == "SAE_J1772_COMBO_US")
                port.power.connector = Connector::CCS1;
            else if(tmpstr == "TYPE_4_CHADEMO")
              port.power.connector = Connector::CHAdeMO;
            else
              throw __LINE__;
          }
          else if(nodeL2.idString("stationModelSocketChargingMode", tmpstr))
          {
                 if(tmpstr == "LEVEL3")
              port.power.level = 3;
            else if(tmpstr == "LEVEL2")
              port.power.level = 2;
            else if(tmpstr == "LEVEL1")
              port.power.level = 1;
            else
              throw __LINE__;
          }
          else if(nodeL2.idBool("rfidCardEnrollmentPending", tmpbool))
          {
            if(!*tmpbool)
              port.price.payment |= Payment::RFID;
          }
          else if(nodeL2.idArray("socketPrices"))
          {
            assert(nodeL2.safeArray().size() == 1);
            for(const safenode_t& nodeL3 : nodeL2.safeArray().front().safeObject())
            {
              if(nodeL3.idFloat("transactionFee", port.price.initial))
              {
              }
              else if(nodeL3.idFloat("kwhPrice", port.price.per_unit))
                 port.price.unit = Unit::KilowattHours;
              else if(nodeL3.idFloat("minutePrice", port.price.per_unit))
                 port.price.unit = Unit::Minutes;
              if(nodeL3.idString("currency", tmpstr))
              {
                if(tmpstr == "USD")
                  port.price.currency = Currency::USD;
                else
                  throw __LINE__;
              }
            }
          }
        }
        nd.station.ports.emplace_back(port);
      }
    }
    else if(nodeL0.idArray("openingTimes"))
    {
      std::optional<int32_t> hour;
      for(const safenode_t& nodeL1 : nodeL0.safeArray())
      {
        int day_of_week = -1;
        for(const safenode_t& nodeL2 : nodeL1.safeObject())
        {
          if(nodeL2.idString("dayOfWeekId", tmpstr))
          {
                 if(tmpstr == "SUNDAY"   ) day_of_week = 0;
            else if(tmpstr == "MONDAY"   ) day_of_week = 1;
            else if(tmpstr == "TUESDAY"  ) day_of_week = 2;
            else if(tmpstr == "WEDNESDAY") day_of_week = 3;
            else if(tmpstr == "THURSDAY" ) day_of_week = 4;
            else if(tmpstr == "FRIDAY"   ) day_of_week = 5;
            else if(tmpstr == "SATURDAY" ) day_of_week = 6;
            else
              throw __LINE__;
            nd.station.schedule.days[day_of_week].emplace();
          }
          else if(nodeL1.idInteger("startHour", hour))
            nd.station.schedule.days[day_of_week]->first = ((*hour / 36000) % 100) + ((*hour / 36000) / 100) * 60;
          else if(nodeL1.idInteger("endHour", hour))
            nd.station.schedule.days[day_of_week]->second = ((*hour / 36000) % 100) + ((*hour / 36000) / 100) * 60;
        }
      }
    }
    else if(nodeL0.idBool("siteHasGate", tmpbool))
    {
      if(*tmpbool)
        append_section(nd.station.restrictions, "gated");
    }
  }

  for(auto& port : nd.station.ports)
    port.display_name = port_name;

  nd.query.parser = Parser::Complete;
  return {{ nd }};
}
