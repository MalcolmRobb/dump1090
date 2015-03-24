var listKMLType = ['Approch', 'Departure', 'Transit', 'Custom1', 'Custom2'];
var listKMLs = localStorage['listKMLs'] || [];

function optionsInitalize() {
	// Write your initalization here
	// Gets called just before the 1-sec function call loop is setup
	$( "#dialog-modal" ).dialog({
		height: 450,
		width:  950,
		modal: true,
		autoOpen: false,
		closeOnEscape: true

	});
}

function optionsModal() {
	$( "#dialog-modal" ).dialog({autoOpen:true,resizable: false,width:950,height:590, beforeClose: function(event, ui) { 
               location.reload();
	  
	},});
}
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
