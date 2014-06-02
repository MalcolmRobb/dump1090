// Define our global variables
var GoogleMap     = null;
var Planes        = {};
var PlanesOnMap   = 0;
var PlanesOnTable = 0;
var PlanesToReap  = 0;
var SelectedPlane = null;
var SpecialSquawk = false;

var iSortCol=Number(localStorage['iSortCol']) ||-1;
var bSortASC=JSON.parse(localStorage['bSortASC'] || 'true');
var bDefaultSortASC=true;
var iDefaultSortCol=3;

CONST_MAPTYPEID = null;
// Get current map settings
CenterLat = Number(localStorage['CenterLat']) || CONST_CENTERLAT;
CenterLon = Number(localStorage['CenterLon']) || CONST_CENTERLON;
ZoomLvl   = Number(localStorage['ZoomLvl']) || CONST_ZOOMLVL;
ZoomLvl   = Number(localStorage['ZoomLvl']) || CONST_ZOOMLVL;
MapTypeId = localStorage['MapTypeId'] || CONST_MAPTYPEID;

var lastOsmTileUrlFormat = OsmTileUrlFormat;

function fetchData() {
	$.getJSON(AjaxPlanesUrl).done(function(data) {
		PlanesOnMap = 0
		SpecialSquawk = false;

		if (data.length < PlanesOnTable){
			// Some of the planes were not sent and should be made reapable
			for (p in Planes) {
				if (Planes.hasOwnProperty(p) && (!Planes[p].reapable)){
					var contains_plane = false;
					for (var j=0; j < data.length; j++) {
						if (data[j].hex == p){
							contains_plane = true;
							break;
						}
					}
					if (!contains_plane){
						Planes[p].funcUpdateOldPlane();
					}
				}
			}
		}
		
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
			plane.funcUpdateData(data[j]);
			
			// Copy the plane into Planes
			Planes[plane.icao] = plane;
		}

		PlanesOnTable = data.length;
	}).fail(function() {
		for (p in Planes) {
			if (Planes.hasOwnProperty(p) && (!Planes[p].reapable)){
				Planes[p].funcUpdateOldPlane();
			}
		}
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

	//Define OSM map type pointing at an OpenStreetMap tile server
	GoogleMap.mapTypes.set("OSM", new google.maps.ImageMapType({
		getTileUrl: function(coord, zoom) {
			return OsmTileUrlFormat.replace('{x}', coord.x).replace('{y}', coord.y).replace('{z}', zoom);
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
    });
    
    google.maps.event.addListener(GoogleMap, 'zoom_changed', function() {
        localStorage['ZoomLvl']  = GoogleMap.getZoom();
    }); 

	google.maps.event.addListener(GoogleMap, 'maptypeid_changed', function() {
		localStorage['MapTypeId'] = GoogleMap.getMapTypeId();
	});

	// Show selected map type, if any
	if ((MapTypeId !== null) && (MapTypeId in GoogleMap.mapTypes)){
		GoogleMap.setMapTypeId(MapTypeId);
	}
	
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
	
	// These will run after page is completely loaded
	$(window).load(function() {
        $('#dialog-modal').css('display', 'inline'); // Show hidden settings-windows content
    });

	// Load up our options page
	optionsInitalize();

	// Did our crafty user need some setup?
	extendedInitalize();
	
	// Setup our timer to poll from the server.
	window.setInterval(function() {
		fetchData();
		refreshTableInfo();
		refreshMap();
		refreshSelected();
		reaper();
		extendedPulse();
	}, 1000);
}

// This looks for planes to reap out of the master Planes variable
function reaper() {
	PlanesToReap = 0;
	// When did the reaper start?
	reaptime = new Date().getTime();
	// Loop the planes
	for (var reap in Planes) {
		// Is this plane possibly reapable?
		if (Planes[reap].reapable == true) {
			// Has it not been seen for <PlaneDeleteTtl> seconds?
			// This way we still have it if it returns before then
			// Due to loss of signal or other reasons
			if ((reaptime - Planes[reap].updated) > (PlaneDeleteTtl * 1000)) {
				// Reap it.
				delete Planes[reap];
			}
			PlanesToReap++;
		}
	};
} 

// Refresh the detail window about the plane
function refreshSelected() {
    var selected = false;
	if (typeof SelectedPlane !== 'undefined' && SelectedPlane != "ICAO" && SelectedPlane != null) {
    	selected = Planes[SelectedPlane];
    }
	
	var columns = 2;

	var table = $('<table>')
		.prop('id', 'selectedinfo')
		.prop('width', '100%')
		;
	var newRow = function(){
		return $('<tr>').appendTo(table);
	};
	var newField = function(row){
		return $('<td>').appendTo(row);
	};
	if (selected) {
    } else {
		table.addClass('dim');
    }
	
	// Flight header line including squawk if needed
	var field = newField(newRow()).prop('colspan', columns).prop('id', 'selectedinfotitle');
	var boldfaced = $('<b>').appendTo(field);
	if (selected && selected.flight == "") {
		boldfaced.text('N/A (' + selected.icao + ')');
	} else if (selected && selected.flight != "") {
		boldfaced.text(selected.flight);
	} else {
		boldfaced.text('DUMP1090');
	}
	
	field.append(document.createTextNode('\u00a0'));
	if (selected && selected.squawk == 7500) { // Lets hope we never see this... Aircraft Hijacking
		field.append($('<span>').addClass('squawk7500').text('\u00a0Squawking: Aircraft Hijacking\u00a0'));
	} else if (selected && selected.squawk == 7600) { // Radio Failure
		field.append($('<span>').addClass('squawk7600').text('\u00a0Squawking: Radio Failure\u00a0'));
	} else if (selected && selected.squawk == 7700) { // General Emergency
		field.append($('<span>').addClass('squawk7700').text('\u00a0Squawking: General Emergency\u00a0'));
	} else if (selected && selected.flight != '') {
		field.append($('<a>').prop('href', 'http://www.flightstats.com/go/FlightStatus/flightStatusByFlight.do?flightNumber=' + encodeURIComponent(selected.flight)).prop('target', '_blank').text('[FlightStats]'));
	}
	
	var row = newRow();
	var field = newField(row);
	if (selected && (selected.vAltitude === true)) {
	    if (Metric) {
			field.text('Altitude: ' + Math.round(selected.altitude / 3.2828) + ' m');
        } else {
			field.text('Altitude: ' + selected.altitude + ' ft');
        }
	} else if (selected && (selected.vAltitude === '')) {
		field.text('Altitude: ' + selected.altitude);
    } else {
		field.text('Altitude: n/a');
    }
		
	var field = newField(row);
	if (selected && selected.squawk != '0000') {
		field.text('Squawk: ' + selected.squawk );
	} else {
		field.text('Squawk: n/a');
	}
	
	var row = newRow();
	var val = 'Speed: ';
	if (selected && selected.vSpeed) {
	    if (Metric) {
	        val += Math.round(selected.speed * 1.852) + ' km/h';
	    } else {
	        val += selected.speed + ' kt';
	    }
	} else {
	    val += 'n/a';
	}
	var field = newField(row);
	field.text(val);
	
	var field = newField(row);
	if (selected) {
		field.text('ICAO (hex): ' + selected.icao);
    } else {
		field.text('ICAO (hex): n/a');
    }
    
	var row = newRow();
	var val = 'Track: ';
	if (selected && selected.vTrack) {
	    val += selected.track + ' (' + normalizeTrack(selected.track, selected.vTrack)[1] +')';
	} else {
	    val += 'n/a';
	}
	row.append($('<td>').text(val)).append($('<td>').text('\u00a0'));

	var row = newRow();
	var field = newField(row).prop('colspan', columns).prop('align', 'center');
	var val = 'Lat/Long: ';

	if (selected && selected.vPosition) {
	    val += selected.latitude + ', ' + selected.longitude;
		field.text(val);
	    
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
			newField(newRow()).prop('colspan', columns).text('Distance from Site: ' + dist + (Metric ? ' km' : ' NM'))
        } // End of SiteShow
	} else {
	    if (SiteShow) {
			newField(newRow()).prop('colspan', columns).text('Distance from Site: n/a ' + (Metric ? ' km' : ' NM'));
	    } else {
    	    val += 'n/a';
			field.text(val);
    	}
	}

	
	var ptl = $('#plane_detail');
	if (ptl.children().length > 0){
		ptl.children().replaceWith(table).remove();
	} else {
		ptl.append(table);
	}
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
	var table = $('<table>').prop('id', 'tableinfo').prop('width', '100%');
	var thead = $('<thead>').css('background-color', '#BBBBBB').css('cursor', 'pointer').appendTo(table);
	var appendHeaderField = function(fieldid, fieldlabel){
		return $('<td>')
			.click(function(){setASC_DESC(fieldid); sortTable('tableinfo', fieldid)})
			.prop('align', 'right')
			.text(fieldlabel)
			.appendTo(thead)
			;
	};
	appendHeaderField('0', 'ICAO');
	appendHeaderField('1', 'Flight');
	appendHeaderField('2', 'Squawk');
	appendHeaderField('3', 'Altitude');
	appendHeaderField('4', 'Speed');
	appendHeaderField('5', 'Track');
	appendHeaderField('6', 'Msgs');
	appendHeaderField('7', 'Seen');
	var tbody = $('<tbody>').appendTo(table);
	for (var tablep in Planes) {
		var tableplane = Planes[tablep]
		if (!tableplane.reapable) {
			var row = $('<tr>').appendTo(tbody);
			row.click(function(){
				var hex = $(this).find('td:first').text();
				if (hex != "ICAO") {
					selectPlaneByHex(hex);
					refreshTableInfo();
					refreshSelected();
				}
			});
			var newRField = function(){
				return $('<td>').prop('align', 'right').appendTo(row);
			};
			var specialStyle = "";
			// Is this the plane we selected?
			if (tableplane.icao == SelectedPlane) {
				specialStyle += " selected";
				row.addClass('selected');
			}
			// Lets hope we never see this... Aircraft Hijacking
			if (tableplane.squawk == 7500) {
				specialStyle += " squawk7500";
				row.addClass('squawk7500');
			}
			// Radio Failure
			if (tableplane.squawk == 7600) {
				specialStyle += " squawk7600";
				row.addClass('squawk7600');
			}
			// Emergency
			if (tableplane.squawk == 7700) {
				specialStyle += " squawk7700";
				row.addClass('squawk7700');
			}
			
			if (tableplane.vPosition == true) {
				row.addClass('plane_table_row').addClass('vPosition');
			} else {
				row.addClass('plane_table_row');
		    }
		    
			row
				.append($('<td>').text(tableplane.icao))
				.append($('<td>').text(tableplane.flight))
				;
			var field = newRField();
			if (tableplane.squawk != '0000' ) {
				field.text(tableplane.squawk);
    	    } else {
				field.text('\u00a0');
    	    }
    	    
			var field_alt = newRField();
			var field_spd = newRField();
    	    if (Metric) {
				field_alt.text(tableplane.vAltitude === true ? Math.round(tableplane.altitude / 3.2828).toString() : (tableplane.vAltitude === '' ? tableplane.altitude : '\u00a0'));
    			field_spd.text(tableplane.vSpeed ? Math.round(tableplane.speed * 1.852).toString() : '\u00a0');
    	    } else {
				field_alt.text(tableplane.vAltitude === true ? tableplane.altitude.toString() : (tableplane.vAltitude === '' ? tableplane.altitude : '\u00a0'));
    			field_spd.text(tableplane.vSpeed ? tableplane.speed.toString() : '\u00a0');
    	    }
			
			var field = newRField();
			if (tableplane.vTrack) {
    			 field.text(normalizeTrack(tableplane.track, tableplane.vTrack)[2].toString());
    	    } else {
				field.text('\u00a0');
    	    }
			newRField().text(tableplane.messages);
			newRField().text(tableplane.seen);
		}
	}

	var ptl = $('#planes_table');
	if (ptl.children().length > 0){
		ptl.children().replaceWith(table).remove();
	} else {
		ptl.append(table);
	}

	if (SpecialSquawk) {
    	$('#SpecialSquawkWarning').css('display', 'inline');
    } else {
        $('#SpecialSquawkWarning').css('display', 'none');
    }

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
	localStorage['bSortASC'] = JSON.stringify(bSortASC);
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
	localStorage['iSortCol'] = iSortCol;

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

function selectPlaneByHex(hex) {
	// If SelectedPlane has something in it, clear out the selected
	if (SelectedPlane != null) {
		Planes[SelectedPlane].is_selected = false;
		Planes[SelectedPlane].funcClearLine();
		Planes[SelectedPlane].markerColor = MarkerColor;
		// If the selected has a marker, make it not stand out
		if (Planes[SelectedPlane].marker) {
			Planes[SelectedPlane].marker.setIcon(Planes[SelectedPlane].funcGetIcon());
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
    localStorage['MapTypeId']   = CONST_MAPTYPEID;
    
    // Try to read values from localStorage else use CONST_s
    CenterLat = Number(localStorage['CenterLat']) || CONST_CENTERLAT;
    CenterLon = Number(localStorage['CenterLon']) || CONST_CENTERLON;
    ZoomLvl   = Number(localStorage['ZoomLvl']) || CONST_ZOOMLVL;
    MapTypeId = localStorage['MapTypeId'] || CONST_MAPTYPEID;
    
    // Set and refresh
	GoogleMap.setZoom(parseInt(ZoomLvl));
	GoogleMap.setCenter(new google.maps.LatLng(parseFloat(CenterLat), parseFloat(CenterLon)));
	
	if (SelectedPlane) {
	    selectPlaneByHex(SelectedPlane);
	}

	refreshSelected();
	refreshTableInfo();
}

function refreshMap() {
	var lastZoom = GoogleMap.getZoom();
	// Try to refresh OSM if tile URL changes
	if (OsmTileUrlFormat !== lastOsmTileUrlFormat){
		lastOsmTileUrlFormat = OsmTileUrlFormat;
		GoogleMap.setZoom(lastZoom - 1);
		window.setTimeout(function(){GoogleMap.setZoom(lastZoom);}, 260);
	};
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
