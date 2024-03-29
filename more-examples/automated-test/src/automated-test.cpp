#include "Particle.h"

#include "AutomatedTest.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_INFO, { // Logging level for non-application messages
	{ "app.pubq", LOG_LEVEL_TRACE },
	{ "app.seqfile", LOG_LEVEL_TRACE }
});


retained uint8_t publishQueueRetainedBuffer[2048];
PublishQueueAsync publishQueue(publishQueueRetainedBuffer, sizeof(publishQueueRetainedBuffer));


AutomatedTest automatedTest;

void setup() {
	Serial.begin();

    waitFor(Serial.isConnected, 8000);
    delay(1000);

	publishQueue.setup();

    automatedTest.setup(&publishQueue);
}

void loop() {
 	// publishQueue.loop();

    automatedTest.loop();
}
