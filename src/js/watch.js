var api = require("./citybikes"),
	loc = require("./location"),
	LL = require('./haversine'),
	keys = require('message_keys'),
	LIST_LENGTH = 6,
	lastIds, olddict;

function sameIds(stations) {
	for(var i=0; i<LIST_LENGTH; i++) {
		if(lastIds[i] != stations[i].id) return false;
	}
	return true;
}

function sendList() {
	var stations = api.nearestStations(loc.lastPosition);
	if(!stations) return;
	var dict = {}; dict[keys.BIKES] = []; dict[keys.SLOTS] = [];
	if(lastIds && !api.getAndClearErrorFlag() && sameIds(stations)) {
		// Just send numbers
		var hasNewData = false;
		for(var i=0; i<LIST_LENGTH; i++) {
			var s = stations[i];
			dict[keys.DISTANCES+i] = Math.round(LL(s).distanceTo(loc.lastPosition));
			dict[keys.HEADINGS+i] = Math.round(loc.lastPosition.bearingTo(LL(s)));
			dict[keys.BIKES].push(Math.min(s.bikes, 255)); //uint8_t
			dict[keys.SLOTS].push(Math.min(s.slots, 255));
			if(!olddict || dict[keys.BIKES][i] != olddict[keys.BIKES][i] || dict[keys.SLOTS][i] != olddict[keys.SLOTS][i])
				hasNewData = true;
		}
		if(!hasNewData) return console.log("No new data");
	} else {
		// Send everything
		lastIds = [];
		for(var i=0; i<LIST_LENGTH; i++) {
			var s = stations[i];
			dict[keys.NAMES+i] = s.name.substring(0,32);
			if(s.address)
				dict[keys.ADDRESSES+i] = s.address.substring(0,64);
			dict[keys.DISTANCES+i] = Math.round(LL(s).distanceTo(loc.lastPosition));
			dict[keys.HEADINGS+i] = Math.round(loc.lastPosition.bearingTo(LL(s)));
			dict[keys.BIKES].push(Math.min(s.bikes, 255)); //uint8_t
			dict[keys.SLOTS].push(Math.min(s.slots, 255));
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