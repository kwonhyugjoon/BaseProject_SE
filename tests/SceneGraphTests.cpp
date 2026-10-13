#include "../SimpleGame/SceneGraph.h"
#include <cassert>
#include <cmath>

namespace
{
	void Expect(Point actual, Point expected)
	{
		assert(std::abs(actual.x - expected.x) < .0001f);
		assert(std::abs(actual.y - expected.y) < .0001f);
	}
}

int main()
{
	SceneGraph scene;
	Actor& parent = scene.World().AddChild("Parent", {10, 20});
	Actor& child = parent.AddChild("Child", {2, 3});
	Actor& grandchild = child.AddChild("Grandchild", {1, 1});
	Expect(child.WorldPosition(), {12, 23});
	Expect(grandchild.WorldPosition(), {13, 24});
	parent.SetLocalPosition({20, 30});
	Expect(child.WorldPosition(), {22, 33});
	Expect(grandchild.WorldPosition(), {23, 34});
	child.SetWorldPosition({25, 36});
	Expect(child.LocalPosition(), {5, 6});
	Expect(grandchild.WorldPosition(), {26, 37});

	// Existing game state borrows this point; scene synchronization must propagate edits once.
	Point& borrowed = parent.Position();
	borrowed.x += 4;
	scene.Update(0);
	Expect(child.WorldPosition(), {29, 36});
	scene.Update(0);
	Expect(child.WorldPosition(), {29, 36});
	Actor& lateChild = parent.AddChild("LateChild", {1, 2});
	Expect(lateChild.WorldPosition(), {25, 32});

	int ticks = 0;
	child.SetUpdate(
	    [&](Actor&, float)
	    {
		    ++ticks;
	    });
	parent.SetEnabled(false);
	scene.Update(.1f);
	assert(ticks == 0);
	parent.SetEnabled(true);
	parent.SetVisible(false);
	scene.Update(.1f);
	assert(ticks == 1 && !grandchild.IsVisible());
	parent.SetVisible(true);
	assert(grandchild.IsVisible());
	parent.SetLayer(ActorLayer::Ground);
	assert(child.Layer() == ActorLayer::Ground);
	child.SetLayer(ActorLayer::Shadow);
	parent.SetLayer(ActorLayer::Effect);
	assert(child.Layer() == ActorLayer::Shadow && lateChild.Layer() == ActorLayer::Effect);

	Actor& owner = scene.World().AddChild("Owner");
	int removedTicks = 0, newTicks = 0;
	Actor& remover = owner.AddChild("Remover");
	Actor& victim = owner.AddChild("Victim");
	victim.SetUpdate(
	    [&](Actor&, float)
	    {
		    ++removedTicks;
	    });
	victim.AddChild("VictimChild")
	    .SetUpdate(
	        [&](Actor&, float)
	        {
		        ++removedTicks;
	        });
	remover.SetUpdate(
	    [&](Actor& self, float)
	    {
		    owner.RemoveChild(victim);
		    owner.AddChild("New").SetUpdate(
		        [&](Actor&, float)
		        {
			        ++newTicks;
		        });
		    self.SetEnabled(false);
	    });
	scene.Update(.1f);
	assert(removedTicks == 0 && newTicks == 0 && owner.Find("Victim") == nullptr);
	scene.Update(.1f);
	assert(newTicks == 1);
	Actor& clearing = owner.AddChild("Clearing");
	clearing.SetUpdate(
	    [&](Actor&, float)
	    {
		    owner.ClearChildren();
	    });
	scene.Update(.1f);
	assert(owner.Children().empty());
	// A second deletion must mark ancestors again after the previous prune.
	Actor& recreated = owner.AddChild("Recreated");
	recreated.SetUpdate(
	    [&](Actor& self, float)
	    {
		    owner.RemoveChild(self);
	    });
	scene.Update(.1f);
	assert(owner.Children().empty());
	scene.Update(.1f);
	assert(owner.Children().empty());
	assert(!scene.Root().RemoveChild(scene.World()));
	assert(!scene.Root().RemoveChild(scene.UI()));
	assert(scene.Root().Find("Child") == &child);
}
