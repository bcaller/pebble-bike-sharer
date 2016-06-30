//From movable type

function d2r(d) { return d * Math.PI / 180; }
function r2d(r) { return r * 180 / Math.PI; }

function bearing(dest, orig) {
    var φ1 = d2r(orig.latitude), φ2 = d2r(dest.latitude);
    var Δλ = d2r(dest.longitude-orig.longitude);

    // see http://mathforum.org/library/drmath/view/55417.html
    var y = Math.sin(Δλ) * Math.cos(φ2);
    var x = Math.cos(φ1)*Math.sin(φ2) -
            Math.sin(φ1)*Math.cos(φ2)*Math.cos(Δλ);
    var θ = Math.atan2(y, x);

    return (r2d(θ)+360) % 360;
}

function distance(z, q) {
    var radius = 6371e3;

    var R = radius;
    var φ1 = d2r(z.latitude),  λ1 = d2r(z.longitude);
    var φ2 = d2r(q.latitude), λ2 = d2r(q.longitude);
    var Δφ = φ2 - φ1;
    var Δλ = λ2 - λ1;

    var a = Math.sin(Δφ/2) * Math.sin(Δφ/2) +
            Math.cos(φ1) * Math.cos(φ2) *
            Math.sin(Δλ/2) * Math.sin(Δλ/2);
    var c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
    var d = R * c;

	return d;
}

module.exports.dist = distance;
module.exports.bearing = bearing;