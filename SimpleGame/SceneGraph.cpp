#include "stdafx.h"
#include "SceneGraph.h"
#include <algorithm>

SceneGraph::SceneGraph() : m_Root("Scene")
{
	m_World = &m_Root.AddChild("World");
	m_UI = &m_Root.AddChild("UI");
	m_UI->SetLayer(ActorLayer::UI);
	m_World->m_Protected = true;
	m_UI->m_Protected = true;
}

void SceneGraph::Collect(Actor& actor)
{
	if (actor.m_PendingDestroy)
		return;
	// The parent was already synchronized by this top-down traversal.
	actor.Synchronize(false);
	if (actor.m_Update)
		m_Traversal.push_back(&actor);
	if (actor.m_PositionBorrowed)
		m_BorrowedPositions.push_back(&actor);
	for (const auto& child : actor.m_Children)
		Collect(*child);
	actor.m_TraversalDirty = false;
}

void SceneGraph::CollectDraw(Actor& actor, ActorLayer inheritedLayer)
{
	actor.Synchronize(false);
	if (!actor.m_Visible || actor.m_PendingDestroy)
		return;
	if (actor.m_DrawCull && !actor.m_DrawCull(actor))
		return;
	ActorLayer layer = actor.m_OverrideLayer ? actor.m_Layer : inheritedLayer;
	if (actor.m_Draw)
	{
		auto& entries = m_DrawLayers[static_cast<size_t>(layer)];
		entries.push_back({&actor, actor.m_Position.x + actor.m_Position.y, entries.size()});
	}
	for (const auto& child : actor.m_Children)
		CollectDraw(*child, layer);
}

void SceneGraph::Update(float dt)
{
	if (m_Root.m_TraversalDirty)
	{
		m_Traversal.clear();
		m_BorrowedPositions.clear();
		Collect(m_Root);
	}
	// Borrowed Point references can change without a setter; preserve their propagation.
	for (Actor* actor : m_BorrowedPositions)
		actor->Synchronize();
	// A snapshot defers new nodes until the next update; deletion waits until traversal ends.
	m_Root.m_Traversing = true;
	for (Actor* actor : m_Traversal)
	{
		actor->Synchronize();
		if (actor->m_Update && actor->IsEnabled())
			actor->m_Update(*actor, dt);
		actor->Synchronize();
	}
	m_Root.m_Traversing = false;
	m_Root.Prune();
}

void SceneGraph::Draw(Renderer& renderer, bool ui)
{
	for (auto& entries : m_DrawLayers)
		entries.clear();
	Actor& root = ui ? *m_UI : *m_World;
	if (!root.IsVisible())
		return;
	root.Synchronize();
	CollectDraw(root, root.Layer());
	auto& world = m_DrawLayers[static_cast<size_t>(ActorLayer::World)];
	std::sort(world.begin(),
	          world.end(),
	          [](const DrawEntry& a, const DrawEntry& b)
	          {
		          return a.depth == b.depth ? a.order < b.order : a.depth < b.depth;
	          });
	m_Root.m_Traversing = true;
	for (const auto& entries : m_DrawLayers)
		for (const auto& entry : entries)
		{
			Actor* actor = entry.actor;
			if (!actor->m_Draw || !actor->IsVisible())
				continue;
			renderer.SetDrawOffset(ui ? actor->WorldPosition() : Point{0, 0});
			actor->m_Draw(*actor, renderer);
		}
	renderer.SetDrawOffset({0, 0});
	m_Root.m_Traversing = false;
	for (auto& entries : m_DrawLayers)
		entries.clear();
	m_Root.Prune();
}

void SceneGraph::DrawWorld(Renderer& renderer)
{
	Draw(renderer, false);
}

void SceneGraph::DrawUI(Renderer& renderer)
{
	Draw(renderer, true);
}
