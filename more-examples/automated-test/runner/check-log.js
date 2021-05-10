// Not yet used

(function(checkLog) {

    checkLog.includeUntil = function(inputArray, checkFunction, options) {
        let outputArray = [];

        let include = true;
        for(let ii = 0; ii < inputArray; ii++) {
            if (checkFunction(options, inputArray[ii])) {
                include = false;
            }
            if (include) {
                outputArray.push(inputArray[ii]);
            }
        }
        return outputArray;
    };

    checkLog.includeAfter = function(inputArray, checkFunction, options) {
        let outputArray = [];

        let include = false;
        for(let ii = 0; ii < inputArray; ii++) {
            if (checkFunction(options, inputArray[ii])) {
                include = true;
            }
            if (include) {
                outputArray.push(inputArray[ii]);
            }
        }
        return outputArray;
    };

    checkLog.filterArray = function(inputArray, checkFunction, options) {
        let outputArray = [];

        for(let ii = 0; ii < inputArray; ii++) {
            if (checkFunction(options, inputArray[ii])) {
                outputArray.push(inputArray[ii]);
            }
        }
        return outputArray;
    };

    checkLog.checkSerialLine = function(options, line) {
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
                    const json = JSON.parse(msg);
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

    checkLog.checkEvent = function(options, event) {
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

}(module.exports));

