(function(){/*


 Copyright 2011 Google Inc.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/
var d="prototype";function e(a){this.set("fontFamily","sans-serif");this.set("fontSize",12);this.set("fontColor","#000000");this.set("strokeWeight",4);this.set("strokeColor","#ffffff");this.set("align","center");this.set("zIndex",1E3);this.setValues(a)}e.prototype=new google.maps.OverlayView;window.MapLabel=e;e[d].changed=function(a){switch(a){case "fontFamily":case "fontSize":case "fontColor":case "strokeWeight":case "strokeColor":case "align":case "text":return h(this);case "maxZoom":case "minZoom":case "position":return this.draw()}};
function h(a){var b=a.a;if(b){var f=b.style;f.zIndex=a.get("zIndex");var c=b.getContext("2d");c.clearRect(0,0,b.width,b.height);c.strokeStyle=a.get("strokeColor");c.fillStyle=a.get("fontColor");c.font=a.get("fontSize")+"px "+a.get("fontFamily");var b=Number(a.get("strokeWeight")),g=a.get("text");if(g){if(b)c.lineWidth=b,c.strokeText(g,b,b);c.fillText(g,b,b);a:{c=c.measureText(g).width+b;switch(a.get("align")){case "left":a=0;break a;case "right":a=-c;break a}a=c/-2}f.marginLeft=a+"px";f.marginTop=
"-0.4em"}}}e[d].onAdd=function(){var a=this.a=document.createElement("canvas");a.style.position="absolute";var b=a.getContext("2d");b.lineJoin="round";b.textBaseline="top";h(this);(b=this.getPanes())&&b.mapPane.appendChild(a)};e[d].onAdd=e[d].onAdd;
e[d].draw=function(){var a=this.getProjection();if(a&&this.a){var b=this.get("position");if(b){b=a.fromLatLngToDivPixel(b);a=this.a.style;a.top=b.y+"px";a.left=b.x+"px";var b=this.get("minZoom"),f=this.get("maxZoom");if(b===void 0&&f===void 0)b="";else{var c=this.getMap();c?(c=c.getZoom(),b=c<b||c>f?"hidden":""):b=""}a.visibility=b}}};e[d].draw=e[d].draw;e[d].onRemove=function(){var a=this.a;a&&a.parentNode&&a.parentNode.removeChild(a)};e[d].onRemove=e[d].onRemove;})()
