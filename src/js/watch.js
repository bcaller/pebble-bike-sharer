var api = require("citybikes");
var loc = require("location");
var haversine = require("haversine");
var LIST_LENGTH = 5;

var lastIds, olddict;

function sameIds(stations) {
	for(var i=0; i<LIST_LENGTH; i++) {
		if(lastIds[i] != stations[i].id) return false;
	}
	return true;
}

function sendList() {
	var stations = api.nearestStations(loc.lastPosition);
	if(!stations) return;
	var dict = {bikes:[],slots:[]};
	if(lastIds && !api.getAndClearErrorFlag() && sameIds(stations)) {
		// Just send numbers
		var hasNewData = false;
		for(var i=0; i<LIST_LENGTH; i++) {
			dict["distance_"+i] = Math.round(haversine.dist(stations[i], loc.lastPosition));
			dict["heading_"+i] = Math.round(haversine.bearing(stations[i], loc.lastPosition));
			dict.bikes.push(Math.min(stations[i].bikes, 255)); //uint8_t
			dict.slots.push(Math.min(stations[i].slots, 255));
			if(!olddict || dict.bikes[i] != olddict.bikes[i] || dict.slots[i] != olddict.slots[i])
				hasNewData = true;
		}
		if(!hasNewData) return console.log("No new data");
	} else {
		// Send everything
		lastIds = [];
		for(var i=0; i<LIST_LENGTH; i++) {
			var s = stations[i];
			dict["name_"+i] = s.name.substring(0,32);
			if(s.address)
				dict["address_"+i] = s.address.substring(0,64);
			dict["distance_"+i] = Math.round(haversine.dist(s, loc.lastPosition));
			dict["heading_"+i] = Math.round(haversine.bearing(s, loc.lastPosition));
			dict.bikes.push(Math.min(s.bikes, 255)); //uint8_t
			dict.slots.push(Math.min(s.slots, 255));
			lastIds.push(s.id);
			olddict = dict;
		}
	}
	console.log("Sending: " + JSON.stringify(dict).substring(0, 200));
	Pebble.sendAppMessage(dict, function() {
		console.log("Send to watch");
	}, function(err) {
		console.log("error sending to watch", err);
	});
}

Pebble.addEventListener('appmessage', function(e) {
	sendList();
});

module.exports.sendList = sendList;