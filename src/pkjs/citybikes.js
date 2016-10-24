var LL = require('./haversine'),
	keys = require('message_keys'),
	glance = require("./glance"),
	location = require("./location"),
	BASE = "http://api.citybik.es",
	NETWORKS_URL = "/v2/networks?fields=name,href,location",
	CHOSEN_KEY = "chosenxx",
	STATION_DATA_KEY = "stdata",
	networks,
	chosenNetwork = false,
	lastRequestedNetworkName = null,
	stations = null,
	stationsLastUpdate,
	STATIONS_UPDATE_INTERVAL = 30 * 1000, //30 seconds
	stations_sorted = false,
	onStationsUpdated,
	DIST_MAX = 40 * 1000, // 40km
	had_prechosen_network = false,
	stations_dict,
	updater_interval,
	has_errored = false,
	mapAsync = require('./async').mapAsync;

var httpCache = {};
var request = function (url, timeout, callback) {
	console.log("GET " + url);
	if(url in httpCache) {
		console.log("Returning from cache");
		callback(httpCache[url]);
		delete httpCache[url];
		return;
	}
	var xhr = new XMLHttpRequest();
	if(timeout) xhr.timeout = timeout;
	xhr.onload = function () {
		callback(JSON.parse(this.responseText));
	};
	xhr.onerror = xhr.ontimeout = reportNoInternet;
	xhr.open('GET', url);
	xhr.send();
};

function updateNetworks(cb) {
	if(!localStorage.getItem(CHOSEN_KEY)) {
		// On first run
		// Or on not finding a nearby network
		localStorage.clear();
		had_prechosen_network = false;
		request(BASE + NETWORKS_URL, 40000, function(data) {
			networks = [];
			for(var i=0; i< data.networks.length; i++) {
				var network = data.networks[i];
				networks.push({
					url: network.href,
					latitude: network.location.latitude,
					longitude: network.location.longitude,
					name: network.name
				});
			}
			console.log(networks.length + " networks downloaded");
			cb();
		});
	} else {
		// We have pre-chosen a network
		had_prechosen_network = true;
		try {
			networks = [JSON.parse(localStorage.getItem(CHOSEN_KEY))];
			if(networks[0] && networks[0].url)
				cb();
			else {
				networks = null;
				throw new Error("Bad cache");
			}
		} catch(err) {
			console.log("Error with localStorage reading network", err);
			localStorage.clear();
			updateNetworks(cb);
		}
	}
}

var busyWaitingForRequests = false;
function setNetwork(coords) {
	stations_sorted = false;
	if(!chosenNetwork && !busyWaitingForRequests && networks) {
		//Find nearest network
		var bestNetwork = networks[0];
		var bestDist = coords.distanceTo(LL(networks[0]));
		var nearbyNetworks = [];
		for(var i=1; i< networks.length; i++) {
			var d = coords.distanceTo(LL(networks[i]));
			//console.log(JSON.stringify(networks[i]) + " " + d);
			if(d < bestDist) {
				bestNetwork = networks[i];
				bestDist = d;
			}
			
			if(d < DIST_MAX) {
				nearbyNetworks.push(networks[i]);
			}
		}

		if(bestDist > DIST_MAX) {
			console.log("No nearby network");
			localStorage.clear();
			if(had_prechosen_network) {
				console.log("Downloading full list of networks");
				updateNetworks(function(){setNetwork(coords);});
			} else {
				location.stopPositionUpdates();  // Die
				reportNoBikeNetworks();
			}
			return false;
		} else if(networks.length > 1) {
			if(coords.distanceTo(LL(networks[0])) < DIST_MAX) {
				nearbyNetworks.push(networks[i]);
			}
			if(nearbyNetworks.length > 1) {
				console.log("NEARBY: " + JSON.stringify(nearbyNetworks));
				return decideMultipleNearby(nearbyNetworks, coords);
			}
		}

		//Commit to network
		chosenNetwork = bestNetwork;
		var asStr = JSON.stringify(chosenNetwork);
		localStorage.setItem(CHOSEN_KEY, asStr);
		console.log("Chosen: " + asStr);

		//Start updater
		clearInterval(updater_interval);
		updater_interval = setInterval(updateStations, STATIONS_UPDATE_INTERVAL);
		updateStations();
	}
	return chosenNetwork;
}


function decideMultipleNearby(nbNetworks, coords) {
	busyWaitingForRequests = true;
	mapAsync(nbNetworks,
			 function(n, cb) {
				 request(BASE + n.url, STATIONS_UPDATE_INTERVAL, function(data) {
					 httpCache[BASE + n.url] = data;
					 var nearest = DIST_MAX;
					 for(var s=0; s< data.network.stations.length; s++) {
						 if(isActiveStation(data.network.stations[s])) {
							 var d = coords.distanceTo(LL(data.network.stations[s]));
							 if(d < nearest) nearest = d;
						 }
					 }
					 cb(nearest);
				 });
			 },
			 function(nets) {
				 var bestNetwork = null;
				 var bestDist = DIST_MAX;
				 for(var n=0; n< nets.length; n++) {
					 var nearest = nets[n];
					 if(nearest < bestDist) {
						 bestNetwork = n;
						 bestDist = nearest;
					 }
				 }
				 
				 busyWaitingForRequests = false;
				 if(bestNetwork!==null) {
					 networks = [nbNetworks[bestNetwork]];
					 console.log("Networks now set to " + JSON.stringify(networks));
					 setNetwork(coords);
				 } else {
					 console.error("WTF no best network");
					 //FIXME: TODO: Choice dialog
				 }
			 });
}


function isActiveStation(station) {
	if(!station.empty_slots && !station.free_bikes) return false;
	if(station.empty_slots <= 0 && station.free_bikes <= 0) return false;
	if(!station.extra) return true;
	if("installed" in station.extra && !station.extra.installed) return false;
	if("status" in station.extra) {
		var status = station.extra.status.toUpperCase();
		if(status == 'CLOSED' || status == 'OFFLINE') return false;
	}
	if("locked" in station.extra && station.extra.locked) return false;
	if("testStation" in station.extra && station.extra.testStation) return false;
	if("statusValue" in station.extra && station.extra.statusValue == 'Not In Service') return false;
	if("online" in station.extra && station.extra.online === false) return false;

	return true;
}

function niceAddress(addr) {
	if(!addr) return;
	return addr.replace(/\bROAD\b/i, "Rd").replace(/\bBOULEVARD\b/i, "Bd").replace(/\bSTREET\b/i, "St");
}

function clampByte(x) {
	return Math.max(0, Math.min(254, x));
}

function newStation(station) {
	var s = {
		id: station.id,
		latitude: station.latitude,
		longitude: station.longitude,
		name: station.extra.name || station.name.replace(/^\d{3,} - /, ""),
		bikes: clampByte(station.free_bikes),
		slots: clampByte(station.empty_slots),
		address: niceAddress(station.extra.address || station.extra.location || station.extra.stAddress2 || station.extra.description || chosenNetwork.name)
	};
	if(s.address == s.name)
		delete s.address;
	return s;
}

function updateStations(network) {
	console.log("Making request for stations");

	request(BASE + (network || chosenNetwork).url, STATIONS_UPDATE_INTERVAL - 3000, function(data) {
		stations_sorted = false;
		lastRequestedNetworkName = data.network.name;
		var s = data.network.stations;
		stations = [];
		stations_dict = {};
		stationsLastUpdate = new Date();
		for(var i=0; i< s.length; i++) {
			if(isActiveStation(s[i])) {
				var station = newStation(s[i]);
				stations.push(station);
				stations_dict[station.id] = station;
			}
		}

		console.log("We have " + stations.length + " active stations.");
		if(onStationsUpdated) onStationsUpdated(stations);

		localStorage.setItem(STATION_DATA_KEY, JSON.stringify({
			lut: (Date.now() + 0),
			stations: stations,
			name: lastRequestedNetworkName
		}));
	});

	if(!stations && had_prechosen_network) {
		console.log("Let's check the cache in the meantime");
		var cachedStr = localStorage.getItem(STATION_DATA_KEY);
		if(cachedStr) {
			try {
				var cached = JSON.parse(cachedStr);
				if(cached.lut && cached.stations && cached.lut > (Date.now() - 5 * 60 * 1000)) {
					lastRequestedNetworkName = cached.name;
					console.log("cached lrnn " + lastRequestedNetworkName);
					stations = cached.stations;
					stations_dict = {};
					for(var i=0; i< stations.length; i++) {
						stations_dict[stations[i].id] = stations[i];
					}
					console.log("We have cached " + stations.length + " active stations.");
					if(onStationsUpdated) onStationsUpdated(stations);
				}
			} catch(err) {}
		}
	}
}

function nearestStations(coords) {
	if(stations) {
		if(!(stations_sorted && stations_sorted[0] == coords.latitude && stations_sorted[1] == coords.longitude)) {
			stations.sort(function(a, b) {
				return coords.distanceTo(LL(a)) - coords.distanceTo(LL(b));
			});
			stations_sorted = [coords.latitude, coords.longitude];
		}

		if(had_prechosen_network && coords.distanceTo(LL(stations[0])) > 3333) {
			console.error("We appear to have moved to a nearby network");
			had_prechosen_network = false;
			localStorage.clear();
		}
		//console.log(JSON.stringify(stations[0]));
		//console.log(JSON.stringify(stations[1]));
	}
	return stations;
}

// If already sorted, performance is ok
function stationById(id) {
	for(var i=0; i< stations.length; i++) {
		if(stations[i].id == id) return stations[i]; 
	}
}

function setOnStationsUpdated(cb) {
	onStationsUpdated = cb;
}

function reportNoBikeNetworks() {
	var dict = {}; dict[keys.ERR] = 4;
	Pebble.sendAppMessage(dict, function() {
		console.log("Send err_no_bike_networks to watch");
	}, function(err) {
		console.log("error sending to watch", err);
	});
	glance();
}

function reportNoInternet() {
	has_errored = true;
	var dict = {}; dict[keys.ERR] = 3;
	Pebble.sendAppMessage(dict, function() {
		console.log("Send err_no_internet to watch");
	}, function(err) {
		console.log("error sending to watch", err);
	});
	glance();
}

module.exports.init = updateNetworks;
module.exports.setNetwork = setNetwork;
module.exports.updateStations = updateStations;
module.exports.nearestStations = nearestStations;
module.exports.setOnStationsUpdated = setOnStationsUpdated;
module.exports.stationById = stationById;
module.exports.getAndClearErrorFlag = function() {
	var old_err = has_errored;
	has_errored = false;
	return old_err;
};
module.exports.getNetworkName = function() {
	console.log("get lrnn " + lastRequestedNetworkName)
	return lastRequestedNetworkName;
};
