Map       = null;
CenterLat = 35.211;
CenterLon = -80.953;
ZoomLvl   = 9;
Planes    = {};
PlanesOnMap  = 0;
PlanesOnGrid = 0;
Selected     = null;

if (localStorage['CenterLat']) { CenterLat = Number(localStorage['CenterLat']); }
if (localStorage['CenterLon']) { CenterLon = Number(localStorage['CenterLon']); }
if (localStorage['ZoomLvl'])   { ZoomLvl   = Number(localStorage['ZoomLvl']); }

function getIconForPlane(plane) {
    var r = 255, g = 255, b = 0;
    var maxalt = 40000; // Max altitude in the average case
    var invalt = maxalt-plane.altitude;
    var selected = (Selected == plane.hex);

    if (invalt < 0) invalt = 0;
    b = parseInt(255/maxalt*invalt);
    return {
        strokeWeight: (selected ? 2 : 1),
        path: google.maps.SymbolPath.FORWARD_CLOSED_ARROW,
        scale: 5,
        fillColor: 'rgb('+r+','+g+','+b+')',
        fillOpacity: 0.9,
        rotation: plane.track
    };
}

function selectPlane() {
    if (!Planes[this.planehex]) return;
    var old = Selected;
    Selected = this.planehex;
    if (Planes[old]) {
        /* Remove the highlight in the previously selected plane. */
        Planes[old].marker.setIcon(getIconForPlane(Planes[old]));
    }
    Planes[Selected].marker.setIcon(getIconForPlane(Planes[Selected]));
    refreshSelectedInfo();
}

function refreshGeneralInfo() {
    var i = document.getElementById('geninfo');

    i.innerHTML  = PlanesOnGrid + ' planes on grid.<br>';
    i.innerHTML += PlanesOnMap + ' planes on map.';
}

function refreshSelectedInfo() {
    var i = document.getElementById('selinfo');
    var p = Planes[Selected];

    if (!p) return;
    var html = 'ICAO: '+p.hex+'<br>';
    if (p.flight.length) {
        html += '<b>'+p.flight+'</b><br>';
    }
    html += 'Altitude: '+p.altitude+' feet<br>';
    html += 'Speed: '+p.speed+' knots<br>';
    html += 'Coordinates: '+p.lat+', '+p.lon+'<br>';
    html += 'Messages: '+p.messages+'<br>';
    html += 'Seen: '+p.seen+' sec<br>';
    i.innerHTML = html;
}

function refreshTableInfo() {
    var i = document.getElementById('tabinfo');

    var html = '<table id="tableinfo" width="100%">';
    html += '<thead style="background-color: #CCCCCC;"><td>Flight</td><td>Sqwk</td><td align="right">Altitude</td><td align="center">Speed</td><td align="center">Track</td><td>Lat</td><td>Long</td><td>Seen</td><td>Msgs</td></thead>';
    for (var p in Planes) {
        if (p == Selected) {
            html += '<tr style="background-color: #F0F0F0;">';
        } else {
            html += '<tr>';
        }
        html += '<td>' + Planes[p].flight + '</td>';
	html += '<td>' + Planes[p].squawk + '</td>';
        html += '<td align="right">' + Planes[p].altitude + '</td>';
        html += '<td align="center">' + Planes[p].speed + '</td>';
        html += '<td align="center">' + Planes[p].track + '</td>';
        html += '<td>' + Planes[p].lat + '</td>';
        html += '<td>' + Planes[p].lon + '</td>';
        html += '<td align="center">' + Planes[p].seen + '</td>';
        html += '<td align="right">' + Planes[p].messages + '</td>';
        html += '</tr>';
    }
    html += '</table>';
    i.innerHTML = html;
}

function fetchData() {
	$.getJSON('data.json', function(data) {
		// Planes that are still with us, and set map count to 0
		var stillhere = {}
		PlanesOnMap = 0;

		// Loop through all the planes in the data packet
		for (var j=0; j < data.length; j++) {

			// Set plane to be this particular plane in the data set
			var plane = data[j];
			// Add to the still here list
			stillhere[plane.hex] = true;
			plane.flight = $.trim(plane.flight);

			// Set the marker to null, for now
			var marker = null;

			// Either update the data or add it
			if (Planes[plane.hex]) {
				// Declare our plane that we are working with from our old data set
				var myplane = Planes[plane.hex];

				// If the lat/long is not 0, we should make a marker for it
				if (plane.lat != 0 && plane.lon != 0) {
					if (myplane.marker != null) {
						marker = myplane.marker;
						var icon = marker.getIcon();
						var newpos = new google.maps.LatLng(plane.lat, plane.lon);
						marker.setPosition(newpos);
						marker.setIcon(getIconForPlane(plane));
						PlanesOnMap++;
					} else {
						// Add new plane to map, dont ask me why this is needed here now...
						marker = new google.maps.Marker({
							position: new google.maps.LatLng(plane.lat, plane.lon),
							map: Map,
							icon: getIconForPlane(plane)
						});
						myplane.marker = marker;
						marker.planehex = plane.hex;
						PlanesOnMap++;

						// Trap clicks for this marker.
						google.maps.event.addListener(marker, 'click', selectPlane);
					}
				}

				// Update all the other information
				myplane.altitude = plane.altitude;
				myplane.speed = plane.speed;
				myplane.lat = plane.lat;
				myplane.lon = plane.lon;
				myplane.track = plane.track;
				myplane.flight = plane.flight;
				myplane.seen = plane.seen;
				myplane.messages = plane.messages;

				// If this is a selected plane, refresh its data outside of the table
				if (myplane.hex == Selected)
					refreshSelectedInfo();
			} else {
				// This is a new plane
				// Do we have a lat/long that is not 0?
				if (plane.lat != 0 && plane.lon != 0) {
					// Add new plane to map
					marker = new google.maps.Marker({
						position: new google.maps.LatLng(plane.lat, plane.lon),
						map: Map,
						icon: getIconForPlane(plane)
					});
					plane.marker = marker;
					marker.planehex = plane.hex;
					PlanesOnMap++;

					// Trap clicks for this marker.
					google.maps.event.addListener(marker, 'click', selectPlane);
				}

				// Copy the plane into Planes
				Planes[plane.hex] = plane;
			}

			// If we have lat/long, we must have a marker, so lets set the marker title
			if (plane.lat != 0 && plane.lon != 0) {
				if (plane.flight.length == 0) {
					marker.setTitle(plane.hex)
				} else {
					marker.setTitle(plane.flight+' ('+plane.hex+')')
				}
			}

		}

		// How many planes have we heard from?
		PlanesOnGrid = data.length;

		/* Remove idle planes. */
		for (var p in Planes) {
			if (!stillhere[p]) {
				if (Planes[p].marker != null)
					Planes[p].marker.setMap(null);
				delete Planes[p];
			}
		}

		refreshTableInfo() ;

	});
}

function checkTime(i) {
    if (i < 10) {
        return "0" + i;
    }
    return i;
}

function printTime() {
    var currentTime = new Date();
    var hours = checkTime(currentTime.getUTCHours());
    var minutes = checkTime(currentTime.getUTCMinutes());
    var seconds = checkTime(currentTime.getUTCSeconds());
    
    if (document.getElementById) {
        document.getElementById('utcTime').innerHTML =
            hours + ":" + minutes + ":" + seconds;
    }
}

function placeFooter() {    
    var windHeight = $(window).height();
    var footerHeight = $('#info_footer').height();
    var offset = parseInt(windHeight) - parseInt(footerHeight);
    
    var footerWidth = parseInt($('#info_footer').width());
    var infoWidth = parseInt($('#info').width());
    var marginLeft = parseInt((infoWidth / 2) - (footerWidth / 2));
    
    $('#info_footer').css('top', offset);
    $('#info_footer').css('margin-left', marginLeft);
}

function resetMap() {
    localStorage['CenterLat'] = 35.211;
    localStorage['CenterLon'] = -80.953;
    localStorage['ZoomLvl']   = 9;
    Map.setZoom(parseInt(localStorage['ZoomLvl']));
    Map.setCenter(new google.maps.LatLng(parseInt(localStorage['CenterLat']), parseInt(localStorage['CenterLon'])));
    Selected = null;
    document.getElementById('selinfo').innerHTML = '';
}

function initialize() {
    var mapTypeIds = [];
    for(var type in google.maps.MapTypeId) {
        mapTypeIds.push(google.maps.MapTypeId[type]);
    }
    mapTypeIds.push("OSM");

    var mapOptions = {
        center: new google.maps.LatLng(CenterLat, CenterLon),
        zoom: ZoomLvl,
        mapTypeId: google.maps.MapTypeId.ROADMAP,
        mapTypeControlOptions: {
            mapTypeIds: mapTypeIds,
        }
    };
    Map = new google.maps.Map(document.getElementById("map_canvas"), mapOptions);

    //Define OSM map type pointing at the OpenStreetMap tile server
    Map.mapTypes.set("OSM", new google.maps.ImageMapType({
        getTileUrl: function(coord, zoom) {
           return "http://tile.openstreetmap.org/" + zoom + "/" + coord.x + "/" + coord.y + ".png";
        },
        tileSize: new google.maps.Size(256, 256),
        name: "OpenStreetMap",
        maxZoom: 18
    }));
    
    // show footer at info-area
    $(function(){
        $(window).resize(function(e){
            placeFooter();
        });
        placeFooter();
        // hide it before it's positioned
        $('#info_footer').css('display','inline');
    });
    
    // Listener for newly created Map
    google.maps.event.addListener(Map, 'center_changed', function() {
        localStorage['CenterLat'] = Map.getCenter().lat();
        localStorage['CenterLon'] = Map.getCenter().lng();
    });
    
    google.maps.event.addListener(Map, 'zoom_changed', function() {
        localStorage['ZoomLvl']  = Map.getZoom();
    });

    // Setup our timer to poll from the server.
    window.setInterval(function() {
        fetchData();
        refreshGeneralInfo();
    }, 1000);
    
    // Faster timer, smoother things
    window.setInterval(function() {
        printTime();
    }, 250);
}
