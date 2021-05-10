#include "Particle.h"

#include "PublishQueueAsyncRK.h"
#include "SerialCommandParserRK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_INFO, { // Logging level for non-application messages
	{ "app.pubq", LOG_LEVEL_TRACE },
	{ "app.seqfile", LOG_LEVEL_TRACE }
});

void publishCounter(bool withAck);
void publishPaddedCounter(int size);

SerialCommandParser<1000, 16> commandParser;
retained uint8_t publishQueueRetainedBuffer[2048];
PublishQueueAsync publishQueue(publishQueueRetainedBuffer, sizeof(publishQueueRetainedBuffer));


class Publisher {
public:
    void setup() {

    }

    void loop() {
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
    }

    void publishPaddedCounter(int size, bool withAck) {
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

        publishQueue.publish(name, buf, PRIVATE | WITH_ACK);
    }

    void resetSettings() {
        count = 1;
        data = "";
        name = "testEvent";
        period = 0;
        size = 0;
    
        numPublished = 0;
        lastPublish = 0;
    }

    // Configuration
    int counter = 0;
    int count = 0;
    String data;
    String name = "testEvent";
    unsigned long period = 0;
    int size = 0;

    // State
    int numPublished = 0;
    unsigned long lastPublish = 0;

};

Publisher publisher;
bool doReset = false;

void setup() {
	Serial.begin();

	// Configuration of prompt and welcome message
    /*
	commandParser
		.withPrompt("> ")
		.withWelcome("Publish Queue Automated Test");
*/
	// Command configuration


	commandParser.addCommandHandler("cloud", "cloud connect or disconnect", [](SerialCommandParserBase *) {
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

	commandParser.addCommandHandler("counter", "set the event counter", [](SerialCommandParserBase *) {
		CommandParsingState *cps = commandParser.getParsingState();
        CommandOptionParsingState *cops;

        cops = cps->getByShortOpt('v');
        if (cops && cops->getNumArgs() == 1) {
            publisher.counter = cops->getArgInt(0);
        }
        else if (cps->getByShortOpt('r')) {
            publisher.counter = rand();
        }

		Log.info("{\"counter\":%d}", publisher.counter);
    })
    .addCommandOption('v', "value", "value to set the counter to", false, 1)
    .addCommandOption('r', "random", "set to random number");

	commandParser.addCommandHandler("freeMemory", "report free memory", [](SerialCommandParserBase *) {
		Log.info("{\"freeMemory\":%lu}", System.freeMemory());
    });

	commandParser.addCommandHandler("publish", "publish an event", [](SerialCommandParserBase *) {
		CommandParsingState *cps = commandParser.getParsingState();
        CommandOptionParsingState *cops;
        
        publisher.resetSettings();

        cops = cps->getByShortOpt('c');
        if (cops && cops->getNumArgs() == 1) {
            publisher.count = cops->getArgInt(0);
        }

        cops = cps->getByShortOpt('d');
        if (cops && cops->getNumArgs() == 1) {
            publisher.data = cops->getArgString(0);
        }

        cops = cps->getByShortOpt('n');
        if (cops && cops->getNumArgs() == 1) {
            publisher.name = cops->getArgString(0);
        }

        cops = cps->getByShortOpt('p');
        if (cops && cops->getNumArgs() == 1) {
            publisher.period = cops->getArgInt(0);
        }

        cops = cps->getByShortOpt('s');
        if (cops && cops->getNumArgs() == 1) {
            publisher.size = cops->getArgInt(0);
        }
	})
    .addCommandOption('c', "count", "number of events to publish", false, 1)
    .addCommandOption('d', "data", "event data", false, 1)
    .addCommandOption('n', "name", "event name", false, 1)
    .addCommandOption('p', "period", "publish period (ms)", false, 1)
    .addCommandOption('s', "size", "size of event data", false, 1);


	commandParser.addCommandHandler("queue", "queue settings", [](SerialCommandParserBase *) {
		CommandParsingState *cps = commandParser.getParsingState();
        CommandOptionParsingState *cops;
        
        if (cps->getByShortOpt('c')) {
            publishQueue.clearEvents();
        }
        
        if (cps->getByShortOpt('p')) {
            publishQueue.setPausePublishing(true);
        }
        
        if (cps->getByShortOpt('r')) {
            publishQueue.setPausePublishing(false);
        }


        
	})
    .addCommandOption('c', "clear", "clear queues")
    .addCommandOption('p', "pause", "pause publishing")
    .addCommandOption('r', "resume", "resume publishing");

	commandParser.addCommandHandler("reset", "reset device", [](SerialCommandParserBase *) {
        doReset = true;
	});

	commandParser.addCommandHandler("version", "report Device OS version", [](SerialCommandParserBase *) {
		Log.info("{\"systemVersion\":\"%s\"}", System.version().c_str());
    });


	commandParser.addHelpCommand();

	// Connect to Serial and start running
	commandParser
		.withSerial(&Serial)
		.setup();

    
    // This allows a graceful shutdown on System.reset()
    Particle.setDisconnectOptions(CloudDisconnectOptions().graceful(true).timeout(5000));

    waitFor(Serial.isConnected, 8000);
    delay(1000);

	publishQueue.setup();

    publisher.setup();
}

void loop() {
    static unsigned long exitTime = 0;
    if (exitTime != 0) {
        if (millis() - exitTime > 500) {
            Log.info("delay outside of loop %lu", millis() - exitTime);
        }
    }
    unsigned long startTime = millis();

    //publishQueue.loop();
    commandParser.loop();
    publisher.loop();

    if (doReset) {
        Log.info("resetting device");
        System.reset();
    }

    if (millis() - startTime > 500) {
        Log.info("delay inside loop %lu", millis() - startTime);
    }
    exitTime = millis();
}
