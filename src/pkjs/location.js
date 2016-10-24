var LL = require('./haversine'),
	keys = require('message_keys'),
	glance = require("./glance"),
	watch = null;

module.exports.lastPosition = null;

function reportLocationError() {
	if(module.exports.lastPosition)
		return;
	var dict = {}; dict[keys.ERR] = 7;
	Pebble.sendAppMessage(dict, function() {
		console.log("Send err_no_location to watch");
	}, function(err) {
		console.log("error sending to watch");
	});
	glance();
}

function beginWatch(onUpdate) {
	if(watch) return;
	watch = "ok";
	
	var update = function(pos) {
		if(pos && pos.coords) {
			var coords = LL(pos.coords);
			/* FIXME HACK*/
			//coords = LL({latitude: 48.863388, longitude: 2.386694});
			//pos.coords = {latitude: 48.8584, longitude: 2.2945};
			//coords = LL({latitude: 40.7527, longitude: -73.9772});
			//coords = {latitude: 40.7425993836325, longitude: -74.0322035551};
			//coords = LL({latitude: 42.350406, longitude: -71.108279}); //Hubway
			/**/
			//coords = LL({latitude: 55.678373, longitude: 37.5745946});
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
			update, function posErr() {
				// PebbleKit JS API does not exactly follow w3 spec for error handling
				stopPositionUpdates();
				reportLocationError();
				// Restart position updates
				actuallyWatch();
			},
			{timeout: 15e3, maximumAge: 45e3, enableHighAccuracy: true}
		);
	};
	
	// First get location with a longer timeout and maybe less accurate to choose network
	navigator.geolocation.getCurrentPosition(
		function(pos) {
			update(pos);
			actuallyWatch();
		}, function() {
			reportLocationError();
			actuallyWatch();
		},
		{timeout: 30e3, maximumAge: 15 * 60 * 1000}
	);
}

function stopPositionUpdates() {
	if(watch && watch != "ok") {
		navigator.geolocation.clearWatch(watch);
		watch = null;
	}
}

module.exports.init = beginWatch;
module.exports.stopPositionUpdates = stopPositionUpdates;