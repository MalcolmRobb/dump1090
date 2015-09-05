// -*- mode: javascript; indent-tabs-mode: t; c-basic-offset: 8 -*-
"use strict";

var NBSP='\u00a0';
var DEGREES='\u00b0'
var UP_TRIANGLE='\u25b2'; // U+25B2 BLACK UP-POINTING TRIANGLE
var DOWN_TRIANGLE='\u25bc'; // U+25BC BLACK DOWN-POINTING TRIANGLE

var TrackDirections = ["North","Northeast","East","Southeast","South","Southwest","West","Northwest"];

// formatting helpers

// track in degrees (0..359)
function format_track_brief(track) {
	if (track === null){
		return "";
	}
	
	return Math.round(track);
}

// track in degrees (0..359)
function format_track_long(track) {
	if (track === null){
		return "n/a";
	}
	
	var trackDir = Math.floor((360 + track % 360 + 22.5) / 45) % 8;
	return Math.round(track) + DEGREES + NBSP + "(" + TrackDirections[trackDir] + ")";
}

// altitude (input: alt in feet)
// brief will always show either Metric or Imperial
function format_altitude_brief(alt, vr) {
	var alt_text;
	
	if (alt === null){
		return "";
	} else if (alt === "ground"){
		return "ground";
	}
	
	if (Metric) {
		alt_text = Math.round(alt / 3.2828) + NBSP; // Altitude to meters
	} else {
		alt_text = Math.round(alt) + NBSP;
	}
	
	// Vertical Rate Triangle
	if (vr > 128){
		return alt_text + UP_TRIANGLE;
	} else if (vr < -128){
		return alt_text + DOWN_TRIANGLE;
	} else {
		return alt_text + NBSP;
	}
}

// alt in ft
function _alt_to_unit(alt, m) {
	if (m)
		return Math.round(alt / 3.2828) + NBSP + "m";
	else
		return Math.round(alt) + NBSP + "ft";
}

function format_altitude_long(alt, vr) {
	var alt_text = "";
	
	if (alt === null) {
		return "n/a";
	} else if (alt === "ground") {
		return "on ground";
	}

	// Primary unit
	alt_text = _alt_to_unit(alt, Metric);

	// Secondary unit
	if (ShowOtherUnits) {
		alt_text = alt_text + ' | ' + _alt_to_unit(alt, !Metric);
	}
	
	if (vr > 128) {
		return UP_TRIANGLE + NBSP + alt_text;
	} else if (vr < -128) {
		return DOWN_TRIANGLE + NBSP + alt_text;
	} else {
		return alt_text;
	}
}

//input: speed in kts
function format_speed_brief(speed) {
	if (speed === null) {
		return "";
	}
	
	if (Metric) {
		return Math.round(speed * 1.852); // knots to kilometers per hour
	} else {
		return Math.round(speed); // knots
	}
}

// speed in kts

function _speed_to_unit(speed, m) {
	if (m)
		return Math.round(speed * 1.852) + NBSP + "km/h";
	else
		return Math.round(speed) + NBSP + "kt";
}

function format_speed_long(speed) {
	if (speed === null) {
		return "n/a";
	}

	// Primary unit
	var speed_text = _speed_to_unit(speed, Metric);

	// Secondary unit
	if (ShowOtherUnits) {
		speed_text = speed_text + ' | ' + _speed_to_unit(speed, !Metric);
	}
	
	return speed_text;
}

// dist in meters
function format_distance_brief(dist) {
	if (dist === null) {
		return "";
	}

	if (Metric) {
		return (dist/1000).toFixed(1); // meters to kilometers
	} else {
		return (dist/1852).toFixed(1); // meters to nautocal miles
	}
}

// dist in metres

function _dist_to_unit(dist, m) {
	if (m)
		return (dist/1000).toFixed(1) + NBSP + "km";
	else
		return (dist/1852).toFixed(1) + NBSP + "NM";
}

function format_distance_long(dist) {
	if (dist === null) {
		return "n/a";
	}

	// Primary unit
	var dist_text = _dist_to_unit(dist, Metric);

	// Secondary unit
	if (ShowOtherUnits) {
		dist_text = dist_text + ' | ' + _dist_to_unit(dist, !Metric);
	}

	return dist_text;
}

// p as a LatLng
function format_latlng(p) {
	return p.lat().toFixed(3) + DEGREES + "," + NBSP + p.lng().toFixed(3) + DEGREES;
}
