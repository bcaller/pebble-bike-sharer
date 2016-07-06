var LL = require('./haversine');
var noop = function(){};
var watch = null;

function beginWatch(onUpdate, onErr) {
	if(watch) return;
	watch = "ok";
	
	var update = function(pos) {
		if(pos && pos.coords) {
			var coords = LL(pos.coords);
			/* FIXME HACK*/
			coords = LL({latitude: 48.863388, longitude: 2.386694});
			//pos.coords = {latitude: 48.8584, longitude: 2.2945};
			//coords = {latitude: 40.7527, longitude: -73.9772};
			//coords = {latitude: 40.7425993836325, longitude: -74.0322035551};
			/**/
			//coords = {latitude: 48.8584, longitude: 2.2945};
			if(!module.exports.lastPosition || coords.distanceTo(module.exports.lastPosition) > 7) {
				module.exports.lastPosition = coords;
				onUpdate(coords);
			} else {
				//console.log("Position too close for update")
			}
		}
	};
	var actuallyWatch = function() {
		watch = navigator.geolocation.watchPosition(
			update, onErr || noop,
			{timeout: 15000, maximumAge: 45000, enableHighAccuracy: true}
		);
	};
	
	// First get location with a longer timeout and maybe less accurate to choose network
	navigator.geolocation.getCurrentPosition(
		function(pos) {
			update(pos);
			actuallyWatch();
		}, function(err) {
			if(onErr) onErr(err);
			actuallyWatch();
		},
		{timeout: 45 * 1000, maximumAge: 15 * 60 * 1000}
	);
}

module.exports.init = beginWatch;
module.exports.lastPosition = null;