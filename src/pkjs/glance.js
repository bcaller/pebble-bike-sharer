function appGlanceSuccess(appGlanceSlices, appGlanceReloadResult) {}

function appGlanceFailure(appGlanceSlices, appGlanceReloadResult) {
    console.log('AppGlance failed!');
}

// Trigger a reload of the slices in the app glance
module.exports = function(networkName, n_bikes, n_spaces, dist) {
	if(networkName) {
		console.log("glancing " + networkName +" "+ n_bikes+" "+n_spaces+" "+dist);
		var appGlanceSlices = [{
				"layout": {
					"subtitleTemplateString": n_bikes + " bikes, " + n_spaces + " spaces within " + dist + "m"
				},
				"expirationTime": new Date((new Date() * 1) + 4 * 60 * 1000).toISOString() // 4 minutes
			}, {
				"layout": {
					"subtitleTemplateString": networkName
				}
		}];
		Pebble.appGlanceReload(appGlanceSlices, appGlanceSuccess, appGlanceFailure);
	} else {
		console.log("clearing glance");
		Pebble.appGlanceReload([], appGlanceSuccess, appGlanceFailure);
	}
};