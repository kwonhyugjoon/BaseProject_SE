#include "stdafx.h"
#include "Renderer.h"
#include "CharacterSprites.h"
#include "PlatformPaths.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstddef>
#include <array>

namespace
{
	template <size_t Count> const std::array<Point, Count>& UnitCircle()
	{
		static const auto points = []
		{
			std::array<Point, Count> result;
			for (size_t i = 0; i < Count; ++i)
			{
				float angle = i * 6.2831853f / Count;
				result[i] = {std::cos(angle), std::sin(angle)};
			}
			return result;
		}();
		return points;
	}

	std::string ReadShader(const char* filename)
	{
		const std::filesystem::path paths[] = {
		    ResourcePath((std::string("Shaders/") + filename).c_str())};

		for (const auto& path : paths)
		{
			std::ifstream file(path);

			if (file)
			{
				std::ostringstream contents;
				contents << file.rdbuf();
				std::string source = contents.str();

				if (source.compare(0, 3, "\xEF\xBB\xBF") == 0)
					source.erase(0, 3);
				return source;
			}
		}
		std::cerr << "Cannot load shader: " << filename << '\n';
		return {};
	}

	GLuint Shader(GLenum type, const std::string& source)
	{
		GLuint shader = glCreateShader(type);
		const char* text = source.c_str();
		glShaderSource(shader, 1, &text, nullptr);
		glCompileShader(shader);
		GLint ok = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);

		if (!ok)
		{
			char log[2048] = {};
			glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
			std::cerr << log << '\n';
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}

	GLuint Program(const char* vertex, const char* fragment)
	{
		GLuint vs = Shader(GL_VERTEX_SHADER, ReadShader(vertex)),
		       fs = Shader(GL_FRAGMENT_SHADER, ReadShader(fragment));

		if (!vs || !fs)
		{
			if (vs)
				glDeleteShader(vs);

			if (fs)
				glDeleteShader(fs);
			return 0;
		}
		GLuint program = glCreateProgram();
		glAttachShader(program, vs);
		glAttachShader(program, fs);
		glLinkProgram(program);
		glDeleteShader(vs);
		glDeleteShader(fs);
		GLint ok = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &ok);

		if (!ok)
		{
			char log[2048] = {};
			glGetProgramInfoLog(program, sizeof(log), nullptr, log);
			std::cerr << log << '\n';
			glDeleteProgram(program);
			return 0;
		}
		return program;
	}
}

Renderer::Renderer(int width, int height) : m_Width(width), m_Height(height)
{
	m_Program = Program("SolidRect.vs", "SolidRect.fs");
	m_Post = Program("PostProcess.vs", "PostProcess.fs");

	if (!m_Program || !m_Post)
		return;
	m_Viewport = glGetUniformLocation(m_Program, "u_Viewport");
	m_Time = glGetUniformLocation(m_Program, "u_Time");
	m_Origin = glGetUniformLocation(m_Program, "u_Origin");
	glUseProgram(m_Program);
	glUniform1i(glGetUniformLocation(m_Program, "u_Atlas"), 1);
	glUniform1i(glGetUniformLocation(m_Program, "u_Font"), 2);
	m_PostScene = glGetUniformLocation(m_Post, "u_Scene");
	m_PostTexel = glGetUniformLocation(m_Post, "u_Texel");
	m_PostEnabled = glGetUniformLocation(m_Post, "u_Enabled");
	glGenVertexArrays(1, &m_Array);
	glGenBuffers(1, &m_Buffer);
	glBindVertexArray(m_Array);
	glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
	    2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, material));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));
	glBindVertexArray(0);
	glGenTextures(1, &m_Atlas);
	glBindTexture(GL_TEXTURE_2D, m_Atlas);
	auto pixels = CreateCharacterAtlas();
	glTexImage2D(GL_TEXTURE_2D,
	             0,
	             GL_RGBA8,
	             SpriteWidth * SpriteFrames,
	             SpriteHeight * SpriteDirections,
	             0,
	             GL_RGBA,
	             GL_UNSIGNED_BYTE,
	             pixels.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glGenFramebuffers(1, &m_Framebuffer);
	glGenTextures(1, &m_Scene);
	ResizeTargets();
	if (!m_Font.Initialize())
	{
		std::cerr << "Cannot initialize the UI font. Check Fonts/NanumGothic-Regular.ttf.\n";
		return;
	}
	glGenTextures(1, &m_FontAtlas);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_FontAtlas);
	std::vector<unsigned char> empty(FontAtlasSize * FontAtlasSize, 0);
	glTexImage2D(GL_TEXTURE_2D,
	             0,
	             GL_R8,
	             FontAtlasSize,
	             FontAtlasSize,
	             0,
	             GL_RED,
	             GL_UNSIGNED_BYTE,
	             empty.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glActiveTexture(GL_TEXTURE0);
	GetGlyph('?');
	m_Initialized = true;
}

Renderer::~Renderer()
{
	glDeleteTextures(1, &m_FontAtlas);
	glDeleteBuffers(1, &m_Buffer);
	glDeleteVertexArrays(1, &m_Array);
	glDeleteProgram(m_Program);
	glDeleteProgram(m_Post);
	glDeleteTextures(1, &m_Scene);
	glDeleteTextures(1, &m_Atlas);
	glDeleteFramebuffers(1, &m_Framebuffer);
}

void Renderer::ResizeTargets()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_Scene);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_Scene, 0);
	m_TargetsReady = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

	if (!m_TargetsReady)
		std::cerr << "Postprocess framebuffer unavailable; rendering directly.\n";
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::Begin(int width, int height, float time, Point worldOrigin)
{
	m_DrawOffset = {0, 0};
	if (width != m_Width || height != m_Height)
	{
		m_Width = width;
		m_Height = height;
		ResizeTargets();
	}
	m_Elapsed = time;
	m_WorldOrigin = worldOrigin;
	m_Vertices.clear();
	glBindFramebuffer(GL_FRAMEBUFFER, m_TargetsReady ? m_Framebuffer : 0);
	glViewport(0, 0, width, height);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(.055f, .105f, .12f, 1);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::Polygon(const std::vector<Point>& points,
                       Color c,
                       Material material,
                       const std::vector<Point>& uv)
{
	AppendPolygon(points.data(),
	              points.size(),
	              c,
	              material,
	              uv.size() == points.size() ? uv.data() : nullptr);
}

void Renderer::Polygon(std::initializer_list<Point> points,
                       Color c,
                       Material material,
                       std::initializer_list<Point> uv)
{
	// Immediate-mode shapes borrow their stack arrays instead of allocating two vectors.
	AppendPolygon(points.begin(),
	              points.size(),
	              c,
	              material,
	              uv.size() == points.size() ? uv.begin() : nullptr);
}

void Renderer::AppendPolygon(
    const Point* points, size_t count, Color c, Material material, const Point* uv)
{
	// Negative material IDs select surface coordinates instead of ground coordinates.
	bool local = uv != nullptr;
	float id = local ? -(float)material - 1 : (float)material;

	for (size_t i = 1; i + 1 < count; ++i)
		for (size_t index : {size_t(0), i, i + 1})
		{
			Point p = {points[index].x + m_DrawOffset.x, points[index].y + m_DrawOffset.y};
			Point t = local ? uv[index] : Point{0, 0};
			m_Vertices.push_back({p.x, p.y, c.r, c.g, c.b, c.a, id, t.x, t.y});
		}
}

void Renderer::Rect(float x, float y, float w, float h, Color c)
{
	Polygon({{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}}, c);
}

void Renderer::Ellipse(float x, float y, float rx, float ry, Color c)
{
	const auto& circle = UnitCircle<32>();
	std::array<Point, 32> points;

	for (size_t i = 0; i < points.size(); ++i)
	{
		points[i] = {x + circle[i].x * rx, y + circle[i].y * ry};
	}
	AppendPolygon(points.data(), points.size(), c, Material::Plain, nullptr);
}

void Renderer::SoftEllipse(float x, float y, float rx, float ry, Color c)
{
	x += m_DrawOffset.x;
	y += m_DrawOffset.y;
	const auto& circle = UnitCircle<24>();
	for (int i = 0; i < 24; ++i)
	{
		Point a = circle[i], b = circle[(i + 1) % circle.size()];
		m_Vertices.push_back({x, y, c.r, c.g, c.b, c.a, 0, 0, 0});
		m_Vertices.push_back({x + a.x * rx, y + a.y * ry, c.r, c.g, c.b, 0, 0, 0, 0});
		m_Vertices.push_back({x + b.x * rx, y + b.y * ry, c.r, c.g, c.b, 0, 0, 0, 0});
	}
}

void Renderer::Character(float x, float y, int facing, int frame, Color coat)
{
	x += m_DrawOffset.x;
	y += m_DrawOffset.y;
	float u0 = (float)frame / SpriteFrames, u1 = (float)(frame + 1) / SpriteFrames;
	float v0 = (float)facing / SpriteDirections, v1 = (float)(facing + 1) / SpriteDirections;
	Vertex a = {x - 24, y - 60, coat.r, coat.g, coat.b, coat.a, 7, u0, v0};
	Vertex b = {x + 24, y - 60, coat.r, coat.g, coat.b, coat.a, 7, u1, v0};
	Vertex c = {x + 24, y + 4, coat.r, coat.g, coat.b, coat.a, 7, u1, v1};
	Vertex d = {x - 24, y + 4, coat.r, coat.g, coat.b, coat.a, 7, u0, v1};

	for (const auto& vertex : {a, b, c, a, c, d})
		m_Vertices.push_back(vertex);
}

void Renderer::Model(Point position, const std::vector<ModelVertex>& vertices, Color tint)
{
	position.x += m_DrawOffset.x;
	position.y += m_DrawOffset.y;
	size_t begin = m_Vertices.size();
	m_Vertices.insert(m_Vertices.end(), vertices.begin(), vertices.end());
	Vertex* appended = m_Vertices.data();
	for (size_t i = begin; i < m_Vertices.size(); ++i)
	{
		Vertex& vertex = appended[i];
		vertex.x += position.x;
		vertex.y += position.y;
		vertex.r *= tint.r;
		vertex.g *= tint.g;
		vertex.b *= tint.b;
		vertex.a *= tint.a;
	}
}

std::vector<ModelVertex> Renderer::CaptureModel(Point origin, const std::function<void()>& draw)
{
	std::vector<Vertex> saved;
	saved.swap(m_Vertices);
	draw();
	std::vector<ModelVertex> model;
	model.swap(m_Vertices);
	m_Vertices.swap(saved);
	for (auto& vertex : model)
	{
		vertex.x -= origin.x + m_DrawOffset.x;
		vertex.y -= origin.y + m_DrawOffset.y;
	}
	return model;
}

void Renderer::Flush()
{
	if (m_Vertices.empty())
		return;
	glUseProgram(m_Program);
	glUniform2f(m_Viewport, (float)m_Width, (float)m_Height);
	glUniform1f(m_Time, m_Elapsed);
	glUniform2f(m_Origin, m_WorldOrigin.x, m_WorldOrigin.y);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_Atlas);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_FontAtlas);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(m_Array);
	glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
	glBufferData(
	    GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(Vertex), m_Vertices.data(), GL_STREAM_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_Vertices.size());
	glBindVertexArray(0);
	m_Vertices.clear();
}

void Renderer::FinishWorld(bool postprocess)
{
	Flush();

	if (!m_TargetsReady)
		return;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_BLEND);
	glUseProgram(m_Post);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_Scene);
	glUniform1i(m_PostScene, 0);
	glUniform2f(m_PostTexel, 1.f / m_Width, 1.f / m_Height);
	glUniform1i(m_PostEnabled, postprocess ? 1 : 0);
	glBindVertexArray(m_Array);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
	glEnable(GL_BLEND);
}

const Renderer::Glyph& Renderer::GetGlyph(uint32_t codepoint)
{
	auto found = m_Glyphs.find(codepoint);
	if (found != m_Glyphs.end())
		return found->second;
	auto bitmap = m_Font.Rasterize(codepoint);
	Glyph glyph = {bitmap.width, bitmap.height, bitmap.left, bitmap.top, bitmap.advance, 0, 0};
	if (glyph.width && glyph.height)
	{
		if (m_FontX + glyph.width + 1 >= FontAtlasSize)
		{
			m_FontX = 1;
			m_FontY += m_FontRowHeight + 1;
			m_FontRowHeight = 0;
		}
		if (m_FontY + glyph.height + 1 >= FontAtlasSize)
			return m_Glyphs.emplace(codepoint, GetGlyph('?')).first->second;
		glyph.x = m_FontX;
		glyph.y = m_FontY;
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_FontAtlas);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexSubImage2D(GL_TEXTURE_2D,
		                0,
		                glyph.x,
		                glyph.y,
		                glyph.width,
		                glyph.height,
		                GL_RED,
		                GL_UNSIGNED_BYTE,
		                bitmap.pixels.data());
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glActiveTexture(GL_TEXTURE0);
		m_FontX += glyph.width + 1;
		m_FontRowHeight = std::max(m_FontRowHeight, glyph.height);
	}
	return m_Glyphs.emplace(codepoint, glyph).first->second;
}

void Renderer::Text(float x, float y, const std::wstring& text, Color c)
{
	x += m_DrawOffset.x;
	y += m_DrawOffset.y;
	float start = x;
	for (size_t i = 0; i < text.size(); ++i)
	{
		uint32_t codepoint = static_cast<uint32_t>(text[i]);
		if (sizeof(wchar_t) == 2 && codepoint >= 0xd800 && codepoint <= 0xdbff &&
		    i + 1 < text.size() && text[i + 1] >= 0xdc00 && text[i + 1] <= 0xdfff)
			codepoint = 0x10000 + ((codepoint - 0xd800) << 10) + (text[++i] - 0xdc00);
		if (codepoint == '\n')
		{
			x = start;
			y += 24;
			continue;
		}
		const auto& glyph = GetGlyph(codepoint);
		if (glyph.width && glyph.height)
		{
			float left = x + glyph.left, top = y + 18 + glyph.top;
			float right = left + glyph.width, bottom = top + glyph.height;
			float u0 = glyph.x / static_cast<float>(FontAtlasSize);
			float v0 = glyph.y / static_cast<float>(FontAtlasSize);
			float u1 = (glyph.x + glyph.width) / static_cast<float>(FontAtlasSize);
			float v1 = (glyph.y + glyph.height) / static_cast<float>(FontAtlasSize);
			const Point points[] = {{left, top}, {right, top}, {right, bottom}, {left, bottom}};
			const Point uv[] = {{u0, v0}, {u1, v0}, {u1, v1}, {u0, v1}};
			for (int index : {0, 1, 2, 0, 2, 3})
				m_Vertices.push_back({points[index].x,
				                      points[index].y,
				                      c.r,
				                      c.g,
				                      c.b,
				                      c.a,
				                      static_cast<float>(Material::Font),
				                      uv[index].x,
				                      uv[index].y});
		}
		x += glyph.advance;
	}
}
