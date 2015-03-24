// --------------------------------------------------------
//
// This file is to configure the configurable settings.
//settings.js houses alot of the functions for this script
// Load this file before script.js file at gmap.html.
//defalt settings can change in settings page
function set_reset(test) {
    storevalue('flightawareshow','true');//can change on settings page link to flght info 
    storevalue('fr24show','true');//can change on settings page  page link to flght info 
    storevalue('flightstatsshow','true');//can change on settings page  page link to flght info 
    storevalue('maplat','30.384');//defalt value for map lat can change on settings page 
    storevalue('maplon','262.300');//defalt value for map lon can change on settings page 
    storevalue('sitelat','30.384');//defalt value for site lat can change on settings page 
    storevalue('sitelon','262.294');//defalt value for site lon can change on settings page 
    storevalue('1r','0');//active planes red of rgb can change on settings page 
    storevalue('1g','255');//active planes green in rgb can change on settings page 
    storevalue('1b','0');//active planes blue in rgb can change on settings page 
    storevalue('2r','30');//slected planes red in rgb   can change on settings page     
    storevalue('2g','30');//slected planes green in rgb can change on settings page 
    storevalue('2b','30');//slected planes blue in rgb can change on settings page  
    storevalue('3r','255');//non active planes red in rgb can change on settings page 
    storevalue('3g','0');//non active planes green in rgb can change on settings page 
    storevalue('3b','0');//non active planes blue in rgb can change on settings page 
    storevalue('setr','0');//truecolor red of rgb can change on settings page 
    storevalue('setg','255');//truecolor green in rgb can change on settings page 
    storevalue('setb','0');//truecolor blue in rgb can change on settings page 
    storevalue('set2r','255');//falsecolor red in rgb can change on settings page 
    storevalue('set2g','0');//falsecolor green in rgb can change on settings page 
    storevalue('set2b','0');//falsecolor blue in rgb can change on settings page 
    storevalue('metric','');//can change on settings page
    storevalue('showsite','true');//can change on settings page
    storevalue('showcircles','true');//can change on settings page
    storevalue('airplanemarker_showextended','true');//can change on settings page
    storevalue('airplanemarker_showtext','true');//can change on settings page
    storevalue('zoom',9);//can change on settings page
    storevalue('darkmode','true');//can change on settings pag
    storevalue('coolclock','true');//can change on settings page
    storevalue('normalclock','');//can change on settings pag//can change on settings page
    storevalue('planedata','');
    return true;
    
}
function reset(){
  set_reset();
  alert("reset was done");
  location.reload();
  
}

// --------------------------------------------------------
// -- Output Settings -------------------------------------
// Show metric values
Metric = getvalue('metic'); // true or false in settings page defalt here

// -- Map settings ----------------------------------------
// The Latitude and Longitude in decimal format
if( getvalue('maplat') !== null && getvalue('maplon')!== null) {
   CONST_CENTERLAT = getvalue('maplat');//map latitude
   CONST_CENTERLON = getvalue('maplon');//map longitude
 }else{
   set_reset();
   location.reload();
 }
CONST_ZOOMLVL   = 9;//map zoom
StandardMapType = 'OSM2'; // mapType[google.maps.MapTypeId.ROADMAP|google.maps.MapTypeId.TERRAIN|google.maps.MapTypeId.SATELLITE|'OSM'|'OSM2'|'OAIP'|'dark_map']. google mapTypeIDs without ' ' eg. google.maps.MapTypeId.ROADMAP

// -- Marker settings -------------------------------------
// The default marker color all in settings page defalt goes here
mR = getvalue('1r');//marker red
mG = getvalue('1g');//marker green
mB = getvalue('1b');//marker blue
seR = getvalue('2r');//slected red
seG = getvalue('2g');//slected green
seB = getvalue('2b');//slected blue
stR = getvalue('3r');//stale red
stG = getvalue('3g');//stale green
stB = getvalue('3b');//stale blue
MarkerColor	= 'rgb('+mR+','+mG+','+mB+')';
SelectedColor	= 'rgb('+seR+','+seG+','+seB+')';
StaleColor	= 'rgb('+stR+','+stG+','+stB+')';

// -- flight info  look up Settings -----------------------

flightawareshow = getvalue('flightawareshow');
fr24show = getvalue('fr24show');
flightstatsshow = getvalue('flightstatsshow');

//settings settings
darkmode = getvalue('darkmode');

// -- Site Settings ---------------------------------------
SiteShow    = getvalue('showsite'); 
SiteCircles = getvalue('showcircles');

// The Latitude and Longitude in decimal format
SiteLat     = getvalue('sitelat');
SiteLon     = getvalue('sitelon');

// In nautical miles or km (depending settings value 'Metric')
SiteCirclesDistances = new Array(5,10,15,20,25,30,35,40,45,50,55,60);

// Extended Airplane Marker
airplanemarker_showextended = getvalue('airplanemarker_showextended'); 
airplanemarker_showtext     = getvalue('airplanemarker_showtext'); 

// Settings darkmode
settingdark = getvalue('darksettings');
