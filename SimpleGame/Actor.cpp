#include "stdafx.h"
#include "Actor.h"
#include <algorithm>

Actor::Actor(std::string name, Point position, ActorLayer layer)
    : m_Name(std::move(name)), m_Position(position), m_SynchronizedPosition(position),
      m_Layer(layer)
{
}

Actor& Actor::AddChild(std::string name, Point localPosition)
{
	Synchronize();
	Point world = {m_Position.x + localPosition.x, m_Position.y + localPosition.y};
	auto child = std::make_unique<Actor>(std::move(name), world, m_Layer);
	child->m_Parent = this;
	child->m_OverrideLayer = false;
	m_Children.push_back(std::move(child));
	MarkTraversalDirty();
	return *m_Children.back();
}

bool Actor::RemoveChild(Actor& child)
{
	auto found = std::find_if(m_Children.begin(),
	                          m_Children.end(),
	                          [&](const std::unique_ptr<Actor>& entry)
	                          {
		                          return entry.get() == &child;
	                          });
	if (found == m_Children.end() || child.m_Protected)
		return false;
	MarkTraversalDirty();
	if (Traversing())
	{
		child.m_PendingDestroy = true;
		MarkForPrune();
	}
	else
		m_Children.erase(found);
	return true;
}

void Actor::ClearChildren()
{
	for (auto& child : m_Children)
		if (!child->m_Protected)
		{
			child->m_PendingDestroy = true;
			MarkForPrune();
		}
	if (!Traversing())
		Prune();
}

bool Actor::Traversing() const
{
	return m_Parent ? m_Parent->Traversing() : m_Traversing;
}

void Actor::Prune()
{
	if (!m_NeedsPrune)
		return;
	m_NeedsPrune = false;
	m_Children.erase(std::remove_if(m_Children.begin(),
	                                m_Children.end(),
	                                [](const std::unique_ptr<Actor>& child)
	                                {
		                                return child->m_PendingDestroy;
	                                }),
	                 m_Children.end());
	for (const auto& child : m_Children)
		child->Prune();
}

void Actor::MarkForPrune()
{
	MarkTraversalDirty();
	if (m_NeedsPrune)
		return;
	m_NeedsPrune = true;
	if (m_Parent)
		m_Parent->MarkForPrune();
}

void Actor::MarkTraversalDirty()
{
	if (m_TraversalDirty)
		return;
	m_TraversalDirty = true;
	if (m_Parent)
		m_Parent->MarkTraversalDirty();
}

Actor* Actor::Find(const std::string& name)
{
	if (m_PendingDestroy)
		return nullptr;
	if (m_Name == name)
		return this;
	for (const auto& child : m_Children)
		if (Actor* found = child->Find(name))
			return found;
	return nullptr;
}

void Actor::Shift(Point delta)
{
	m_Position.x += delta.x;
	m_Position.y += delta.y;
	m_SynchronizedPosition.x += delta.x;
	m_SynchronizedPosition.y += delta.y;
	for (const auto& child : m_Children)
		child->Shift(delta);
}

void Actor::Synchronize(bool synchronizeParent)
{
	if (synchronizeParent && m_Parent)
		m_Parent->Synchronize();
	Point delta = {m_Position.x - m_SynchronizedPosition.x,
	               m_Position.y - m_SynchronizedPosition.y};
	if (delta.x == 0 && delta.y == 0)
		return;
	m_SynchronizedPosition = m_Position;
	for (const auto& child : m_Children)
		child->Shift(delta);
}

Point& Actor::Position()
{
	Synchronize();
	if (!m_PositionBorrowed)
	{
		m_PositionBorrowed = true;
		MarkTraversalDirty();
	}
	return m_Position;
}

Point Actor::WorldPosition() const
{
	const_cast<Actor*>(this)->Synchronize();
	return m_Position;
}

Point Actor::LocalPosition() const
{
	Point world = WorldPosition();
	Point parent = m_Parent ? m_Parent->WorldPosition() : Point{0, 0};
	return {world.x - parent.x, world.y - parent.y};
}

void Actor::SetWorldPosition(Point position)
{
	Synchronize();
	m_Position = position;
	Synchronize();
}

void Actor::SetLocalPosition(Point position)
{
	Point parent = m_Parent ? m_Parent->WorldPosition() : Point{0, 0};
	SetWorldPosition({parent.x + position.x, parent.y + position.y});
}

bool Actor::IsVisible() const
{
	return !m_PendingDestroy && m_Visible && (!m_Parent || m_Parent->IsVisible());
}

bool Actor::IsEnabled() const
{
	return !m_PendingDestroy && m_Enabled && (!m_Parent || m_Parent->IsEnabled());
}
