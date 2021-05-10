const SerialPort = require('serialport');

(function(serialMonitor) {
    serialMonitor.lines = [];
    serialMonitor.partial = '';
    serialMonitor.monitors = [];

    serialMonitor.resetLines = function() {
        serialMonitor.lines = [];
    };

    serialMonitor.run = function(config) {
        if (!config.serialPort) {
            console.log('missing serialPort path in config.json');
            return false;
        }
        
        serialMonitor.config = config;

        serialMonitor.port = new SerialPort(serialMonitor.config.serialPort, {
        });

        serialMonitor.port.on('open', function () {
            console.log('serial port opened');
            serialMonitor.readyResolve();
        });

        serialMonitor.port.on('close', function () {
            console.log('serial port closed');
            setTimeout(function() {
                serialMonitor.port.open();
            }, 3000);
        });

        serialMonitor.port.on('error', function (err) {
            console.log('Serial Port Error: ', err.message);
        })
        serialMonitor.port.on('data', function (data) {
            // console.log('Data:', data.toString());
            serialMonitor.partial += data.toString();

            let tempLines;
            if (serialMonitor.partial.endsWith('\n')) {
                tempLines = serialMonitor.partial;
                serialMonitor.partial = '';
            }
            else {
                const lastNewline = serialMonitor.partial.lastIndexOf('\n');
                if (lastNewline >= 0) {
                    tempLines = serialMonitor.partial.substr(0, lastNewline + 1);
                    serialMonitor.partial = serialMonitor.partial.substr(lastNewline + 1);
                }
            }
            if (tempLines) {
                for(let line of tempLines.split('\n')) {
                    line = line.trim();
                    if (line) {
                        serialMonitor.lines.push(line);
                        console.log('serial ' + line);   

                        for(let ii = 0; ii < serialMonitor.monitors.length; ii++) {
                            let mon = serialMonitor.monitors[ii];
                            mon.resolveData = line; 
                            if (mon.checkLine(line)) {
                                // Remove this monitor
                                serialMonitor.monitors.splice(ii, 1);
                                ii--;
        
                                mon.completion();
                            }
                        }
        
                    }
                }
            }
        })

        return true;
    };

    serialMonitor.ready = new Promise((resolve, reject) => {
        serialMonitor.readyResolve = resolve;
    });

    serialMonitor.write = async function(str) {
        const prom = new Promise((resolve, reject) => {
            serialMonitor.port.write(str, function (err) {
                if (err) {
                    reject(err.message);
                    return;
                }

                resolve();
            });
        });
        await prom;
    };

    serialMonitor.command = async function(cmd) {
        const mon = serialMonitor.monitor({
            msgAny:true,
            noHistoryCheck:true,
            timeout:5000,
            detail:'cmd=' + cmd
        });

        await serialMonitor.write(cmd + '\n');

        return mon;
    }

    serialMonitor.jsonCommand = async function(cmd) {
        const mon = serialMonitor.monitor({
            msgJson:true,
            noHistoryCheck:true,
            timeout:5000,
            detail:'cmd=' + cmd + ' (json)'
        });

        await serialMonitor.write(cmd + '\n');

        return mon;
    }
   
    serialMonitor.monitor = function(options) {
        let mon = {};
        mon.options = options;

        mon.checkLine = function(line) {
            const m = line.match(/([0-9]+) \[([^\]]+)\] ([A-Z]+): (.*)/);
            if (m && m.length >= 5) {
                const ts = parseInt(m[1], 10);
                const category = m[2];
                const level = m[3];
                const msg = m[4];

                if (options.category) {
                    if (options.category !== category) {
                        return false;
                    }
                }
                if (options.level) {
                    if (options.level !== level) {
                        return false;
                    }
                }
                if (options.msgIs) {
                    if (msg != options.msgIs) {
                        return false;
                    }
                }                
                if (options.msgIncludes) {
                    if (!msg.includes(options.msgIncludes)) {
                        return false;
                    }
                }                
                if (options.msgJson) {
                    try {
                        mon.resolveData = JSON.parse(msg);
                        return true;
                    }
                    catch(e) {
                        return false;
                    }
                }
                if (options.msgAny) {
                    mon.resolveData = msg;
                    return true;
                }
            }

            if (options.lineIncludes) {
                if (!line.includes(options.lineIncludes)) {
                    return false;
                }
            }
            
            return true;
        };

        mon.completion = function() {
            if (mon.completionResolve) {
                if (mon.options.timer) {
                    clearTimeout(mon.options.timer);
                    mon.options.timer = null;
                }
                // Caller is waiting on this
                mon.completionResolve(mon.resolveData);
            }
        };

        if (options.historyOnly) {
            for(const line of serialMonitor.lines) {
                if (mon.checkLine(line)) {
                    return true;
                }
            }
            return false;
        }

        if (!options.noHistoryCheck) {
            // See if a recently received event can resolve this
            for(const line of serialMonitor.lines) {
                if (mon.checkLine(line)) {
                    return Promise.resolve(line);
                }
            }    
        }

        serialMonitor.monitors.push(mon);

        if (mon.options.timeout) {
            mon.timer = setTimeout(function() {
                if (mon.completionReject) {
                    const detail = options.detail || '';
                    mon.completionReject('serial timeout ' + detail);
                }
            }, mon.options.timeout);
        }

        return new Promise((resolve, reject) => {
            mon.completionResolve = resolve;
            mon.completionReject = reject;
        });;
    };


    

}(module.exports));

