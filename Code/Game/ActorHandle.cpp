#include "Game/ActorHandle.hpp"

const ActorHandle ActorHandle::INVALID = ActorHandle(MAX_ACTOR_UID + 1, MAX_ACTOR_INDEX + 1);

ActorHandle::ActorHandle()
	: m_data(0)
{

}

ActorHandle::ActorHandle(unsigned int inUID, unsigned int inIndex)
{
	unsigned int uid = inUID & MAX_ACTOR_UID;
	unsigned int index = inIndex & MAX_ACTOR_INDEX;

	m_data = (uid << 16) | index;
}

bool ActorHandle::IsValid() const
{
	return *this != INVALID;
}

unsigned int ActorHandle::GetIndex() const
{
	return m_data & MAX_ACTOR_INDEX;
}

bool ActorHandle::operator==(const ActorHandle& other) const
{
	unsigned int otherData = other.m_data;

	return m_data == otherData;
}

bool ActorHandle::operator!=(const ActorHandle& other) const
{
	unsigned int otherData = other.m_data;

	return m_data != otherData;
}
