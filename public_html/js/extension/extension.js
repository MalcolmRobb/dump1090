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
        displayTime();
	displayTimegmt();
}

function extendedPulse() {
	// This will get called every second after all the main functions
}

function displayTime() {
        if(getvalue('normalclock')){
	  time = new Date();
	  var hours = time.getHours();
	  var minutes = time.getMinutes();
	  var seconds = time.getSeconds();
	  if (hours < 10) {
	      hours = '0' + hours;
	  }
	  if (minutes < 10) {
	      minutes = '0' + minutes;
	  }
	  if (seconds < 10) {
	      seconds = '0' + seconds;
	  }
	  var html = '';
	  html += '<table width="100%">';
	  html += '<td><tr>Local Time: ' + hours + ":" + minutes + ":" + seconds;
	  html += '</td></tr>';
	  html += '</table>';
	  document.getElementById('time').innerHTML = html;
	  setTimeout('displayTime()',1000);
	}else{
	  return true; 
	}
}
function displayTimegmt() {
        if(getvalue('normalclock')){
	  time = new Date();
	  var hours = time.getUTCHours();
	  var minutes = time.getUTCMinutes();
	  var seconds = time.getUTCSeconds();
	  if (hours < 10) {
	      hours = '0' + hours;
	  }
	  if (minutes < 10) {
	      minutes = '0' + minutes;
	  }
	  if (seconds < 10) {
	      seconds = '0' + seconds;
	  }
	  var html = '';
	  html += '<table width="100%">';
	  html += '<td><tr>UTC Time: ' + hours + ":" + minutes + ":" + seconds;       
	  html += '</td></tr>';
	  html += '</table>';
	  document.getElementById('timegmt').innerHTML = html;
	  setTimeout('displayTimegmt()',1000);
	}else{
	 return true; 
	}
}
function coolclock(){
  var html = '';
	html +='<td>Local Time</td>';
	html+='<td>';
	html+='<canvas id="localclock" class="CoolClock:classic:40"></canvas>';
	html+='</td>';
	html+='<td">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;UTC Time</td>';
	html+='<td>';
	html+='<canvas style="margin-left:10px;" id="gmtclock" class="CoolClock:classic:40::0"></canvas>';
	html+='</td>'; 
	document.getElementById('timestamps').innerHTML = html;
}
function normalclock(){
	var html = '';
	html+='<div style="display:inline-block;" id="time"></div>';
	html+='<div style="display:inline-block;margin-left:150px;" id="timegmt"></div>';
	document.getElementById('normalclockcanvas').innerHTML = html;
}
