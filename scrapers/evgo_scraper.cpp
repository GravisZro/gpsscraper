#include "evgo_scraper.h"

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
pair_data_t EVGoScraper::BuildQuery(const pair_data_t& input)
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
      data.query.parser = Parser::MapArea;
      data.query.URL = "https://account.evgo.com/stationFacade/findSitesInBounds";
      data.query.post_data = "{\"filterByIsManaged\":true,\"filterByBounds\":"
                             "{\"northEastLat\":" + std::to_string(data.query.bounds.northEast().latitude) +
                             ",\"northEastLng\":" + std::to_string(data.query.bounds.northEast().longitude) +
                             ",\"southWestLat\":" + std::to_string(data.query.bounds.southWest().latitude) +
                             ",\"southWestLng\":" + std::to_string(data.query.bounds.southWest().longitude) +
                             "}}";
      data.query.header_fields = { { "Content-Type", "application/json" } };
      break;

    case Parser::BuildQuery | Parser::Station:
      data.query.parser = Parser::Station;
      data.query.URL = "https://account.evgo.com/stationFacade/findStationsBySiteId";
      data.query.post_data = "{\"filterByIsManaged\":true,\"filterBySiteId\":" +  std::to_string(*data.query.node_id) +"}";
      data.query.header_fields = { { "Content-Type", "application/json" } };
      break;

    case Parser::BuildQuery | Parser::Port:
      data.query.parser = Parser::Port;
      data.query.URL = "https://account.evgo.com/stationFacade/findStationById?stationId=" + std::to_string(*data.query.node_id);
      break;
  }
  return data;
}

std::vector<pair_data_t> EVGoScraper::Parse(const pair_data_t& data, const std::string& input)
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
  return std::vector<pair_data_t>();
}

inline shortjson::node_t response_parse(const std::string& input)
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

std::vector<pair_data_t> EVGoScraper::ParseMapArea(const pair_data_t& data, const std::string& input)
{
  std::vector<pair_data_t> return_data;
  std::vector<uint64_t> child_ids;
  auto response = response_parse(input);

  for(shortjson::node_t& nodeL0 : response.toArray())
  {
    std::optional<uint64_t> quantity, id;
    std::optional<double> latitude, longitude;

    if(nodeL0.type != shortjson::Field::Object)
      throw __LINE__;

    for(shortjson::node_t& nodeL1 : nodeL0.toObject())
    {
      if(nodeL1.identifier == "@class")
      {
        auto val = safe_string<__LINE__>(nodeL1);
        if(!val || *val != "com.driivz.appserver.bl.dto.imp.SiteMapDataDtoImp")
          throw __LINE__;
      }
      else if(nodeL1.identifier == "q")
        quantity = safe_int<__LINE__>(nodeL1);
      else if(nodeL1.identifier == "id")
        id = safe_int<__LINE__>(nodeL1);
      else if(nodeL1.identifier == "latitude")
        latitude = safe_float64<__LINE__>(nodeL1);
      else if(nodeL1.identifier == "longitude")
        longitude = safe_float64<__LINE__>(nodeL1);
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
        child_ids.push_back(*id);
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
    nd.query.parser = Parser::UpdateRecord | Parser::MapArea;
    nd.query.child_ids = child_ids;
    return_data.emplace(std::begin(return_data), nd);
  }

  return return_data;
}


std::vector<pair_data_t> EVGoScraper::ParseStation([[maybe_unused]] const pair_data_t& data, const std::string& input)
{
  std::vector<pair_data_t> return_data;
  auto response = response_parse(input);

  for(shortjson::node_t& nodeL0 : response.toArray())
  {
    if(nodeL0.type != shortjson::Field::Object)
      throw __LINE__;

    for(shortjson::node_t& nodeL1 : nodeL0.toObject())
    {
      if(nodeL1.identifier == "@class")
      {
        auto val = safe_string<__LINE__>(nodeL1);
        if(!val || *val != "com.driivz.stationserver.bl.dto.imp.StationDtoImp")
          throw __LINE__;
      }
      else if(nodeL1.identifier == "id")
      {
        pair_data_t nd;
        {
          nd.query.parser = Parser::BuildQuery | Parser::Port;
          nd.query.node_id = safe_int<__LINE__>(nodeL1);
        }
        return_data.emplace_back(nd);
      }
    }
  }

  return return_data;
}

std::vector<pair_data_t> EVGoScraper::ParsePort(const pair_data_t& data, const std::string& input)
{
  pair_data_t nd;
  auto response = response_parse(input);

  nd.station.network_id = data.station.network_id;
  nd.station.access_public = true;

  std::optional<std::string> port_name;
  bool credit_card_ok = false;
  for(shortjson::node_t& nodeL0 : response.toObject())
  {
    if(nodeL0.identifier == "@class")
    {
      auto val = safe_string<__LINE__>(nodeL0);
      if(!val || *val != "com.driivz.stationserver.bl.dto.imp.StationDtoImp")
        throw __LINE__;
    }
    else if(nodeL0.identifier == "id")
      nd.station.station_id = safe_int<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "addressAddress1")
    {
      auto val = safe_string<__LINE__>(nodeL0);
      try
      {
        auto pos = std::find_if(std::begin(*val), std::end(*val),
                       [](unsigned char c){ return std::isspace(c); });
        nd.station.contact.street_number = std::stoi(std::string(std::begin(*val), pos));
        pos = std::next(pos);
        nd.station.contact.street_name = std::string(pos, std::end(*val));
      }
      catch(...)
      {
        nd.station.contact.street_name = *val;
      }
    }
    else if(nodeL0.identifier == "addressCity")
      nd.station.contact.city = safe_string<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "addressUsaStateCode")
      nd.station.contact.state = safe_string<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "addressCountryIso3Code")
      nd.station.contact.country = safe_string<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "addressZipCode")
      nd.station.contact.zipcode = safe_string<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "latitude")
      nd.station.location.latitude = safe_float64<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "longitude")
      nd.station.location.longitude = safe_float64<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "stationModelName")
    {
      auto val = safe_string<__LINE__>(nodeL0);
      if(val)
      {
        std::transform(std::begin(*val), std::end(*val), std::begin(*val),
                       [](unsigned char c){ return c == 'K' ? 'k' : c; });

        ext::string core(*val);
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
          std::clog << "station model: " << *val << std::endl;
      }
    }
    else if(nodeL0.identifier == "caption")
      port_name = safe_string<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "stationSockets")
    {
      for(shortjson::node_t& nodeL1 : nodeL0.toArray())
      {
        port_t port;
        if(credit_card_ok)
          port.price.payment |= Payment::Credit;
        for(shortjson::node_t& nodeL2 : nodeL1.toObject())
        {
          if(nodeL2.identifier == "id")
            port.port_id = safe_int<__LINE__>(nodeL2);
          else if(nodeL2.identifier == "stationModelSocketSocketTypeId")
          {
            auto val = safe_string<__LINE__>(nodeL2);
            if(val == "TYPE_1_J1772_YAZAKI")
              port.power.connector = Connector::J1772;
            else if(val == "SAE_J1772_COMBO_US")
                port.power.connector = Connector::CCS1;
            else if(val == "TYPE_4_CHADEMO")
              port.power.connector = Connector::CHAdeMO;
            else
              throw __LINE__;
          }
          else if(nodeL2.identifier == "stationModelSocketChargingMode")
          {
            auto val = safe_string<__LINE__>(nodeL2);
            if(!val)
              throw __LINE__;
            else if(val == "LEVEL3")
              port.power.level = 3;
            else if(val == "LEVEL2")
              port.power.level = 2;
            else if(val == "LEVEL1")
              port.power.level = 1;
            else
              throw __LINE__;
          }
          else if(nodeL2.identifier == "stationModelSocketMaximumPower")
            port.power.kw = safe_float64<__LINE__>(nodeL2);
          else if(nodeL2.identifier == "rfidCardEnrollmentPending")
          {
            if(auto val = safe_bool<__LINE__>(nodeL2); !val && !*val)
              port.price.payment |= Payment::RFID;
          }
          else if(nodeL2.identifier == "socketPrices")
          {
            assert(nodeL2.toArray().size() == 1);
            for(shortjson::node_t& nodeL3 : nodeL2.toArray().front().toObject())
            {
              if(nodeL3.identifier == "currency")
              {
                auto val = safe_string<__LINE__>(nodeL3);
                if(val && *val == "USD")
                  port.price.currency = Currency::USD;
                else
                  throw __LINE__;
              }
              else if(nodeL3.identifier == "kwhPrice")
              {
                if(auto val = safe_float64<__LINE__>(nodeL3); val)
                {
                  port.price.unit = Unit::Kilowatts;
                  port.price.per_unit = val;
                }
              }
              else if(nodeL3.identifier == "transactionFee")
                port.price.initial = safe_float64<__LINE__>(nodeL3);
              else if(nodeL3.identifier == "minutePrice")
              {
                if(auto val = safe_float64<__LINE__>(nodeL3); val)
                {
                  port.price.unit = Unit::Minutes;
                  port.price.per_unit = val;
                }
              }
            }
          }
        }
        nd.station.ports.emplace_back(port);
      }
    }
    else if(nodeL0.identifier == "siteDisplayName")
      nd.station.name = safe_string<__LINE__>(nodeL0);
    else if(nodeL0.identifier == "openingTimes")
    {
      if(nodeL0.type == shortjson::Field::Array)
      {
        for(shortjson::node_t& nodeL1 : nodeL0.toArray())
        {
          int day_of_week = -1;
          for(shortjson::node_t& nodeL2 : nodeL1.toObject())
          {
            if(nodeL2.identifier == "dayOfWeekId")
            {
              auto val = safe_string<__LINE__>(nodeL2);
              if(val)
              {
                     if(*val == "SUNDAY"   ) day_of_week = 0;
                else if(*val == "MONDAY"   ) day_of_week = 1;
                else if(*val == "TUESDAY"  ) day_of_week = 2;
                else if(*val == "WEDNESDAY") day_of_week = 3;
                else if(*val == "THURSDAY" ) day_of_week = 4;
                else if(*val == "FRIDAY"   ) day_of_week = 5;
                else if(*val == "SATURDAY" ) day_of_week = 6;
                else
                  throw __LINE__;
              }
              nd.station.schedule.days[day_of_week].emplace();
            }
            else if(nodeL2.identifier == "startHour")
            {
              if(auto val = safe_int<__LINE__>(nodeL2);*val)
                nd.station.schedule.days[day_of_week]->first = ((*val / 36000) % 100) + ((*val / 36000) / 100) * 60;
            }
            else if(nodeL2.identifier == "endHour")
            {
              if(auto val = safe_int<__LINE__>(nodeL2);*val)
                nd.station.schedule.days[day_of_week]->second = ((*val / 36000) % 100) + ((*val / 36000) / 100) * 60;
            }
          }
        }
      }
      else
        throw __LINE__;
    }
    else if(nodeL0.identifier == "siteHasGate")
    {
      if(auto gated = safe_bool<__LINE__>(nodeL0); gated && *gated)
        append_section(nd.station.restrictions, "gated");
    }
    else if(nodeL0.identifier == "notesForDriver")
      nd.station.description = safe_string<__LINE__>(nodeL0);
  }

  for(auto& port : nd.station.ports)
    port.display_name = port_name;

  nd.query.parser = Parser::Complete;
  return {{ nd }};
}
