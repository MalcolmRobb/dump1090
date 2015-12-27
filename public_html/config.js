// --------------------------------------------------------
//
// This file is to configure the configurable settings.
// Load this file before script.js file at gmap.html.
//
// --------------------------------------------------------

// -- Output Settings -------------------------------------
// Show metric values
Metric = false; // true or false (Boolean)

// -- Map settings ----------------------------------------
// The default Latitude and Longitude in decimal format
CONST_CENTERLAT = 45.0;
CONST_CENTERLON = 9.0;
// The google maps zoom level, 0 - 16, lower is further out
CONST_ZOOMLVL   = 5;

// -- Marker settings -------------------------------------
// The default marker color
MarkerColor	  = "rgb(127, 127, 127)";
SelectedColor = "rgb(225, 225, 225)";
StaleColor = "rgb(190, 190, 190)";

// -- Site Settings ---------------------------------------
// If SiteFromLoc is true, a call will be made to the server
// to fetch any location settings passed to it on startup in
// the form of --lat and --lon, or --home (which can be a
// free text string which will use Google Maps to resolve to
// a known location).  If a location is successfully identified
// from the server's data, SiteShow will automatically be set
// to "true" as well, and SiteLat & SiteLon will be updated
// to the identified coordinates.
SiteFromLoc = true;

SiteShow    = false; // Boolean
// The Latitude and Longitude in decimal format (use defaults)
SiteLat     = CONST_CENTERLAT;
SiteLon     = CONST_CENTERLON;

SiteCircles = true; // true or false (Only shown if SiteShow is true)
// In nautical miles or km (depending settings value 'Metric')
SiteCirclesDistances = new Array(100,150,200);
SiteSweep   = true; // Draw a line from the Site to the selected Plane
SweepColor  = 'rgb(0, 255, 255)'; // ...in this color

// If true, turns projections on.  Projections are a line in front
// of a plane based on its last known heading and speed, set
// ProjectionDist seconds in the future.  This allows for interpolation
// between positional updates, and in practice has proven remarkably
// accurate, though tight turns with infrequent updates will be off.
// Note: Does not account for acceleration, deceleration, or curved
// paths.
PlaneProjection = true;
ProjectionColor = 'rgb(0, 0, 255)'; // Color of the projection line
ProjectionDist  = 60; // Time in seconds

// Shows a smaller, lighter plane icon (derived from the plane's icon
// setting, whatever that may be) moving along the projection line at
// a rate consistent with the last known speed.
ProjectionAnimation = true;
