var deadPaths=[]; // If the user has so selected, somwehere to put dead planes's tracks

var planeObject = {
	oldlat		: null,
	oldlon		: null,
	oldalt		: null,

	// Basic location information
	altitude	: null,
	speed		: null,
	track		: null,
	latitude	: null,
	longitude	: null,
	
	// Info about the plane
	flight		: null,
	squawk		: null,
	icao		: null,
	is_selected	: false,	

	// Data packet numbers
	messages	: null,
	seen		: null,
	seenLL		: null,

	// Vaild...
	vPosition	: false,
	vTrack		: false,

	// GMap Details
	marker		: null,
	markerColor	: MarkerColor,
	guess		: null,
	guessUpdate	: 0,
	lines		: [], //unused so far
	line		: null,
	trackdata	: new Array(),
	trackline	: new Array(),

	// When was this last updated?
	updated		: null,
	reapable	: false,

	// Appends data to the running track so we can get a visual tail on the plane
	// Only useful for a long running browser session.

	getIconForGuess : function() {
	    var r = 125, g = 125, b = 123;
	    return {
		strokeWeight: (this.is_selected ? 3 : 1),
		scale: 2,
		path: google.maps.SymbolPath.FORWARD_OPEN_ARROW,
		fillColor: 'rgb('+r+','+g+','+b+')',
		fillOpacity: 0.5,
                //anchor: new google.maps.Point(1, 1), // Set anchor to middle of plane.
		rotation: this.track
	    };
	},

	updateGuess: function (timestamp) {
	    /*if (!guessing) {
		    return;
	    } */
	    if (this.speed<1) {return(null)}; // Not moving. Why guess?
	    /* timestamp is in ms, convert it into hours */
	    var dTime=(timestamp-this.guessUpdate)/(1000*60*60);
	    var Dist=this.speed*dTime; /* get Nm travelled */
	    var dLat=Dist*tabCos[Math.floor(this.track)]*latPerNm;
	    var dLon=Dist*tabSin[Math.floor(this.track)]*lonPerNm;
	    this.guessUpdate=timestamp;
	    this.guessLat+=dLat;
	    this.guessLon+=dLon;
	    if (this.guess) {
		    this.guess.setPosition(new google.maps.LatLng(this.guessLat, this.guessLon));
		    this.guess.visible=true;
	         
	    } else {
		    this.guess= new google.maps.Marker({
			position: new
			google.maps.LatLng(this.guessLat, this.guessLon),
			map: GoogleMap,
			icon: this.getIconForGuess(),
			visible: true
		    });
		    google.maps.event.addListener(this.guess, 'click', this.funcSelectPlane);
	    }
		this.guess.setTitle('estimate: ' + this.icao);
	},

	funcAddToTrack	: function(){
			// TODO: Write this function out
			this.trackdata.push([this.latitude, this.longitude, this.altitude, this.track, this.speed]);
			this.trackline.push(new google.maps.LatLng(this.latitude, this.longitude));
		},

	// This is to deemphasize the line from the screen if we deselect the plane
	funcDeselectLine : function() {
			if (this.line) {
				this.line.setOptions({strokeWeight:1, 
					strokeColor: '#000000'});
			}
		},
	// this is to shoe the line on the screen without extendint it.
	funcShowLine : function() {
			if (this.line) {
				this.line.setOptions({
					strokeWeight: (this.is_selected ? 2 : 1),
					strokeColor: (this.reapable)?'#ff0000' : '#000000'
				});
			        this.line.setMap(GoogleMap);
			} else {
				this.line = new google.maps.Polyline({
					strokeColor: (this.reapable)?'#ff0000' : '#000000',
					strokeOpacity: 1.0,
					strokeWeight: (this.is_selected) ? 3:1,
					map: GoogleMap,
					path: this.trackline
				});
			}
		},
	// This is to remove the line from the screen if we deselect the plane
	funcClearLine	: function() {
			if (this.line) {
				this.line.setMap(null);
				this.line = null;
			}
		},

	// Should create an icon for us to use on the map...
	funcGetIcon	: function() {
			// If this marker is selected we should make it lighter than the rest.
			if (this.is_selected == true) {
				this.markerColor = SelectedColor;
			}
			
			// Plane marker
            var baseSvg = {
                planeData : "M 1.9565564,41.694305 C 1.7174505,40.497708 1.6419973,38.448747 " +
                    "1.8096508,37.70494 1.8936398,37.332056 2.0796653,36.88191 2.222907,36.70461 " +
                    "2.4497603,36.423844 4.087816,35.47248 14.917931,29.331528 l 12.434577," +
                    "-7.050718 -0.04295,-7.613412 c -0.03657,-6.4844888 -0.01164,-7.7625804 " +
                    "0.168134,-8.6194061 0.276129,-1.3160905 0.762276,-2.5869575 1.347875," +
                    "-3.5235502 l 0.472298,-0.7553719 1.083746,-0.6085497 c 1.194146,-0.67053522 " +
                    "1.399524,-0.71738842 2.146113,-0.48960552 1.077005,0.3285939 2.06344," +
                    "1.41299352 2.797602,3.07543322 0.462378,1.0469993 0.978731,2.7738408 " +
                    "1.047635,3.5036272 0.02421,0.2570284 0.06357,3.78334 0.08732,7.836246 0.02375," +
                    "4.052905 0.0658,7.409251 0.09345,7.458546 0.02764,0.04929 5.600384,3.561772 " +
                    "12.38386,7.805502 l 12.333598,7.715871 0.537584,0.959688 c 0.626485,1.118378 " +
                    "0.651686,1.311286 0.459287,3.516442 -0.175469,2.011604 -0.608966,2.863924 " +
                    "-1.590344,3.127136 -0.748529,0.200763 -1.293144,0.03637 -10.184829,-3.07436 " +
                    "C 48.007733,41.72562 44.793806,40.60197 43.35084,40.098045 l -2.623567," +
                    "-0.916227 -1.981212,-0.06614 c -1.089663,-0.03638 -1.985079,-0.05089 -1.989804," +
                    "-0.03225 -0.0052,0.01863 -0.02396,2.421278 -0.04267,5.339183 -0.0395,6.147742 " +
                    "-0.143635,7.215456 -0.862956,8.845475 l -0.300457,0.680872 2.91906,1.361455 " +
                    "c 2.929379,1.366269 3.714195,1.835385 4.04589,2.41841 0.368292,0.647353 " +
                    "0.594634,2.901439 0.395779,3.941627 -0.0705,0.368571 -0.106308,0.404853 " +
                    "-0.765159,0.773916 L 41.4545,62.83158 39.259237,62.80426 c -6.030106,-0.07507 " +
                    "-16.19508,-0.495041 -16.870991,-0.697033 -0.359409,-0.107405 -0.523792," +
                    "-0.227482 -0.741884,-0.541926 -0.250591,-0.361297 -0.28386,-0.522402 -0.315075," +
                    "-1.52589 -0.06327,-2.03378 0.23288,-3.033615 1.077963,-3.639283 0.307525," +
                    "-0.2204 4.818478,-2.133627 6.017853,-2.552345 0.247872,-0.08654 0.247455," +
                    "-0.102501 -0.01855,-0.711959 -0.330395,-0.756986 -0.708622,-2.221756 -0.832676," +
                    "-3.224748 -0.05031,-0.406952 -0.133825,-3.078805 -0.185533,-5.937448 -0.0517," +
                    "-2.858644 -0.145909,-5.208974 -0.209316,-5.222958 -0.06341,-0.01399 -0.974464," +
                    "-0.0493 -2.024551,-0.07845 L 23.247235,38.61921 18.831373,39.8906 C 4.9432155," +
                    "43.88916 4.2929558,44.057819 3.4954426,43.86823 2.7487826,43.690732 2.2007966," +
                    "42.916622 1.9565564,41.694305 z"
            };

			// If the squawk code is one of the international emergency codes,
			// match the info window alert color.
			if (this.squawk == 7500) {
				this.markerColor = "rgb(255, 85, 85)";
			}
			if (this.squawk == 7600) {
				this.markerColor = "rgb(0, 255, 255)";
			}
			if (this.squawk == 7700) {
				this.markerColor = "rgb(255, 255, 0)";
			}

			// If we have not overwritten color by now, an extension still could but
			// just keep on trucking.  :)

			return {
                strokeWeight: (this.is_selected ? 2 : 1),
                path:  "M 0,0 "+ baseSvg["planeData"],
                scale: 0.4,
                fillColor: this.markerColor,
                fillOpacity: 0.9,
                anchor: new google.maps.Point(32, 32), // Set anchor to middle of plane.
                rotation: this.track
            };
		},

	// TODO: Trigger actions of a selecting a plane
	funcSelectPlane	: function(selectedPlane){
			selectPlaneByHex(this.icao);
		},
	updateTrail : function(trail,oldlat,oldlon) {
		//The trail from the server is most recent first. Assuming there is
		//enough data, then one of the lat/lon pairs must be equal/close to the
		//previous location. DOES NOT replace funcUpdateLines, as the trail
		//does not include present location.
		if (trail && trail[0] && (trail[0][0]!=oldlat || trail[0][1]!=oldlon)) { 
		    var l=0;
		    var minDelta=0.00001;
		    while(l<trail.length && trail[l] && (Math.abs(trail[l][0] - oldlat) > minDelta ||  Math.abs(trail[l][1] - oldlon) > minDelta )) { /* Scan to find the new data */
			    //console.log(Math.abs(trail[l][0] - oldlat) + ', ' + Math.abs(trail[l][1] - oldlon));
			    l++;
		    }
		    // Did we hit the end or find oldlat,oldlon?  In any case we need to subtract one from l
		    l--;
		    //console.log('Trail update:' + this.icao +':' + trail.length + ' > ' + l ) ;
		    if (l>=0) {
				/* GoogleMap Polylines are a little strange in their
				 * relationship to the original trackline.  If we push
				 * to the PolyLine's path, then it pushes to trackline
				 * and redraws the map. If we just push to trackline,
				 * the map is only redrawn if we later push to the
				 * PolyLine's path. Thus we mut choose which to push to (pushing to
				 * both makes loops). */
				if (!this.line) { 
					for (var k=l; k>=0 && trail[k]; k--) { /* reverse as we do this, since newest data is reported first, and the api needs additions pushed */
						this.trackdata.push([trail[k][0], trail[k][1], null, null, null]);
						this.trackline.push(new google.maps.LatLng(trail[k][0],trail[k][1]));
					}
				} else {
					var path=this.line.getPath();
					for (var k=l; k>=0 && trail[k]; k--) { /* reverse as we do this, since newest data is reported first, and the api needs additions pushed */
						this.trackdata.push([trail[k][0], trail[k][1], null, null, null]);
						path.push(new google.maps.LatLng(trail[k][0],trail[k][1]));
					}
				}
			}
		}
	},

	// Update our data
	funcUpdateData	: function(data,now){
			// So we can find out if we moved
			var oldlat 	= this.latitude;
			var oldlon	= this.longitude;
			var oldalt	= this.altitude;

			// Update all of our data
			this.updated	= now;
			this.altitude	= data.altitude;
			this.speed	= data.speed;
			this.track	= data.track;
			this.latitude	= data.lat;
			this.longitude	= data.lon;
			this.flight	= data.flight;
			this.squawk	= data.squawk;
			this.icao	= data.hex;
			this.messages	= data.messages;
			this.seen	= data.seen;
			this.seenLL	= data.seenLL;
			this.range	= data.range;
			this.elevation	= data.elevation;
			this.bearing	= data.bearing;


			// If no packet in over 58 seconds, consider the plane reapable
			// This way we can hold it, but not show it just in case the plane comes back
			if (this.seen > 58) {
			    if (!this.reapable) {
				this.reapable = true;
				if (this.guess) {  // Remove guess marker
					this.guess.setMap(null);
					this.guess = null;
				}
				if (this.marker) { // Remove plane marker
					this.marker.setMap(null);
					this.marker = null;
				}
				if (this.line) { // remove trail.. maybe.
					if (trailDisplay=='showall') {
						this.line.setOptions({strokeColor: "#ff0000", strokeWeight:1});
					} else {
						this.line.setMap(null);
						this.line = null;
					}
				}
				if (SelectedPlane == this.icao) {
					if (this.is_selected) {
						this.is_selected = false;
					}
					SelectedPlane = null;
				}
			    }
			} else {
				if (this.reapable == true) {
					this.reapable = false;
				}
			}

			// If we've been paused long enough there could be new 
			// track data that should be added, even if the plane is
			// now old, in case the plane comes back,
			// or if we're plotting and keeping
			// all trails. Therefore, we always process the track history. 
			// Note that trail does not contain the current position, so check that it actually moved
			if (data.trail && data.trail.length && (oldlat != this.latitude || oldlon != this.longitude) ) {
				//console.log('Trail update:' + this.icao +':' + data.trail.length ) ;
				this.updateTrail(data.trail,oldlat,oldlon);
			}

			// Is the position valid?
			if ((data.validposition == 1) && (this.reapable == false)) {
				this.vPosition = true;

				// Detech if the plane has moved
				changeLat = false;
				changeLon = false;
				changeAlt = false;
				if (oldlat != this.latitude) {
					changeLat = true;
				}
				if (oldlon != this.longitude) {
					changeLon = true;
				}
				if (oldalt != this.altitude) {
					changeAlt = true;
				}
				// Right now we only care about lat/long, if alt is updated only, oh well
				if ((changeLat == true) || (changeLon == true)) {
					if (this.guess) {
					    this.guess.setMap(null);
					    this.guess=null;
					}
					this.funcAddToTrack();
					if (this.is_selected || form.elements['traildisplay'].value!='selected' ) {
						this.line = this.funcUpdateLines();
					}
					this.guessUpdate=now-1000*this.seenLL;
					this.guessLat=this.latitude;
					this.guessLon=this.longitude;
				} 
				if (this.seenLL>5) {
				    this.updateGuess(now)
				} else {
				    if (this.guess) {
					this.guess.visible=false;
				    }
				}
				this.marker = this.funcUpdateMarker();
				PlanesOnMap++;
			} else {
				this.vPosition = false;
			}


			// Do we have a valid track for the plane?
			if (data.validtrack == 1)
				this.vTrack = true;
			else
				this.vTrack = false;
		},

	// Update our marker on the map
	funcUpdateMarker: function() {
			if (this.marker) {
				this.marker.setPosition(new google.maps.LatLng(this.latitude, this.longitude));
				this.marker.setIcon(this.funcGetIcon());
			} else {
				this.marker = new google.maps.Marker({
					position: new google.maps.LatLng(this.latitude, this.longitude),
					map: GoogleMap,
					icon: this.funcGetIcon(),
					visible: true
				});

				// This is so we can match icao address
				this.marker.icao = this.icao;

				// Trap clicks for this marker.
				google.maps.event.addListener(this.marker, 'click', this.funcSelectPlane);
			}

			// Setting the marker title
			if (this.flight.length == 0) {
				this.marker.setTitle(this.hex);
			} else {
				this.marker.setTitle(this.flight+' ('+this.icao+')');
			}
			return this.marker;
		},

	// Update our planes tail line,
	// TODO: Make this multi colored based on options
	//		altitude (default) or speed
	funcUpdateLines: function() {
			if (this.line) {
				var path = this.line.getPath();
				path.push(new google.maps.LatLng(this.latitude, this.longitude));
			} else {
				this.line = new google.maps.Polyline({
					strokeColor: '#000000',
					strokeOpacity: 1.0,
					strokeWeight: (this.is_selected) ? 3:1,
					map: GoogleMap,
					path: this.trackline
				});
			}
			return this.line;
		}
};
