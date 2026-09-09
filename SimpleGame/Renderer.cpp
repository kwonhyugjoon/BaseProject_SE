#include "stdafx.h"
#include "Renderer.h"
#include "CharacterSprites.h"
#include <windows.h>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstddef>

namespace
{
	std::string ReadShader(const char* filename)
	{
		char executable[MAX_PATH]={};
		GetModuleFileNameA(nullptr,executable,MAX_PATH);
		std::string folder(executable);
		folder=folder.substr(0,folder.find_last_of("\\/")+1);
		const std::string paths[]={std::string("Shaders/")+filename,
			std::string("SimpleGame/Shaders/")+filename,folder+"Shaders/"+filename};
		for(const auto& path:paths)
		{
			std::ifstream file(path);
			if(file)
			{
				std::ostringstream contents;contents<<file.rdbuf();std::string source=contents.str();
				if(source.compare(0,3,"\xEF\xBB\xBF")==0)source.erase(0,3);
				return source;
			}
		}
		std::cerr<<"Cannot load shader: "<<filename<<'\n';return {};
	}
	GLuint Shader(GLenum type,const std::string& source)
	{
		GLuint shader=glCreateShader(type);const char* text=source.c_str();
		glShaderSource(shader,1,&text,nullptr);glCompileShader(shader);
		GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
		if(!ok)
		{
			char log[2048]={};glGetShaderInfoLog(shader,sizeof(log),nullptr,log);
			std::cerr<<log<<'\n';glDeleteShader(shader);return 0;
		}
		return shader;
	}
	GLuint Program(const char* vertex,const char* fragment)
	{
		GLuint vs=Shader(GL_VERTEX_SHADER,ReadShader(vertex)),fs=Shader(GL_FRAGMENT_SHADER,ReadShader(fragment));
		if(!vs||!fs) {if(vs)glDeleteShader(vs);if(fs)glDeleteShader(fs);return 0;}
		GLuint program=glCreateProgram();glAttachShader(program,vs);glAttachShader(program,fs);
		glLinkProgram(program);glDeleteShader(vs);glDeleteShader(fs);
		GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);
		if(!ok)
		{
			char log[2048]={};glGetProgramInfoLog(program,sizeof(log),nullptr,log);
			std::cerr<<log<<'\n';glDeleteProgram(program);return 0;
		}
		return program;
	}
}

Renderer::Renderer(int width,int height):m_Width(width),m_Height(height)
{
	m_Program=Program("SolidRect.vs","SolidRect.fs");
	m_Post=Program("PostProcess.vs","PostProcess.fs");
	if(!m_Program||!m_Post)return;
	m_Viewport=glGetUniformLocation(m_Program,"u_Viewport");
	m_Time=glGetUniformLocation(m_Program,"u_Time");m_Origin=glGetUniformLocation(m_Program,"u_Origin");
	glUseProgram(m_Program);glUniform1i(glGetUniformLocation(m_Program,"u_Atlas"),1);
	m_PostScene=glGetUniformLocation(m_Post,"u_Scene");m_PostTexel=glGetUniformLocation(m_Post,"u_Texel");
	m_PostEnabled=glGetUniformLocation(m_Post,"u_Enabled");
	glGenVertexArrays(1,&m_Array);glGenBuffers(1,&m_Buffer);
	glBindVertexArray(m_Array);glBindBuffer(GL_ARRAY_BUFFER,m_Buffer);
	glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
	glEnableVertexAttribArray(1);glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,r));
	glEnableVertexAttribArray(2);glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,material));
	glEnableVertexAttribArray(3);glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,u));
	glBindVertexArray(0);
	glGenTextures(1,&m_Atlas);glBindTexture(GL_TEXTURE_2D,m_Atlas);
	auto pixels=CreateCharacterAtlas();
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,SpriteWidth*SpriteFrames,SpriteHeight*SpriteDirections,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glGenFramebuffers(1,&m_Framebuffer);glGenTextures(1,&m_Scene);ResizeTargets();
	m_Font=CreateFontW(-18,0,0,0,FW_MEDIUM,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Malgun Gothic");
	m_Initialized=m_Font!=nullptr;
}

Renderer::~Renderer()
{
	for(const auto& glyph:m_Glyphs)glDeleteLists(glyph.second,1);
	if(m_Font)DeleteObject((HFONT)m_Font);
	glDeleteBuffers(1,&m_Buffer);glDeleteVertexArrays(1,&m_Array);
	glDeleteProgram(m_Program);glDeleteProgram(m_Post);
	glDeleteTextures(1,&m_Scene);glDeleteTextures(1,&m_Atlas);glDeleteFramebuffers(1,&m_Framebuffer);
}

void Renderer::ResizeTargets()
{
	glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,m_Scene);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,m_Width,m_Height,0,GL_RGBA,GL_FLOAT,nullptr);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glBindFramebuffer(GL_FRAMEBUFFER,m_Framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,m_Scene,0);
	m_TargetsReady=glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE;
	if(!m_TargetsReady)std::cerr<<"Postprocess framebuffer unavailable; rendering directly.\n";
	glBindFramebuffer(GL_FRAMEBUFFER,0);
}

void Renderer::Begin(int width,int height,float time,Point worldOrigin)
{
	if(width!=m_Width||height!=m_Height){m_Width=width;m_Height=height;ResizeTargets();}
	m_Elapsed=time;m_WorldOrigin=worldOrigin;m_Vertices.clear();
	glBindFramebuffer(GL_FRAMEBUFFER,m_TargetsReady?m_Framebuffer:0);
	glViewport(0,0,width,height);glDisable(GL_DEPTH_TEST);glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(.055f,.105f,.12f,1);glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::Polygon(const std::vector<Point>& points,Color c,Material material,const std::vector<Point>& uv)
{
	// Negative material IDs select surface coordinates instead of ground coordinates.
	bool local=uv.size()==points.size();
	float id=local?-(float)material-1:(float)material;
	for(size_t i=1;i+1<points.size();++i)
		for(size_t index:{size_t(0),i,i+1})
		{
			Point p=points[index],t=local?uv[index]:Point{0,0};
			m_Vertices.push_back({p.x,p.y,c.r,c.g,c.b,c.a,id,t.x,t.y});
		}
}
void Renderer::Rect(float x,float y,float w,float h,Color c)
{
	Polygon({{x,y},{x+w,y},{x+w,y+h},{x,y+h}},c);
}
void Renderer::Ellipse(float x,float y,float rx,float ry,Color c)
{
	std::vector<Point> points;
	for(int i=0;i<32;++i){float a=i*6.2831853f/32;points.push_back({x+std::cos(a)*rx,y+std::sin(a)*ry});}
	Polygon(points,c);
}
void Renderer::SoftEllipse(float x,float y,float rx,float ry,Color c)
{
	for(int i=0;i<40;++i)
	{
		float a=i*6.2831853f/40,b=(i+1)*6.2831853f/40;
		m_Vertices.push_back({x,y,c.r,c.g,c.b,c.a,0,0,0});
		m_Vertices.push_back({x+std::cos(a)*rx,y+std::sin(a)*ry,c.r,c.g,c.b,0,0,0,0});
		m_Vertices.push_back({x+std::cos(b)*rx,y+std::sin(b)*ry,c.r,c.g,c.b,0,0,0,0});
	}
}
void Renderer::Character(float x,float y,int facing,int frame,Color coat)
{
	float u0=(float)frame/SpriteFrames,u1=(float)(frame+1)/SpriteFrames;
	float v0=(float)facing/SpriteDirections,v1=(float)(facing+1)/SpriteDirections;
	Vertex a={x-24,y-60,coat.r,coat.g,coat.b,coat.a,7,u0,v0};
	Vertex b={x+24,y-60,coat.r,coat.g,coat.b,coat.a,7,u1,v0};
	Vertex c={x+24,y+4,coat.r,coat.g,coat.b,coat.a,7,u1,v1};
	Vertex d={x-24,y+4,coat.r,coat.g,coat.b,coat.a,7,u0,v1};
	for(const auto& vertex:{a,b,c,a,c,d})m_Vertices.push_back(vertex);
}
void Renderer::Flush()
{
	if(m_Vertices.empty())return;
	glUseProgram(m_Program);glUniform2f(m_Viewport,(float)m_Width,(float)m_Height);
	glUniform1f(m_Time,m_Elapsed);glUniform2f(m_Origin,m_WorldOrigin.x,m_WorldOrigin.y);
	glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,m_Atlas);glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(m_Array);glBindBuffer(GL_ARRAY_BUFFER,m_Buffer);
	glBufferData(GL_ARRAY_BUFFER,m_Vertices.size()*sizeof(Vertex),m_Vertices.data(),GL_STREAM_DRAW);
	glDrawArrays(GL_TRIANGLES,0,(GLsizei)m_Vertices.size());glBindVertexArray(0);m_Vertices.clear();
}
void Renderer::FinishWorld(bool postprocess)
{
	Flush();
	if(!m_TargetsReady)return;
	glBindFramebuffer(GL_FRAMEBUFFER,0);glDisable(GL_BLEND);
	glUseProgram(m_Post);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,m_Scene);
	glUniform1i(m_PostScene,0);glUniform2f(m_PostTexel,1.f/m_Width,1.f/m_Height);
	glUniform1i(m_PostEnabled,postprocess?1:0);
	glBindVertexArray(m_Array);glDrawArrays(GL_TRIANGLES,0,3);glBindVertexArray(0);
	glEnable(GL_BLEND);
}

void Renderer::Text(float x, float y, const std::wstring& text, Color c)
{
	// Windows bitmap glyphs support Korean without an external font asset.
	// Bitmap display lists require the compatibility context requested at startup.
	Flush();
	glUseProgram(0);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glOrtho(0, m_Width, m_Height, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	glColor4f(c.r, c.g, c.b, c.a);
	glRasterPos2f(x, y + 18);
	HDC dc = wglGetCurrentDC();
	HGDIOBJ previous = SelectObject(dc, (HFONT)m_Font);
	for (wchar_t ch : text)
	{
		auto it = m_Glyphs.find(ch);
		if (it == m_Glyphs.end())
		{
			GLuint list = glGenLists(1);
			if (!list) continue;
			if (!wglUseFontBitmapsW(dc, ch, 1, list)) { glDeleteLists(list, 1); continue; }
			it = m_Glyphs.emplace(ch, list).first;
		}
		glCallList(it->second);
	}
	SelectObject(dc, previous);
}
