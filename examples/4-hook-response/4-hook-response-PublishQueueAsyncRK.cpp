#include "Particle.h"

#include "PublishQueueAsyncRK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_TRACE);

retained uint8_t publishQueueRetainedBuffer[2048];
PublishQueueAsync publishQueue(publishQueueRetainedBuffer, sizeof(publishQueueRetainedBuffer));

// This is the amount of time between publishes
const std::chrono::milliseconds publishPeriod = 60s;

const char *eventName = "testHook1";

int counter = 0;
unsigned long lastPublish = 0;

void publishCounter();

void setup() {
	// Uncomment to make it easier to see the serial logs at startup
    waitFor(Serial.isConnected, 15000);
    delay(1000);

    publishQueue.withHookResponse(eventName);
	publishQueue.setup();
}

void loop() {
    if (millis() - lastPublish >= publishPeriod.count()) {
		lastPublish = millis();
        publishCounter();
    }
}

void publishCounter() {
	Log.info("publishing");

	char buf[32];
	snprintf(buf, sizeof(buf), "{\"counter\":%d}", counter++);
	publishQueue.publish(eventName, buf, 60, PRIVATE, NO_ACK);
}

