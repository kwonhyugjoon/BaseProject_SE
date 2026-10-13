#include "stdafx.h"
#include <winsock2.h>
#include <Windows.h>
#include <shellapi.h>
#include "MonitorServer.h"
#include <chrono>
#include <fstream>
#include <iterator>

namespace
{
	constexpr unsigned short Port = 8765;
	const wchar_t* Url = L"http://127.0.0.1:8765/";
	const char* Routes[] = {"/", "/monitor.css", "/monitor.js"};
	const char* ContentTypes[] = {"text/html", "text/css", "application/javascript"};
	const wchar_t* AssetNames[] = {L"index.html", L"monitor.css", L"monitor.js"};

	bool LoadAssets(std::array<std::string, 3>& assets)
	{
		wchar_t executable[MAX_PATH];
		DWORD length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
		if (!length || length >= MAX_PATH)
			return false;
		std::wstring path(executable, length);
		std::wstring directory = path.substr(0, path.find_last_of(L"\\/")) + L"\\Monitor\\";
		for (const auto& root :
		     {directory, std::wstring(L"Monitor\\"), std::wstring(L"SimpleGame\\Monitor\\")})
		{
			bool complete = true;
			for (size_t i = 0; i < assets.size(); ++i)
			{
				std::ifstream file((root + AssetNames[i]).c_str(), std::ios::binary);
				if (!file)
				{
					complete = false;
					break;
				}
				assets[i].assign(std::istreambuf_iterator<char>(file), {});
			}
			if (complete)
				return true;
		}
		return false;
	}

	int Ready(SOCKET socket, bool writing)
	{
		fd_set sockets;
		FD_ZERO(&sockets);
		FD_SET(socket, &sockets);
		timeval timeout = {0, 100000};
		return select(
		    0, writing ? nullptr : &sockets, writing ? &sockets : nullptr, nullptr, &timeout);
	}
}

MonitorServer::~MonitorServer()
{
	Stop();
}

bool MonitorServer::Start()
{
	if (m_Running)
		return true;
	Stop();
	if (!LoadAssets(m_Assets))
	{
		printf("Monitor: missing Monitor assets. Rebuild to copy HTML/CSS/JS.\n");
		return false;
	}

	WSADATA data;
	int error = WSAStartup(MAKEWORD(2, 2), &data);
	if (error)
	{
		printf("Monitor: WSAStartup failed (%d).\n", error);
		return false;
	}
	m_Listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_Listener == INVALID_SOCKET)
	{
		printf("Monitor: socket failed (%d).\n", WSAGetLastError());
		WSACleanup();
		return false;
	}
	BOOL exclusive = TRUE;
	setsockopt(m_Listener,
	           SOL_SOCKET,
	           SO_EXCLUSIVEADDRUSE,
	           reinterpret_cast<const char*>(&exclusive),
	           sizeof(exclusive));
	sockaddr_in address = {};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = htons(Port);
	if (bind(m_Listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR ||
	    listen(m_Listener, 8) == SOCKET_ERROR)
	{
		printf("Monitor: cannot listen on 127.0.0.1:%u (%d). Game continues without monitor.\n",
		       Port,
		       WSAGetLastError());
		Stop();
		return false;
	}
	u_long nonblocking = 1;
	if (ioctlsocket(m_Listener, FIONBIO, &nonblocking) == SOCKET_ERROR)
	{
		Stop();
		return false;
	}
	m_Running = true;
	m_Thread = std::thread(&MonitorServer::Serve, this);
	printf("Monitor: http://127.0.0.1:%u/ (F9 opens browser)\n", Port);
	return true;
}

void MonitorServer::Stop()
{
	m_Running = false;
	if (m_Thread.joinable())
		m_Thread.join();
	if (m_Listener != INVALID_SOCKET)
	{
		closesocket(m_Listener);
		m_Listener = INVALID_SOCKET;
		WSACleanup();
	}
}

void MonitorServer::Publish(std::string json)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Json = std::move(json);
}

void MonitorServer::OpenBrowser() const
{
	if (m_Running)
		ShellExecuteW(nullptr, L"open", Url, nullptr, nullptr, SW_SHOWNORMAL);
}

void MonitorServer::Serve()
{
	while (m_Running)
	{
		int ready = Ready(m_Listener, false);
		if (ready == SOCKET_ERROR)
		{
			printf("Monitor: listener failed (%d).\n", WSAGetLastError());
			m_Running = false;
			return;
		}
		if (!ready)
			continue;
		SOCKET client = accept(m_Listener, nullptr, nullptr);
		if (client == INVALID_SOCKET)
			continue;
		u_long nonblocking = 1;
		if (ioctlsocket(client, FIONBIO, &nonblocking) != SOCKET_ERROR)
			Respond(client);
		closesocket(client);
	}
}

void MonitorServer::Respond(std::uintptr_t client)
{
	using Clock = std::chrono::steady_clock;
	auto deadline = Clock::now() + std::chrono::seconds(1);
	std::string request;
	char buffer[2048];
	while (m_Running && Clock::now() < deadline && request.size() < 8192 &&
	       request.find("\r\n\r\n") == std::string::npos)
	{
		int ready = Ready(client, false);
		if (ready == SOCKET_ERROR)
			return;
		if (!ready)
			continue;
		int received = recv(client, buffer, sizeof(buffer), 0);
		if (received == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
			continue;
		if (received <= 0)
			return;
		request.append(buffer, received);
	}
	if (request.size() > 8192 || request.find("\r\n\r\n") == std::string::npos)
		return;

	std::string body = "Not found", status = "404 Not Found", type = "text/plain";
	if (request.compare(0, 4, "GET ") != 0)
	{
		status = "405 Method Not Allowed";
		body = "GET only";
	}
	else
	{
		size_t end = request.find(' ', 4);
		std::string route = request.substr(4, end - 4);
		route = route.substr(0, route.find('?'));
		if (route == "/api/state")
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			body = m_Json.empty() ? "{\"error\":\"State not ready\"}" : m_Json;
			status = m_Json.empty() ? "503 Service Unavailable" : "200 OK";
			type = "application/json";
		}
		else
		{
			for (size_t i = 0; i < m_Assets.size(); ++i)
				if (route == Routes[i] || (i == 0 && route == "/index.html"))
				{
					body = m_Assets[i];
					type = ContentTypes[i];
					status = "200 OK";
					break;
				}
		}
	}

	std::string response = "HTTP/1.1 " + status + "\r\nContent-Type: " + type +
	                       "; charset=utf-8\r\nContent-Length: " + std::to_string(body.size()) +
	                       "\r\nCache-Control: no-store\r\nConnection: close\r\n"
	                       "X-Content-Type-Options: nosniff\r\n\r\n" +
	                       body;
	deadline = Clock::now() + std::chrono::seconds(1);
	size_t sent = 0;
	while (m_Running && sent < response.size() && Clock::now() < deadline)
	{
		int ready = Ready(client, true);
		if (ready == SOCKET_ERROR)
			return;
		if (!ready)
			continue;
		int count =
		    send(client, response.data() + sent, static_cast<int>(response.size() - sent), 0);
		if (count == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
			continue;
		if (count <= 0)
			return;
		sent += count;
	}
}
