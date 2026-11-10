#pragma once
#include "Renderer.h"
#include <functional>
#include <memory>
#include <utility>

enum class ActorLayer
{
	Ground,
	Shadow,
	World,
	Effect,
	UI
};

class SceneGraph;

class Actor
{
  public:
	using UpdateFunction = std::function<void(Actor&, float)>;
	using DrawFunction = std::function<void(Actor&, Renderer&)>;

	explicit Actor(std::string name, Point position = {0, 0}, ActorLayer layer = ActorLayer::World);
	Actor(const Actor&) = delete;
	Actor& operator=(const Actor&) = delete;

	Actor& AddChild(std::string name, Point localPosition = {0, 0});
	bool RemoveChild(Actor& child);
	void ClearChildren();
	Actor* Find(const std::string& name);

	Actor* Parent() const
	{
		return m_Parent;
	}

	const std::vector<std::unique_ptr<Actor>>& Children() const
	{
		return m_Children;
	}

	const std::string& Name() const
	{
		return m_Name;
	}

	Point& Position();
	Point WorldPosition() const;
	Point LocalPosition() const;
	void SetLocalPosition(Point position);
	void SetWorldPosition(Point position);

	void SetVisible(bool visible)
	{
		m_Visible = visible;
	}

	bool IsVisible() const;

	void SetEnabled(bool enabled)
	{
		m_Enabled = enabled;
	}

	bool IsEnabled() const;

	void SetLayer(ActorLayer layer)
	{
		m_Layer = layer;
		m_OverrideLayer = true;
	}

	ActorLayer Layer() const
	{
		return m_OverrideLayer || !m_Parent ? m_Layer : m_Parent->Layer();
	}

	void SetUpdate(UpdateFunction update)
	{
		if (static_cast<bool>(m_Update) != static_cast<bool>(update))
			MarkTraversalDirty();
		m_Update = std::move(update);
	}

	void SetDraw(DrawFunction draw)
	{
		m_Draw = std::move(draw);
	}

	void SetDrawCull(std::function<bool(const Actor&)> visible)
	{
		m_DrawCull = std::move(visible);
	}

  private:
	friend class SceneGraph;
	void Synchronize(bool synchronizeParent = true);
	void Shift(Point delta);
	bool Traversing() const;
	void Prune();
	void MarkForPrune();
	void MarkTraversalDirty();
	std::string m_Name;
	Actor* m_Parent = nullptr;
	Point m_Position, m_SynchronizedPosition;
	ActorLayer m_Layer;
	bool m_Visible = true, m_Enabled = true;
	bool m_NeedsPrune = false;
	bool m_TraversalDirty = true, m_PositionBorrowed = false;
	bool m_OverrideLayer = true, m_PendingDestroy = false, m_Traversing = false,
	     m_Protected = false;
	std::vector<std::unique_ptr<Actor>> m_Children;
	UpdateFunction m_Update;
	DrawFunction m_Draw;
	std::function<bool(const Actor&)> m_DrawCull;
};
