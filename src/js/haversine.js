var LatLon = require('geodesy-spherical');

var ll = module.exports = function(z) { return new LatLon(z.latitude||z.lat, z.longitude||z.lon); }

function bearing(dest, orig) {
	return ll(orig).bearingTo(ll(dest));
}

module.exports.dist = function distance(z, q) {
    return ll(z).distanceTo(ll(q));
};
module.exports.bearing = bearing;