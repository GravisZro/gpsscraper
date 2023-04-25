#include "echarge.h"

#include <algorithm>
#include <iostream>
#include <map>

#include <shortjson/shortjson.h>

#include "utilities.h"


pair_data_t EchargeScraper::BuildQuery(const pair_data_t& input) const
{
  pair_data_t data;
  switch(input.query.parser)
  {
    default: throw std::string(__FILE__).append(": unknown parser: ").append(std::to_string(int(input.query.parser)));

    case Parser::BuildQuery | Parser::Initial:
      data.query.parser = Parser::Initial;
      data.query.bounds = { { 29.46, 68.69 }, { -149.37, -49.17 } };
      break;

    case Parser::BuildQuery | Parser::MapArea:
    {
      data.query.parser = Parser::MapArea;
      data.query.URL= "https://account.echargenetwork.com/api/network/markers";
      ext::string post_data =
          R"(
          {
            "FilteringOptions":
            {
              "AvailableStationsOnly":false,
              "FastDCStationsOnly":false,
              "ShowOtherNetworkStations":true
            },
            "Bounds":
            {
              "SouthWest":
              {
                "Lat":%1,
                "Lng":%2
              },
              "NorthEast":
              {
                "Lat":%3,
                "Lng":%4
              }
            }
          })";
      post_data.erase(std::set<char>{'\n',' '});
      post_data.replace("%1", std::to_string(input.query.bounds.southWest().latitude));
      post_data.replace("%2", std::to_string(input.query.bounds.southWest().longitude));
      post_data.replace("%3", std::to_string(input.query.bounds.northEast().latitude));
      post_data.replace("%4", std::to_string(input.query.bounds.northEast().longitude));
      std::cout << post_data << std::endl;

      data.query.post_data = post_data;
      data.query.header_fields = { { "Content-Type", "application/json" }, };
      data.query.bounds = input.query.bounds;
      break;
    }
    case Parser::BuildQuery | Parser::Station:
      data.query.parser = Parser::Station;
      if(!input.station.station_id)
        throw __LINE__;
      data.query.URL = "https://account.echargenetwork.com/api/network/stations";
      data.query.post_data = "[" + *data.query.node_id + "]";


      break;
  }
  return data;
}

std::vector<pair_data_t> EchargeScraper::Parse(const pair_data_t& data, const std::string& input) const
{
  try
  {
    switch(data.query.parser)
    {
      default: throw std::string(__FILE__).append(": unknown parser: ").append(std::to_string(int(data.query.parser)));
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
  return {};
}

/*
[
  {
    "Id": "616491cd-fac7-407f-8c79-81720d04c560",
    "Ids": [
      "616491cd-fac7-407f-8c79-81720d04c560"
    ],
    "ParkName": "Watson Lake, Northern Lights Centre",
    "Status": 1,
    "LatLng": {
      "Lat": 60.061964,
      "Lng": -128.711131
    },
    "Count": 1,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 1,
    "IsCluster": false
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 45.50424,
      "Lng": -74.24451
    },
    "Count": 6949,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 0,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 49.04333,
      "Lng": -123.04202
    },
    "Count": 1039,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 0,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 43.49163,
      "Lng": -80.81281
    },
    "Count": 402,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 0,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 50.89871,
      "Lng": -115.84187
    },
    "Count": 424,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 1,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 47.29085,
      "Lng": -65.87851
    },
    "Count": 648,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 0,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 34.10863,
      "Lng": -118.38507
    },
    "Count": 370,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 6,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 49.84242,
      "Lng": -97.68399
    },
    "Count": 47,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 1,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 39.26754,
      "Lng": -84.52797
    },
    "Count": 255,
    "AvailableCount": null,
    "Level": 2,
    "NetworkId": 6,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 54.57826,
      "Lng": -125.85835
    },
    "Count": 20,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 1,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 40.7589,
      "Lng": -74.25945
    },
    "Count": 216,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 6,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 47.54851,
      "Lng": -52.75565
    },
    "Count": 16,
    "AvailableCount": null,
    "Level": 2,
    "NetworkId": 1,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 51.24423,
      "Lng": -105.4752
    },
    "Count": 49,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 1,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 48.42319,
      "Lng": -88.26187
    },
    "Count": 16,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 1,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 63.77255,
      "Lng": -146.82634
    },
    "Count": 5,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 0,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 38.27907,
      "Lng": -122.75596
    },
    "Count": 26,
    "AvailableCount": null,
    "Level": 2,
    "NetworkId": 6,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 56.29566,
      "Lng": -117.18079
    },
    "Count": 6,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 1,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 61.41289,
      "Lng": -135.50701
    },
    "Count": 21,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 1,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 44.7233,
      "Lng": -90.72668
    },
    "Count": 12,
    "AvailableCount": null,
    "Level": 2,
    "NetworkId": 6,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": [
      "a80d5c6f-ed8a-4fa4-b17a-0181197f728e",
      "52d3a6ea-4cde-4d39-9771-94e35b50a9e6",
      "aa73fb17-5c0b-4a63-bfb8-d1720b5200c9"
    ],
    "ParkName": "Columbia REA",
    "Status": 1,
    "LatLng": {
      "Lat": 46.08103,
      "Lng": -118.28439
    },
    "Count": 3,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 6,
    "IsCluster": false
  },
  {
    "Id": null,
    "Ids": [
      "e2a44387-a593-461d-a65b-a7827b9a26f4",
      "5789f1ef-1bea-4e18-b9e8-c4da412e6d4f"
    ],
    "ParkName": "Greenstone Building",
    "Status": 0,
    "LatLng": {
      "Lat": 62.45283,
      "Lng": -114.37233
    },
    "Count": 2,
    "AvailableCount": null,
    "Level": 2,
    "NetworkId": 1,
    "IsCluster": false
  },
  {
    "Id": null,
    "Ids": [
      "d0f70ca2-c9a9-473c-ad71-1e3172a54370",
      "5438070b-9ab1-47c7-ac9a-57db621296b0"
    ],
    "ParkName": "AddÉnergie - Laboratoire",
    "Status": 0,
    "LatLng": {
      "Lat": -180,
      "Lng": -180
    },
    "Count": 2,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 1,
    "IsCluster": false
  }
]


[
  {
    "Id": "bfe7c168-e049-4eae-80dc-a283d9b2453c",
    "Ids": [
      "bfe7c168-e049-4eae-80dc-a283d9b2453c"
    ],
    "ParkName": "CA Dealership - Fresno Buick GMC",
    "Status": 1,
    "LatLng": {
      "Lat": 36.81892,
      "Lng": -119.79229
    },
    "Count": 1,
    "AvailableCount": null,
    "Level": 3,
    "NetworkId": 6,
    "IsCluster": false
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 37.40324,
      "Lng": -122.10907
    },
    "Count": 13,
    "AvailableCount": null,
    "Level": 2,
    "NetworkId": 6,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 34.1025,
      "Lng": -118.39682
    },
    "Count": 366,
    "AvailableCount": null,
    "Level": 2,
    "NetworkId": 6,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": null,
    "ParkName": "",
    "Status": 1,
    "LatLng": {
      "Lat": 39.15491,
      "Lng": -123.40286
    },
    "Count": 13,
    "AvailableCount": null,
    "Level": 2,
    "NetworkId": 6,
    "IsCluster": true
  },
  {
    "Id": null,
    "Ids": [
      "4eafac88-9764-48ec-9fb4-e0e0c26f74c3",
      "0495da10-c2f4-4625-8e43-406b4113c31e",
      "9863f372-ca4f-4786-b239-61b9e79c0819"
    ],
    "ParkName": "Azure Palm Hot Springs Resort & Day Spa Oasis",
    "Status": 1,
    "LatLng": {
      "Lat": 33.95377,
      "Lng": -116.4823
    },
    "Count": 3,
    "AvailableCount": null,
    "Level": 2,
    "NetworkId": 6,
    "IsCluster": false
  }
]
*/
std::vector<pair_data_t> EchargeScraper::ParseMapArea(const std::string& input) const
{
  return {};
}

std::vector<pair_data_t> EchargeScraper::ParseStation([[maybe_unused]] const pair_data_t& data, const std::string& input) const
{
  std::map<std::string, pair_data_t> stations;
  auto stationData = [&stations](const safenode_t& node, int line) -> pair_data_t&
  {
    if(node.type != shortjson::Field::Object)
      throw line;
    return stations[node.identifier];
  };

  std::optional<std::string> tmpstr;
  safenode_t root = shortjson::Parse(input);

  if(root.type != shortjson::Field::Object)
    throw __LINE__;
  for(const auto& nodeL0 : root.safeObject())
  {
    if(nodeL0.idObject("tariffs"))
    {
      for(const auto& nodeL1 : nodeL0.safeObject())
      {
        auto& nd = stationData(nodeL1, __LINE__);
        for(const auto& nodeL2 : nodeL1.safeObject())
        {
          if(nodeL2.identifier == "Id")
          {
            if(nodeL2.type != shortjson::Field::String ||
               nodeL2.toString() != nodeL1.identifier)
              throw __LINE__;
          }
          else if(nodeL2.idString("Currency", tmpstr))
          {
            throw __LINE__;
            //nd.station.price.currency;
          }
          else if(nodeL2.idString("InfoUrl", nd.station.contact.URL))
          {
            if(nd.station.contact.URL && nd.station.contact.URL->empty())
              nd.station.contact.URL.reset();
          }
          else if(nodeL2.idObject("LocalizedDescriptions"))
          {
            for(const auto& nodeL3 : nodeL2.safeObject())
              nodeL3.idString("en", nd.station.price.text);
          }
        }
      }
    }
    else if(nodeL0.idObject("ports"))
    {
      for(const auto& nodeL1 : nodeL0.safeObject())
      {
        auto nd = stationData(nodeL1, __LINE__);
        port_t port;
        for(const auto& nodeL2 : nodeL1.safeObject())
        {
          if(nodeL2.identifier == "Id" ||
             nodeL2.identifier == "SiteId" ||
             nodeL2.identifier == "ChargingStationId")
          {
            if(nodeL2.type != shortjson::Field::String ||
               nodeL2.toString() != nodeL1.identifier)
              throw __LINE__;
          }
          else if(nodeL2.idString("State", tmpstr))
          {
            if(tmpstr == "Available")
              port.status = Status::Operational;
          }
          else if(nodeL2.idArray("Connectors"))
          {
            if(nodeL2.safeArray().size() != 1)
              throw __LINE__;
            for(const auto& nodeL3 : nodeL2.safeArray().front().safeObject())
            {
              if(nodeL3.idString("Type", tmpstr))
              {
                if(tmpstr == "IEC_62196_T1")
                  port.power.connector = Connector::J1772;
                else
                  throw __LINE__;
              }
              else if(int32_t int_val = 0; nodeL3.idInteger("Power", int_val))
              {
                port.power.kw = double(int_val) / 1000;
                *port.power.kw /= 1000;
                if(port.power.kw < 10.0)
                  port.power.level = 1;
                else if(port.power.kw < 50.0)
                  port.power.level = 2;
                else
                  port.power.level = 3;
              }
            }
          }
        }
        nd.station.ports.emplace_back(port);
      }
    }
    else if(nodeL0.idObject("chargingStations"))
    {
      for(const auto& nodeL1 : nodeL0.safeObject())
      {
        auto& nd = stationData(nodeL1, __LINE__);
        for(const auto& nodeL2 : nodeL1.safeObject())
        {
          if(nodeL2.idString("Id", nd.station.ports.front().port_id) ||
             nodeL2.idString("Name", nd.station.ports.front().display_name))
          {
          }
          else if(nodeL2.idString("SiteId", tmpstr))
          {
            if(tmpstr != nodeL1.identifier)
              throw __LINE__;
          }
          else if (nodeL2.idObject("Coordinates"))
          {
            for(const auto& nodeL3 : nodeL2.safeObject())
            {
              nodeL3.idFloat("Latitude", nd.station.location.latitude) ||
              nodeL3.idFloat("Longitude", nd.station.location.longitude);
            }
          }
          else if(nodeL0.idObject("Address"))
          {
            for(const safenode_t& nodeL1 : nodeL0.safeObject())
            {
              if(nodeL1.idString("City", nd.station.contact.city) ||
                 nodeL1.idString("Country", nd.station.contact.country) ||
                 nodeL1.idString("PostalCode", nd.station.contact.postal_code) ||
                 nodeL1.idString("Province", nd.station.contact.state) ||
                 nodeL1.idStreet("Line1", nd.station.contact.street_number, nd.station.contact.street_name))
              {
              }
              else if(nodeL1.identifier == "Line2")
              {
                if(nodeL1.idString("Line2", tmpstr))
                  std::cout << "Line2: " << *tmpstr << std::endl;
              }
              else
                throw __LINE__;
            }
          }
        }
      }
    }
    else if(nodeL0.idObject("sites"))
    {
      for(const auto& nodeL1 : nodeL0.safeObject())
      {
        auto& nd = stationData(nodeL1, __LINE__);
        for(const auto& nodeL2 : nodeL1.safeObject())
        {
          if(nodeL2.identifier == "Id")
          {
            if(nodeL2.type != shortjson::Field::String ||
               nodeL2.toString() != nodeL1.identifier)
              throw __LINE__;
          }
          else if(nodeL2.idString("Name", nd.station.name) ||
             nodeL2.idEnum("NetworkId", nd.station.network_id))
          {
          }
        }
      }
    }
    else
      throw __LINE__;
  }


  std::vector<pair_data_t> return_data;
  for (auto& [k, v] : stations)
    return_data.emplace_back(std::move(v));

  return return_data;
}
