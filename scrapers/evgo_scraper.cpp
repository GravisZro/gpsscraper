#include "evgo_scraper.h"

#include <iostream>
#include <algorithm>

#include <shortjson/shortjson.h>
#include <cassert>


std::stack<station_info_t> EVGoScraper::Parse(const station_info_t& station_info, const ext::string& input)
{
  switch(station_info.parser)
  {
    default: assert(false);
    case Parser::Index: return ParseIndexes(station_info, input);
    case Parser::Station: return ParseStation(station_info, input);
  }
  return std::stack<station_info_t>();
}

std::stack<station_info_t> EVGoScraper::Init(void)
{
  std::stack<station_info_t> return_data;
  return_data.push(progress_info_t {
                     .parser = Parser::Index,
                     .details_URL = "https://account.evgo.com/stationFacade/findSitesInBounds",
                     .post_data = R"({"filterByIsManaged":true,"filterByBounds":{"northEastLat":56.2,"northEastLng":-11.3,"southWestLat":15.3,"southWestLng":180}})",
                     .header_fields = { { "X-JSON-TYPES", "None" }, { "Content-Type", "application/json" }, },
                   });
  return return_data;
}

std::stack<station_info_t> EVGoScraper::ParseIndexes(const station_info_t& station_info, const ext::string& input)
{
  std::stack<station_info_t> return_data;
  try
  {
    std::list<intmax_t> station_list;

    if(input == R"({"errors":[],"success":true,"data":[]})")
    {
      ext::string old_post_data = station_info.post_data;
      size_t offset = old_post_data.find_after_or_throw(R"("filterBySiteId":)", 0, 1);
      size_t end = old_post_data.find_before_or_throw("}", offset, 1);
      std::string station_id = old_post_data.substr(offset, end - offset);

      station_info_t tmp_data = progress_info_t {
                                .parser = Parser::Station,
                                .details_URL = "https://account.evgo.com/stationFacade/findStationById?stationId=" + station_id,
                                .post_data = nullptr,
                                .header_fields = { { "X-JSON-TYPES", "None" }, },
                              };
      tmp_data.network_id = network::EVgo;
      tmp_data.network_station_id = std::stoi(station_id);
      return_data.push(tmp_data);
    }
    else
    {
      shortjson::node_t root = shortjson::Parse(input);

      if(root.type != shortjson::Field::Object &&
         root.type != shortjson::Field::Array)
        throw 2;

      auto reply = root.toArray();

      auto pos = std::begin(reply);
      if(pos->identifier != "errors" ||
         pos->type != shortjson::Field::Array ||
         !pos->toArray().empty())
        throw 3;

      pos = std::next(pos);
      if(pos->identifier != "success" ||
         pos->type != shortjson::Field::Boolean ||
         !pos->toBool())
        throw 4;

      pos = std::next(pos);
      if(pos->identifier != "data" ||
         pos->type != shortjson::Field::Array)
        throw 5;

      for(shortjson::node_t& child_node : pos->toArray())
      {
        if(child_node.type != shortjson::Field::Object)
          throw 7;

        auto& subnode = child_node.toArray().front();
        if(subnode.identifier != "id")
          throw 8;

        if(subnode.type != shortjson::Field::Integer)
          throw 9;

        return_data.push(progress_info_t {
                           .parser = Parser::Index,
                           .details_URL = "https://account.evgo.com/stationFacade/findStationsBySiteId",
                           .post_data = R"({"filterBySiteId":)" + std::to_string(subnode.toNumber()) + "}",
                           .header_fields = { { "X-JSON-TYPES", "None" }, { "Content-Type", "application/json" }, },
                         });
      }
    }

  }
  catch(int exit_id)
  {
    std::cerr << "EVGoScraper::ParseIndex threw: " << exit_id << std::endl;
    std::cerr << "page dump:" << std::endl << input << std::endl;
  }
  return return_data;
}

std::stack<station_info_t> EVGoScraper::ParseStation(const station_info_t& station_info, const ext::string& input)
{
  std::stack<station_info_t> return_data;

/*
{
  "errors": [],
  "success": true,
  "data": {
    "id": 6419,
    "deleted": false,
    "dirty": false,
    "caption": "CYGNUS",
    "latitude": 48.74393,
    "longitude": -122.461854,
    "isManaged": true,
    "stationStatusId": "AVAILABLE",
    "stationAccessLevelId": "PUBLIC",
    "identityKey": "EVFC-NRGS-122018-480-0554",
    "offline": false,
    "map1": "55840347.JPG",
    "map2": "85639686.JPG",
    "picture1": "63049444.JPG",
    "picture2": "46791619.JPG",
    "addressUsaStateName": "Washington",
    "addressUsaStateCode": "WA",
    "addressCountryName": "United States",
    "addressCountryId": 235,
    "addressCountryIso3Code": "USA",
    "addressCountryIso2Code": "US",
    "addressAddress1": "1030 Lakeway Dr",
    "addressCity": "Bellingham",
    "addressZipCode": "98229",
    "addressRegion": "Whatcom",
    "stationModelName": "BTC Slimline 480V 50kW|CHAdeMO CCS",
    "stationModelInstructionsVideoUrl": "",
    "stationOwnerId": 206,
    "stationOwnerName": "Whole Foods",
    "chargingSpeedId": "ULTRA_FAST",
    "stationSockets": [
      {
        "id": 7578,
        "deleted": false,
        "dirty": false,
        "identityKey": "1",
        "name": "CHAdeMO",
        "socketStatusId": "AVAILABLE",
        "maximumPower": 50,
        "stationModelSocketSocketTypeId": "TYPE_4_CHADEMO",
        "stationModelSocketChargingMode": "LEVEL3",
        "stationModelSocketMaximumPower": 50,
        "stationModelSocketVoltageType": "DC",
        "stationIsManaged": true,
        "stationInMaintenance": false,
        "stationId": 6419,
        "socketPrices": [
          {
            "deleted": false,
            "dirty": false,
            "billingSpCurrencyId": 1,
            "currency": "USD",
            "billingSpCurrencyCurrency": "USD",
            "stationId": 6419,
            "stationSocketId": 7578,
            "billingPlanId": 18,
            "billingPlanCode": "79999_POST",
            "kwhPrice": 0.47,
            "transactionFee": 0.99,
            "socketType": "TYPE_4_CHADEMO",
            "futureReservationFee": 0
          }
        ],
        "reserved": false,
        "siteDisplayName": "Whole Foods Bellingham",
        "inMaintenance": false,
        "socketTariffsAreDirty": false,
        "hasTeslaAdapter": false,
        "teslaInMaintenance": false,
        "rfidCardEnrollmentPending": false
      },
      {
        "id": 7579,
        "deleted": false,
        "dirty": false,
        "identityKey": "2",
        "name": "SAE Combo",
        "socketStatusId": "AVAILABLE",
        "maximumPower": 50,
        "stationModelSocketSocketTypeId": "SAE_J1772_COMBO_US",
        "stationModelSocketChargingMode": "LEVEL3",
        "stationModelSocketMaximumPower": 50,
        "stationModelSocketVoltageType": "DC",
        "stationIsManaged": true,
        "stationInMaintenance": false,
        "stationId": 6419,
        "socketPrices": [
          {
            "deleted": false,
            "dirty": false,
            "billingSpCurrencyId": 1,
            "currency": "USD",
            "billingSpCurrencyCurrency": "USD",
            "stationId": 6419,
            "stationSocketId": 7579,
            "billingPlanId": 18,
            "billingPlanCode": "79999_POST",
            "kwhPrice": 0.47,
            "transactionFee": 0.99,
            "socketType": "SAE_J1772_COMBO_US",
            "futureReservationFee": 0
          }
        ],
        "reserved": false,
        "siteDisplayName": "Whole Foods Bellingham",
        "inMaintenance": false,
        "socketTariffsAreDirty": false,
        "hasTeslaAdapter": false,
        "teslaInMaintenance": false,
        "rfidCardEnrollmentPending": false
      }
    ],
    "openingTimes": [],
    "inMaintenance": false,
    "siteId": 2240,
    "siteName": "Whole Foods Bellingham",
    "siteDisplayName": "Whole Foods Bellingham",
    "partyLogo": "https://account.evgo.com/resources/img/themes/EVgo/logos/EVgoLogoSq400x400.png",
    "siteStationAccessLevel": "PUBLIC",
    "propertyId": 2210,
    "siteHasGate": false,
    "canRegisterForNotifyWhenAvailable": false,
    "markForReplacement": false
  }
}
*/

  std::cout << "station data: " << std::endl << input << std::endl << std::endl;

  return return_data;
}
