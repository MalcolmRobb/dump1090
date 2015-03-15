// --------------------------------------------------------
//
// This file is to configure the configurable settings.
// Load this file before script.js file at gmap.html.
//
// --------------------------------------------------------

// -- Output Settings -------------------------------------
// Show metric values
Metric = false; // true or false

// -- Map settings ----------------------------------------
// The Latitude and Longitude in decimal format
CONST_CENTERLAT = 30.384;
CONST_CENTERLON = 262.293;
// The google maps zoom level, 0 - 16, lower is further out
CONST_ZOOMLVL   = 9;
StandardMapType = 'OSM2'; // mapType[google.maps.MapTypeId.ROADMAP|google.maps.MapTypeId.TERRAIN|google.maps.MapTypeId.SATELLITE|'OSM'|'OSM2'|'OAIP'|'dark_map']. google mapTypeIDs without ' ' eg. google.maps.MapTypeId.ROADMAP

// -- Marker settings -------------------------------------
// The default marker color
MarkerColor	  = "rgb(0, 255, 0)";
SelectedColor = "rgb(225, 225, 225)";
StaleColor = "rgb(255, 0, 0)";

// -- flight info  look up Settings -----------------------
flightawareshow = true;
fr24show = false;
flightstatsshow = false;

// -- Site Settings ---------------------------------------
SiteShow    = true; // true or false

// The Latitude and Longitude in decimal format
SiteLat     = 30.384;
SiteLon     = 262.293;
SiteCircles = true; // true or false (Only shown if SiteShow is true)
// In nautical miles or km (depending settings value 'Metric')
SiteCirclesDistances = new Array(5,10,15,20,25,30,35,40,45,50,55,60);

// Extended Airplane Marker
airplanemarker_showextended = true;
airplanemarker_showtext     = true;
