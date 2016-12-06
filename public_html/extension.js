// -----------------------------------------------------
//
// This file is so users can modify how the page acts
// without diving to deep in the code.  This way we can
// also try out or hold custom code for ourselves and
// not check it into the repo.
//
// There is a div id'ed as plane_extension for use with
// this javascript file.
// -----------------------------------------------------

function extendedInitalize() {
	// Write your initalization here
	// Gets called just before the 1-sec function call loop is setup
}

function extendedPulse() {
	// This will get called every second after all the main functions
}

function toggleRadar() {
        if (DisplayRadar == 1) {
            DisplayRadar = 0;
            kmlLayer.setMap(null);
        } else {
            DisplayRadar = 1;
            var kmlUrl = 'http://' + RadarKmlFqdn + '/latest_radaronly_Radar_data.kml'; // Modified NWS CONUS KML file
            var kmlOptions = {
                suppressInfoWindows: true,
                preserveViewport: true,
                map: GoogleMap
            };
            kmlLayer = new google.maps.KmlLayer(kmlUrl, kmlOptions);
        }
}