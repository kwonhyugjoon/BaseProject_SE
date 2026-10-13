#pragma once
#include <string>
#include <vector>
#include <map>
#include <initializer_list>
#include <functional>
#include "Dependencies/glew.h"

struct Point
{
	float x, y;
};

struct Color
{
	float r, g, b, a;
};
enum class Material
{
	Plain,
	Grass,
	Dirt,
	Stone,
	Wood,
	Roof,
	Water,
	Sprite,
	Plaster,
	Foliage,
	Flame,
	Pond
};

struct ModelVertex
{
	float x, y, r, g, b, a, material, u, v;
};

class Renderer
{
  public:
	Renderer(int width, int height);
	~Renderer();

	bool IsInitialized() const
	{
		return m_Initialized;
	}

	void Begin(int width, int height, float time, Point worldOrigin);
	void FinishWorld(bool postprocess);

	void SetDrawOffset(Point offset)
	{
		m_DrawOffset = offset;
	}

	void Polygon(const std::vector<Point>& points,
	             Color color,
	             Material material = Material::Plain,
	             const std::vector<Point>& uv = {});
	void Polygon(std::initializer_list<Point> points,
	             Color color,
	             Material material = Material::Plain,
	             std::initializer_list<Point> uv = {});
	void Rect(float x, float y, float w, float h, Color color);
	void Ellipse(float x, float y, float rx, float ry, Color color);
	void SoftEllipse(float x, float y, float rx, float ry, Color color);
	void Character(float x, float y, int facing, int frame, Color coat);
	void Model(Point position, const std::vector<ModelVertex>& vertices, Color tint = {1, 1, 1, 1});
	std::vector<ModelVertex> CaptureModel(Point origin, const std::function<void()>& draw);
	void Flush();
	void Text(float x, float y, const std::wstring& text, Color color);

  private:
	using Vertex = ModelVertex;

	void ResizeTargets();
	void AppendPolygon(
	    const Point* points, size_t count, Color color, Material material, const Point* uv);
	GLuint m_Program = 0, m_Post = 0, m_Buffer = 0, m_Array = 0, m_Atlas = 0;
	GLuint m_Framebuffer = 0, m_Scene = 0;
	GLint m_Viewport = -1, m_Time = -1, m_Origin = -1, m_PostScene = -1, m_PostTexel = -1,
	      m_PostEnabled = -1;
	int m_Width, m_Height;
	float m_Elapsed = 0;
	Point m_WorldOrigin = {0, 0};
	Point m_DrawOffset = {0, 0};
	bool m_Initialized = false, m_TargetsReady = false;
	std::vector<Vertex> m_Vertices;
	std::map<wchar_t, GLuint> m_Glyphs;
	std::vector<GLuint> m_TextLists;
	void* m_Font = nullptr;
};
