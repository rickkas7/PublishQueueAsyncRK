
#include "Particle.h"

// This must be included before PublishQueueAsyncRK.h to add in FRAM support
#include "MB85RC256V-FRAM-RK.h"
#include "PublishQueueAsyncRK.h"

#include "AutomatedTest.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_INFO, { // Logging level for non-application messages
	{ "app.pubq", LOG_LEVEL_TRACE },
	{ "app.seqfile", LOG_LEVEL_TRACE }
});


MB85RC256V fram(Wire, 0);

PublishQueueAsyncFRAM publishQueue(fram, 0, 4096); // Use first 4K of FRAM for event queue


AutomatedTest automatedTest;

void setup() {
	Serial.begin();

    waitFor(Serial.isConnected, 8000);
    delay(1000);

	fram.begin();

	publishQueue.setup();

    automatedTest.setup(&publishQueue);
}

void loop() {
 	// publishQueue.loop();

    automatedTest.loop();
}


