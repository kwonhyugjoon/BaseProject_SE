#pragma once
#include "Renderer.h"
#include <algorithm>
#include <cmath>

// Rebuilt once per frame from Actor positions; queries only inspect nearby trunks.
class TreeCollisionGrid
{
  public:
	TreeCollisionGrid(int width, int height)
	    : m_Columns((width + 7) / 8), m_Rows((height + 7) / 8), m_Cells(m_Columns * m_Rows)
	{
	}

	void Clear()
	{
		for (auto& cell : m_Cells)
			cell.clear();
	}

	void Add(Point position)
	{
		m_Cells[Y(position.y) * m_Columns + X(position.x)].push_back(position);
	}

	bool Blocks(Point position) const
	{
		for (int y = Y(position.y - .6f); y <= Y(position.y + .6f); ++y)
			for (int x = X(position.x - .6f); x <= X(position.x + .6f); ++x)
				for (Point tree : m_Cells[y * m_Columns + x])
				{
					float dx = tree.x - position.x, dy = tree.y - position.y;
					if (dx * dx + dy * dy < .6f * .6f)
						return true;
				}
		return false;
	}

  private:
	int X(float value) const
	{
		return std::max(0, std::min(m_Columns - 1, static_cast<int>(std::floor(value / 8))));
	}

	int Y(float value) const
	{
		return std::max(0, std::min(m_Rows - 1, static_cast<int>(std::floor(value / 8))));
	}

	int m_Columns, m_Rows;
	std::vector<std::vector<Point>> m_Cells;
};
