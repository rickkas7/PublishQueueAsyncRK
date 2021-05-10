
const fs = require('fs');
const path = require('path');

const argv = require('yargs').argv;

const eventMonitor = require('./event-monitor.js');

const serialMonitor = require('./serial-monitor.js');

const cloudManipulator = require('./cloud-manipulator.js');

const testSuite = require('./test-suite.js');


let config = JSON.parse(fs.readFileSync(path.join(__dirname, 'config.json'), 'utf8'));

if (argv.skipResetTests) {
    testSuite.skipResetTests = true;
}
if (argv.skipCloudManipulatorTests) {
    testSuite.skipCloudManipulatorTests = true;
}

if (!cloudManipulator.run(config)) {
    console.log('failed to start cloud manipulator');
    process.exit(1);
}

// console.log('config', config);
if (!eventMonitor.run(config)) {
    console.log('failed to start event monitor');
    process.exit(1);
}

if (!serialMonitor.run(config)) {
    console.log('failed to start serial monitor');
    process.exit(1);
}


// Pipe the data into another stream (like a parser or standard out)
// const lineStream = port.pipe(new Readline())

Promise.all([serialMonitor.ready, eventMonitor.ready, cloudManipulator.ready]).then((values) => {
    console.log('ready!');

    testSuite.run(config, cloudManipulator, eventMonitor, serialMonitor);
});
