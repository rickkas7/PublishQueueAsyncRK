#include "AutomatedTest.h"

void AutomatedTest::setup(PublishQueueAsyncBase *publishQueue) {
    this->publishQueue = publishQueue;

	// Configuration of prompt and welcome message
	commandParser.addCommandHandler("cloud", "cloud connect or disconnect", [this](SerialCommandParserBase *) {
		CommandParsingState *cps = commandParser.getParsingState();

        bool wait = (cps->getByShortOpt('w') != 0);

        if (cps->getByShortOpt('c')) {
            Log.info("connecting to the Particle cloud");
            Particle.connect();
            if (wait) {
                waitUntil(Particle.connected);
            }
        }
        else
        if (cps->getByShortOpt('d')) {
            Log.info("disconnecting from the Particle cloud");
            Particle.disconnect();            
            if (wait) {
                waitUntilNot(Particle.connected);
            }
        }
        else {
    		Log.info("{\"cloudConnected\":%s}", Particle.connected() ? "true" : "false");
        }
	})
    .addCommandOption('c', "connect", "connect to cloud")
    .addCommandOption('d', "disconnect", "disconnect from cloud")
    .addCommandOption('w', "wait", "wait until complete");

	commandParser.addCommandHandler("counter", "set the event counter", [this](SerialCommandParserBase *) {
		CommandParsingState *cps = commandParser.getParsingState();
        CommandOptionParsingState *cops;

        cops = cps->getByShortOpt('v');
        if (cops && cops->getNumArgs() == 1) {
            counter = cops->getArgInt(0);
        }
        else if (cps->getByShortOpt('r')) {
            counter = rand();
        }

		Log.info("{\"counter\":%d}", counter);
    })
    .addCommandOption('v', "value", "value to set the counter to", false, 1)
    .addCommandOption('r', "random", "set to random number");

	commandParser.addCommandHandler("freeMemory", "report free memory", [this](SerialCommandParserBase *) {
		Log.info("{\"freeMemory\":%lu}", System.freeMemory());
    });

	commandParser.addCommandHandler("publish", "publish an event", [this](SerialCommandParserBase *) {
		CommandParsingState *cps = commandParser.getParsingState();
        CommandOptionParsingState *cops;
        
        resetSettings();

        cops = cps->getByShortOpt('c');
        if (cops && cops->getNumArgs() == 1) {
            count = cops->getArgInt(0);
        }

        cops = cps->getByShortOpt('d');
        if (cops && cops->getNumArgs() == 1) {
            data = cops->getArgString(0);
        }

        cops = cps->getByShortOpt('n');
        if (cops && cops->getNumArgs() == 1) {
            name = cops->getArgString(0);
        }

        cops = cps->getByShortOpt('p');
        if (cops && cops->getNumArgs() == 1) {
            period = cops->getArgInt(0);
        }

        cops = cps->getByShortOpt('s');
        if (cops && cops->getNumArgs() == 1) {
            size = cops->getArgInt(0);
        }
	})
    .addCommandOption('c', "count", "number of events to publish", false, 1)
    .addCommandOption('d', "data", "event data", false, 1)
    .addCommandOption('n', "name", "event name", false, 1)
    .addCommandOption('p', "period", "publish period (ms)", false, 1)
    .addCommandOption('s', "size", "size of event data", false, 1);


	commandParser.addCommandHandler("queue", "queue settings", [this, publishQueue](SerialCommandParserBase *) {
		CommandParsingState *cps = commandParser.getParsingState();
        CommandOptionParsingState *cops;
        
        if (cps->getByShortOpt('c')) {
            publishQueue->clearEvents();
        }
        
        if (cps->getByShortOpt('p')) {
            publishQueue->setPausePublishing(true);
        }
        
        if (cps->getByShortOpt('r')) {
            publishQueue->setPausePublishing(false);
        }


        
	})
    .addCommandOption('c', "clear", "clear queues")
    .addCommandOption('p', "pause", "pause publishing")
    .addCommandOption('r', "resume", "resume publishing");

	commandParser.addCommandHandler("reset", "reset device", [this](SerialCommandParserBase *) {
        doReset = true;
	});

	commandParser.addCommandHandler("version", "report Device OS version", [this](SerialCommandParserBase *) {
		Log.info("{\"systemVersion\":\"%s\"}", System.version().c_str());
    });


	commandParser.addHelpCommand();

	// Connect to Serial and start running
	commandParser
		.withSerial(&Serial)
		.setup();

    
    // This allows a graceful shutdown on System.reset()
    Particle.setDisconnectOptions(CloudDisconnectOptions().graceful(true).timeout(5000));

}

void AutomatedTest::loop() {
    if (exitTime != 0) {
        if (millis() - exitTime > 500) {
            Log.info("delay outside of loop %lu", millis() - exitTime);
        }
    }
    unsigned long startTime = millis();

    //publishQueue->loop();
    commandParser.loop();

    if (doReset) {
        Log.info("resetting device");
        System.reset();
    }

    if (numPublished < count) {
        if (period > 0) {
            if (millis() - lastPublish >= period) {
                lastPublish = millis();
                numPublished++;

                publishPaddedCounter(size, true);
            }
        }
        else {
            while(numPublished < count) {
                numPublished++;
                publishPaddedCounter(size, true);
            }
        }
    }

    if (millis() - startTime > 500) {
        Log.info("delay inside loop %lu", millis() - startTime);
    }
    exitTime = millis();

}



void AutomatedTest::publishPaddedCounter(int size, bool withAck) {
    // This message is monitored by the automated test tool. If you edit this, change that too.
    Log.info("publishing padded counter=%d size=%d", counter, size);

    char buf[particle::protocol::MAX_EVENT_DATA_LENGTH + 1];

    if (size > 0) {
        snprintf(buf, sizeof(buf), "%08d", counter++);
        if (size > (int)(sizeof(buf) - 1)) {
            size = (int)(sizeof(buf) - 1);
        }

        char c = 'A';
        for(size_t ii = strlen(buf); ii < (size_t)size; ii++) {
            buf[ii] = c;
            if (++c > 'Z') {
                c = 'A';
            }
        }
        buf[size] = 0;
    }
    else {
        snprintf(buf, sizeof(buf), "%d", counter++);
    }

    publishQueue->publish(name, buf, PRIVATE | WITH_ACK);
}

void AutomatedTest::resetSettings() {
    count = 0;
    data = "";
    name = "testEvent";
    period = 0;
    size = 0;

    numPublished = 0;
    lastPublish = 0;
}

