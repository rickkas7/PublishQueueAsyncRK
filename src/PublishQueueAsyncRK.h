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
 * And it uses retained memory, so the events are saved you reboot or go into sleep mode. They'll be
 * transmitted when you finally connect to the cloud again.
 *
 * https://github.com/rickkas7/PublishQueueAsyncRK
 * License: MIT
 */
class PublishQueueAsync {
public:
	/**
	 * @brief Construct a publish queue
	 *
	 * You normally allocate one of these as a global object. You should not create more than one, as
	 * the rate limiting would not work right.
	 */
	PublishQueueAsync(uint8_t *retainedBuffer, uint16_t retainedBufferSize);

	/**
	 * @brief You normally allocate this as a global object and never delete it
	 */
	virtual ~PublishQueueAsync();

	/**
	 * @brief Start the thread. You should call this from setup.
	 *
	 * Since version 1.0.1 did not have the setup method, if you don't setup() it will be set up when you first
	 * publish, but starting the thread earlier is suggested.
	 */
	void setup();

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
		return publish(eventName, "", 60, flags1, flags2);
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
	inline 	bool publish(const char *eventName, const char *data, PublishFlags flags1, PublishFlags flags2 = PublishFlags()) {
		return publish(eventName, data, 60, flags1, flags2);
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
	bool publish(const char *eventName, const char *data, int ttl, PublishFlags flags1, PublishFlags flags2 = PublishFlags());


	/**
	 * @brief Sets the retry after publish failure time
	 *
	 * @param value The time in milliseconds (default: 30000, or 30 seconds)
	 *
	 * If a publish fails, this is the amount of time to wait before trying to send the event again.
	 */
	inline PublishQueueAsync &withFailureRetryMs(unsigned long value) { failureRetryMs = value; return *this; };

	/**
	 * @brief Remove any saved events
	 *
	 * @return true if the operation succeeded, or false if an event is currently being sent so the
	 * events cannot be deleted.
	 */
	bool clearEvents();

private:
	bool discardOldEvent(bool secondEvent);

	uint8_t *skipEvent(uint8_t *start);

	void threadFunction();
	static void threadFunctionStatic(void *param);

	void startState();
	void checkQueueState();
	void waitRetryState();

	uint8_t *retainedBuffer;
	uint16_t retainedBufferSize;
	Thread *thread = NULL;
	uint8_t *nextFree;

	unsigned long failureRetryMs = 30000;

	// State handler stuff
	std::function<void(PublishQueueAsync&)> stateHandler = &PublishQueueAsync::startState;
	unsigned long stateTime = 0;
	unsigned long lastPublish = 0;
	bool isSending = false;
	bool haveSetup = false;
};


#endif /* __PUBLISHQUEUEASYNCRK_H */
