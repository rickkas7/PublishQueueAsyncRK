#include "Particle.h"

#include "PublishQueueAsyncRK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

retained uint8_t publishQueueRetainedBuffer[2048];
PublishQueueAsync publishQueue(publishQueueRetainedBuffer, sizeof(publishQueueRetainedBuffer));

const unsigned long PUBLISH_PERIOD_MS = 30000;
unsigned long lastPublish = 8000 - PUBLISH_PERIOD_MS;
int counter = 0;


void setup() {
	Serial.begin();
	publishQueue.setup();
}

void loop() {

	if (millis() - lastPublish >= PUBLISH_PERIOD_MS) {
		lastPublish = millis();

		Log.info("publishing");

		char buf[32];
		snprintf(buf, sizeof(buf), "%d", counter++);
		publishQueue.publish("testEvent", buf, 60, PRIVATE, WITH_ACK);
	}
}

