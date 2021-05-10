const expect = require('expect');

var Particle = require('particle-api-js');
var particle = new Particle();

(function(eventMonitor) {

    eventMonitor.monitors = [];

    eventMonitor.events = [];

    eventMonitor.resetEvents = function() {
        eventMonitor.events = [];
    };

    
    eventMonitor.run = function(config) {
        if (!config.accessToken) {
            console.log('missing accessToken in config.json');
            return false;
        }
        if (!config.deviceId) {
            console.log('missing deviceId in config.json');
            return false;
        }
        
        eventMonitor.config = config;

        particle.getEventStream({ deviceId: eventMonitor.config.deviceId, auth: eventMonitor.config.accessToken }).then(function (stream) {
            console.log('event stream opened');
            eventMonitor.readyResolve();
        
            stream.on('event', function (data) {
                console.log("Event: ", data);

                // data.data, data.ttl, data.published_at, data.coreid, data.name
                // published_at: '2021-04-20T15:05:37.885Z'

                eventMonitor.events.push(data);


                for(let ii = 0; ii < eventMonitor.monitors.length; ii++) {
                    let mon = eventMonitor.monitors[ii];
                    mon.resolveData = data; 
                    if (mon.checkEvent(data)) {
                        // Remove this monitor
                        eventMonitor.monitors.splice(ii, 1);
                        ii--;

                        mon.completion();
                    }
                }
            });
        });

        return true;
    };

    eventMonitor.ready = new Promise((resolve, reject) => {
        eventMonitor.readyResolve = resolve;
    })

    eventMonitor.counterEvents = function(options) {
        let monPromiseArray = [];

        if (!options.start) options.start = 0;
        if (!options.num) options.num = 1;
        if (!options.timeout) options.timeout = 15000;

        for(let counter = options.start; counter < (options.start + options.num); counter++) {

            let str = counter.toString();
            if (options.size > 0) {
                if (str.length < 8) {
                    str = '00000000'.substr(0, 8 - str.length) + str;
                }
                const startLen = str.length;
                for(let ii = str.length; ii < options.size; ii++) {
                    str += 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'.substr((ii - startLen) % 26, 1);
                }
            }
            // console.log('searching for ' + str);

            let opts = Object.assign({
                dataIs: str
            }, options);

            const monPromise = eventMonitor.monitor(opts);
            monPromiseArray.push(monPromise);
        }
        return Promise.all(monPromiseArray);
    };

    eventMonitor.monitor = function(options) {
        let mon = {};
        mon.options = options;

        mon.completion = function() {
            if (mon.completionResolve) {
                if (mon.options.timer) {
                    clearTimeout(mon.options.timer);
                    mon.options.timer = null;
                }
                if (!options.expectTimeout) {
                    // Caller is waiting on this (normal case)
                    mon.completionResolve(mon.resolveData);
                }
                else {
                    // Not expected, reject instead
                    mon.completionReject('unexpected result');
                }
            }
        };

        mon.checkEvent = function(event) {
            if (options.nameIs) {
                if (event.name !== options.nameIs) {
                    return false;
                }
            }
            if (options.nameIncludes) {
                if (!event.name.includes(options.nameIncludes)) {
                    return false;
                }
            }
            if (options.dataIs) {
                if (event.data !== options.dataIs) {
                    return false;
                }
            }
            if (options.dataIncludes) {
                if (!event.name.includes(options.dataIncludes)) {
                    return false;
                }
            }
            
            return true;
        };

        if (options.historyOnly) {
            for(const event of eventMonitor.events) {
                if (mon.checkEvent(event)) {
                    return true;
                }
            }
            return false;
        }
        
        if (!options.noHistoryCheck) {
            // See if a recently received event can resolve this
            for(const event of eventMonitor.events) {
                if (mon.checkEvent(event)) {
                    return Promise.resolve(event);
                }
            }
        }

        if (mon.options.timeout) {
            mon.timer = setTimeout(function() {
                if (mon.completionReject) {
                    if (!options.expectTimeout) {
                        mon.completionReject('event timeout ' + JSON.stringify(mon.options));
                    }
                    else {
                        mon.completionResolve();   
                    }
                }
            }, mon.options.timeout);
        }

        eventMonitor.monitors.push(mon);

        return new Promise((resolve, reject) => {
            mon.completionResolve = resolve;
            mon.completionReject = reject;
        });
    };


}(module.exports));

