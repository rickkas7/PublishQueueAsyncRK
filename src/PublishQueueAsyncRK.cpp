
#include "PublishQueueAsyncRK.h"


static const uint32_t RETAINED_BUF_HEADER_MAGIC = 0xd19cab61;

typedef struct {
	uint32_t	magic;			// RETAINED_BUF_HEADER_MAGIC
	uint16_t	size;			// retainedBufferSize, in case it changed
	uint16_t	numEvents;		// number of events, see the EventData structure
} RetainedBufHeader;

typedef struct {
	int ttl;
	uint8_t flags;
	// eventName (c-string, packed)
	// eventData (c-string, packed)
	// padded to 4-byte alignment
} EventData;

static Logger log("app.pubq");

PublishQueueAsync::PublishQueueAsync(uint8_t *retainedBuffer, uint16_t retainedBufferSize) :
		retainedBuffer(retainedBuffer), retainedBufferSize(retainedBufferSize) {

	// Initialize the retained buffer
	bool initBuffer = false;

	volatile RetainedBufHeader *hdr = reinterpret_cast<RetainedBufHeader *>(retainedBuffer);
	if (hdr->magic == RETAINED_BUF_HEADER_MAGIC && hdr->size == retainedBufferSize) {
		// Calculate the next write offset
		uint8_t *end = &retainedBuffer[retainedBufferSize];

		nextFree = &retainedBuffer[sizeof(RetainedBufHeader)];
		for(uint16_t ii = 0; ii < hdr->numEvents; ii++) {
			nextFree = skipEvent(nextFree);
			if (nextFree > end) {
				// Overflowed buffer, must be corrupted
				initBuffer = true;
				break;
			}
		}
	}
	else {
		// Not valid
		initBuffer = true;
	}

	//initBuffer = true; // Uncomment to discard old data

	if (initBuffer) {
		hdr->magic = RETAINED_BUF_HEADER_MAGIC;
		hdr->size = retainedBufferSize;
		hdr->numEvents = 0;
		nextFree = &retainedBuffer[sizeof(RetainedBufHeader)];
	}
}

PublishQueueAsync::~PublishQueueAsync() {

}

void PublishQueueAsync::setup() {
	haveSetup = true;

	thread = new Thread("PublishQueueAsync", threadFunctionStatic, this, OS_THREAD_PRIORITY_DEFAULT, 2048);
}

bool PublishQueueAsync::publish(const char *eventName, const char *data, int ttl, PublishFlags flags1, PublishFlags flags2) {

	if (!haveSetup) {
		setup();
	}

	if (data == NULL) {
		data = "";
	}

	// Size is the size of the header, the two c-strings (with null terminators), rounded up to a multiple of 4
	size_t size = sizeof(EventData) + strlen(eventName) + strlen(data) + 2;
	if ((size % 4) != 0) {
		size += 4 - (size % 4);
	}

	log.info("queueing eventName=%s data=%s ttl=%d flags1=%d flags2=%d size=%d", eventName, data, ttl, flags1.value(), flags2.value(), size);

	if  (size > (retainedBufferSize - sizeof(RetainedBufHeader))) {
		// Special case: event is larger than the retained buffer. Rather than throw out all events
		// before discovering this, check that case first
		return false;
	}

	while(true) {
		SINGLE_THREADED_BLOCK() {
			uint8_t *end = &retainedBuffer[retainedBufferSize];
			if ((size_t)(end - nextFree) >= size) {
				// There is room to fit this
				EventData *eventData = reinterpret_cast<EventData *>(nextFree);
				eventData->ttl = ttl;
				eventData->flags = flags1.value() | flags2.value();

				char *cp = reinterpret_cast<char *>(nextFree);
				cp += sizeof(EventData);

				strcpy(cp, eventName);
				cp += strlen(cp) + 1;

				strcpy(cp, data);

				nextFree += size;

				RetainedBufHeader *hdr = reinterpret_cast<RetainedBufHeader *>(retainedBuffer);
				hdr->numEvents++;
				// log.info("numEvents=%u", hdr->numEvents);
				return true;
			}

			// If there's only one event, there's nothing left to discard, this event is too large
			// to fit with the existing first event (which we can't delete because it might be
			// in the process of being sent)
			RetainedBufHeader *hdr = reinterpret_cast<RetainedBufHeader *>(retainedBuffer);
			if (hdr->numEvents == 1) {
				return false;
			}

			// Discard the oldest event (false) if we're not currently sending.
			// If we are sending (isSending=true), discard the second oldest event
			discardOldEvent(isSending);


			return false;
		}
	}

	return true;
}

bool PublishQueueAsync::clearEvents() {
	bool result = false;

	SINGLE_THREADED_BLOCK() {
		RetainedBufHeader *hdr = reinterpret_cast<RetainedBufHeader *>(retainedBuffer);
		if (!isSending) {
			hdr->numEvents = 0;
			result = true;
		}
	}

	return result;
}

uint8_t *PublishQueueAsync::skipEvent(uint8_t *start) {
	start += sizeof(EventData);
	start += strlen(reinterpret_cast<char *>(start)) + 1;
	start += strlen(reinterpret_cast<char *>(start)) + 1;

	// Align
	size_t offset = start - retainedBuffer;
	if ((offset % 4) != 0) {
		start += 4 - (offset % 4);
	}


	return start;
}


bool PublishQueueAsync::discardOldEvent(bool secondEvent) {
	// log.info("discardOldEvent secondEvent=%d", secondEvent);
	SINGLE_THREADED_BLOCK() {
		RetainedBufHeader *hdr = reinterpret_cast<RetainedBufHeader *>(retainedBuffer);
		uint8_t *start = &retainedBuffer[sizeof(RetainedBufHeader)];
		uint8_t *end = &retainedBuffer[retainedBufferSize];

		if (secondEvent) {
			if (hdr->numEvents < 2) {
				return false;
			}
			start = skipEvent(start);
		}
		else {
			if (hdr->numEvents < 1) {
				return false;
			}
		}

		// Remove the event at start
		uint8_t *next = skipEvent(start);
		size_t len = next - start;

		size_t after = end - next;
		if (after > 0) {
			// Move events down
			memmove(start, next, after);
		}

		nextFree -= len;
		hdr->numEvents--;

		log.trace("discardOldEvent secondEvent=%d start=%lx next=%lx end=%lx numEvents=%u",
				secondEvent, (uint32_t)start, (uint32_t)next, (uint32_t)end, hdr->numEvents);
	}

	return true;
}


void PublishQueueAsync::threadFunction() {
	// Call the stateHandler forever
	while(true) {
		stateHandler(*this);
		os_thread_yield();
	}
}

void PublishQueueAsync::startState() {
	// If we had other initialization to do, this would be a good place to do it.

	// Ready to process events
	stateHandler = &PublishQueueAsync::checkQueueState;
}

void PublishQueueAsync::checkQueueState() {
	// Is there data waiting to go out?
	volatile RetainedBufHeader *hdr = reinterpret_cast<RetainedBufHeader *>(retainedBuffer);

	bool haveEvent = false;
	SINGLE_THREADED_BLOCK() {
		haveEvent = (hdr->numEvents > 0);
	}

	if (haveEvent && Particle.connected() && millis() - lastPublish >= 1010) {
		// We have an event and can probably publish
		isSending = true;

		EventData *data = reinterpret_cast<EventData *>(&retainedBuffer[sizeof(RetainedBufHeader)]);
		const char *eventName = reinterpret_cast<const char *>(&retainedBuffer[sizeof(RetainedBufHeader) + sizeof(EventData)]);
		const char *eventData = eventName;
		eventData += strlen(eventData) + 1;

		PublishFlags flags(PublishFlag(data->flags));

		// For reasons that are not entirely obvious to me, you can't use WITH_ACK. If you specify it
		// on the Photon or Electron, Particle.publish will immediately return false. This only happens
		// with this code running in a separate thread. It works fine from the main thread.

		log.info("publishing %s %s ttl=%d flags=%x", eventName, eventData, data->ttl, flags.value());
		bool bResult = Particle.publish(eventName, eventData, data->ttl, flags);
		if (bResult) {
			// Successfully published
			log.info("published successfully");
			discardOldEvent(false);
		}
		else {
			// Did not successfully transmit, try again after retry time
			log.info("published failed, will retry in %lu ms", failureRetryMs);
			stateHandler = &PublishQueueAsync::waitRetryState;
		}
		isSending = false;
		lastPublish = millis();
	}
	else {
		// No event or can't publish yet (not connected or published too recently)
	}

}

void PublishQueueAsync::waitRetryState() {
	if (millis() - lastPublish >= failureRetryMs) {
		stateHandler = &PublishQueueAsync::checkQueueState;
	}
}


// [static]
void PublishQueueAsync::threadFunctionStatic(void *param) {
	static_cast<PublishQueueAsync *>(param)->threadFunction();
}

