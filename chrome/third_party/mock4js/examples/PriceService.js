/**
 * PriceCache
 */
function PriceCache() {
}

PriceCache.prototype = {
	getCachedPrice: function(instrumentId) {
	},
	setCachedPrice: function(instrumentId, price) {
	}
}

/**
 * PriceFetcher
 */
function PriceFetcher() {
}

PriceFetcher.prototype = {
	getPriceFromServer: function(instrumentId) {
	}
}


/**
 * PriceService
 */
function PriceService(priceFetcher, priceCache) {
	this._priceFetcher = priceFetcher;
	this._priceCache = priceCache;
}

PriceService.prototype = {
	getPrice: function(instrumentId) {
		var price = this._priceCache.getCachedPrice(instrumentId);
		if(price==null) {
			price = this._priceFetcher.getPriceFromServer(instrumentId);
			this._priceCache.setCachedPrice(instrumentId, price);
		}
		return price;
	}
}
