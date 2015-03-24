/**
 * CoolClock 2.1.4
 * Copyright 2010, Simon Baird
 * Released under the BSD License.
 *
 * Display an analog clock using canvas.
 * http://randomibis.com/coolclock/
 *
 */

// Constructor for CoolClock objects
window.CoolClock = function(options) {
	return this.init(options);
}

// Config contains some defaults, and clock skins
CoolClock.config = {
	tickDelay: 1000,
	longTickDelay: 15000,
	defaultRadius: 85,
	renderRadius: 100,
	defaultSkin: "chunkySwiss",
	// Should be in skin probably...
	// (TODO: allow skinning of digital display)
	showSecs: true,
	showAmPm: true,

	skins:	{
		// There are more skins in moreskins.js
		// Try making your own skin by copy/pasting one of these and tweaking it
		swissRail: {
			outerBorder: { lineWidth: 2, radius:95, color: "black", alpha: 1 },
			smallIndicator: { lineWidth: 2, startAt: 88, endAt: 92, color: "black", alpha: 1 },
			largeIndicator: { lineWidth: 4, startAt: 79, endAt: 92, color: "black", alpha: 1 },
			hourHand: { lineWidth: 8, startAt: -15, endAt: 50, color: "black", alpha: 1 },
			minuteHand: { lineWidth: 7, startAt: -15, endAt: 75, color: "black", alpha: 1 },
			secondHand: { lineWidth: 1, startAt: -20, endAt: 85, color: "red", alpha: 1 },
			secondDecoration: { lineWidth: 1, startAt: 70, radius: 4, fillColor: "red", color: "red", alpha: 1 }
		},
		chunkySwiss: {
			outerBorder: { lineWidth: 4, radius:97, color: "black", alpha: 1 },
			smallIndicator: { lineWidth: 4, startAt: 89, endAt: 93, color: "black", alpha: 1 },
			largeIndicator: { lineWidth: 8, startAt: 80, endAt: 93, color: "black", alpha: 1 },
			hourHand: { lineWidth: 12, startAt: -15, endAt: 60, color: "black", alpha: 1 },
			minuteHand: { lineWidth: 10, startAt: -15, endAt: 85, color: "black", alpha: 1 },
			secondHand: { lineWidth: 4, startAt: -20, endAt: 85, color: "red", alpha: 1 },
			secondDecoration: { lineWidth: 2, startAt: 70, radius: 8, fillColor: "red", color: "red", alpha: 1 }
		},
		chunkySwissOnBlack: {
			outerBorder: { lineWidth: 4, radius:97, color: "white", alpha: 1 },
			smallIndicator: { lineWidth: 4, startAt: 89, endAt: 93, color: "white", alpha: 1 },
			largeIndicator: { lineWidth: 8, startAt: 80, endAt: 93, color: "white", alpha: 1 },
			hourHand: { lineWidth: 12, startAt: -15, endAt: 60, color: "white", alpha: 1 },
			minuteHand: { lineWidth: 10, startAt: -15, endAt: 85, color: "white", alpha: 1 },
			secondHand: { lineWidth: 4, startAt: -20, endAt: 85, color: "red", alpha: 1 },
			secondDecoration: { lineWidth: 2, startAt: 70, radius: 8, fillColor: "red", color: "red", alpha: 1 }
		},
		swissRail: {
		outerBorder:      { lineWidth: 2, radius: 95, color: "black", alpha: 1 },
		smallIndicator:   { lineWidth: 2, startAt: 88, endAt: 92, color: "black", alpha: 1 },
		largeIndicator:   { lineWidth: 4, startAt: 79, endAt: 92, color: "black", alpha: 1 },
		hourHand:         { lineWidth: 8, startAt: -15, endAt: 50, color: "black", alpha: 1 },
		minuteHand:       { lineWidth: 7, startAt: -15, endAt: 75, color: "black", alpha: 1 },
		secondHand:       { lineWidth: 1, startAt: -20, endAt: 85, color: "red", alpha: 1 },
		secondDecoration: { lineWidth: 1, startAt: 70, radius: 4, fillColor: "red", color: "red", alpha: 1 }
	},

	chunkySwiss: {
		outerBorder:      { lineWidth: 4, radius: 97, color: "black", alpha: 1 },
		smallIndicator:   { lineWidth: 4, startAt: 89, endAt: 93, color: "black", alpha: 1 },
		largeIndicator:   { lineWidth: 8, startAt: 80, endAt: 93, color: "black", alpha: 1 },
		hourHand:         { lineWidth: 12, startAt: -15, endAt: 60, color: "black", alpha: 1 },
		minuteHand:       { lineWidth: 10, startAt: -15, endAt: 85, color: "black", alpha: 1 },
		secondHand:       { lineWidth: 4, startAt: -20, endAt: 85, color: "red", alpha: 1 },
		secondDecoration: { lineWidth: 2, startAt: 70, radius: 8, fillColor: "red", color: "red", alpha: 1 }
	},

	chunkySwissOnBlack: {
		outerBorder:      { lineWidth: 4, radius: 97, color: "white", alpha: 1 },
		smallIndicator:   { lineWidth: 4, startAt: 89, endAt: 93, color: "white", alpha: 1 },
		largeIndicator:   { lineWidth: 8, startAt: 80, endAt: 93, color: "white", alpha: 1 },
		hourHand:         { lineWidth: 12, startAt: -15, endAt: 60, color: "white", alpha: 1 },
		minuteHand:       { lineWidth: 10, startAt: -15, endAt: 85, color: "white", alpha: 1 },
		secondHand:       { lineWidth: 4, startAt: -20, endAt: 85, color: "red", alpha: 1 },
		secondDecoration: { lineWidth: 2, startAt: 70, radius: 8, fillColor: "red", color: "red", alpha: 1 }
	},

	fancy: {
		outerBorder:      { lineWidth: 5, radius: 95, color: "green", alpha: 0.7 },
		smallIndicator:   { lineWidth: 1, startAt: 80, endAt: 93, color: "black", alpha: 0.4 },
		largeIndicator:   { lineWidth: 1, startAt: 30, endAt: 93, color: "black", alpha: 0.5 },
		hourHand:         { lineWidth: 8, startAt: -15, endAt: 50, color: "blue", alpha: 0.7 },
		minuteHand:       { lineWidth: 7, startAt: -15, endAt: 92, color: "red", alpha: 0.7 },
		secondHand:       { lineWidth: 10, startAt: 80, endAt: 85, color: "blue", alpha: 0.3 },
		secondDecoration: { lineWidth: 1, startAt: 30, radius: 50, fillColor: "blue", color: "red", alpha: 0.15 }
	},

	machine: {
		outerBorder:      { lineWidth: 60, radius: 55, color: "#dd6655", alpha: 1 },
		smallIndicator:   { lineWidth: 4, startAt: 80, endAt: 95, color: "white", alpha: 1 },
		largeIndicator:   { lineWidth: 14, startAt: 77, endAt: 92, color: "#dd6655", alpha: 1 },
		hourHand:         { lineWidth: 18, startAt: -15, endAt: 40, color: "white", alpha: 1 },
		minuteHand:       { lineWidth: 14, startAt: 24, endAt: 100, color: "#771100", alpha: 0.5 },
		secondHand:       { lineWidth: 3, startAt: 22, endAt: 83, color: "green", alpha: 0 },
		secondDecoration: { lineWidth: 1, startAt: 52, radius: 26, fillColor: "#ffcccc", color: "red", alpha: 0.5 }
	},

	simonbaird_com: {
		hourHand:         { lineWidth: 80, startAt: -15, endAt: 35,  color: 'magenta', alpha: 0.5 },
		minuteHand:       { lineWidth: 80, startAt: -15, endAt: 65,  color: 'cyan', alpha: 0.5 },
		secondDecoration: { lineWidth: 1,  startAt: 40,  radius: 40, color: "#fff", fillColor: 'yellow', alpha: 0.5 }
	},

	// by bonstio, http://bonstio.net
	classic/*was gIG*/: {
		outerBorder:      { lineWidth: 185, radius: 1, color: "#E5ECF9", alpha: 1 },
		smallIndicator:   { lineWidth: 2, startAt: 89, endAt: 94, color: "#3366CC", alpha: 1 },
		largeIndicator:   { lineWidth: 4, startAt: 83, endAt: 94, color: "#3366CC", alpha: 1 },
		hourHand:         { lineWidth: 5, startAt: 0, endAt: 60, color: "black", alpha: 1 },
		minuteHand:       { lineWidth: 4, startAt: 0, endAt: 80, color: "black", alpha: 1 },
		secondHand:       { lineWidth: 1, startAt: -20, endAt: 85, color: "red", alpha: .85 },
		secondDecoration: { lineWidth: 3, startAt: 0, radius: 2, fillColor: "black", color: "black", alpha: 1 }
	},

	modern/*was gIG2*/: {
		outerBorder:      { lineWidth: 185, radius: 1, color: "#E5ECF9", alpha: 1 },
		smallIndicator:   { lineWidth: 5, startAt: 88, endAt: 94, color: "#3366CC", alpha: 1 },
		largeIndicator:   { lineWidth: 5, startAt: 88, endAt: 94, color: "#3366CC", alpha: 1 },
		hourHand:         { lineWidth: 8, startAt: 0, endAt: 60, color: "black", alpha: 1 },
		minuteHand:       { lineWidth: 8, startAt: 0, endAt: 80, color: "black", alpha: 1 },
		secondHand:       { lineWidth: 5, startAt: 80, endAt: 85, color: "red", alpha: .85 },
		secondDecoration: { lineWidth: 3, startAt: 0, radius: 4, fillColor: "black", color: "black", alpha: 1 }
	},

	simple/*was gIG3*/: {
		outerBorder:      { lineWidth: 185, radius: 1, color: "#E5ECF9", alpha: 1 },
		smallIndicator:   { lineWidth: 10, startAt: 90, endAt: 94, color: "#3366CC", alpha: 1 },
		largeIndicator:   { lineWidth: 10, startAt: 90, endAt: 94, color: "#3366CC", alpha: 1 },
		hourHand:         { lineWidth: 8, startAt: 0, endAt: 60, color: "black", alpha: 1 },
		minuteHand:       { lineWidth: 8, startAt: 0, endAt: 80, color: "black", alpha: 1 },
		secondHand:       { lineWidth: 5, startAt: 80, endAt: 85, color: "red", alpha: .85 },
		secondDecoration: { lineWidth: 3, startAt: 0, radius: 4, fillColor: "black", color: "black", alpha: 1 }
	},

	// by securephp
	securephp: {
		outerBorder:      { lineWidth: 100, radius: 0.45, color: "#669900", alpha: 0.3 },
		smallIndicator:   { lineWidth: 2, startAt: 80, endAt: 90 , color: "green", alpha: 1 },
		largeIndicator:   { lineWidth: 8.5, startAt: 20, endAt: 40 , color: "green", alpha: 0.4 },
		hourHand:         { lineWidth: 3, startAt: 0, endAt: 60, color: "black", alpha: 1 },
		minuteHand:       { lineWidth: 2, startAt: 0, endAt: 75, color: "black", alpha: 1 },
		secondHand:       { lineWidth: 1, startAt: -10, endAt: 80, color: "blue", alpha: 0.8 },
		secondDecoration: { lineWidth: 1, startAt: 70, radius: 4, fillColor: "blue", color: "red", alpha: 1 }
	},

	Tes2: {
		outerBorder:      { lineWidth: 4, radius: 95, color: "black", alpha: 0.5 },
		smallIndicator:   { lineWidth: 1, startAt: 10, endAt: 50 , color: "#66CCFF", alpha: 1 },
		largeIndicator:   { lineWidth: 8.5, startAt: 60, endAt: 70, color: "#6699FF", alpha: 1 },
		hourHand:         { lineWidth: 5, startAt: -15, endAt: 60, color: "black", alpha: 0.7 },
		minuteHand:       { lineWidth: 3, startAt: -25, endAt: 75, color: "black", alpha: 0.7 },
		secondHand:       { lineWidth: 1.5, startAt: -20, endAt: 88, color: "red", alpha: 1 },
		secondDecoration: { lineWidth: 1, startAt: 20, radius: 4, fillColor: "blue", color: "red", alpha: 1 }
	},


	Lev: {
		outerBorder:      { lineWidth: 10, radius: 95, color: "#CCFF33", alpha: 0.65 },
		smallIndicator:   { lineWidth: 5, startAt: 84, endAt: 90, color: "#996600", alpha: 1 },
		largeIndicator:   { lineWidth: 40, startAt: 25, endAt: 95, color: "#336600", alpha: 0.55 },
		hourHand:         { lineWidth: 4, startAt: 0, endAt: 65, color: "black", alpha: 0.9 },
		minuteHand:       { lineWidth: 3, startAt: 0, endAt: 80, color: "black", alpha: 0.85 },
		secondHand:       { lineWidth: 1, startAt: 0, endAt: 85, color: "black", alpha: 1 },
		secondDecoration: { lineWidth: 2, startAt: 5, radius: 10, fillColor: "black", color: "black", alpha: 1 }
	},

	Sand: {
		outerBorder:      { lineWidth: 1, radius: 70, color: "black", alpha: 0.5 },
		smallIndicator:   { lineWidth: 3, startAt: 50, endAt: 70, color: "#0066FF", alpha: 0.5 },
		largeIndicator:   { lineWidth: 200, startAt: 80, endAt: 95, color: "#996600", alpha: 0.75 },
		hourHand:         { lineWidth: 4, startAt: 0, endAt: 65, color: "black", alpha: 0.9 },
		minuteHand:       { lineWidth: 3, startAt: 0, endAt: 80, color: "black", alpha: 0.85 },
		secondHand:       { lineWidth: 1, startAt: 0, endAt: 85, color: "black", alpha: 1 },
		secondDecoration: { lineWidth: 2, startAt: 5, radius: 10, fillColor: "black", color: "black", alpha: 1 }
	},

	Sun: {
		outerBorder:      { lineWidth: 100, radius: 140, color: "#99FFFF", alpha: 0.2 },
		smallIndicator:   { lineWidth: 300, startAt: 50, endAt: 70, color: "black", alpha: 0.1 },
		largeIndicator:   { lineWidth: 5, startAt: 80, endAt: 95, color: "black", alpha: 0.65 },
		hourHand:         { lineWidth: 4, startAt: 0, endAt: 65, color: "black", alpha: 0.9 },
		minuteHand:       { lineWidth: 3, startAt: 0, endAt: 80, color: "black", alpha: 0.85 },
		secondHand:       { lineWidth: 1, startAt: 0, endAt: 90, color: "black", alpha: 1 },
		secondDecoration: { lineWidth: 2, startAt: 5, radius: 10, fillColor: "black", color: "black", alpha: 1 }
	},

	Tor: {
		outerBorder:      { lineWidth: 10, radius: 88, color: "#996600", alpha: 0.9 },
		smallIndicator:   { lineWidth: 6, startAt: -10, endAt: 73, color: "green", alpha: 0.3 },
		largeIndicator:   { lineWidth: 6, startAt: 73, endAt: 100, color: "black", alpha: 0.65 },
		hourHand:         { lineWidth: 4, startAt: 0, endAt: 65, color: "black", alpha: 1 },
		minuteHand:       { lineWidth: 3, startAt: 0, endAt: 80, color: "black", alpha: 1 },
		secondHand:       { lineWidth: 1, startAt: -73, endAt: 73, color: "black", alpha: 0.8 },
		secondDecoration: { lineWidth: 2, startAt: 5, radius: 10, fillColor: "black", color: "black", alpha: 1 }
	},

	Cold: {
		outerBorder:      { lineWidth: 15, radius: 90, color: "black", alpha: 0.3 },
		smallIndicator:   { lineWidth: 15, startAt: -10, endAt: 95, color: "blue", alpha: 0.1 },
		largeIndicator:   { lineWidth: 3, startAt: 80, endAt: 95, color: "blue", alpha: 0.65 },
		hourHand:         { lineWidth: 4, startAt: 0, endAt: 65, color: "black", alpha: 1 },
		minuteHand:       { lineWidth: 3, startAt: 0, endAt: 80, color: "black", alpha: 1 },
		secondHand:       { lineWidth: 1, startAt: 0, endAt: 85, color: "black", alpha: 0.8 },
		secondDecoration: { lineWidth: 5, startAt: 30, radius: 10, fillColor: "black", color: "black", alpha: 1 }
	},

	Babosa: {
		outerBorder:      { lineWidth: 100, radius: 25, color: "blue", alpha: 0.25 },
		smallIndicator:   { lineWidth: 3, startAt: 90, endAt: 95, color: "#3366CC", alpha: 1 },
		largeIndicator:   { lineWidth: 4, startAt: 75, endAt: 95, color: "#3366CC", alpha: 1 },
		hourHand:         { lineWidth: 4, startAt: 0, endAt: 60, color: "black", alpha: 1 },
		minuteHand:       { lineWidth: 3, startAt: 0, endAt: 85, color: "black", alpha: 1 },
		secondHand:       { lineWidth: 12, startAt: 75, endAt: 90, color: "red", alpha: 0.8 },
		secondDecoration: { lineWidth: 3, startAt: 0, radius: 4, fillColor: "black", color: "black", alpha: 1 }
	},

	Tumb: {
		outerBorder:      { lineWidth: 105, radius: 5, color: "green", alpha: 0.4 },
		smallIndicator:   { lineWidth: 1, startAt: 93, endAt: 98, color: "green", alpha: 1 },
		largeIndicator:   { lineWidth: 50, startAt: 0, endAt: 89, color: "red", alpha: 0.14 },
		hourHand:         { lineWidth: 4, startAt: 0, endAt: 65, color: "black", alpha: 1 },
		minuteHand:       { lineWidth: 3, startAt: 0, endAt: 80, color: "black", alpha: 1 },
		secondHand:       { lineWidth: 1, startAt: 0, endAt: 85, color: "black", alpha: 0.8 },
		secondDecoration: { lineWidth: 5, startAt: 50, radius: 90, fillColor: "black", color: "black", alpha: 0.05 }
	},

	Stone: {
		outerBorder:      { lineWidth: 15, radius: 80, color: "#339933", alpha: 0.5 },
		smallIndicator:   { lineWidth: 2, startAt: 70, endAt: 90, color: "#FF3300", alpha: 0.7 },
		largeIndicator:   { lineWidth: 15, startAt: 0, endAt: 29, color: "#FF6600", alpha: 0.3 },
		hourHand:         { lineWidth: 4, startAt: 0, endAt: 65, color: "black", alpha: 1 },
		minuteHand:       { lineWidth: 3, startAt: 0, endAt: 75, color: "black", alpha: 1 },
		secondHand:       { lineWidth: 1, startAt: 0, endAt: 85, color: "black", alpha: 0.8 },
		secondDecoration: { lineWidth: 5, startAt: 50, radius: 90, fillColor: "black", color: "black", alpha: 0.05 }
	},

	Disc: {
		outerBorder:      { lineWidth: 105, radius: 1, color: "#666600", alpha: 0.2 },
		smallIndicator:   { lineWidth: 1, startAt: 58, endAt: 95, color: "#669900", alpha: 0.8 },
		largeIndicator:   { lineWidth: 6, startAt: 25, endAt: 35, color: "#666600", alpha: 1 },
		hourHand:         { lineWidth: 4, startAt: 0, endAt: 65, color: "black", alpha: 1 },
		minuteHand:       { lineWidth: 3, startAt: 0, endAt: 75, color: "black", alpha: 1 },
		secondHand:       { lineWidth: 1, startAt: -75, endAt: 75, color: "#99CC00", alpha: 0.8 },
		secondDecoration: { lineWidth: 5, startAt: 50, radius: 90, fillColor: "#00FF00", color: "green", alpha: 0.05 }
	},

	// By Yoo Nhe
	watermelon: {
		outerBorder:      { lineWidth: 100, radius: 1.7, color: "#d93d04", alpha: 5 },
		smallIndicator:   { lineWidth: 2, startAt: 50, endAt: 70, color: "#d93d04", alpha: 5 },
		largeIndicator:   { lineWidth: 2, startAt: 45, endAt: 94, color: "#a9bf04", alpha: 1 },
		hourHand:         { lineWidth: 5, startAt: -20, endAt: 80, color: "#8c0d17", alpha: 1 },
		minuteHand:       { lineWidth: 2, startAt: -20, endAt: 80, color: "#7c8c03", alpha: .9 },
		secondHand:       { lineWidth: 2, startAt: 70, endAt: 94, color: "#d93d04", alpha: .85 },
		secondDecoration: { lineWidth: 1, startAt: 70, radius: 3, fillColor: "red", color: "black", alpha: .7 }
	}

	},

	// Test for IE so we can nurse excanvas in a couple of places
	isIE: !!document.all,

	// Will store (a reference to) each clock here, indexed by the id of the canvas element
	clockTracker: {},

	// For giving a unique id to coolclock canvases with no id
	noIdCount: 0
};

// Define the CoolClock object's methods
CoolClock.prototype = {

	// Initialise using the parameters parsed from the colon delimited class
	init: function(options) {
		// Parse and store the options
		this.canvasId       = options.canvasId;
		this.skinId         = options.skinId || CoolClock.config.defaultSkin;
		this.displayRadius  = options.displayRadius || CoolClock.config.defaultRadius;
		this.showSecondHand = typeof options.showSecondHand == "boolean" ? options.showSecondHand : true;
		this.gmtOffset      = (options.gmtOffset != null && options.gmtOffset != '') ? parseFloat(options.gmtOffset) : null;
		this.showDigital    = typeof options.showDigital == "boolean" ? options.showDigital : false;
		this.logClock       = typeof options.logClock == "boolean" ? options.logClock : false;
		this.logClockRev    = typeof options.logClock == "boolean" ? options.logClockRev : false;

		this.tickDelay      = CoolClock.config[ this.showSecondHand ? "tickDelay" : "longTickDelay" ];

		// Get the canvas element
		this.canvas = document.getElementById(this.canvasId);

		// Make the canvas the requested size. It's always square.
		this.canvas.setAttribute("width",this.displayRadius*2);
		this.canvas.setAttribute("height",this.displayRadius*2);
		this.canvas.style.width = this.displayRadius*2 + "px";
		this.canvas.style.height = this.displayRadius*2 + "px";

		// Explain me please...?
		this.renderRadius = CoolClock.config.renderRadius;
		this.scale = this.displayRadius / this.renderRadius;

		// Initialise canvas context
		this.ctx = this.canvas.getContext("2d");
		this.ctx.scale(this.scale,this.scale);

		// Keep track of this object
		CoolClock.config.clockTracker[this.canvasId] = this;

		// Start the clock going
		this.tick();

		return this;
	},

	// Draw a circle at point x,y with params as defined in skin
	fullCircleAt: function(x,y,skin) {
		this.ctx.save();
		this.ctx.globalAlpha = skin.alpha;
		this.ctx.lineWidth = skin.lineWidth;

		if (!CoolClock.config.isIE) {
			this.ctx.beginPath();
		}

		if (CoolClock.config.isIE) {
			// excanvas doesn't scale line width so we will do it here
			this.ctx.lineWidth = this.ctx.lineWidth * this.scale;
		}

		this.ctx.arc(x, y, skin.radius, 0, 2*Math.PI, false);

		if (CoolClock.config.isIE) {
			// excanvas doesn't close the circle so let's fill in the tiny gap
			this.ctx.arc(x, y, skin.radius, -0.1, 0.1, false);
		}

		if (skin.fillColor) {
			this.ctx.fillStyle = skin.fillColor
			this.ctx.fill();
		}
		else {
			// XXX why not stroke and fill
			this.ctx.strokeStyle = skin.color;
			this.ctx.stroke();
		}
		this.ctx.restore();
	},

	// Draw some text centered vertically and horizontally
	drawTextAt: function(theText,x,y) {
		this.ctx.save();
		this.ctx.font = '15px sans-serif';
		var tSize = this.ctx.measureText(theText);
		if (!tSize.height) tSize.height = 15; // no height in firefox.. :(
		this.ctx.fillText(theText,x - tSize.width/2,y - tSize.height/2);
		this.ctx.restore();
	},

	lpad2: function(num) {
		return (num < 10 ? '0' : '') + num;
	},

	tickAngle: function(second) {
		// Log algorithm by David Bradshaw
		var tweak = 3; // If it's lower the one second mark looks wrong (?)
		if (this.logClock) {
			return second == 0 ? 0 : (Math.log(second*tweak) / Math.log(60*tweak));
		}
		else if (this.logClockRev) {
			// Flip the seconds then flip the angle (trickiness)
			second = (60 - second) % 60;
			return 1.0 - (second == 0 ? 0 : (Math.log(second*tweak) / Math.log(60*tweak)));
		}
		else {
			return second/60.0;
		}
	},

	timeText: function(hour,min,sec) {
		var c = CoolClock.config;
		return '' +
			(c.showAmPm ? ((hour%12)==0 ? 12 : (hour%12)) : hour) + ':' +
			this.lpad2(min) +
			(c.showSecs ? ':' + this.lpad2(sec) : '') +
			(c.showAmPm ? (hour < 12 ? ' am' : ' pm') : '')
		;
	},

	// Draw a radial line by rotating then drawing a straight line
	// Ha ha, I think I've accidentally used Taus, (see http://tauday.com/)
	radialLineAtAngle: function(angleFraction,skin) {
		this.ctx.save();
		this.ctx.translate(this.renderRadius,this.renderRadius);
		this.ctx.rotate(Math.PI * (2.0 * angleFraction - 0.5));
		this.ctx.globalAlpha = skin.alpha;
		this.ctx.strokeStyle = skin.color;
		this.ctx.lineWidth = skin.lineWidth;

		if (CoolClock.config.isIE)
			// excanvas doesn't scale line width so we will do it here
			this.ctx.lineWidth = this.ctx.lineWidth * this.scale;

		if (skin.radius) {
			this.fullCircleAt(skin.startAt,0,skin)
		}
		else {
			this.ctx.beginPath();
			this.ctx.moveTo(skin.startAt,0)
			this.ctx.lineTo(skin.endAt,0);
			this.ctx.stroke();
		}
		this.ctx.restore();
	},

	render: function(hour,min,sec) {
		// Get the skin
		var skin = CoolClock.config.skins[this.skinId];
		if (!skin) skin = CoolClock.config.skins[CoolClock.config.defaultSkin];

		// Clear
		this.ctx.clearRect(0,0,this.renderRadius*2,this.renderRadius*2);

		// Draw the outer edge of the clock
		if (skin.outerBorder)
			this.fullCircleAt(this.renderRadius,this.renderRadius,skin.outerBorder);

		// Draw the tick marks. Every 5th one is a big one
		for (var i=0;i<60;i++) {
			(i%5)  && skin.smallIndicator && this.radialLineAtAngle(this.tickAngle(i),skin.smallIndicator);
			!(i%5) && skin.largeIndicator && this.radialLineAtAngle(this.tickAngle(i),skin.largeIndicator);
		}

		// Write the time
		if (this.showDigital) {
			this.drawTextAt(
				this.timeText(hour,min,sec),
				this.renderRadius,
				this.renderRadius+this.renderRadius/2
			);
		}

		// Draw the hands
		if (skin.hourHand)
			this.radialLineAtAngle(this.tickAngle(((hour%12)*5 + min/12.0)),skin.hourHand);

		if (skin.minuteHand)
			this.radialLineAtAngle(this.tickAngle((min + sec/60.0)),skin.minuteHand);

		if (this.showSecondHand && skin.secondHand)
			this.radialLineAtAngle(this.tickAngle(sec),skin.secondHand);

		// Second hand decoration doesn't render right in IE so lets turn it off
		if (!CoolClock.config.isIE && this.showSecondHand && skin.secondDecoration)
			this.radialLineAtAngle(this.tickAngle(sec),skin.secondDecoration);
	},

	// Check the time and display the clock
	refreshDisplay: function() {
		var now = new Date();
		if (this.gmtOffset != null) {
			// Use GMT + gmtOffset
			var offsetNow = new Date(now.valueOf() + (this.gmtOffset * 1000 * 60 * 60));
			this.render(offsetNow.getUTCHours(),offsetNow.getUTCMinutes(),offsetNow.getUTCSeconds());
		}
		else {
			// Use local time
			this.render(now.getHours(),now.getMinutes(),now.getSeconds());
		}
	},

	// Set timeout to trigger a tick in the future
	nextTick: function() {
		setTimeout("CoolClock.config.clockTracker['"+this.canvasId+"'].tick()",this.tickDelay);
	},

	// Check the canvas element hasn't been removed
	stillHere: function() {
		return document.getElementById(this.canvasId) != null;
	},

	// Main tick handler. Refresh the clock then setup the next tick
	tick: function() {
		if (this.stillHere()) {
			this.refreshDisplay()
			this.nextTick();
		}
	}
};

// Find all canvas elements that have the CoolClock class and turns them into clocks
CoolClock.findAndCreateClocks = function() {
	// (Let's not use a jQuery selector here so it's easier to use frameworks other than jQuery)
	var canvases = document.getElementsByTagName("canvas");
	for (var i=0;i<canvases.length;i++) {
		// Pull out the fields from the class. Example "CoolClock:chunkySwissOnBlack:1000"
		var fields = canvases[i].className.split(" ")[0].split(":");
		if (fields[0] == "CoolClock") {
			if (!canvases[i].id) {
				// If there's no id on this canvas element then give it one
				canvases[i].id = '_coolclock_auto_id_' + CoolClock.config.noIdCount++;
			}
			// Create a clock object for this element
			new CoolClock({
				canvasId:       canvases[i].id,
				skinId:         fields[1],
				displayRadius:  fields[2],
				showSecondHand: fields[3]!='noSeconds',
				gmtOffset:      fields[4],
				showDigital:    fields[5]=='showDigital',
				logClock:       fields[6]=='logClock',
				logClockRev:    fields[6]=='logClockRev'
			});
		}
	}
};

// If you don't have jQuery then you need a body onload like this: <body onload="CoolClock.findAndCreateClocks()">
// If you do have jQuery and it's loaded already then we can do it right now
if (window.jQuery) jQuery(document).ready(CoolClock.findAndCreateClocks);
