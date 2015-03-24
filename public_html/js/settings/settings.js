
function storevalue(valuename,value) {
      var storeval = localStorage.setItem(valuename, value);
  }
  function getvalue(valuename) {
      var value = localStorage.getItem(valuename);
      return value
  } 
function validateFormrgb(form,name1,name2,name3) {
    var r = document.forms[form][name1].value;
    var g = document.forms[form][name2].value;
    var b = document.forms[form][name3].value;
    if (r == null || r == "" || g == null || g == "" || b == null || b == "") {
        //alert("R,G,B must be filled out");
        return false;
    }
    storevalue(name1,r);
    storevalue(name2,g);
    storevalue(name3,b);
}
function validateFormlatlon(form,name1,name2) {
    var lat = document.forms[form][name1].value;
    var lon = document.forms[form][name2].value;
    if (lat == null || lat == "" || lon == null || lon == "") {
        //alert("Lat and Lon must be filled out");
        return false;
    }
    storevalue(name1,lat);
    storevalue(name2,lon);
}
function componentToHex(c) {
	    var hex = c.toString(16);
	    return hex.length == 1 ? "0" + hex : hex;
	}

	function rgbToHex(r, g, b) {
	    return "#" + componentToHex(r) + componentToHex(g) + componentToHex(b);
	}

function hexToRgb(hex) {
    var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
    } : null;
}
function setup(){
  fr24();
  flightaware();
  flightstats();
  dark();
  metric();
  showsite();
  showcircles();
  airplanemarker();
  airplanemarkera();
  coolclock1();
  normalclock1();
  planedata1();
}
color1 = rgbToHex( Number(getvalue('setr')), Number(getvalue('setg')), Number(getvalue('setb'))  );
color2 = rgbToHex( Number(getvalue('set2r')), Number(getvalue('set2g')), Number(getvalue('set2b'))  );
  function flightaware(){
		  if(getvalue('flightawareshow')){
		    document.getElementById("flightaware").style.background=color1;//green//true
		    document.getElementById("flightaware1").style.background=color2;//red//false
		  }else{
		    document.getElementById("flightaware").style.background=color2;//red//true
		    document.getElementById("flightaware1").style.background=color1;//green//false
}}
function planedata1(){
		  if(getvalue('planedata')){
		    document.getElementById("planedata").style.background=color1;//green//true
		    document.getElementById("planedata1").style.background=color2;//red//false
		  }else{
		    document.getElementById("planedata").style.background=color2;//red//true
		    document.getElementById("planedata1").style.background=color1;//green//false
}}
 function normalclock1(){
		  if(getvalue('normalclock')){
		    document.getElementById("normalclock").style.background=color1;//green//true
		    document.getElementById("normalclock1").style.background=color2;//red//false
		  }else{
		    document.getElementById("normalclock").style.background=color2;//red//true
		    document.getElementById("normalclock1").style.background=color1;//green//false
}}
  function coolclock1(){
		  if(getvalue('coolclock')){
		    document.getElementById("coolclock").style.background=color1;//green//true
		    document.getElementById("coolclock1").style.background=color2;//red//false
		  }else{
		    document.getElementById("coolclock").style.background=color2;//red//true
		    document.getElementById("coolclock1").style.background=color1;//green//false
}}
function fr24(){
		    if(getvalue('fr24show')){
		      document.getElementById("fr24").style.background=color1;//green//true
		      document.getElementById("fr241").style.background=color2;//red//false
		    }else{
		      document.getElementById("fr24").style.background=color2;//red//true
		      document.getElementById("fr241").style.background=color1;//green//false
}}
		    
function flightstats(){
		    if(getvalue('flightstatsshow')){
		      document.getElementById("flightstats").style.background=color1;//green//true
		      document.getElementById("flightstats1").style.background=color2;//red//false
		    }else{
		      document.getElementById("flightstats").style.background=color2;//red//true
		      document.getElementById("flightstats1").style.background=color1;//green//false
}}  
function dark(){
		    if(getvalue('darkmode')){
		      document.getElementById("dark").style.background=color1;//green//true
		      document.getElementById("dark1").style.background=color2;//red//false
		    }else{
		      document.getElementById("dark").style.background=color2;//red//true
		      document.getElementById("dark1").style.background=color1;//green//false
}} 
function metric(){
		    if(getvalue('metric')){
		      document.getElementById("metric").style.background=color1;//green//true
		      document.getElementById("metric1").style.background=color2;//red//false
		    }else{
		      document.getElementById("metric").style.background=color2;//red//true
		      document.getElementById("metric1").style.background=color1;//green//false
}}
function showsite(){
		    if(getvalue('showsite')){
		      document.getElementById("showsite").style.background=color1;//green//true
		      document.getElementById("showsite1").style.background=color2;//red//false
		    }else{
		      document.getElementById("showsite").style.background=color2;//red//true
		      document.getElementById("showsite1").style.background=color1;//green//false
}}
function showcircles(){
		    if(getvalue('showcircles')){
		      document.getElementById("showcircles").style.background=color1;//green//true
		      document.getElementById("showcircles1").style.background=color2;//red//false
		    }else{
		      document.getElementById("showcircles").style.background=color2;//red//true
		      document.getElementById("showcircles1").style.background=color1;//green//false
}}
function airplanemarker(){
		    if(getvalue('airplanemarker_showtext')){
		      document.getElementById("airplanemarker").style.background=color1;//green//true
		      document.getElementById("airplanemarker1").style.background=color2;//red//false
		    }else{
		      document.getElementById("airplanemarker").style.background=color2;//red//true
		      document.getElementById("airplanemarker1").style.background=color1;//green//false
}}
function airplanemarkera(){
		    if(getvalue('airplanemarker_showextended')){
		      document.getElementById("airplanemarkera").style.background=color1;//green//true
		      document.getElementById("airplanemarkera1").style.background=color2;//red//false
		    }else{
		      document.getElementById("airplanemarkera").style.background=color2;//red//true
		      document.getElementById("airplanemarkera1").style.background=color1;//green//false
}}

$(window).load(function() {
		// Animate loader off screen
		$(".se-pre-con").fadeOut("slow");;
	});

