#include "Particle.h"

#include "PublishQueueAsyncRK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

retained uint8_t publishQueueRetainedBuffer[2048];
PublishQueueAsync publishQueue(publishQueueRetainedBuffer, sizeof(publishQueueRetainedBuffer));

enum {
	TEST_IDLE = 0, // Don't do anything
	TEST_COUNTER, // 1 publish, period milliseconds is param0
	TEST_PUBLISH_FAST, // 2 publish events as fast as possible, number is param0, optional size in param2
	TEST_PUBLISH_OFFLINE // 3 go offline, publish some events, then go back online, number is param0, optional size in param2
};

const size_t MAX_PARAM = 4;
const unsigned long PUBLISH_PERIOD_MS = 30000;
unsigned long lastPublish = 8000 - PUBLISH_PERIOD_MS;
int counter = 0;
int testNum;
int intParam[MAX_PARAM];
String stringParam[MAX_PARAM];
size_t numParam;

int testHandler(String cmd);
void publishCounter();
void publishPaddedCounter(int size);

void setup() {
	Serial.begin();
	Particle.function("test", testHandler);
}

void loop() {

	if (testNum == TEST_COUNTER) {
		int publishPeriod = intParam[0];
		if (publishPeriod < 1) {
			publishPeriod = 15000;
		}

		if (millis() - lastPublish >= (unsigned long) publishPeriod) {
			lastPublish = millis();

			Log.info("TEST_COUNTER period=%d", publishPeriod);
			publishCounter();
		}
	}
	else
	if (testNum == TEST_PUBLISH_FAST) {
		testNum = TEST_IDLE;

		int count = intParam[0];
		int size = intParam[1];

		Log.info("TEST_PUBLISH_FAST count=%d", count);

		for(int ii = 0; ii < count; ii++) {
			publishPaddedCounter(size);
		}
	}
	else
	if (testNum == TEST_PUBLISH_OFFLINE) {
		testNum = TEST_IDLE;

		int count = intParam[0];
		int size = intParam[1];

		Log.info("TEST_PUBLISH_OFFLINE count=%d", count);

		Log.info("Going to Particle.disconnect()...");
		Particle.disconnect();
		delay(2000);

		for(int ii = 0; ii < count; ii++) {
			publishPaddedCounter(size);
		}

		Log.info("Going to Particle.connect()...");
		Particle.connect();
	}
}

void publishCounter() {
	Log.info("publishing counter=%d", counter);

	char buf[32];
	snprintf(buf, sizeof(buf), "%d", counter++);
	publishQueue.publish("testEvent", buf, 50, PRIVATE);
}

void publishPaddedCounter(int size) {
	Log.info("publishing padded counter=%d size=%d", counter, size);

	char buf[256];
	snprintf(buf, sizeof(buf), "%05d", counter++);

	if (size > 0) {
		if (size > (sizeof(buf) - 1)) {
			size = sizeof(buf) - 1;
		}

		char c = 'A';
		for(size_t ii = strlen(buf); ii < size; ii++) {
			buf[ii] = c;
			if (++c > 'Z') {
				c = 'A';
			}
		}
		buf[size] = 0;
	}

	publishQueue.publish("testEvent", buf, PRIVATE);
}


int testHandler(String cmd) {
	char *mutableCopy = strdup(cmd.c_str());

	char *cp = strtok(mutableCopy, ",");
	testNum = atoi(cp);

	for(numParam = 0; numParam < MAX_PARAM; numParam++) {
		cp = strtok(NULL, ",");
		if (!cp) {
			break;
		}
		intParam[numParam] = atoi(cp);
		stringParam[numParam] = cp;
	}
	for(size_t ii = numParam; ii < MAX_PARAM; ii++) {
		intParam[ii] = 0;
		stringParam[ii] = "";
	}


	free(mutableCopy);
	return 0;
}
