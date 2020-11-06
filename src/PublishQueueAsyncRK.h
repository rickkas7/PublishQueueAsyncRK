#ifndef __PUBLISHQUEUEASYNCRK_H
#define __PUBLISHQUEUEASYNCRK_H

#include "Particle.h"

/**
 * @brief Library for asynchronous Particle.publish on the Particle Photon, Electron, and other devices.
 *
 * This library is designed for fire-and-forget publishing of events. It allows you to publish, even
 * when not connected to the cloud, and the events are saved until connected. It also buffers events
 * so you can call it a bunch of times rapidly and the events are metered out one per second to stay
 * within the publish limits.
 *
 * Also, it's entirely non-blocking. The publishing occurs from a separate thread so the loop is never
 * blocked.
 *
 * Version 0.1.0 and later support storage options including retained memory, FRAM, SPI flash, and
 * SD card.
 *
 * https://github.com/rickkas7/PublishQueueAsyncRK
 * License: MIT
 */

/**
 * @brief Magic bytes used in retained memory and FRAM to detect if the data structures look valid-ish
 */
static const uint32_t PUBLISH_QUEUE_HEADER_MAGIC = 0xd19cab61;

/**
 * @brief Structure stored at the beginning of retained memory.
 *
 * It's followed by a packed event PublishQueueEventData structures.
 *
 * It's also what's stored in FRAM, or in the event file on SPIFFS or SdFat file system.
 */
typedef struct { // 8 bytes
	uint32_t	magic;			//!< PUBLISH_QUEUE_HEADER_MAGIC
	uint16_t	size;			//!< retainedBufferSize, in case it changed, or for file systems, this is the number of events that have been sent already
	uint16_t	numEvents;		//!< number of events, see the PublishQueueEventData structure
} PublishQueueHeader;

/**
 * @brief Event data structure.
 *
 */
typedef struct { // 8 bytes
	int ttl;					//!< Event TTL (not actually used by the cloud, but we can send it up if sent)
	uint8_t flags;				//!< Event flags (like PRIVATE or WITH_ACK)
	uint8_t reserved1;			//!< Not currently used (compiler will pad structure out to 8 bytes anyway)
	uint16_t reserved2;			//!< Not currently used
	// eventName (c-string, packed)
	// eventData (c-string, packed)
	// padded to 4-byte alignment
} PublishQueueEventData;

/**
 * @brief Logger class that logs to app.pubq
 */
extern Logger pubqLogger;

/**
 * @brief Abstract base class for async publish queue.
 *
 * You don't instantiate one of these, you instead instantitate a concrete subclass like:
 * - PublishQueueAsyncRetained
 * - PublishQueueAsyncFRAM
 * - PublishQueueAsyncSpiffs
 * - PublishQueueAsyncSdFat
 */
class PublishQueueAsyncBase {
public:
	/**
	 * @brief Construct a publish queue
	 */
	PublishQueueAsyncBase();

	/**
	 * @brief You normally allocate this as a global object and never delete it
	 */
	virtual ~PublishQueueAsyncBase();

	/**
	 * @brief Start the thread. You must call this from setup.
	 *
	 * Since version 0.0.1 did not have the setup method, if you don't setup() it will be set up when you first
	 * publish, but starting the thread earlier is suggested, but only with retained memory (PublishQueueAsync).
	 *
	 * In 0.1.0 or later you must call setup(), or the library will not function with FRAM, Spiffs, or SdFat.
	 */
	virtual void setup();

	/**
	 * @brief Overload for publishing an event
	 *
	 * @param eventName The name of the event (63 character maximum).
	 *
	 * @param flags1 Normally PRIVATE. You can also use PUBLIC, but one or the other must be specified.
	 *
	 * @param flags2 (optional) You can use NO_ACK or WITH_ACK if desired.
	 *
	 * @return true if the event was queued or false if it was not.
	 *
	 * This function almost always returns true. If you queue more events than fit in the buffer the
	 * oldest (sometimes second oldest) is discarded.
	 */
	inline 	bool publish(const char *eventName, PublishFlags flags1, PublishFlags flags2 = PublishFlags()) {
		return publishCommon(eventName, "", 60, flags1, flags2);
	}

	/**
	 * @brief Overload for publishing an event
	 *
	 * @param eventName The name of the event (63 character maximum).
	 *
	 * @param data The event data (255 bytes maximum, 622 bytes in system firmware 0.8.0-rc.4 and later).
	 *
	 * @param flags1 Normally PRIVATE. You can also use PUBLIC, but one or the other must be specified.
	 *
	 * @param flags2 (optional) You can use NO_ACK or WITH_ACK if desired.
	 *
	 * @return true if the event was queued or false if it was not.
	 *
	 * This function almost always returns true. If you queue more events than fit in the buffer the
	 * oldest (sometimes second oldest) is discarded.
	 */
	inline  bool publish(const char *eventName, const char *data, PublishFlags flags1, PublishFlags flags2 = PublishFlags()) {
		return publishCommon(eventName, data, 60, flags1, flags2);
	}

	/**
	 * @brief Overload for publishing an event
	 *
	 * @param eventName The name of the event (63 character maximum).
	 *
	 * @param data The event data (255 bytes maximum, 622 bytes in system firmware 0.8.0-rc.4 and later).
	 *
	 * @param ttl The time-to-live value. If not specified in one of the other overloads, the value 60 is
	 * used. However, the ttl is ignored by the cloud, so it doesn't matter what you set it to. Essentially
	 * all events are discarded immediately if not subscribed to so they essentially have a ttl of 0.
	 *
	 * @param flags1 Normally PRIVATE. You can also use PUBLIC, but one or the other must be specified.
	 *
	 * @param flags2 (optional) You can use NO_ACK or WITH_ACK if desired.
	 *
	 * @return true if the event was queued or false if it was not.
	 *
	 * This function almost always returns true. If you queue more events than fit in the buffer the
	 * oldest (sometimes second oldest) is discarded.
	 */
	inline  bool publish(const char *eventName, const char *data, int ttl, PublishFlags flags1, PublishFlags flags2 = PublishFlags()) {
		return publishCommon(eventName, data, ttl, flags1, flags2);
	}

	/**
	 * @brief Common publish function. All other overloads lead here. This is a pure virtual function, implemented in subclasses.
	 *
	 * @param eventName The name of the event (63 character maximum).
	 *
	 * @param data The event data (255 bytes maximum, 622 bytes in system firmware 0.8.0-rc.4 and later).
	 *
	 * @param ttl The time-to-live value. If not specified in one of the other overloads, the value 60 is
	 * used. However, the ttl is ignored by the cloud, so it doesn't matter what you set it to. Essentially
	 * all events are discarded immediately if not subscribed to so they essentially have a ttl of 0.
	 *
	 * @param flags1 Normally PRIVATE. You can also use PUBLIC, but one or the other must be specified.
	 *
	 * @param flags2 (optional) You can use NO_ACK or WITH_ACK if desired.
	 *
	 * @return true if the event was queued or false if it was not.
	 *
	 * This function almost always returns true. If you queue more events than fit in the buffer the
	 * oldest (sometimes second oldest) is discarded.
	 */
	virtual bool publishCommon(const char *eventName, const char *data, int ttl, PublishFlags flags1, PublishFlags flags2 = PublishFlags()) = 0;

	/**
	 * @brief Sets the retry after publish failure time
	 *
	 * @param value The time in milliseconds (default: 30000, or 30 seconds)
	 *
	 * If a publish fails, this is the amount of time to wait before trying to send the event again.
	 */
	inline PublishQueueAsyncBase &withFailureRetryMs(unsigned long value) { failureRetryMs = value; return *this; };

	/**
	 * @brief Remove any saved events
	 *
	 * @return true if the operation succeeded, or false if an event is currently being sent so the
	 * events cannot be deleted.
	 */
	virtual bool clearEvents() = 0;

	/**
	 * @brief Get the oldest event that hasn't been published yet
	 *
	 * Returns a pointer to a PublishQueueEventData structure. This will remain valid until getOldestEvent()
	 * is called again.
	 */
	virtual PublishQueueEventData *getOldestEvent() = 0;

	/**
	 * @brief Discards the oldest event or second oldest event
	 *
	 * @param secondEvent True to discard the second oldest event
	 *
	 * When using retained memory, the getOldestEvent() function returns a pointer to retained memory. This
	 * pointer must remain valid while in the process of publishing. If the retained buffer is full, we
	 * want to discard and old event to make room for a newer event, but we can't dispose of the oldest
	 * event, because it may be in use, so we pass true for secondEvent.
	 *
	 * After successfully publishingm this is called with secondEvent = false to discard the event we
	 * just published.
	 *
	 * For file systems (SPIFFS and SdCard), secondEvent is never set to true.
	 */
	virtual bool discardOldEvent(bool secondEvent) = 0;


	/**
	 * @brief Get the number of events in the queue (0 = empty)
	 */
	virtual uint16_t getNumEvents() const = 0;

	/**
	 * @brief Pause publishing, even if it would be allowed because the cloud is connected
	 *
	 * This is used by the test suite, or you could use it in special cases.
	 */
	void setPausePublishing(bool pause) { pausePublishing = pause; };

	/**
	 * @brief Returns true if publishing is manually paused.
	 *
	 * It does not take into account the cloud connection state, only manual pausing.
	 */
	bool getPausePublishing() const { return pausePublishing; };

	/**
	 * @brief Obtain a mutex lock
	 *
	 * This is done before any operation that modifies the retianed buffer or other shared
	 * resources, typically using the StMutexLock class instead of calling mutexLock()
	 * and mutexUnlock() directly.
	 *
	 * Prior to version 0.1.0 there was no mutex and SINGLE_THREADED_BLOCK was used, but the
	 * mutex is more efficient and safer. When using a SINGLE_THREADED_BLOCK there is a greater
	 * chance of accidentally causing deadlock.
	 */
	void mutexLock() const;

	/**
	 * @brief Unlock the mutex
	 */
	void mutexUnlock() const;

	/**
	 * @brief Maximum size of PublishQueueEventData with strings (696 bytes)
	 *
	 * PublishQueueEventData contains flags and ttl (8 bytes)
	 * 65 is the maximum event name length (64) + trailing null
	 * 623 is the maximum event value length (622) + trailing null
	 * It's padded to a 4-byte boundary, by 696 is already at a 4-byte boundary
	 *
	 * Note: This size is not used for the retained memory subclass, but since it's used by
	 * multiple other subclasses, it's included here.
	 */
	static const size_t EVENT_BUF_SIZE = sizeof(PublishQueueEventData) + 65 + 623;

protected:
	/**
	 * @brief The thread function for the publish thread
	 */
	void threadFunction();

	/**
	 * @brief Static version of the thread function
	 */
	static void threadFunctionStatic(void *param);

	/**
	 * @brief Worker thread state machine start handler
	 */
	void startState();

	/**
	 * @brief Worker thread state machine check queue state handler
	 */
	void checkQueueState();

	/**
	 * @brief Worker thread state machine wait to retry publishing state handler
	 */
	void waitRetryState();

	/**
	 * @brief Thread object, created in setup()
	 */
	Thread *thread = NULL;

	/**
	 * @brief Mutex to protect against concurrent access, created in setup()
	 */
	os_mutex_t mutex;

	/**
	 * @brief Default time to wait before trying to publish again after failure
	 *
	 * Default is 30000 milliseconds (30 seconds)
	 */
	unsigned long failureRetryMs = 30000;

	/**
	 * @brief State handler function pointer
	 *
	 * Set to startState, checkQueueState, or waitRetryState
	 */
	std::function<void(PublishQueueAsyncBase&)> stateHandler = &PublishQueueAsyncBase::startState;

	/**
	 * @brief Last millis value for certain state changes like waitRetryState
	 */
	unsigned long stateTime = 0;

	/**
	 * @brief milis() value for the last publish, used to control the frequency of publishes
	 */
	unsigned long lastPublish = 0;

	/**
	 * @brief true if we're currently publishing
	 *
	 * This is used to control whether we discard the oldest or second oldest event if the retained buffer
	 * is full.
	 */
	bool isSending = false;

	/**
	 * @brief True if setup() has been called.
	 *
	 * You should always call the setup() method from setup().
	 */
	bool haveSetup = false;

	/**
	 * @brief True if publishing has been manually paused
	 */
	bool pausePublishing = false;
};

/**
 * @brief Class to store the publish queue in retained memory.
 *
 * Also works for regular RAM, though it won't survive a reset or SLEEP_MODE_DEEP.
 */
class PublishQueueAsyncRetained : public PublishQueueAsyncBase {
public:
	/**
	 * @brief Construct a publish queue
	 *
	 * You normally allocate one of these as a global object. You should not create more than one, as
	 * the rate limiting would not work right.
	 *
	 * @param retainedBuffer Pointer to the buffer in retained or regular memory
	 *
	 * @param retainedBufferSize Buffer size. Must be at least 704 bytes, but it's best for it to be
	 * at least 1024 bytes, and ideally larger than that.
	 */
	PublishQueueAsyncRetained(uint8_t *retainedBuffer, uint16_t retainedBufferSize);

	/**
	 * @brief You normally allocate this as a global object and never delete it
	 */
	virtual ~PublishQueueAsyncRetained();

	/**
	 * @brief Publish an event. All other overloads lead here.
	 *
	 * @param eventName The name of the event (63 character maximum).
	 *
	 * @param data The event data (255 bytes maximum, 622 bytes in system firmware 0.8.0-rc.4 and later).
	 *
	 * @param ttl The time-to-live value. If not specified in one of the other overloads, the value 60 is
	 * used. However, the ttl is ignored by the cloud, so it doesn't matter what you set it to. Essentially
	 * all events are discarded immediately if not subscribed to so they essentially have a ttl of 0.
	 *
	 * @param flags1 Normally PRIVATE. You can also use PUBLIC, but one or the other must be specified.
	 *
	 * @param flags2 (optional) You can use NO_ACK or WITH_ACK if desired.
	 *
	 * @return true if the event was queued or false if it was not.
	 *
	 * This function almost always returns true. If you queue more events than fit in the buffer the
	 * oldest (sometimes second oldest) is discarded.
	 */
	virtual bool publishCommon(const char *eventName, const char *data, int ttl, PublishFlags flags1, PublishFlags flags2 = PublishFlags());

	/**
	 * @brief Get the oldest event that hasn't been published yet
	 *
	 * Returns a pointer to a PublishQueueEventData structure. This will remain valid until getOldestEvent()
	 * is called again.
	 */
	virtual PublishQueueEventData *getOldestEvent();

	/**
	 * @brief Remove any saved events
	 *
	 * @return true if the operation succeeded, or false if an event is currently being sent so the
	 * events cannot be deleted.
	 */
	virtual bool clearEvents();

	/**
	 * @brief Discards the oldest event or second oldest event
	 *
	 * @param secondEvent True to discard the second oldest event
	 *
	 * When using retained memory, the getOldestEvent() function returns a pointer to retained memory. This
	 * pointer must remain valid while in the process of publishing. If the retained buffer is full, we
	 * want to discard and old event to make room for a newer event, but we can't dispose of the oldest
	 * event, because it may be in use, so we pass true for secondEvent.
	 */
	bool discardOldEvent(bool secondEvent);

	/**
	 * @brief Given a pointer into the retained buffer, finds the offset of the next event
	 *
	 * @param start Where to start (pointer to RAM, not an offset)
	 *
	 * @returns A pointer to the beginning of the next event
	 */
	uint8_t *skipEvent(uint8_t *start);

	/**
	 * @brief Get the number of events in the queue (0 = empty)
	 */
	uint16_t getNumEvents() const;


protected:
	uint8_t *retainedBuffer;		//!< Pointer to the beginning of the retained (or regular) RAM buffer
	uint16_t retainedBufferSize;	//!< Size of the buffer in bytes. Must be at least 704 bytes!
	uint8_t *nextFree = NULL;		//!< Pointer into the retained buffer for the next free space (where the next queued publish will be stored)

};

/**
 * @brief Backward compatible API so code build for version 0.0.5 and earlier will still compile
 */
class PublishQueueAsync : public PublishQueueAsyncRetained {
public:
	/**
	 * @brief Construct a publish queue
	 *
	 * You normally allocate one of these as a global object. You should not create more than one, as
	 * the rate limiting would not work right.
	 */
	PublishQueueAsync(uint8_t *retainedBuffer, uint16_t retainedBufferSize) : PublishQueueAsyncRetained(retainedBuffer, retainedBufferSize) {};

	/**
	 * @brief You normally allocate this as a global object and never delete it
	 */
	virtual ~PublishQueueAsync() {};
};

/**
 * @brief Class to automatically lock and unlock the mutex. Create as a variable on the stack.
 *
 * This is used to make sure the mutex is always unlocked, for example if there is a return
 * statement in the middle of the function. When the stack variable goes out of scope it will
 * always unlock the mutex.
 */
class StMutexLock {
public:
	/**
	 * @brief Call the mutexLock() method of publishQueue()
	 *
	 * Instantiate this object on the stack so unlock can be done when the variable goes out of
	 * scope, such as when exiting a block or function.
	 */
	StMutexLock(const PublishQueueAsyncBase *publishQueue) : publishQueue(publishQueue) {
		publishQueue->mutexLock();
	}

	/**
	 * @brief Unlock the mutex on destructor
	 */
	~StMutexLock() {
		publishQueue->mutexUnlock();
	}

	/**
	 * @brief Saved publishQueue, used in destructor
	 */
	const PublishQueueAsyncBase *publishQueue;
};

#if defined(__MB85RC256V_FRAM_RK) || defined(DOXYGEN_BUILD)

/**
 * @brief Support for MB85RC256V-FRAM-RK library.
 *
 * If you include "MB85RC256V-FRAM-RK.h" before PublishQueueAsyncRK.h, this code will be enabled
 */
class PublishQueueAsyncFRAM : public PublishQueueAsyncBase {
public:
	/**
	 * @brief Constrctor for FRAM base class
	 *
	 * @param fram The MB85RC256V object for the FRAM. You must call begin() on the FRAM before calling setup() on this object.
	 *
	 * @param start Optional start address, default is 0 (beginning of FRAM)
	 *
	 * @param len Optional length, default is size of FRAM. Note that this is a length relative to start, not an ending address.
	 */
	PublishQueueAsyncFRAM(MB85RC &fram, size_t start = 0, size_t len = 0) : fram(fram), start(start), len(len) {
		if (len == 0) {
			len = fram.length() - start;
		}
	}

	/**
	 * @brief Destructor. You norrmally allocate one of these as a global variable and don't delete it.
	 */
	virtual ~PublishQueueAsyncFRAM() {

	}

	virtual void setup() {
		// Do superclass setup (starting the thread)
		PublishQueueAsyncBase::setup();

		// Initialize the retained buffer
		bool initBuffer = false;

		if (!fram.readData(start, (uint8_t *)&header, sizeof(PublishQueueHeader))) {
			pubqLogger.error("failed to read FRAM");
			return;
		}

		if (header.magic == PUBLISH_QUEUE_HEADER_MAGIC && header.size == len) {
			// Calculate the next write offset

			nextFree = start + sizeof(PublishQueueHeader);
			for(uint16_t ii = 0; ii < header.numEvents; ii++) {
				nextFree = skipEvent(nextFree, eventBuf);
				if (nextFree > (start + len)) {
					// Overflowed buffer, must be corrupted
					// pubqLogger.info("FRAM contents invalid, reinitializing");
					initBuffer = true;
					break;
				}
			}
		}
		else {
			// Not valid
			initBuffer = true;
			// pubqLogger.info("No magic bytes or invalid length");
		}

		// initBuffer = true; // Uncomment to discard old data

		if (initBuffer) {
			header.magic = PUBLISH_QUEUE_HEADER_MAGIC;
			header.size = len;
			header.numEvents = 0;
			if (!fram.writeData(start, (uint8_t *)&header, sizeof(PublishQueueHeader))) {
				pubqLogger.error("failed to write FRAM");
				return;
			}

			nextFree = start + sizeof(PublishQueueHeader);
			pubqLogger.info("FRAM reinitialized start=%u len=%u", start, len);
		}
		else {
			pubqLogger.info("FRAM numEvents=%u nextFree=%u", header.numEvents, nextFree);
		}

		haveSetup = true;
	}

	virtual bool publishCommon(const char *eventName, const char *data, int ttl, PublishFlags flags1, PublishFlags flags2 = PublishFlags()) {
		if (!haveSetup) {
			return false;
		}

		if (data == NULL) {
			data = "";
		}

		// Size is the size of the header, the two c-strings (with null terminators), rounded up to a multiple of 4
		size_t size = sizeof(PublishQueueEventData) + strlen(eventName) + strlen(data) + 2;
		if ((size % 4) != 0) {
			size += 4 - (size % 4);
		}

		// pubqLogger.info("queueing eventName=%s data=%s ttl=%d flags1=%d flags2=%d size=%d", eventName, data, ttl, flags1.value(), flags2.value(), size);

		if  (size > (len - sizeof(PublishQueueHeader))) {
			// Special case: event is larger than the FRAM. Rather than throw out all events
			// before discovering this, check that case first
			return false;
		}

		while(true) {
			{
				StMutexLock lock(this);

				if ((start + len - nextFree) >= size) {
					// There is room to fit this
					// pubqLogger.info("writing event nextFree=%u size=%u", nextFree, size);

					PublishQueueEventData *eventData = (PublishQueueEventData *)eventBuf;
					eventData->ttl = ttl;
					eventData->flags = flags1.value() | flags2.value();

					char *cp = (char *) eventBuf;
					cp += sizeof(PublishQueueEventData);

					strcpy(cp, eventName);
					cp += strlen(cp) + 1;

					strcpy(cp, data);

					fram.writeData(nextFree, (uint8_t *)&eventBuf, size);

					nextFree += size;
					header.numEvents++;
					fram.writeData(start, (uint8_t *)&header, sizeof(PublishQueueHeader));

					// pubqLogger.info("after writing nextFree=%u numEvents=%u", nextFree, header.numEvents);

					return true;
				}

				pubqLogger.info("need to discard event, FRAM is full");

				// If there's only one event, there's nothing left to discard, this event is too large
				// to fit with the existing first event (which we can't delete because it might be
				// in the process of being sent)
				if (header.numEvents == 1) {
					return false;
				}
			}

			// Discard the oldest event (false) if we're not currently sending.
			// If we are sending (isSending=true), discard the second oldest event
			if (!discardOldEvent(isSending)) {
				// There isn't an event to discard, so we don't have enough room
				return false;
			}
		}

		// Not reached
		return false;
	}

	/**
	 * @brief Get the oldest event that hasn't been published yet
	 *
	 * Returns a pointer to a PublishQueueEventData structure in the publishBuf member variable.
	 * This will remain valid until getOldestEvent() is called again.
	 */
	virtual PublishQueueEventData *getOldestEvent() {
		StMutexLock lock(this);

		if (header.numEvents == 0) {
			return NULL;
		}

		size_t addr = start + sizeof(PublishQueueHeader);
		skipEvent(addr, publishBuf);

		// skipEvent will leave the event in publishBuf, which we then return
		// pubqLogger.trace("getOldestEvent found an event");

		return (PublishQueueEventData *)publishBuf;
	}

	/**
	 * @brief Remove any saved events
	 *
	 * @return Always returns true.
	 *
	 * Since the FRAM code copies the event being published to a separate buffer, the events can always be cleared.
	 */
	virtual bool clearEvents() {
		StMutexLock lock(this);
		header.numEvents = 0;
		fram.writeData(start, (uint8_t *)&header, sizeof(PublishQueueHeader));
		return true;
	}

	/**
	 * @brief Discards the oldest event or second oldest event
	 *
	 * @param secondEvent True to discard the second oldest event
	 *
	 * When using retained memory, the getOldestEvent() function returns a pointer to retained memory. This
	 * pointer must remain valid while in the process of publishing. If the retained buffer is full, we
	 * want to discard and old event to make room for a newer event, but we can't dispose of the oldest
	 * event, because it may be in use, so we pass true for secondEvent.
	 */
	virtual bool discardOldEvent(bool secondEvent) {

		StMutexLock lock(this);

		if (header.numEvents == 0) {
			return false;
		}

		size_t addr = start + sizeof(PublishQueueHeader);
		size_t prevAddr = addr;


		uint16_t ii = 1;

		// Skip the first event
		addr = skipEvent(addr, eventBuf);

		// If we're currently publishing, delete the second event instead
		if (secondEvent) {
			if (header.numEvents == 1) {
				// Only have one event so we can't delete the second event
				return false;
			}
			prevAddr = addr;
			addr = skipEvent(addr, eventBuf);
			ii++;
		}

		// pubqLogger.info("discardOldEvent secondEvent=%d prevAddr=%u addr=%u nextFree=%u", secondEvent, prevAddr, addr, nextFree); // TEMPORARY

		if (nextFree > addr) {
			// pubqLogger.info("moveData from %u to %u len=%u", addr, prevAddr, nextFree - addr);
			fram.moveData(addr, prevAddr, nextFree - addr);
		}
		nextFree -= (addr - prevAddr);

		header.numEvents--;
		fram.writeData(start, (uint8_t *)&header, sizeof(PublishQueueHeader));

		// pubqLogger.info("after numEvents=%u nextFree=%u", header.numEvents, nextFree); // TEMPORARY

		return true;
	}

	/**
	 * @brief Given an address in FRAM, finds the offset of the next event
	 *
	 * @param addr Where to start (address in FRAM, not relative to start!)
	 *
	 * @param buf Buffer to store the event in. Typically either eventBuf or publishBuf.
	 *
	 * @returns Address of the the next event
	 */
	size_t skipEvent(size_t addr, uint8_t *buf) {
		// Read event at start
		size_t count = (start + len) - addr;
		if (count > EVENT_BUF_SIZE) {
			count = EVENT_BUF_SIZE;
		}

		fram.readData(addr, buf, count);

		// pubqLogger.info("skipEvent addr=%u ttl=%d flags=%02x", addr, ((PublishQueueEventData *)buf)->ttl, ((PublishQueueEventData *)buf)->flags);

		size_t next = addr;

		next += sizeof(PublishQueueEventData);
		next += strlen((const char *)&buf[next - addr]) + 1;
		next += strlen((const char *)&buf[next - addr]) + 1;

		// Align
		if ((next % 4) != 0) {
			next += 4 - (next % 4);
		}

		// pubqLogger.trace("skipEvent addr=%u next=%u", addr, next);

		return next;
	}

	/**
	 * @brief Get the number of events in the queue (0 = empty)
	 */
	uint16_t getNumEvents() const {
		uint16_t numEvents = 0;

		{
			StMutexLock lock(this);

			numEvents = header.numEvents;
		}

		return numEvents;
	}


protected:
	MB85RC &fram;		//!< Object for the FRAM
	size_t start;		//!< Start offset (0 = beginning of FRAM)
	size_t len;			//!< Length to use (relative to start!)

	/**
	 * @brief The header, copied from FRAM
	 */
	PublishQueueHeader header;

	/**
	 * @brief This holds a single event during scanning and writing.
	 *
	 * Because the data needs to be in RAM to be written to FRAM, this buffer is required.
	 */
	uint8_t eventBuf[EVENT_BUF_SIZE];

	/**
	 * @brief This holds a single event during publish.
	 *
	 * Because the publish code does not copy the data, we need to keep the data around
	 * even after getOldestEvent() returns. This is the buffer that holds the data.
	 *
	 * It's separate from eventBuf because eventBuf is used to publish new data to the queue.
	 */
	uint8_t publishBuf[EVENT_BUF_SIZE];

	/**
	 * @brief Offset of the next free entry in FRAM. This includes "start" so don't add it again!
	 */
	size_t nextFree = 0;
};

#endif /* __MB85RC256V_FRAM_RK */


#if defined(__SPIFFSPARTICLERK_H) || defined(SdFat_h) || defined(DOXYGEN_BUILD) || defined(HAL_PLATFORM_FILESYSTEM)
#ifndef PUBLISH_QUEUE_USE_FS
#define PUBLISH_QUEUE_USE_FS
#endif
#endif

#ifdef PUBLISH_QUEUE_USE_FS
// This section is for file system-based storage methods (SPIFFS, SdFat, etc.)

/**
 * @brief Abstract base class for file system-based storage
 */
class PublishQueueAsyncFileSystemBase : public PublishQueueAsyncBase {
public:
	/**
	 * @brief Abstract base constructor. Doesn't do anything.
	 */
	PublishQueueAsyncFileSystemBase() {
	}

	/**
	 * @brief Abstract base destructor. Doesn't do anything.
	 */
	virtual ~PublishQueueAsyncFileSystemBase() {

	}
	/**
	 * @brief Open the events file
	 */
	virtual bool openFile() = 0;

	/**
	 * @brief Close the events file
	 */
	virtual bool closeFile() = 0;

	/**
	 * @brief Truncate a file to a specified length in bytes
	 *
	 * @param size Size is bytes. Must be <= the current length of the file.
	 *
	 * Note: Do not use truncate to make the file larger! While this works for POSIX, it
	 * does not work for SPIFFS so we just always assumes it does not work.
	 */
	virtual bool truncate(size_t size) = 0;

	/**
	 * @brief Read bytes from the file
	 *
	 * @param seekTo The file offset to seek to if >= 0. Must be <= file length. Or pass
	 * -1 to seek to the end of the file to append.
	 *
	 * @param buffer Buffer to fill with data
	 *
	 * @param length Number of bytes to read. Can be > than the number of bytes in the file.
	 *
	 * @returns Number of bytes read. Returns 0 on error.
	 */
	virtual size_t readBytes(int seekTo, uint8_t *buffer, size_t length) = 0;

	/**
	 * @brief Write bytes to the file
	 *
	 * @param seekTo The file offset to seek to if >= 0. Must be <= file length. Or pass
	 * -1 to seek to the end of the file to append.
	 *
	 * @param buffer Buffer to write to the file
	 *
	 * @param length Number of bytes to write.
	 */
	virtual size_t writeBytes(int seekTo, const uint8_t *buffer, size_t length) = 0;

	/**
	 * @brief Get length of the file (or a negative error code on error)
	 */
	virtual int getLength() = 0;

};

/**
 * @brief Class to automatically open and close the events file. Create as a variable on the stack.
 *
 * This is used to make sure the file is always closed, for example if there is a return
 * statement in the middle of the function. When the stack variable goes out of scope it will
 * always close the file.
 */
class StFileOpenClose {
public:
	/**
	 * @brief Constructor opens the events file
	 */
	StFileOpenClose(PublishQueueAsyncFileSystemBase *fs) : fs(fs) {
		fs->openFile();
	}

	/**
	 * @brief Destructor closes the events file
	 */
	~StFileOpenClose() {
		fs->closeFile();
	}

	/**
	 * @brief File system base object, used so closeFile() can be called on it from the destructor.
	 */
	PublishQueueAsyncFileSystemBase *fs;
};

/**
 * @brief Abstract class for file system-based events storage (SPIFFS, SdCard, etc.)
 *
 * All of the file systems work in a similar manner, with only a set of virtual methods
 * like openFile(), readBytes, writeBytes(), etc. subclasses for each concrete subclass.
 *
 * Each operation is atomic. The mutex is obtained, the file opened, manipulated,
 * then closed. This less efficient than keeping the file open, but is less likely to
 * lose data if the device is reset. It also makes file system corruption less likely.
 *
 * The file contains binary data, basically the same thing as retained memory, with a
 * few minor changes.
 *
 * The file begins with a file header, just like the retained memory version. It has
 * magic bytes and other fields that can be used to check validity.
 *
 * The size in the file header is not the size of the file, since that's unnecessary.
 * It is, however, the number of events that have already been sent, as explained below.
 *
 * In setup() the file integrity is checked. If the events file is not valid, then the
 * contents are deleted.
 *
 * When a publish is queued, the event is appeneded to the file and the numEvents in the
 * header updated.
 *
 * When a publish is completed, the size in the header (actually, the number of events sent)
 * is incremented. If size == numEvents, then both are set to 0 and the file truncated
 * to the size of the file header.
 *
 * The reason for this is that unlike RAM or FRAM, it's really inefficient to remove
 * data from the beginning of the file. Since the most common situation is that a
 * bunch of events are queued and eventually all of them are transmitted, the code
 * is optimized for this most common situation.
 */
class PublishQueueAsyncFileSystem : public PublishQueueAsyncFileSystemBase {
public:
	/**
	 * @brief Abstract base class constructor
	 */
	PublishQueueAsyncFileSystem() {
	}

	/**
	 * @brief Abstract base class destructor
	 */
	virtual ~PublishQueueAsyncFileSystem() {
	}

	/**
	 * @brief setup() must be called from the main setup() function!
	 */
	virtual void setup() {
		// Do superclass setup (starting the thread)
		PublishQueueAsyncBase::setup();

		// Lock the mutex and open the events file. Closes and unlocks the mutex on exit.
		StMutexLock lock(this);

		{
			StFileOpenClose openClose(this);

			// Initialize the file
			bool initBuffer = false;

			size_t len = (size_t) getLength();

			if (len < sizeof(PublishQueueHeader) || readBytes(0, (uint8_t *)&header, sizeof(PublishQueueHeader)) != sizeof(PublishQueueHeader)) {
				initBuffer = true;
				pubqLogger.info("no data in events file, will generate new");
			}

			if (!initBuffer && header.magic == PUBLISH_QUEUE_HEADER_MAGIC) {
				pubqLogger.trace("numEvents=%u numSent=%u", header.numEvents, header.size);
				oldestPos = sizeof(PublishQueueHeader);

				if (header.numEvents == header.size) {
					pubqLogger.info("all events have been sent, reinitializing");
					initBuffer = true;
				}
				else
				if (header.numEvents > 0) {
					// Calculate the offset of the oldest event and validate the file structure
					size_t addr = sizeof(PublishQueueHeader);
					for(uint16_t ii = 0; ii < header.numEvents; ii++) {
						size_t next = skipEvent(addr, eventBuf);
						if (addr == 0) {
							// Overflowed buffer, must be corrupted
							pubqLogger.info("Overflowed buffer on initial read, reinitializing");
							initBuffer = true;
							break;
						}
						if (ii == header.size) {
							oldestPos = addr;
						}
						addr = next;
					}
					if (!initBuffer) {
						pubqLogger.info("file data looks valid oldestPos=%u", oldestPos);
					}
				}
			}
			else {
				// Not valid
				initBuffer = true;
				pubqLogger.info("No magic bytes or invalid length");
			}

			//initBuffer = true; // Uncomment to discard old data

			if (initBuffer) {
				// In case the file is reused, truncate to zero length before adding in the header
				truncate(0);

				// For file system queues, size is not the size in bytes, but the number of events that have already been sent!
				header.magic = PUBLISH_QUEUE_HEADER_MAGIC;
				header.size = 0;
				header.numEvents = 0;
				if (!writeBytes(0, (uint8_t *)&header, sizeof(PublishQueueHeader))) {
					pubqLogger.error("failed to write file header");
					return;
				}

				oldestPos = sizeof(PublishQueueHeader);
				pubqLogger.info("initialized events file");
			}
			else {
				pubqLogger.info("using events file with size=%u numEvents=%u oldestPos=%u", header.size, header.numEvents, oldestPos);
			}
		}

		haveSetup = true;
	}

	/**
	 * @brief Append the publish data to the events file
	 */
	virtual bool publishCommon(const char *eventName, const char *data, int ttl, PublishFlags flags1, PublishFlags flags2 = PublishFlags()) {
		if (!haveSetup) {
			return false;
		}

		if (data == NULL) {
			data = "";
		}

		// Size is the size of the header, the two c-strings (with null terminators), rounded up to a multiple of 4
		size_t size = sizeof(PublishQueueEventData) + strlen(eventName) + strlen(data) + 2;
		if ((size % 4) != 0) {
			size += 4 - (size % 4);
		}

		//pubqLogger.trace("queueing eventName=%s data=%s ttl=%d flags1=%d flags2=%d size=%d", eventName, data, ttl, flags1.value(), flags2.value(), size);

		StMutexLock lock(this);
		StFileOpenClose openClose(this);

		// pubqLogger.info("writing event size=%u", size);

		PublishQueueEventData *eventData = (PublishQueueEventData *)eventBuf;
		eventData->ttl = ttl;
		eventData->flags = flags1.value() | flags2.value();

		char *cp = (char *) eventBuf;
		cp += sizeof(PublishQueueEventData);

		strcpy(cp, eventName);
		cp += strlen(cp) + 1;

		strcpy(cp, data);

		// Append to file (-1)
		writeBytes(-1, (uint8_t *)&eventBuf, size);

		// Update the file header
		header.numEvents++;
		writeBytes(0, (uint8_t *)&header, sizeof(PublishQueueHeader));

		pubqLogger.trace("after writing numEvents=%u fileLength=%d", header.numEvents, getLength());

		return true;

	}

	/**
	 * @brief Get the oldest event that hasn't been published yet
	 *
	 * Returns a pointer to a PublishQueueEventData structure in the publishBuf member variable.
	 * This will remain valid until getOldestEvent() is called again.
	 */
	virtual PublishQueueEventData *getOldestEvent() {
		StMutexLock lock(this);

		if (header.size >= header.numEvents) {
			return NULL;
		}

		{
			StFileOpenClose openClose(this);

			size_t next = skipEvent(oldestPos, publishBuf);
			if (next == 0) {
				// pubqLogger.trace("getOldestEvent failed, oldestPos=%u, clearing events", oldestPos);
				return NULL;
			}

			// skipEvent will leave the event in publishBuf, which we then return
			// pubqLogger.trace("getOldestEvent found an event at oldestPos=%u, next=%u", oldestPos, next);

			oldestPos = next;

			return (PublishQueueEventData *)publishBuf;
		}
	}

	/**
	 * @brief Remove any saved events
	 *
	 * @return Always returns true.
	 *
	 * Since the FRAM code copies the event being published to a separate buffer, the events can always be cleared.
	 */
	virtual bool clearEvents() {
		StMutexLock lock(this);
		{
			StFileOpenClose openClose(this);

			header.numEvents = header.size = 0;
			writeBytes(0, (uint8_t *)&header, sizeof(PublishQueueHeader));
			return truncate(sizeof(PublishQueueHeader));
		}
	}

	/**
	 * @brief Discard an event
	 *
	 * Note: secondEvent will never be true for file systems, which always append an event
	 */
	virtual bool discardOldEvent(bool secondEvent) {

		StMutexLock lock(this);

		if (header.numEvents == 0) {
			return false;
		}

		{
			StFileOpenClose openClose(this);

			header.size++;
			if (header.size == header.numEvents) {
				// pubqLogger.trace("sent all events, truncating file");
				header.size = header.numEvents = 0;
				oldestPos = sizeof(PublishQueueHeader);
				truncate(oldestPos);
			}
			writeBytes(0, (uint8_t *)&header, sizeof(PublishQueueHeader));

			//pubqLogger.trace("discardOldestEvent numEvents=%u numSent=%u fileLength=%d", header.numEvents, header.size, getLength());

			return false;
		}
	}

	/**
	 * @brief Skip to the next event
	 *
	 * Note: You must obtain a mutex lock and open the events file before calling this!
	 */
	size_t skipEvent(size_t addr, uint8_t *buf) {
		// Read event at addr
		size_t len = (size_t) getLength();

		size_t count = len - addr;
		if (count == 0) {
			// pubqLogger.info("skipEvent called with no more events at len=%u addr=%u", len, addr);
			return 0;
		}

		if (count > EVENT_BUF_SIZE) {
			count = EVENT_BUF_SIZE;
		}

		// pubqLogger.info("skipEvent len=%u count=%u", len, count);

		readBytes(addr, buf, count);

		// pubqLogger.info("skipEvent addr=%u ttl=%d flags=%02x", addr, ((PublishQueueEventData *)buf)->ttl, ((PublishQueueEventData *)buf)->flags);

		size_t next = addr;

		next += sizeof(PublishQueueEventData);
		// pubqLogger.info("skipEvent event=%s", &buf[next - addr]);

		next += strlen((const char *)&buf[next - addr]) + 1;
		// pubqLogger.info("skipEvent data=%s", &buf[next - addr]);

		next += strlen((const char *)&buf[next - addr]) + 1;

		// Align
		if ((next % 4) != 0) {
			next += 4 - (next % 4);
		}

		// pubqLogger.trace("skipEvent addr=%u next=%u", addr, next);

		return next;
	}

	/**
	 * @brief Get the number of events in the queue (0 = empty)
	 */
	uint16_t getNumEvents() const {
		uint16_t numEvents = 0;

		{
			StMutexLock lock(this);

			numEvents = header.numEvents;
		}

		return numEvents;
	}

protected:
	/**
	 * @brief The header, copied from the file system
	 */
	PublishQueueHeader header;

	/**
	 * @brief This holds a single event during scanning and writing.
	 *
	 * Because the data needs to be in RAM to be written to FRAM, this buffer is required.
	 */
	uint8_t eventBuf[EVENT_BUF_SIZE];

	/**
	 * @brief This holds a single event during publish.
	 *
	 * Because the publish code does not copy the data, we need to keep the data around
	 * even after getOldestEvent() returns. This is the buffer that holds the data.
	 *
	 * It's separate from eventBuf because eventBuf is used to publish new data to the queue.
	 */
	uint8_t publishBuf[EVENT_BUF_SIZE];

	/**
	 * @brief Offset in the file for the oldest event. We begin publishing at this offset.
	 *
	 * This is set in setup() and updated in discardOldEvent().
	 */
	size_t oldestPos = 0;
};

#endif /* PUBLISH_QUEUE_USE_FS */

#if defined(__SPIFFSPARTICLERK_H) || defined(DOXYGEN_BUILD)

/**
 * @brief Concrete subclass to store events on SPIFFS file system
 */
class PublishQueueAsyncSpiffs : public PublishQueueAsyncFileSystem {
public:
	/**
	 * @brief Store events on a SPIFFS file system
	 *
	 * @param spiffs The SpiffsParticle object for the file system to store the data on
	 *
	 * @param filename The filename to store the events in.
	 */
	PublishQueueAsyncSpiffs(SpiffsParticle &spiffs, const char *filename) : spiffs(spiffs), filename(filename) {
	}

	/**
	 * @brief Destructor
	 */
	virtual ~PublishQueueAsyncSpiffs() {

	}

	/**
	 * @brief Open the events file
	 */
	virtual bool openFile() {
		file = spiffs.openFile(filename, SPIFFS_O_CREAT|SPIFFS_O_RDWR);
		return true;
	}

	/**
	 * @brief Close the events file
	 */
	virtual bool closeFile() {
		file.close();
		return true;
	}

	/**
	 * @brief Set file position
	 *
	 * @param seekTo The file offset to seek to if >= 0. Must be <= file length. Or pass
	 * -1 to seek to the end of the file to append.
	 *
	 * @returns true on success or false on error
	 */
	virtual bool seek(int seekTo) {
		if (seekTo >= 0) {
			return file.lseek(seekTo, SPIFFS_SEEK_SET) >= 0;
		}
		else {
			return file.lseek(0, SPIFFS_SEEK_END) >= 0;
		}
	}

	/**
	 * @brief Read bytes from the file
	 *
	 * @param seekTo The file offset to seek to if >= 0. Must be <= file length. Or pass
	 * -1 to seek to the end of the file to append.
	 *
	 * @param buffer Buffer to fill with data
	 *
	 * @param length Number of bytes to read. Can be > than the number of bytes in the file.
	 *
	 * @returns Number of bytes read. Returns 0 on error.
	 */
	virtual size_t readBytes(int seekTo, uint8_t *buffer, size_t length) {
		if (!seek(seekTo)) {
			pubqLogger.error("readBytes seek failed seekTo=%d", seekTo);
			return 0;
		}
		return file.readBytes((char *)buffer, length);
	}

	/**
	 * @brief Write bytes to the file
	 *
	 * @param seekTo The file offset to seek to if >= 0. Must be <= file length. Or pass
	 * -1 to seek to the end of the file to append.
	 *
	 * @param buffer Buffer to write to the file
	 *
	 * @param length Number of bytes to write.
	 */
	virtual size_t writeBytes(int seekTo, const uint8_t *buffer, size_t length) {
		if (!seek(seekTo)) {
			pubqLogger.error("writeBytes seek failed seekTo=%d", seekTo);
			return 0;
		}
		return file.write(buffer, length);
	}

	/**
	 * @brief Get length of the file or a negative error code
	 */
	virtual int getLength() {
		return (int) file.length();
	}

	/**
	 * @brief Truncate a file to a specified length in bytes
	 *
	 * Note: Do not use truncate to make the file larger! While this works for POSIX, it
	 * does not work for SPIFFS so we just always assumes it does not work.
	 */
	virtual bool truncate(size_t size) {
		return file.truncate((s32_t)size) == SPIFFS_OK;
	}


protected:
	SpiffsParticle &spiffs;		//!< SpiffsParticle object for the file system to store events on
	String filename;			//!< Name of the events file (set in the constructor)
	SpiffsParticleFile file;	//!< Object for the events file
};

#endif /* __SPIFFSPARTICLERK_H */


#if defined(SdFat_h) || defined(DOXYGEN_BUILD)

/**
 * @brief Concrete subclass for storing events on a SdFat file system
 */
class PublishQueueAsyncSdFat : public PublishQueueAsyncFileSystem {
public:
	/**
	 * @brief Store events on a SdFat file system
	 *
	 * @param sdFat The SdFat object for the file system to store the data on
	 *
	 * @param filename The filename to store the events in should be 8.3 format (events.dat, for example)
	 */
	PublishQueueAsyncSdFat(SdFat &sdFat, const char *filename) : sdFat(sdFat), filename(filename) {
	}

	virtual ~PublishQueueAsyncSdFat() {
	}

	/**
	 * @brief Open the events file
	 */
	virtual bool openFile() {
		return file.open(filename, O_RDWR | O_CREAT) != 0;
	}

	/**
	 * @brief Close the events file
	 */
	virtual bool closeFile() {
		file.close();
		return true;
	}

	/**
	 * @brief Set file position
	 */
	virtual bool seek(int seekTo) {
		if (seekTo >= 0) {
			return file.seekSet(seekTo);
		}
		else {
			return file.seekEnd();
		}
	}

	/**
	 * @brief Read bytes from the file
	 *
	 * @param seekTo The file offset to seek to if >= 0. Must be <= file length. Or pass
	 * -1 to seek to the end of the file to append.
	 *
	 * @param buffer Buffer to fill with data
	 *
	 * @param length Number of bytes to read. Can be > than the number of bytes in the file.
	 *
	 * @returns Number of bytes read. Returns 0 on error.
	 */
	virtual size_t readBytes(int seekTo, uint8_t *buffer, size_t length) {
		if (!seek(seekTo)) {
			pubqLogger.error("readBytes seek failed seekTo=%d", seekTo);
			return 0;
		}
		return file.read((char *)buffer, length);
	}

	/**
	 * @brief Write bytes to the file
	 *
	 * @param seekTo The file offset to seek to if >= 0. Must be <= file length. Or pass
	 * -1 to seek to the end of the file to append.
	 *
	 * @param buffer Buffer to write to the file
	 *
	 * @param length Number of bytes to write.
	 */
	virtual size_t writeBytes(int seekTo, const uint8_t *buffer, size_t length) {
		if (!seek(seekTo)) {
			pubqLogger.error("writeBytes seek failed seekTo=%d", seekTo);
			return 0;
		}
		return file.write(buffer, length);
	}

	/**
	 * @brief Get length of the file or a negative error code
	 */
	virtual int getLength() {
		return (int) file.fileSize();
	}

	/**
	 * @brief Truncate a file to a specified length in bytes
	 *
	 * Note: Do not use truncate to make the file larger! While this works for POSIX, it
	 * does not work for SPIFFS so we just always assumes it does not work.
	 */
	virtual bool truncate(size_t size) {
		return file.truncate((uint32_t)size);
	}


protected:
	SdFat &sdFat;			//!< SdFat object for the file system to store the events on
	String filename;		//!< Filename for the events file (set in constructor)
	SdFile file;			//!< SdFat file object for the events file
};
#endif /* SdFat_h */

#if HAL_PLATFORM_FILESYSTEM
#include <fcntl.h>
#include <sys/stat.h>

/**
 * @brief Concrete subclass for storing events on a Particle Gen 3 LittleFS POSIX file system
 */
class PublishQueueAsyncPOSIX : public PublishQueueAsyncFileSystem {
public:
	/**
	 * @brief Store events on a SdFat file system
	 *
	 * @param filename The filename to store the events in
	 */
	PublishQueueAsyncPOSIX(const char *filename) : filename(filename) {
	}

	virtual ~PublishQueueAsyncPOSIX() {
	}

	/**
	 * @brief Open the events file
	 */
	virtual bool openFile() {
		fd = open(filename, O_RDWR | O_CREAT);

		return (fd != -1);
	}

	/**
	 * @brief Close the events file
	 */
	virtual bool closeFile() {
		if (fd != -1) {
			close(fd);
			fd = -1;
		}
		return true;
	}

	/**
	 * @brief Set file position
	 */
	virtual bool seek(int seekTo) {
		if (seekTo >= 0) {
			return lseek(fd, seekTo, SEEK_SET) >= 0;
		}
		else {
			return lseek(fd, 0, SEEK_END) >= 0;
		}
	}

	/**
	 * @brief Read bytes from the file
	 *
	 * @param seekTo The file offset to seek to if >= 0. Must be <= file length. Or pass
	 * -1 to seek to the end of the file to append.
	 *
	 * @param buffer Buffer to fill with data
	 *
	 * @param length Number of bytes to read. Can be > than the number of bytes in the file.
	 *
	 * @returns Number of bytes read. Returns 0 on error.
	 */
	virtual size_t readBytes(int seekTo, uint8_t *buffer, size_t length) {
		if (!seek(seekTo)) {
			pubqLogger.error("readBytes seek failed seekTo=%d", seekTo);
			return 0;
		}
		int count = read(fd, buffer, length);
		if (count > 0) {
			return count;
		}
		else {
			return 0;
		}
	}

	/**
	 * @brief Write bytes to the file
	 *
	 * @param seekTo The file offset to seek to if >= 0. Must be <= file length. Or pass
	 * -1 to seek to the end of the file to append.
	 *
	 * @param buffer Buffer to write to the file
	 *
	 * @param length Number of bytes to write.
	 */
	virtual size_t writeBytes(int seekTo, const uint8_t *buffer, size_t length) {
		if (!seek(seekTo)) {
			pubqLogger.error("writeBytes seek failed seekTo=%d", seekTo);
			return 0;
		}
		int count = write(fd, buffer, length);
		if (count > 0) {
			// pubqLogger.trace("writeBytes seekTo=%d count=%d length=%u", seekTo, count, length);
			return count;
		}
		else {
			pubqLogger.error("writeBytes failed count=%d length=%u", count, length);
			return 0;
		}
	}

	/**
	 * @brief Get length of the file or a negative error code
	 */
	virtual int getLength() {
		struct stat sb;

		fstat(fd, &sb);
		return sb.st_size;
	}

	/**
	 * @brief Truncate a file to a specified length in bytes
	 *
	 */
	virtual bool truncate(size_t size) {
		// Note: This requires Device OS 2.0.0-rc.3 or later!
		return ftruncate(fd, (s32_t)size) == 0;
	}


protected:
	String filename;		//!< Filename for the events file (set in constructor)
	int fd = -1;			//!< File descriptor for the events file
};

#endif /* HAL_PLATFORM_FILESYSTEM */


#endif /* __PUBLISHQUEUEASYNCRK_H */
