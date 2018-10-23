#ifndef __PUBLISHQUEUEASYNCRK_H
#define __PUBLISHQUEUEASYNCRK_H

#include "Particle.h"

// This is a fork of the regular library to support 0.6.x, which does not support PublishFlags. This
// makes the code more cumbersome, so this is implemented as a fork. It won't be merged back into the
// master branch.

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
	 * @brief Overload for publishing an event
	 *
	 * @param eventName The name of the event (63 character maximum).
	 *
	 * @param flags1 Normally PRIVATE. You can also use PUBLIC, but one or the other must be specified.
	 *
	 * @param flags2 (optional) You can use NO_ACK if desired. You should not use WITH_ACK as events are
	 * not published if you use WITH_ACK and a worker thread as this code does.
	 *
	 * @return true if the event was queued or false if it was not.
	 *
	 * This function almost always returns true. If you queue more events than fit in the buffer the
	 * oldest (sometimes second oldest) is discarded.
	 */
	inline 	bool publish(const char *eventName, PublishFlag flags1 = PUBLIC) {
		return publish(eventName, "", 60, flags1.flag(), 0);
	}

	inline 	bool publish(const char *eventName, PublishFlag flags1, PublishFlag flags2) {
		return publish(eventName, "", 60, flags1.flag(), flags2.flag());
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
	 * @param flags2 (optional) You can use NO_ACK if desired. You should not use WITH_ACK as events are
	 * not published if you use WITH_ACK and a worker thread as this code does.
	 *
	 * @return true if the event was queued or false if it was not.
	 *
	 * This function almost always returns true. If you queue more events than fit in the buffer the
	 * oldest (sometimes second oldest) is discarded.
	 */
	inline 	bool publish(const char *eventName, const char *data, PublishFlag flags1 = PUBLIC) {
		return publish(eventName, data, 60, flags1.flag(), 0);
	}

	inline 	bool publish(const char *eventName, const char *data, PublishFlag flags1, PublishFlag flags2) {
		return publish(eventName, data, 60, flags1.flag(), flags2.flag());
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
	 * @param flags2 (optional) You can use NO_ACK if desired. You should not use WITH_ACK as events are
	 * not published if you use WITH_ACK and a worker thread as this code does.
	 *
	 * @return true if the event was queued or false if it was not.
	 *
	 * This function almost always returns true. If you queue more events than fit in the buffer the
	 * oldest (sometimes second oldest) is discarded.
	 */
	inline bool publish(const char *eventName, const char *data, int ttl, PublishFlag flags1) {
		return publish(eventName, data, 60, flags1.flag(), 0);
	}

	bool publish(const char *eventName, const char *data, int ttl, PublishFlag flags1, PublishFlag flags2);


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
	Thread thread;
	uint8_t *nextFree;

	unsigned long failureRetryMs = 30000;

	// State handler stuff
	std::function<void(PublishQueueAsync&)> stateHandler = &PublishQueueAsync::startState;
	unsigned long stateTime = 0;
	unsigned long lastPublish = 0;
	bool isSending = false;
};


#endif /* __PUBLISHQUEUEASYNCRK_H */
