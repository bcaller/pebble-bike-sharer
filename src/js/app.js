var api = require("citybikes");
var location = require("location");
var watch = require("watch");

function onLocationUpdate(coords) {
	location.lastPosition = coords;
	if(api.setNetwork(coords)) {
		watch.sendList(coords);
	}
}

api.setOnStationsUpdated(function onStationUpdate(stations) {
	console.log("onStationUpdate");
	if(location.lastPosition)
		onLocationUpdate(location.lastPosition);
});

location.init(onLocationUpdate);

api.init(function() {
	if(location.lastPosition) {
		onLocationUpdate(location.lastPosition);
	}
});