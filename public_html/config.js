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
CONST_CENTERLAT = 45.0;
CONST_CENTERLON = 9.0;
// The google maps zoom level, 0 - 16, lower is further out
CONST_ZOOMLVL   = 5;
StandardMapType = 'OSM2'; // mapType[google.maps.MapTypeId.ROADMAP|google.maps.MapTypeId.TERRAIN|google.maps.MapTypeId.SATELLITE|'OSM'|'OSM2'|'OAIP'|'dark_map']. google mapTypeIDs without ' ' eg. google.maps.MapTypeId.ROADMAP

// -- Marker settings -------------------------------------
// The default marker color
MarkerColor	  = "rgb(127, 127, 127)";
SelectedColor = "rgb(225, 225, 225)";
StaleColor = "rgb(190, 190, 190)";

// -- Site Settings ---------------------------------------
SiteShow    = false; // true or false
// The Latitude and Longitude in decimal format
SiteLat     = 45.0;
SiteLon     = 9.0;

SiteCircles = true; // true or false (Only shown if SiteShow is true)
// In nautical miles or km (depending settings value 'Metric')
SiteCirclesDistances = new Array(50,100,150,200);

// Extended Airplane Marker
airplanemarker_showextended = true;
airplanemarker_showtext     = true;
