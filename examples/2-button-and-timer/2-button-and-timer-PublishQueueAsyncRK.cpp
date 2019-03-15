#include "Particle.h"

#include "PublishQueueAsyncRK.h"

// This example publish an event periodically using a software timer and also when you
// press the MODE button.

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

retained uint8_t publishQueueRetainedBuffer[2048];
PublishQueueAsync publishQueue(publishQueueRetainedBuffer, sizeof(publishQueueRetainedBuffer));

const unsigned long PUBLISH_PERIOD_MS = 30000;

void timerHandler();
void buttonHandler();
void publishCounter();


Timer timer(PUBLISH_PERIOD_MS, timerHandler);

int counter = 0;


void setup() {
	Serial.begin();
	System.on(button_click, buttonHandler);
	timer.start();
	publishQueue.setup();
}

void loop() {
}

void publishCounter() {
	Log.info("publishing");

	char buf[32];
	snprintf(buf, sizeof(buf), "%d", counter++);
	publishQueue.publish("testEvent", buf, 60, PRIVATE, WITH_ACK);
}

void timerHandler() {
	// This is run from the timer thread
	publishCounter();
}

void buttonHandler() {
	publishCounter();
}

