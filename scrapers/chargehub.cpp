#include "chargehub.h"

// https://apiv2.chargehub.com/api/locationsmap?latmin=11.950390327744657&latmax=69.13019374345745&lonmin=112.08930292841586&lonmax=87.47992792841585&limit=10000000&key=olitest&remove_networks=12&remove_levels=1,2&remove_connectors=0,1,2,3,5,6,7&remove_other=0&above_power=

// https://apiv2.chargehub.com/api/stations/details?station_id=85986&language=en


#include <iostream>
#include <algorithm>
#include <regex>

#include <shortjson/shortjson.h>
#include "utilities.h"

std::string to_lowercase(const std::string& input)
{
  std::string output = input;
  std::transform(std::begin(input), std::end(input), std::begin(output),
                 [](unsigned char c){ return std::tolower(c); });
  return output;
}

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
/*/
      data.query.parser = Parser::Station;
      data.query.URL = "https://apiv2.chargehub.com/api/stations/details?language=en&station_id=113425";
      break;
/*/
      data.query.parser = Parser::Initial;
      data.query.bounds = { { 12.5, 55.0 }, { 90.0, -90.0 } };
      break;

    case Parser::BuildQuery | Parser::MapArea:
      data.query.parser = Parser::MapArea;
      data.query.URL= "https://apiv2.chargehub.com/api/locationsmap"
                      "?latmin=" + std::to_string(input.query.bounds.latitude.min) +
                      "&latmax=" + std::to_string(input.query.bounds.latitude.max) +
                      "&lonmin=" + std::to_string(input.query.bounds.longitude.min) +
                      "&lonmax=" + std::to_string(input.query.bounds.longitude.max) +
                      "&limit=1000&key=olitest&remove_networks=&remove_levels=&remove_connectors=&remove_other=0&above_power=";
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

Connector connector_from_string(const std::optional<std::string>& input)
{
  std::string normalized = to_lowercase(*input);

  if(normalized == "nema 5-15")
    return Connector::Nema515;
  else if(normalized == "nema 5-20")
    return Connector::Nema520;
  else if(normalized == "nema 14-50")
    return Connector::Nema1450;
  else if(normalized == "chademo")
    return Connector::CHAdeMO;
  else if(normalized == "chademo & chademo")
    return Connector::Pair | Connector::CHAdeMO;
  else if(normalized == "ev plug (j1772)")
    return Connector::J1772;
  else if(normalized == "j1772 combo" || normalized == "ccs")
    return Connector::CCS1;
  else if(normalized == "ccs & ccs")
    return Connector::Pair | Connector::CCS1;
  else if(normalized == "tesla" ||
          normalized == "tesla supercharger")
    return Connector::Tesla;
  else if(normalized == "ccs & chademo")
    return Connector::CCS1 | Connector::CHAdeMO;
  throw __LINE__;
  return Connector::Unknown;
}

std::optional<bool> get_access_public(const std::optional<std::string>& input)
{
  if(input == "Public")
    return true;
  if(input == "Restricted")
    return false;
  if(input == "Unknown")
    return std::optional<bool>();
  throw __LINE__;
}

Payment get_payment_methods(const std::optional<std::string>& input)
{
  Payment rv = Payment::Undefined;
  if(input)
  {
    if(input->find("Network App") != std::string::npos)
      rv |= Payment::API;
    if(input->find("Network RFID") != std::string::npos)
      rv |= Payment::RFID;
    if(input->find("Online") != std::string::npos)
      rv |= Payment::Website;
    if(input->find("Phone") != std::string::npos)
      rv |= Payment::PhoneCall;
    if(input->find("Cash") != std::string::npos)
      rv |= Payment::Cash;
    if(input->find("Cheque") != std::string::npos)
      rv |= Payment::Cheque;
    if(input->find("Visa") != std::string::npos ||
       input->find("Mastercard") != std::string::npos ||
       input->find("Discover") != std::string::npos ||
       input->find("American Express") != std::string::npos)
      rv |= Payment::Credit;
  }
  return rv;
}

bool is_private(const std::optional<std::string>& input)
{
  if(!input)
    return false;
  std::string tmp = to_lowercase(*input);

  if(tmp.find("use only") != std::string::npos ||
     tmp.find("customer") != std::string::npos ||
     tmp.find("dealership") != std::string::npos ||
     tmp.find("garage") != std::string::npos ||
     tmp.find("lot") != std::string::npos ||
     tmp.find("permit holder") != std::string::npos ||
     tmp.find("client") != std::string::npos ||
     tmp.find("fee") != std::string::npos)
    return true;

  return false;
}

void price_processor(price_t& price)
{
  if(price.text == "Cost: Free")
  {
    price.unit = Unit::Free;
    price.payment = Payment::Free;
    price.per_unit.reset();
    price.text.reset();
  }
  else if(price.text == "Cost: Unknown")
  {
    price.unit = Unit::Unknown;
    price.payment = Payment::Undefined;
    price.per_unit.reset();
    price.text.reset();
  }
  else if(price.unit == Unit::Session && price.per_unit)
  {
    price.initial = price.per_unit;
    price.unit.reset();
    price.per_unit.reset();
    price.text.reset();
  }
  else if(price.unit && price.per_unit && price.text)
  {
    if(std::regex_match(*price.text, std::regex("^\\$([[:digit:]]*[.][[:digit:]][[:digit:]])[[:space:]]/[[:space:]](kWh|hr|min)", std::regex_constants::extended)))
      price.text.reset();
  }
  else if(!price.unit && !price.per_unit && price.text)
  {
    std::smatch match;
    if(std::regex_match(*price.text, match, std::regex("^\\$([[:digit:]]*[.][[:digit:]][[:digit:]])[[:space:]]/[[:space:]](kWh|hr|min)", std::regex_constants::extended)))
    {
      enum
      {
        UnitPrice = 1,
        UnitType,
      };

      price.per_unit = ext::from_string<double>(match[UnitPrice].str());
      if(match[UnitType].str() == "hr")
        price.unit = Unit::Hours;
      else if(match[UnitType].str() == "kWh")
        price.unit = Unit::KilowattHours;
      else if(match[UnitType].str() == "min")
        price.unit = Unit::Minutes;
      price.text.reset();
    }
  }
}


enum Language
{
  English = 0,
  French,
};

enum DayValues
{
  Monday = 0,
  Tuesday,
  Wednesday,
  Thursday,
  Friday,
  Saturday,
  Sunday,
  Error,
  Ambiguous,
};

constexpr std::array<std::array<std::string_view, 7>, 2> day_name_data =
{
  {
    { "monday", "tuesday",  "wednesday",  "thursday", "friday",   "saturday", "sunday",   },
    { "lundi",  "mardi",    "mercredi",   "jeudi",    "vendredi", "samedi",   "dimanche", },
  }
};


DayValues day_number(Language lang, const std::string& day)
{
  switch(lang)
  {
    case English:
      switch(day.at(0))
      {
        case 'm': return Monday;
        case 't': return day.size() == 1 ? Ambiguous : day.at(1) == 'h' ? Thursday : Tuesday;
        case 'w': return Wednesday;
        case 'f': return Friday;
        case 's': return day.size() == 1 ? Ambiguous : day.at(1) == 'u' ? Sunday : Saturday;
      }
      break;
    case French:
      switch(day.at(0))
      {
        case 'l': return Monday;
        case 'm': return day.size() == 1 ? Ambiguous : day.at(1) == 'e' ? Wednesday : Tuesday;
        case 'j': return Thursday;
        case 'v': return Friday;
        case 's': return Saturday;
        case 'd': return Sunday;
      }
      break;
  }
  return Error;
}

uint32_t time_to_minutes(const std::string& time)
{
  enum
  {
    Hours = 1,
    Minutes,
  };
  std::smatch match;
  if(std::regex_search(time, match, std::regex("^([[:digit:]]{1,2}):([[:digit:]]{2})$", std::regex_constants::extended)))
    return (ext::from_string<uint32_t>(match[Hours].str()) * 60) +
        ext::from_string<uint32_t>(match[Minutes].str());

  return 0;
}

std::string days(Language lang, std::size_t length = std::string::npos)
{
  std::string result;
  for(auto& name : day_name_data[lang])
  {
    if(!result.empty())
      result.push_back('|');
    result.append(name.substr(0, length));
  }
  return "(" + result + ")";
}

enum class RegexId
{
  DayList = 0,
  DayRange,
};

const std::regex& day_regex(RegexId type, Language lang, std::size_t length = std::string::npos)
{
  using regexset = const std::map<std::pair<Language, std::size_t>, std::regex>;
  auto build = [](const std::string_view regexstr) -> regexset
   {
    return
    {
      { { English, std::string::npos }, std::regex(ext::string(regexstr).arg(days(English)), std::regex_constants::extended) },
      { { French , std::string::npos }, std::regex(ext::string(regexstr).arg(days(French )), std::regex_constants::extended) },
      { { English, 3 }, std::regex(ext::string(regexstr).arg(days(English, 3)), std::regex_constants::extended) },
      { { French , 3 }, std::regex(ext::string(regexstr).arg(days(French , 3)), std::regex_constants::extended) },
      { { English, 2 }, std::regex(ext::string(regexstr).arg(days(English, 2)), std::regex_constants::extended) },
      { { French , 2 }, std::regex(ext::string(regexstr).arg(days(French , 2)), std::regex_constants::extended) },
      { { English, 1 }, std::regex(ext::string(regexstr).arg(days(English, 1)), std::regex_constants::extended) },
      { { French , 1 }, std::regex(ext::string(regexstr).arg(days(French , 1)), std::regex_constants::extended) },
    };
   };
  static const std::map<RegexId, regexset> regexes =
  {
    {
      { RegexId::DayList, build("^((%0:?[[:space:]])+)(closed|([[:digit:]]{1,2}:[[:digit:]]{2})[[:space:]]?[~-][[:space:]]?([[:digit:]]{1,2}:[[:digit:]]{2}))") },
      { RegexId::DayRange, build("^%0[[:space:]]?-[[:space:]]?%0:?[[:space:]](closed|([[:digit:]]{1,2}:[[:digit:]]{2})[[:space:]]?[~-][[:space:]]?([[:digit:]]{1,2}:[[:digit:]]{2}))") },
    }
  };

  //regexes.at(type).at(std::make_pair(lang, length)).
  return regexes.at(type).at(std::make_pair(lang, length));
}

std::string process_schedule(const std::optional<std::string>& input)
{

//  auto list = ext::string("^((%0:?[[:space:]])+)(closed|([[:digit:]]{1,2}:[[:digit:]]{2})[[:space:]]?[~-][[:space:]]?([[:digit:]]{1,2}:[[:digit:]]{2}))");
//  auto range = ext::string("^%0[[:space:]]?-[[:space:]]?%0:?[[:space:]](closed|([[:digit:]]{1,2}:[[:digit:]]{2})[[:space:]]?[~-][[:space:]]?([[:digit:]]{1,2}:[[:digit:]]{2}))");
//  std::cout << (range.arg(days(English, 2))) << std::endl;

  schedule_t output;
  if(input->empty())
  {
  }
  else if(input == "24 Hours")
    output = "0,1440;0,1440;0,1440;0,1440;0,1440;0,1440;0,1440";
  else
  {
    //std::cout << "original: " << *input << std::endl;
    if(input->find(':') && (input->find('~') || input->find('-')))
    {
      std::string haystack = to_lowercase(*input);
      std::smatch match;

      while(!haystack.empty())
      {
        Language lang;
        std::set<DayValues> selected_days;
        std::string start_time, end_time;

        if(std::regex_search(haystack, match, std::regex("^([[:digit:]]{1,2}:[[:digit:]]{2})[[:space:]]?[~-][[:space:]]?([[:digit:]]{1,2}:[[:digit:]]{2})$", std::regex_constants::extended)))
        {
          enum
          {
            StartTime = 1,
            EndTime,
          };

          if(match[StartTime].matched)
            start_time = match[StartTime].str();
          if(match[EndTime].matched)
            end_time = match[EndTime].str();
          selected_days = { Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday, };
        }
        else if((lang = English, std::regex_search(haystack, match, day_regex(RegexId::DayRange, lang))) ||
           (lang = French,  std::regex_search(haystack, match, day_regex(RegexId::DayRange, lang))) ||
           (lang = English, std::regex_search(haystack, match, day_regex(RegexId::DayRange, lang, 3))) ||
           (lang = French,  std::regex_search(haystack, match, day_regex(RegexId::DayRange, lang, 3))) ||
           (lang = English, std::regex_search(haystack, match, day_regex(RegexId::DayRange, lang, 2))) ||
           (lang = French,  std::regex_search(haystack, match, day_regex(RegexId::DayRange, lang, 2))) ||
           (lang = English, std::regex_search(haystack, match, day_regex(RegexId::DayRange, lang, 1))) ||
           (lang = French,  std::regex_search(haystack, match, day_regex(RegexId::DayRange, lang, 1))))
        {
          enum
          {
            StartDay = 1,
            EndDay,
            StartTime = 4,
            EndTime,
          };

          const int end_day = day_number(lang, match[EndDay].str());
          int pos = day_number(lang, match[StartDay].str());
          if(pos > end_day)
          {
            while(pos < 7)
              selected_days.insert(DayValues(pos++));
            pos = 0;
          }

          while(pos <= end_day)
            selected_days.insert(DayValues(pos++));

          if(selected_days.contains(Ambiguous) ||
             selected_days.contains(Error))
            throw __LINE__;

          if(match[StartTime].matched)
            start_time = match[StartTime].str();
          if(match[EndTime].matched)
            end_time = match[EndTime].str();
        }
        else if((lang = English, std::regex_search(haystack, match, day_regex(RegexId::DayList, lang))) ||
                (lang = French,  std::regex_search(haystack, match, day_regex(RegexId::DayList, lang))) ||
                (lang = English, std::regex_search(haystack, match, day_regex(RegexId::DayList, lang, 3))) ||
                (lang = French,  std::regex_search(haystack, match, day_regex(RegexId::DayList, lang, 3))) ||
                (lang = English, std::regex_search(haystack, match, day_regex(RegexId::DayList, lang, 2))) ||
                (lang = French,  std::regex_search(haystack, match, day_regex(RegexId::DayList, lang, 2))) ||
                (lang = English, std::regex_search(haystack, match, day_regex(RegexId::DayList, lang, 1))) ||
                (lang = French,  std::regex_search(haystack, match, day_regex(RegexId::DayList, lang, 1))))
        {
          enum
          {
            DayList = 1,
            StartTime = 5,
            EndTime,
          };

          auto day_list = ext::string(match[DayList].str()).split_string({' '});
          for(const auto& pos : day_list)
            if(!pos.empty())
              selected_days.insert(day_number(lang, pos));

          if(match[StartTime].matched)
            start_time = match[StartTime].str();
          if(match[EndTime].matched)
            end_time = match[EndTime].str();
        }

        if(!selected_days.empty())
        {
          haystack = match.suffix();

          int start_time_mins = time_to_minutes(start_time);
          int end_time_mins = time_to_minutes(end_time);

          for(auto daynum : selected_days)
            if(!start_time.empty() && !end_time.empty())
              output.days[daynum] = std::make_pair(start_time_mins, end_time_mins);

        }
        else
        {
          output = *input;
          std::cout << "bad schedule: " << std::string(output) << std::endl;
          break;
        }

        while(haystack.front() == '/' ||
              haystack.front() == ',' ||
              haystack.front() == ';' ||
              haystack.front() == ' ' ||
              haystack.starts_with(" and "))
        {
          if(haystack.starts_with(" and "))
            haystack.erase(0, 5);
          else
            haystack.erase(0, 1);
        }
      }
    }
  }
  return output;
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
         nodeL1.idEnum("NetworkId", nd.station.network_id) ||
         nodeL1.idString("PriceString", nd.station.price.text))
      {
      }
      else if(nodeL1.idString("AccessTime", tmpstr))
      {
        nd.station.schedule = process_schedule(tmpstr);
        if(is_private(tmpstr))
          nd.station.access_public = false;
      }
      else if(nodeL1.idString("AccessType", tmpstr))
        nd.station.access_public = get_access_public(tmpstr);
      else if(nodeL1.idString("PaymentMethods", tmpstr))
        nd.station.price.payment |= get_payment_methods(tmpstr);
      else if(nodeL1.idArray("PlugsArray"))
      {
        price_processor(nd.station.price);
        for(auto& nodeL2 : nodeL1.safeArray())
        {
          port_t port;
          port.price = nd.station.price;

          for(auto& nodeL3 : nodeL2.safeObject())
          {
            if(nodeL3.idInteger("Level", port.power.level) ||
               nodeL3.idFloatUnit("Amp", port.power.amp) ||
               nodeL3.idFloatUnit("Kw", port.power.kw) ||
               nodeL3.idFloatUnit("Volt", port.power.volt) ||
               nodeL3.idFloat("ChargingRate", port.price.per_unit) ||
               nodeL3.idEnum("ChargingRateUnit", port.price.unit) ||
               nodeL3.idEnum("Status", port.status) ||
               nodeL3.idString("PriceString", port.price.text))
            {
            }
            else if(nodeL3.idString("PaymentMethods", tmpstr))
              port.price.payment |= get_payment_methods(tmpstr);
            else if(nodeL3.idArray("Ports"))
            {
              price_processor(port.price);
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
                thisport.power.connector = connector_from_string(tmpstr);
            }
          }
        }
      }
    }

    if(nd.station.description && nd.station.description->empty())
      nd.station.description.reset();

    return_data.emplace_back(nd);
  }
  return return_data;
}
