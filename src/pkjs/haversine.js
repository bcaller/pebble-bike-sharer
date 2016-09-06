var LatLon = require('geodesy-spherical');
module.exports = function(z) { return new LatLon(z.latitude||z.lat, z.longitude||z.lon); }