#ifndef MESSAGES_H_
#define MESSAGES_H_
#include <vector>
#include <memory>
#include <deque>
#include <atomic>

class MessagePublisher;
class MessageChannel;

/*
Endpoints may represent a character or an object able to receive messages. Each
endpoint has a unique ID and timestamp of last access.  An endpoint may have
multiple messages queues, perhaps one queue per each sender. An endpoint starts
out with zero queues, and every time a new sender sends it a message it adds a
new one for that sender. The ID may be, for example, a player or object's UUID.

Each message queue has its endpoint ID and a queue ID. The queue ID may be the ID
of the sender, and if the sender is also an endpoint, its ID will be the ID of
its endpoint. (Endpoints can be both receivers and publishers at the same time,
though some endpoints may only be used as one or the other - it doesn't matter
in the data model.)

A message queue stores messages. Each message has a queue ID, a message
timestamp, and the message data. Each message queue can store a maximum number
of messages; when the messages begin to go over the limit, the oldest entries
are overwritten with the new entries, as in a circular queue.

Upon creation, each endpoint is registered in a central message bus (message
 broker, whatever you want to call it). Publishers wishing to message an
 endpoint find the ID of the endpoint in the broker, either directly or through
 an associated name. With every search, endpoints are checked for the last time
 they were used and are destroyed if they have not been used for too
 long. Endpoints should be sorted by ID and date of last access from oldest to
 newest, so that in the course of a linear search the first endpoint not old
 enough to be removed marks the beginning of all the good endpoints; that way we
 don't have to check every one. Endpoint IDs are stored as a foreign key with
 the object or character they are associated with.

If an endpoint is connected to an active message sink (i.e: an online object or
player), the endpoint will forward received messages immediately, in the fashion
of chain of responsibility: if an object carried by a player character receives
a message while the player is connected to the character, the object will notify
the character, and the character will notify its connected players.

Active engine objects, whether objects or characters, declare interest in
receiving specific types of messages by registering callbacks with the message
broker, and are associated with an endpoint queue by the broker - if one does
not already exist for the object, one will be created. Backlogged messages that
arrived on that endpoint queue are sent through the callbacks either immediately
or upon request.

Whether or not an active object is attached to it, existing endpoints still
store messages. If an endpoint is being actively messaged, it will be retained
in memory to enqueue new messages, and periodically persisted. Inactive
endpoints will stay on disk, and are searched for and loaded on demand when
messages come in for them. Active queues that fall below a threshhold of
activity will be persisted and removed from memory to make room for other
queues, as necessary.

Outgoing queues must be provided to permit time-delayed messages, and could be
associated with an endpoint similarly to incoming queues though they would need
to store more information: in addition to a queue ID, timestamp, and message
data, each message would also contain the endpoint ID of the intended recipient,
an additional timestamp for date of delivery, and a message type field.

 */

class MessageSubscriber
{
public:
	//MessageSubscribers must be stored in dynamically allocated memory
	//so that the validity of the this pointer here is guaranteed
	MessageSubscriber();
	~MessageSubscriber();
	void Subscribe(std::weak_ptr<MessagePublisher> publisher);
	void Unsubscribe();
	void OnReceivedMessage(const std::string& msg);
private:
	std::shared_ptr<MessageChannel> m_channel;
	std::weak_ptr<MessagePublisher> m_publisher;
};

class MessageChannel
{
	//MessageChannel shall be owned by MessageSubscriber,
	//therefore its pointer will always be valid inside this class
	friend class MessageSubscriber;
	MessageChannel(MessageSubscriber* endpoint)
	{
		m_pEndpoint = endpoint;
	}
public:
	~MessageChannel()
	{
	}

	void PushMessage(const std::string& msg)
	{
		m_pEndpoint->OnReceivedMessage(msg);
	}

private:
	MessageSubscriber* m_pEndpoint;
};

class MessagePublisher
{

public:
	void PushMessage(const std::string& msg)
	{
		pthread_rwlock_rdlock(&m_subchans_rwlock);
		for(auto it = m_subchans.begin(); it != m_subchans.end(); ++it)
		{
			if(auto chan = it->lock())
			{
				chan->PushMessage(msg);
			}
		}
		pthread_rwlock_unlock(&m_subchans_rwlock);
	}

	void AttachChannel(std::shared_ptr<MessageChannel> channel)
	{
		pthread_rwlock_wrlock(&m_subchans_rwlock);
		m_subchans.push_back(channel);
		pthread_rwlock_unlock(&m_subchans_rwlock);
	}

	void DetachChannel(std::shared_ptr<MessageChannel> channel)
	{
		pthread_rwlock_wrlock(&m_subchans_rwlock);
		for(auto it = m_subchans.begin(); it != m_subchans.end();)
		{
			if(it->lock() == channel)
			{
				auto erasethis = it;
				++it;
				m_subchans.erase(erasethis);
				continue;
			}
			++it;
		}
		pthread_rwlock_unlock(&m_subchans_rwlock);
	}

	MessagePublisher()
	{
		pthread_rwlock_init(&m_subchans_rwlock, 0);
	}

	~MessagePublisher()
	{
		pthread_rwlock_destroy(&m_subchans_rwlock);
	}
private:
	std::vector<std::weak_ptr<MessageChannel> > m_subchans;
	pthread_rwlock_t m_subchans_rwlock;
};

#endif
