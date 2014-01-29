// Define our global variables
var GoogleMap     = null;
var Planes        = {};
var PlanesOnMap   = 0;
var PlanesOnTable = 0;
var PlanesToReap  = 0;
var SelectedPlane = null;
var SpecialSquawk = false;

var iSortCol=-1;
var bSortASC=true;
var bDefaultSortASC=true;
var iDefaultSortCol=3;
var Azimuths=[]; /* Azimuth paths */
var tabSin=[]; /* Lookup array [0-360] for Sin(degrees) */
var tabCos=[]; /* as above. */
var latPerNm=1.0/60.0;  /* Approx translation from Nm to delta latitude: Historically 1Nm=1arcMin. This is good enough for guesses */
var lonPerNm; /* set later, and updated on changes to CenterLat */
var wait=0;
var waitsecs=10; // Initial query frequency
var oldwait=1; // Default query frequency, once page is loaded, assuming no user settings.
var hidden, visibilityChange;
var traillen=10000; // How many trail pointes to ask for - initially all.
var form=null; // HTML form element
var trailDisplay='clean';
var trailRemember=true;
////////////////////////////////////////////////////////////
// 
// Alter the update frequency.  Reduces load on server and client machines
//

// Manual control
function setfreq(n) {
    waitsecs=n;
}

//Automatic update control:
// If the page is hidden, don't update so often.
// if the page is shown, restore update frequency
function handleVisibilityChange() {
  if (document[hidden]) {
    oldwait=waitsecs;
    if (waitsecs<45) {
	waitsecs=45;
    }
  } else {
    waitsecs=oldwait;
  }
}

// Cross-browser shim so we can notice when the page is displayed or not.

if (typeof document.hidden !== "undefined") { // Opera 12.10 and Firefox 18 and later support 
  hidden = "hidden";
  visibilityChange = "visibilitychange";
} else if (typeof document.mozHidden !== "undefined") {
  hidden = "mozHidden";
  visibilityChange = "mozvisibilitychange";
} else if (typeof document.msHidden !== "undefined") {
  hidden = "msHidden";
  visibilityChange = "msvisibilitychange";
} else if (typeof document.webkitHidden !== "undefined") {
  hidden = "webkitHidden";
  visibilityChange = "webkitvisibilitychange";
}


// Handle page visibility change   
document.addEventListener(visibilityChange, handleVisibilityChange, false);



// Get current map settings
CenterLat = Number(localStorage['CenterLat']) || CONST_CENTERLAT;
CenterLon = Number(localStorage['CenterLon']) || CONST_CENTERLON;
ZoomLvl   = Number(localStorage['ZoomLvl']) || CONST_ZOOMLVL;

lonPerNm=latPerNm/Math.sin(CenterLat*3.14159/180) ; /* Very rough translation from Nm to delta longitude.  */

///////////////////////////////////////////////////////////////
//
// Overlay of aziumuth plot (maximum range in different directions).
// Allow several to be plotted (and deleted) so that comparisons can be made
//
// First the list control:
function updateAziList() {
    var div=document.getElementById("aziplotlist");
    var fmt="[<span onclick=\"clear_azi(_SEQ_);\">Remove azimuth plot _TITLE_</span>]<br>";
    var html="";
    for(var i=Azimuths.length-1; i>=0; i--) {
    	var fragment=fmt.replace(/_SEQ_/g,i);
    	html += fragment.replace(/_TITLE_/g,Azimuths[i].title);
    }
    div.innerHTML=html;
}

function clear_azi(n) {
    if (n>=Azimuths.length) {
	    n=Azimuths.length-1;
    }
    Azimuths[n].line.setMap(null);
    Azimuths.splice(n,1);
    updateAziList();
}

//Callback from JSON call.
//TODO: alter here and server to include historical plots to be loaded 
function newazi(data) {
    var AziCoordinates = [];
    for (var j=0;j<data.length;j++) {
	var lat,lon;
	lat=data[j][0];
	lon=data[j][1];
	AziCoordinates.push(new google.maps.LatLng(lat,lon)); 
    }
    Azimuths.push( {
    	line: new google.maps.Polyline({ path: AziCoordinates, strokeColor: "#007f00", strokeOpacity: 1.0, strokeWeight: 1 }),
	title: new Date().toString(),
	});
    Azimuths[Azimuths.length-1].line.setMap(GoogleMap);
    updateAziList();
}

function show_azi() {
    $.getJSON('/azi.json',function(data) {newazi(data)} );
}



////////////////////////////////////////////////////
//
// Manual hide / clean up of tracks from vanished and active planes
//


function hide_tracks() {
    if (deadPaths.length) {
	for(var i=deadPaths.length-1; i>=0; i--) {
	    deadPaths[i].setMap(null);
	}
    }
}

function cleanup_tracks() {
    if (deadPaths.length) {
	for(var i=deadPaths.length-1; i>=0; i--) {
	    deadPaths[i].setMap(null);
	    deadPaths[i]=null;
	}
    }
    deadPaths.length=0;
    for(var p in Planes) {
	if (Planes[p].line) {
	    Planes[p].line.setMap(null);
	}
    }

}

// What trail data do we want to fetch from ffetchData?
function getFetchOptions() {
	var arg="";
	if (!form) {
			form=document.getElementById("option_form");
	}
	var eles=form.elements;
	var mode;
	mode= eles['trailmode'].value;
	if (mode=="off") {
		arg="";
		traillen=10000;
	} else if (mode=="selected") {
		if (typeof SelectedPlane !== 'undefined' && SelectedPlane != "ICAO" && SelectedPlane != null) {
			arg=SelectedPlane;
			if (traillen<900)  {
				arg="?" + traillen+","+SelectedPlane;
			}
			traillen=2+wait; // Enough positions to hopefully avoid gaps, but not silly quantities of useless data
		} 
	} else { //All planes
		arg="?*";
		if (traillen<900)  {
			arg="?" + traillen+",*";
		}
		traillen=2+wait;
	}
	return(arg);
}

function fetchData() {
	var url='/dump1090/data.json' + getFetchOptions();
	//console.log(url);
	$.getJSON(url, function(data) {
		PlanesOnMap = 0
		SpecialSquawk = false;
		var now=new Date().getTime();
		
		// Loop through all the planes in the data packet
		for (var j=0; j < data.length; j++) {
			// Do we already have this plane object in Planes?
			// If not make it.
			if (Planes[data[j].hex]) {
				var plane = Planes[data[j].hex];
			} else {
				var plane = jQuery.extend(true, {}, planeObject);
			}
			
			/* For special squawk tests
			if (data[j].hex == '48413x') {
            	data[j].squawk = '7700';
            } //*/
            
            // Set SpecialSquawk-value
            if (data[j].squawk == '7500' || data[j].squawk == '7600' || data[j].squawk == '7700') {
                SpecialSquawk = true;
            }

			// Call the function update
			plane.funcUpdateData(data[j],now);
			
			// Copy the plane into Planes
			Planes[plane.icao] = plane;
		}

		PlanesOnTable = data.length;
	});
}

// Initalizes the map and starts up our timers to call various functions
function initialize() {
	// Make a list of all the available map IDs
	var mapTypeIds = [];
	for(var type in google.maps.MapTypeId) {
		mapTypeIds.push(google.maps.MapTypeId[type]);
	}
	// Push OSM on to the end
	mapTypeIds.push("OSM");
	mapTypeIds.push("dark_map");

	// Styled Map to outline airports and highways
	var styles = [
		{
			"featureType": "administrative",
			"stylers": [
				{ "visibility": "off" }
			]
		},{
			"featureType": "landscape",
			"stylers": [
				{ "visibility": "off" }
			]
		},{
			"featureType": "poi",
			"stylers": [
				{ "visibility": "off" }
			]
		},{
			"featureType": "road",
			"stylers": [
				{ "visibility": "off" }
			]
		},{
			"featureType": "transit",
			"stylers": [
				{ "visibility": "off" }
			]
		},{
			"featureType": "landscape",
			"stylers": [
				{ "visibility": "on" },
				{ "weight": 8 },
				{ "color": "#000000" }
			]
		},{
			"featureType": "water",
			"stylers": [
			{ "lightness": -74 }
			]
		},{
			"featureType": "transit.station.airport",
			"stylers": [
				{ "visibility": "on" },
				{ "weight": 8 },
				{ "invert_lightness": true },
				{ "lightness": 27 }
			]
		},{
			"featureType": "road.highway",
			"stylers": [
				{ "visibility": "simplified" },
				{ "invert_lightness": true },
				{ "gamma": 0.3 }
			]
		},{
			"featureType": "road",
			"elementType": "labels",
			"stylers": [
				{ "visibility": "off" }
			]
		}
	]

	// Add our styled map
	var styledMap = new google.maps.StyledMapType(styles, {name: "Dark Map"});

	// Define the Google Map
	var mapOptions = {
		center: new google.maps.LatLng(CenterLat, CenterLon),
		zoom: ZoomLvl,
		mapTypeId: google.maps.MapTypeId.ROADMAP,
		mapTypeControlOptions: {
			mapTypeIds: mapTypeIds
		}
	};

	GoogleMap = new google.maps.Map(document.getElementById("map_canvas"), mapOptions);

	//Define OSM map type pointing at the OpenStreetMap tile server
	GoogleMap.mapTypes.set("OSM", new google.maps.ImageMapType({
		getTileUrl: function(coord, zoom) {
			return "http://tile.openstreetmap.org/" + zoom + "/" + coord.x + "/" + coord.y + ".png";
		},
		tileSize: new google.maps.Size(256, 256),
		name: "OpenStreetMap",
		maxZoom: 18
	}));

	GoogleMap.mapTypes.set("dark_map", styledMap);
	
	// Listeners for newly created Map
    google.maps.event.addListener(GoogleMap, 'center_changed', function() {
        localStorage['CenterLat'] = GoogleMap.getCenter().lat();
        localStorage['CenterLon'] = GoogleMap.getCenter().lng();
		lonPerNm=latPerNm/Math.sin(CenterLat*3.14159/180) ; /* Very rough translation from Nm to delta longitude.  */
    });
    
    google.maps.event.addListener(GoogleMap, 'zoom_changed', function() {
        localStorage['ZoomLvl']  = GoogleMap.getZoom();
    }); 
	
	// Add home marker if requested
	if (SiteShow && (typeof SiteLat !==  'undefined' || typeof SiteLon !==  'undefined')) {
	    var siteMarker  = new google.maps.LatLng(SiteLat, SiteLon);
	    var markerImage = new google.maps.MarkerImage(
	        'http://maps.google.com/mapfiles/kml/pal4/icon57.png',
            new google.maps.Size(32, 32),   // Image size
            new google.maps.Point(0, 0),    // Origin point of image
            new google.maps.Point(16, 16)); // Position where marker should point 
	    var marker = new google.maps.Marker({
          position: siteMarker,
          map: GoogleMap,
          icon: markerImage,
          title: 'My Radar Site',
          zIndex: -99999
        });
        
        if (SiteCircles) {
            for (var i=0;i<SiteCirclesDistances.length;i++) {
              drawCircle(marker, SiteCirclesDistances[i]); // in meters
            }
        }
	}

	/* Populate the lookup tables */
	var deg=3.14159/180;
	for(var i=89;--i;) {
		var sinI=Math.sin(i*deg);
		tabSin[180-i]=tabSin[i]=sinI;
		tabSin[360-i]=tabSin[i+180]=-sinI;
		tabCos[90-i]=tabCos[270+i]=sinI;
		tabCos[i+90]=tabCos[270-i]=-sinI;
	}
	/* endpoints */
	tabCos[90]=tabCos[270]=tabSin[0]=tabSin[360]=tabSin[180]=0;
	tabCos[0]=tabCos[360]=tabSin[90]=1;
	tabSin[270]=tabCos[180]-1;

    	funcOptionsChanged(); // check current options. 
	
	// These will run after page is complitely loaded
	$(window).load(function() {
	    $('#dialog-modal').css('display', 'inline'); // Show hidden settings-windows content
	});

	// After an initial page load time, set waitsecs to the normal value
	setTimeout(function() {
	    if (!form) {
		form=document.getElementById("option_form");
	    }
	    if ( +form.elements['updfreq'].value) {
		oldwait= +form.elements['updfreq'].value;
	    }
	    waitsecs=oldwait;
	},5000);

	// Load up our options page
	optionsInitalize();

	// Did our crafty user need some setup?
	extendedInitalize();
	
	// Setup our timer to poll from the server.
	window.setInterval(function() {
	    wait++;
	    if (wait >= waitsecs) {
		fetchData();
		wait=0;
		refreshTableInfo();
		refreshSelected();
		reaper();
		extendedPulse();
	    }
	}, 1000);
}

// This looks for planes to reap out of the master Planes variable
function reaper() {
	PlanesToReap = 0;
	// When did the reaper start?
	var reaptime = new Date().getTime();
	// Loop the planes
	for (var reap in Planes) {
		// Is this plane possibly reapable?
		if (Planes[reap].reapable == true) {
			// Has it not been seen for 5 minutes?
			// This way we still have it if it returns before then
			// Due to loss of signal or other reasons
			if ((reaptime - Planes[reap].updated) > 300000) {
				// Reap it.
				plane=Planes[reap];
				if (plane.guess) {
					plane.guess.setMap(null);
					plane.guess=null;
				}
				if (plane.line) {
			     	    // Unlikely to reach here with trailDisplay!=showall, but just in case..
				    plane.line.setOptions({strokeColor: "#7f0000", map:(trailDisplay=='showall')?GoogleMap:null});
				    deadPaths.push(plane.line);
				    plane.line=null;
				} else { 
			            if (trailRemember) {
					deadPaths.push(new google.maps.Polyline({
					    strokeColor: '#7f0000',
					    strokeOpacity: 1.0,
					    strokeWeight: 1,
					    map: (trailDisplay=='showall')?GoogleMap:null,
					    path: plane.trackline
					}));
				    }
				}
				delete Planes[reap];
			}
			PlanesToReap++;
		}
	};
} 

// Refresh the detail window about the plane
function refreshSelected() {
    var selected = false;
	if (typeof SelectedPlane !== 'undefined' && SelectedPlane != "ICAO" && SelectedPlane != null && Planes[SelectedPlane]) {
    	selected = Planes[SelectedPlane];
    }
	
	var columns = 2;
	var html = '';
	
	if (selected) {
    	html += '<table id="selectedinfo" width="100%">';
    } else {
        html += '<table id="selectedinfo" class="dim" width="100%">';
    }
	
	// Flight header line including squawk if needed
	if (selected && selected.flight == "") {
	    html += '<tr><td colspan="' + columns + '" id="selectedinfotitle"><b>N/A (' +
	        selected.icao + ')</b>';
	} else if (selected && selected.flight != "") {
	    html += '<tr><td colspan="' + columns + '" id="selectedinfotitle"><b>' +
	        selected.flight + '</b>';
	} else {
	    html += '<tr><td colspan="' + columns + '" id="selectedinfotitle"><b>DUMP1090</b>';
	}
	
	if (selected && selected.squawk == 7500) { // Lets hope we never see this... Aircraft Hijacking
		html += '&nbsp;<span class="squawk7500">&nbsp;Squawking: Aircraft Hijacking&nbsp;</span>';
	} else if (selected && selected.squawk == 7600) { // Radio Failure
		html += '&nbsp;<span class="squawk7600">&nbsp;Squawking: Radio Failure&nbsp;</span>';
	} else if (selected && selected.squawk == 7700) { // General Emergency
		html += '&nbsp;<span class="squawk7700">&nbsp;Squawking: General Emergency&nbsp;</span>';
	} else if (selected && selected.flight != '') {
	    html += '&nbsp;<a href="http://www.flightstats.com/go/FlightStatus/flightStatusByFlight.do?';
        html += 'flightNumber='+selected.flight+'" target="_blank">[FlightStats]</a>';
	}
	html += '<td></tr>';
	
	if (selected) {
	    if (Metric) {
        	html += '<tr><td>Altitude: ' + Math.round(selected.altitude / 3.2828) + ' m</td>';
        } else {
            html += '<tr><td>Altitude: ' + selected.altitude + ' ft</td>';
        }
    } else {
        html += '<tr><td>Altitude: n/a</td>';
    }
		
	if (selected && selected.squawk != '0000') {
		html += '<td>Squawk: ' + selected.squawk + '</td></tr>';
	} else {
	    html += '<td>Squawk: n/a</td></tr>';
	}
	
	html += '<tr><td>Speed: ' 
	if (selected) {
	    if (Metric) {
	        html += Math.round(selected.speed * 1.852) + ' km/h';
	    } else {
	        html += selected.speed + ' kt';
	    }
	} else {
	    html += 'n/a';
	}
	html += '</td>';
	
	if (selected) {
        html += '<td>ICAO (hex): ' + selected.icao + '</td></tr>';
    } else {
        html += '<td>ICAO (hex): n/a</td></tr>'; // Something is wrong if we are here
    }
    
    html += '<tr><td>Track: ' 
	if (selected && selected.vTrack) {
	    html += selected.track + ' (' + normalizeTrack(selected.track, selected.vTrack)[1] +')';
	} else {
	    html += 'n/a';
	}
	html += '</td><td>&nbsp;</td></tr>';

	html += '<tr><td colspan="' + columns + '" align="center">Lat/Long: ';
	if (selected && selected.vPosition) {
	    html += selected.latitude + ', ' + selected.longitude + '</td></tr>';
	    
	    // Let's show some extra data if we have site coordinates
	    if (SiteShow) {
            var siteLatLon  = new google.maps.LatLng(SiteLat, SiteLon);
            var planeLatLon = new google.maps.LatLng(selected.latitude, selected.longitude);
            var dist = google.maps.geometry.spherical.computeDistanceBetween (siteLatLon, planeLatLon);
            
            if (Metric) {
                dist /= 1000;
            } else {
                dist /= 1852;
            }
            dist = (Math.round((dist)*10)/10).toFixed(1);
            html += '<tr><td colspan="' + columns + '">Distance from Site: ' + dist +
                (Metric ? ' km' : ' NM') + '</td></tr>';
        } // End of SiteShow
	} else {
	    if (SiteShow) {
	        html += '<tr><td colspan="' + columns + '">Distance from Site: n/a ' + 
	            (Metric ? ' km' : ' NM') + '</td></tr>';
	    } else {
    	    html += 'n/a</td></tr>';
    	}
	}

	html += '</table>';
	
	document.getElementById('plane_detail').innerHTML = html;
}

// Right now we have no means to validate the speed is good
// Want to return (n/a) when we dont have it
// TODO: Edit C code to add a valid speed flag
// TODO: Edit js code to use said flag
function normalizeSpeed(speed, valid) {
	return speed	
}

// Returns back a long string, short string, and the track if we have a vaild track path
function normalizeTrack(track, valid){
	x = []
	if ((track > -1) && (track < 22.5)) {
		x = ["North", "N", track]
	}
	if ((track > 22.5) && (track < 67.5)) {
		x = ["North East", "NE", track]
	}
	if ((track > 67.5) && (track < 112.5)) {
		x = ["East", "E", track]
	}
	if ((track > 112.5) && (track < 157.5)) {
		x = ["South East", "SE", track]
	}
	if ((track > 157.5) && (track < 202.5)) {
		x = ["South", "S", track]
	}
	if ((track > 202.5) && (track < 247.5)) {
		x = ["South West", "SW", track]
	}
	if ((track > 247.5) && (track < 292.5)) {
		x = ["West", "W", track]
	}
	if ((track > 292.5) && (track < 337.5)) {
		x = ["North West", "NW", track]
	}
	if ((track > 337.5) && (track < 361)) {
		x = ["North", "N", track]
	}
	if (!valid) {
		x = [" ", "n/a", ""]
	}
	return x
}

// Refeshes the larger table of all the planes
function refreshTableInfo() {
	var html = '<table id="tableinfo" width="100%">';
	html += '<thead style="background-color: #BBBBBB; cursor: pointer;">';
	html += '<td onclick="setASC_DESC(\'0\');sortTable(\'tableinfo\',\'0\');">ICAO</td>';
	html += '<td onclick="setASC_DESC(\'1\');sortTable(\'tableinfo\',\'1\');">Flight</td>';
	html += '<td onclick="setASC_DESC(\'2\');sortTable(\'tableinfo\',\'2\');" ' +
	    'align="right">Squawk</td>';
	html += '<td onclick="setASC_DESC(\'3\');sortTable(\'tableinfo\',\'3\');" ' +
	    'align="right">Altitude</td>';
	html += '<td onclick="setASC_DESC(\'4\');sortTable(\'tableinfo\',\'4\');" ' +
	    'align="right">Speed</td>';
	html += '<td onclick="setASC_DESC(\'5\');sortTable(\'tableinfo\',\'5\');" ' +
	    'align="right">Track</td>';
	html += '<td onclick="setASC_DESC(\'6\');sortTable(\'tableinfo\',\'6\');" ' +
	    'align="right">Msgs</td>';
	html += '<td onclick="setASC_DESC(\'7\');sortTable(\'tableinfo\',\'7\');" ' +
	    'align="right">Seen</td>';
	html += '<td onclick="setASC_DESC(\'8\');sortTable(\'tableinfo\',\'7\');" ' +
	    'align="right">Azi</td>';
	html += '<td onclick="setASC_DESC(\'9\');sortTable(\'tableinfo\',\'7\');" ' +
	    'align="right">Ele</td>';
	html += '<td onclick="setASC_DESC(\'10\');sortTable(\'tableinfo\',\'7\');" ' +
	    'align="right">Range</td></thead><tbody>';
	for (var tablep in Planes) {
		var tableplane = Planes[tablep]
		if (!tableplane.reapable) {
			var specialStyle = "";
			// Is this the plane we selected?
			if (tableplane.icao == SelectedPlane) {
				specialStyle += " selected";
			}
			// Lets hope we never see this... Aircraft Hijacking
			if (tableplane.squawk == 7500) {
				specialStyle += " squawk7500";
			}
			// Radio Failure
			if (tableplane.squawk == 7600) {
				specialStyle += " squawk7600";
			}
			// Emergancy
			if (tableplane.squawk == 7700) {
				specialStyle += " squawk7700";
			}
			
			if (tableplane.vPosition == true) {
				html += '<tr class="plane_table_row vPosition' + specialStyle + '">';
			} else {
				html += '<tr class="plane_table_row ' + specialStyle + '">';
		    }
		    
			html += '<td>' + tableplane.icao + '</td>';
			html += '<td>' + tableplane.flight + '</td>';
			if (tableplane.squawk != '0000' ) {
    			html += '<td align="right">' + tableplane.squawk + '</td>';
    	    } else {
    	        html += '<td align="right">&nbsp;</td>';
    	    }
    	    
    	    if (Metric) {
    			html += '<td align="right">' + Math.round(tableplane.altitude / 3.2828) + '</td>';
    			html += '<td align="right">' + Math.round(tableplane.speed * 1.852) + '</td>';
    	    } else {
    	        html += '<td align="right">' + tableplane.altitude + '</td>';
    	        html += '<td align="right">' + tableplane.speed + '</td>';
    	    }
			
			html += '<td align="right">';
			if (tableplane.vTrack) {
    			 html += normalizeTrack(tableplane.track, tableplane.vTrack)[2];
    			 // html += ' (' + normalizeTrack(tableplane.track, tableplane.vTrack)[1] + ')';
    	    } else {
    	        html += '&nbsp;';
    	    }
    	    html += '</td>';
			html += '<td align="right">' + tableplane.messages + '</td>';
			html += '<td align="right">' + tableplane.seen + '</td>';
			html += '<td align="right">' + tableplane.bearing + '</td>';
			html += '<td align="right">' + tableplane.elevation + '</td>';
			html += '<td align="right">' + tableplane.range + '</td>';
			html += '</tr>';
		}
	}
	html += '</tbody></table>';

	document.getElementById('planes_table').innerHTML = html;

	if (SpecialSquawk) {
    	$('#SpecialSquawkWarning').css('display', 'inline');
    } else {
        $('#SpecialSquawkWarning').css('display', 'none');
    }

	// Click event for table
	$('#planes_table').find('tr').click( function(){
		var hex = $(this).find('td:first').text();
		if (hex != "ICAO") {
			selectPlaneByHex(hex);
			refreshTableInfo();
			refreshSelected();
		}
	});

	sortTable("tableinfo");
}

// Credit goes to a co-worker that needed a similar functions for something else
// we get a copy of it free ;)
function setASC_DESC(iCol) {
	if(iSortCol==iCol) {
		bSortASC=!bSortASC;
	} else {
		bSortASC=bDefaultSortASC;
	}
}

function sortTable(szTableID,iCol) { 
	//if iCol was not provided, and iSortCol is not set, assign default value
	if (typeof iCol==='undefined'){
		if(iSortCol!=-1){
			var iCol=iSortCol;
		} else {
			var iCol=iDefaultSortCol;
		}
	}

	//retrieve passed table element
	var oTbl=document.getElementById(szTableID).tBodies[0];
	var aStore=[];

	//If supplied col # is greater than the actual number of cols, set sel col = to last col
	if (typeof oTbl.rows[0] !== 'undefined' && oTbl.rows[0].cells.length <= iCol) {
		iCol=(oTbl.rows[0].cells.length-1);
    }

	//store the col #
	iSortCol=iCol;

	//determine if we are delaing with numerical, or alphanumeric content
	var bNumeric = false;
	if ((typeof oTbl.rows[0] !== 'undefined') &&
	    (!isNaN(parseFloat(oTbl.rows[0].cells[iSortCol].textContent ||
	    oTbl.rows[0].cells[iSortCol].innerText)))) {
	    bNumeric = true;
	}

	//loop through the rows, storing each one inro aStore
	for (var i=0,iLen=oTbl.rows.length;i<iLen;i++){
		var oRow=oTbl.rows[i];
		vColData=bNumeric?parseFloat(oRow.cells[iSortCol].textContent||oRow.cells[iSortCol].innerText):String(oRow.cells[iSortCol].textContent||oRow.cells[iSortCol].innerText);
		aStore.push([vColData,oRow]);
	}

	//sort aStore ASC/DESC based on value of bSortASC
	if (bNumeric) { //numerical sort
		aStore.sort(function(x,y){return bSortASC?x[0]-y[0]:y[0]-x[0];});
	} else { //alpha sort
		aStore.sort();
		if(!bSortASC) {
			aStore.reverse();
	    }
	}

	//rewrite the table rows to the passed table element
	for(var i=0,iLen=aStore.length;i<iLen;i++){
		oTbl.appendChild(aStore[i][1]);
	}
	aStore=null;
}

function funcOptionsChanged() {
    if (!form) {
	    form=document.getElementById("option_form");
    }
    trailDisplay=form.elements['traildisplay'].value;
    if (form.elements['remember']) {
	trailRemember=form.elements['remember'].value;
    } else {
	trailRemember=true;
    }
    if (trailDisplay=='showall') {
	trailRemember=true;
    }
    var cbHTML="<input type=checkbox name=remember value=1 " + (trailRemember?'checked':'') + "> Remember trails for planes that go off scope";
    var div=document.getElementById("extratrailoption");
    if (trailDisplay=='selected')  {
        traillen=10000; /* ask for everything next update */
	div.innerHTML=cbHTML;
	for(var p in Planes) {
	    if (!Planes[p].is_selected) {
		    Planes[p].funcClearLine();
	    } else {
	    	Planes[p].funcShowLine();
	    }
	}
	for(var t=deadPaths.length-1;t>=0; t--) {
	    deadPaths[t].setMap(none);
	}
    } else if (trailDisplay=='clean') { // Only active planes.
	div.innerHTML=cbHTML;
        for(var p in Planes) {
	    if (Planes[p].reapable) {
		Planes[p].funcClearLine();
	    } else {
		Planes[p].funcShowLine();
	    } 
	}
	for(var t=deadPaths.length-1;t>=0; t--) {
	    deadPaths[t].setMap(null);
	}
    } else { // trailDisplay=='showall' : Show all planes AND deadPaths
	div.innerHTML='[<span onclick="hide_tracks()">Hide oldest tracks</span>]<br>[<span onclick="cleanup_tracks()">Clean up inactive tracks</span>]<BR>';
	for(var p in Planes) {
	    if (!Planes[p].line) {
		Planes[p].funcShowLine();	
	    } 
	}
	for(var t=deadPaths.length-1;t>=0; t--) {
	    deadPaths[t].setMap(GoogleMap);
	}
    }
}

function selectPlaneByHex(hex) {
	// If SelectedPlane has something in it, clear out the selected
	if (SelectedPlane != null && Planes[SelectedPlane]) {
		Planes[SelectedPlane].is_selected = false;
		if (form.elements['traildisplay'].value == 'selected') {
			Planes[SelectedPlane].funcClearLine();
		} else {
			Planes[SelectedPlane].funcDeselectLine();
		}
		Planes[SelectedPlane].markerColor = MarkerColor;
		// If the selected has a marker, make it not stand out
		if (Planes[SelectedPlane].marker) {
			Planes[SelectedPlane].marker.setIcon(Planes[SelectedPlane].funcGetIcon());
		}
		if (Planes[SelectedPlane].guess) {
			Planes[SelectedPlane].guess.setIcon(Planes[SelectedPlane].getIconForGuess());
		}
	}

	// If we are clicking the same plane, we are deselected it.
	if (String(SelectedPlane) != String(hex)) {
		// Assign the new selected
		SelectedPlane = hex;
		Planes[SelectedPlane].is_selected = true;
		// If the selected has a marker, make it stand out
		if (Planes[SelectedPlane].marker) {
			Planes[SelectedPlane].funcUpdateLines();
			Planes[SelectedPlane].marker.setIcon(Planes[SelectedPlane].funcGetIcon());
		}
		if (Planes[SelectedPlane].guess) {
			Planes[SelectedPlane].guess.setIcon(Planes[SelectedPlane].getIconForGuess());
		}
	} else { 
		SelectedPlane = null;
	}
    refreshSelected();
    refreshTableInfo();
}

function resetMap() {
    // Reset localStorage values
    localStorage['CenterLat'] = CONST_CENTERLAT;
    localStorage['CenterLon'] = CONST_CENTERLON;
    localStorage['ZoomLvl']   = CONST_ZOOMLVL;
    
    // Try to read values from localStorage else use CONST_s
    CenterLat = Number(localStorage['CenterLat']) || CONST_CENTERLAT;
    CenterLon = Number(localStorage['CenterLon']) || CONST_CENTERLON;
    ZoomLvl   = Number(localStorage['ZoomLvl']) || CONST_ZOOMLVL;
    lonPerNm=latPerNm/Math.sin(CenterLat*3.14159/180) ; /* Very rough translation from Nm to delta longitude.  */
    
    // Set and refresh
	GoogleMap.setZoom(parseInt(ZoomLvl));
	GoogleMap.setCenter(new google.maps.LatLng(parseFloat(CenterLat), parseFloat(CenterLon)));
	
	if (SelectedPlane) {
	    selectPlaneByHex(SelectedPlane);
	}

	refreshSelected();
	refreshTableInfo();
}

function drawCircle(marker, distance) {
    if (typeof distance === 'undefined') {
        return false;
        
        if (!(!isNaN(parseFloat(distance)) && isFinite(distance)) || distance < 0) {
            return false;
        }
    }
    
    distance *= 1000.0;
    if (!Metric) {
        distance *= 1.852;
    }
    
    // Add circle overlay and bind to marker
    var circle = new google.maps.Circle({
      map: GoogleMap,
      radius: distance, // In meters
      fillOpacity: 0.0,
      strokeWeight: 1,
      strokeOpacity: 0.3
    });
    circle.bindTo('center', marker, 'position');
}
