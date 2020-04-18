/**
 * Subscriber
 */
function Subscriber() {
}

Subscriber.prototype = {
	receive: function(message) {
	}
}

/**
 * Publisher
 */
function Publisher() {
	this._subscribers = [];
}

Publisher.prototype = {
	publish: function(message) {
		for(var i=0; i<this._subscribers.length; i++) {
			var subscriber = this._subscribers[i];
			subscriber.receive(message);
		}
	},
	add: function(subscriber) {
		this._subscribers.push(subscriber);
	}
}