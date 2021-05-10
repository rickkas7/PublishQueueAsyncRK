const expect = require('expect');

(function(testSuite) {
    testSuite.skipResetTests = false;
    testSuite.skipCloudManipulatorTests = false;


    testSuite.run = async function(config, cloudManipulator, eventMonitor, serialMonitor) {

        console.log('running test suite');
        testSuite.config = config;
        testSuite.cloudManipulator = cloudManipulator;
        testSuite.eventMonitor = eventMonitor;
        testSuite.serialMonitor = serialMonitor;

        let counter;

        const tests = {
            'warmup':async function(testName) {
                await testSuite.serialMonitor.command('queue -c');
                await testSuite.serialMonitor.command('cloud -dw');
                await testSuite.serialMonitor.command('cloud -c');

                await testSuite.serialMonitor.monitor({msgIs:'Cloud connected', timeout:120000}); 
                await testSuite.serialMonitor.command('publish -c 1');

                await testSuite.eventMonitor.counterEvents({
                    start:counter,
                    num:1,
                    nameIs:'testEvent',
                    timeout:120000
                });
            },
            'simple 2':async function(testName) {
                await testSuite.serialMonitor.command('queue -c');
            
                await testSuite.serialMonitor.command('publish -c 2');
    
                await testSuite.eventMonitor.counterEvents({
                    start:counter,
                    num:2,
                    nameIs:'testEvent',
                    timeout:15000
                });
    
                if (!testSuite.serialMonitor.monitor({msgIs:'published successfully', historyOnly:true})) {
                    throw 'missing published successfully';
                }
                
            },
            'simple 10':async function(testName) { // 1
                await testSuite.serialMonitor.command('queue -c');

                await testSuite.serialMonitor.command('publish -c 10');

                await testSuite.eventMonitor.counterEvents({
                    start:counter,
                    num:10,
                    nameIs:'testEvent',
                    timeout:30000
                });
    
            },
            '622 byte 3':async function(testName) { 
                const size = 622;

                await testSuite.serialMonitor.command('queue -c');

                await testSuite.serialMonitor.command('publish -c 3 -s ' + size);

                await testSuite.eventMonitor.counterEvents({
                    start:counter,
                    num:3,
                    nameIs:'testEvent',
                    size: size,
                    timeout:30000
                });
    
            },
            '622 byte discard 3':async function(testName) { 
                await testSuite.serialMonitor.command('queue -c');

                await testSuite.serialMonitor.command('queue -p');

                await testSuite.serialMonitor.command('publish -c 6 -s 622');

                await testSuite.serialMonitor.command('queue -r');

                await testSuite.eventMonitor.counterEvents({
                    start:counter + 3,
                    num:3,
                    nameIs:'testEvent',
                    size: 622,
                    timeout:30000
                });
    
            },
            'offline 5':async function(testName) { // 2
                // Offline, 5 events, then online
                await testSuite.serialMonitor.command('queue -c');

                await testSuite.serialMonitor.command('cloud -d');

                await testSuite.serialMonitor.command('publish -c 5');

                await testSuite.serialMonitor.command('cloud -c');

                await testSuite.eventMonitor.counterEvents({
                    start:counter,
                    num:5,
                    nameIs:'testEvent',
                    timeout:15000
                });

            },
            'offline 5 reset':async function(testName) { // 3
                if (testSuite.skipResetTests) {
                    console.log('skipping ' + testName + ' (testSuite.skipResetTests = true)');
                    return;
                }

                // Offline, 5 events, reset
                await testSuite.serialMonitor.command('queue -c');

                await testSuite.serialMonitor.command('cloud -d');

                await testSuite.serialMonitor.command('publish -c 5');

                await testSuite.serialMonitor.command('reset');

                await testSuite.serialMonitor.monitor({msgIs:'Cloud connected', timeout:120000}); 

                await testSuite.eventMonitor.counterEvents({
                    start:counter,
                    num:5,
                    nameIs:'testEvent',
                    timeout:30000
                });

            },
            'publish slowly':async function(testName) { // 6
                // Publish slowly
                await testSuite.serialMonitor.command('queue -c');

                await testSuite.serialMonitor.command('publish -c 10 -p 2000');

                await testSuite.eventMonitor.counterEvents({
                    start:counter,
                    num:10,
                    nameIs:'testEvent',
                    timeout:20000
                });

            },            
            'data loss':async function(testName) {
                if (testSuite.skipCloudManipulatorTests) {
                    console.log('skipping ' + testName + ' (testSuite.skipCloudManipulatorTests = true)');
                    return;
                }

                await testSuite.serialMonitor.command('queue -c');

                cloudManipulator.setData(false);

                await testSuite.serialMonitor.command('publish -c 1');

                await testSuite.eventMonitor.counterEvents({
                    expectTimeout: true,
                    start:counter,
                    num:1,
                    nameIs:'testEvent',
                    timeout:30000                    
                });

                await testSuite.serialMonitor.monitor({msgIs:'publish failed, will retry in 30000 ms', timeout:45000}); 

                /*
                serial 0000119187 [app.pubq] INFO: publish failed, will retry in 30000 ms
                */
                
                cloudManipulator.setData(true);

                await testSuite.eventMonitor.counterEvents({
                    start:counter,
                    num:1,
                    nameIs:'testEvent',
                    timeout:45000                    
                });

            },

            'data loss 2':async function(testName) {
                if (testSuite.skipCloudManipulatorTests) {
                    console.log('skipping ' + testName + ' (testSuite.skipCloudManipulatorTests = true)');
                    return;
                }

                await testSuite.serialMonitor.command('queue -c');

                cloudManipulator.setData(false);

                await testSuite.serialMonitor.command('publish -c 1');

                await testSuite.eventMonitor.counterEvents({
                    expectTimeout: true,
                    start:counter,
                    num:1,
                    nameIs:'testEvent',
                    timeout:30000                    
                });

                await testSuite.serialMonitor.monitor({msgIs:'publish failed, will retry in 30000 ms', timeout:45000}); 

                await testSuite.serialMonitor.command('publish -c 1');

                /*
                serial 0000119187 [app.pubq] INFO: publish failed, will retry in 30000 ms
                */
                
                cloudManipulator.setData(true);

                await testSuite.eventMonitor.counterEvents({
                    start:counter,
                    num:2,
                    nameIs:'testEvent',
                    timeout:45000                    
                });

            },


            'data loss 3':async function(testName) {
                if (testSuite.skipCloudManipulatorTests) {
                    console.log('skipping ' + testName + ' (testSuite.skipCloudManipulatorTests = true)');
                    return;
                }

                await testSuite.serialMonitor.command('queue -c');

                cloudManipulator.setData(false);

                await testSuite.serialMonitor.command('publish -c 2');

                await testSuite.eventMonitor.counterEvents({
                    expectTimeout: true,
                    start:counter,
                    num:1,
                    nameIs:'testEvent',
                    timeout:30000                    
                });

                await testSuite.serialMonitor.monitor({msgIs:'publish failed, will retry in 30000 ms', timeout:45000}); 

                /*
                serial 0000119187 [app.pubq] INFO: publish failed, will retry in 30000 ms
                */
                
                cloudManipulator.setData(true);

                await testSuite.eventMonitor.counterEvents({
                    start:counter,
                    num:2,
                    nameIs:'testEvent',
                    timeout:45000                    
                });

            },


            'data loss reset':async function(testName) {
                if (testSuite.skipCloudManipulatorTests) {
                    console.log('skipping ' + testName + ' (testSuite.skipCloudManipulatorTests = true)');
                    return;
                }
                if (testSuite.skipResetTests) {
                    console.log('skipping ' + testName + ' (testSuite.skipResetTests = true)');
                    return;
                }

                await testSuite.serialMonitor.command('queue -c');

                cloudManipulator.setData(false);

                await testSuite.serialMonitor.command('publish -c 3');

                await testSuite.serialMonitor.monitor({msgIs:'publish failed, will retry in 30000 ms', timeout:45000}); 
                
                cloudManipulator.setData(true);

                await testSuite.serialMonitor.command('reset');

                await testSuite.serialMonitor.monitor({msgIs:'Cloud connected', timeout:120000}); 

                await testSuite.eventMonitor.counterEvents({
                    start:counter,
                    num:3,
                    nameIs:'testEvent',
                    timeout:30000                    
                });

            },


            'data loss long':async function(testName) {
                if (testSuite.skipCloudManipulatorTests) {
                    console.log('skipping ' + testName + ' (testSuite.skipCloudManipulatorTests = true)');
                    return;
                }

                await testSuite.serialMonitor.command('queue -c');

                cloudManipulator.setData(false);

                await testSuite.serialMonitor.command('publish -c 10 -p 10000');
            
                await testSuite.serialMonitor.monitor({msgIs:'publishing padded counter=' + (counter + 9) + ' size=0', timeout:300000}); 
                
                cloudManipulator.setData(true);

                await testSuite.eventMonitor.counterEvents({
                    start:counter,
                    num:10,
                    nameIs:'testEvent',
                    timeout:300000                    
                });

            },
            'continuous events':async function(testName) { // 7
                // Events test (continuous)
                await testSuite.serialMonitor.command('queue -c');

                let origFreeMemory;

                await new Promise((resolve, reject) => {
                    setInterval(async function() {
                        testSuite.eventMonitor.resetEvents();
                        testSuite.serialMonitor.resetLines();
    
                        const mem = await testSuite.serialMonitor.jsonCommand('freeMemory');
                        console.log('test starting freeMemory=' + mem.freeMemory);

                        if (!origFreeMemory) {
                            origFreeMemory = mem.freeMemory;
                        }
                        if (origFreeMemory != mem.freeMemory) {
                            console.log('free memory changed, originally ' + origFreeMemory);
                        }
                     
                        await testSuite.serialMonitor.command('publish -c 3');
    
                        await testSuite.eventMonitor.counterEvents({
                            start:counter,
                            num:3,
                            nameIs:'testEvent'
                        });
                        
                        counter += 3;
                        console.log('publish 3 events complete counter=' + counter);
                    }, 15000);              
                });

            },
        };

        const testsKeys = Object.keys(tests);

        const startWith = '';
        if (startWith) {
            while(testsKeys.length) {
                if (testsKeys[0] != startWith) {
                    testsKeys.shift();
                }
            }
        }

        while(testsKeys.length) {
            const testName = testsKeys.shift();

            try {
                console.log('**************************************************************************');
                console.log('');
                console.log('Running test ' + testName);
                console.log('');
                console.log('**************************************************************************');
                testSuite.cloudManipulator.reset();
                testSuite.eventMonitor.resetEvents();
                testSuite.serialMonitor.resetLines();
                const counterObj = await testSuite.serialMonitor.jsonCommand('counter -r');
                counter = counterObj.counter;
    
                const mem = await testSuite.serialMonitor.jsonCommand('freeMemory');
                console.log('test ' + testName + ' starting freeMemory=' + mem.freeMemory);
    
                const startMs = Date.now();

                await tests[testName](testName);

                const endMs = Date.now();
                console.log('test ' + testName + ' completed in ' + (endMs - startMs) + ' ms');
            }
            catch(e) {
                console.log('############ Test Failure! ' + testName);
                console.trace('test ' + testName + ' failed', e);
            }
        }

 

            // await testSuite.eventMonitor.monitor({nameIs:'spark/status', dataIs:'online', timeout:120000});
 
            // await testSuite.serialMonitor.monitor({nameIs:'not found', timeout:5000});    
            // await testSuite.serialMonitor.monitor({msgIs:'Cloud connected', timeout:120000}); 

    };

}(module.exports));

