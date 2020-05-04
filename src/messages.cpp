#include "messages.h"

MessageSubscriber::MessageSubscriber()
{
	MessageChannel* newchan = new MessageChannel(this);
	m_channel = std::shared_ptr<MessageChannel>(newchan);
}

MessageSubscriber::~MessageSubscriber()
{
	Unsubscribe();
}


void MessageSubscriber::Subscribe(std::weak_ptr<MessagePublisher> publisher)
{
	m_publisher = std::move(publisher);
	if(auto pub = publisher.lock())
	{
		pub->AttachChannel(m_channel);
	}
}

void MessageSubscriber::Unsubscribe()
{
	if(auto pub = m_publisher.lock())
	{
		pub->DetachChannel(m_channel);
	}
}

void MessageSubscriber::OnReceivedMessage(const std::string& msg)
{

}
