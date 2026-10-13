#pragma once
#include "Actor.h"
#include <array>

class SceneGraph
{
  public:
	SceneGraph();

	Actor& Root()
	{
		return m_Root;
	}

	Actor& World()
	{
		return *m_World;
	}

	Actor& UI()
	{
		return *m_UI;
	}

	void Update(float dt);
	void DrawWorld(Renderer& renderer);
	void DrawUI(Renderer& renderer);

  private:
	void Collect(Actor& actor);
	void CollectDraw(Actor& actor, ActorLayer inheritedLayer);
	void Draw(Renderer& renderer, bool ui);
	Actor m_Root;
	Actor* m_World;
	Actor* m_UI;
	std::vector<Actor*> m_Traversal;

	struct DrawEntry
	{
		Actor* actor;
		float depth;
		size_t order;
	};

	std::array<std::vector<DrawEntry>, 5> m_DrawLayers;
};
