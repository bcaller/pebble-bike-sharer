function mapAsync(arr, f, cb) {
	var len = arr.length;
	var d = new Array(len);
	function doneOne(j) {
		return function(res) {
			d[j] = res;
			if(--len===0) {
				cb(d);
			}
		};
	}
	for(var i=0; i<arr.length; i++) {
		f(arr[i], doneOne(i));
	}
}


module.exports.mapAsync = mapAsync;