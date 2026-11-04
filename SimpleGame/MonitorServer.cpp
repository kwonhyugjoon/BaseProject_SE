#include "stdafx.h"
#include "PlatformPaths.h"
#ifdef _WIN32
#include <winsock2.h>
#include <Windows.h>
#include <shellapi.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <spawn.h>
#include <unistd.h>
extern char** environ;
#endif
#include "MonitorServer.h"
#include <chrono>
#include <fstream>
#include <iterator>

namespace
{

#ifdef _WIN32
	using Socket = SOCKET;
	constexpr Socket InvalidSocket = INVALID_SOCKET;
	constexpr int SocketError = SOCKET_ERROR;

	void CloseSocket(Socket socket)
	{
		closesocket(socket);
	}

	int LastSocketError()
	{
		return WSAGetLastError();
	}

	bool WouldBlock()
	{
		return WSAGetLastError() == WSAEWOULDBLOCK;
	}

	bool Nonblocking(Socket socket)
	{
		u_long enabled = 1;
		return ioctlsocket(socket, FIONBIO, &enabled) != SocketError;
	}
#else
	using Socket = int;
	constexpr Socket InvalidSocket = -1;
	constexpr int SocketError = -1;

	void CloseSocket(Socket socket)
	{
		close(socket);
	}

	int LastSocketError()
	{
		return errno;
	}

	bool WouldBlock()
	{
		return errno == EAGAIN || errno == EWOULDBLOCK;
	}

	bool Nonblocking(Socket socket)
	{
		int flags = fcntl(socket, F_GETFL, 0);
		return flags >= 0 && fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
	}
#endif

	constexpr unsigned short Port = 8765;
	const wchar_t* Url = L"http://127.0.0.1:8765/";
	const char* Routes[] = {"/", "/monitor.css", "/monitor.js"};
	const char* ContentTypes[] = {"text/html", "text/css", "application/javascript"};
	const char* AssetNames[] = {"index.html", "monitor.css", "monitor.js"};

	bool LoadAssets(std::array<std::string, 3>& assets)
	{
		for (const auto& root : {ExecutableDirectory() / "Monitor",
		                         std::filesystem::path("Monitor"),
		                         std::filesystem::path("SimpleGame/Monitor")})
		{
			bool complete = true;
			for (size_t i = 0; i < assets.size(); ++i)
			{
				std::ifstream file(root / AssetNames[i], std::ios::binary);
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

	int Ready(std::uintptr_t handle, bool writing)
	{
		Socket socket = static_cast<Socket>(handle);
		fd_set sockets;
		FD_ZERO(&sockets);
		FD_SET(socket, &sockets);
		timeval timeout = {0, 100000};
#ifdef _WIN32
		int count = 0;
#else
		int count = socket + 1;
#endif
		return select(
		    count, writing ? nullptr : &sockets, writing ? &sockets : nullptr, nullptr, &timeout);
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

#ifdef _WIN32
	WSADATA data;
	int error = WSAStartup(MAKEWORD(2, 2), &data);
	if (error)
	{
		printf("Monitor: WSAStartup failed (%d).\n", error);
		return false;
	}
#endif
	m_Listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (static_cast<Socket>(m_Listener) == InvalidSocket)
	{
		printf("Monitor: socket failed (%d).\n", LastSocketError());
#ifdef _WIN32
		WSACleanup();
#endif
		return false;
	}
#ifdef _WIN32
	BOOL exclusive = TRUE;
	setsockopt(m_Listener,
	           SOL_SOCKET,
	           SO_EXCLUSIVEADDRUSE,
	           reinterpret_cast<const char*>(&exclusive),
	           sizeof(exclusive));
#endif
	sockaddr_in address = {};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = htons(Port);
	if (bind(static_cast<Socket>(m_Listener),
	         reinterpret_cast<sockaddr*>(&address),
	         sizeof(address)) == SocketError ||
	    listen(static_cast<Socket>(m_Listener), 8) == SocketError)
	{
		printf("Monitor: cannot listen on 127.0.0.1:%u (%d). Game continues without monitor.\n",
		       Port,
		       LastSocketError());
		Stop();
		return false;
	}
	if (!Nonblocking(static_cast<Socket>(m_Listener)))
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
	if (static_cast<Socket>(m_Listener) != InvalidSocket)
	{
		CloseSocket(static_cast<Socket>(m_Listener));
		m_Listener = static_cast<std::uintptr_t>(-1);
#ifdef _WIN32
		WSACleanup();
#endif
	}
}

void MonitorServer::Publish(std::string json)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Json = std::move(json);
}

void MonitorServer::OpenBrowser() const
{
	if (!m_Running)
		return;
#ifdef _WIN32
	ShellExecuteW(nullptr, L"open", Url, nullptr, nullptr, SW_SHOWNORMAL);
#else
#ifdef __APPLE__
	const char* command = "open";
#else
	const char* command = "xdg-open";
#endif
	char* arguments[] = {
	    const_cast<char*>(command), const_cast<char*>("http://127.0.0.1:8765/"), nullptr};
	pid_t process;
	if (posix_spawnp(&process, command, nullptr, nullptr, arguments, environ) == 0)
		std::thread(
		    [process]()
		    {
			    waitpid(process, nullptr, 0);
		    })
		    .detach();
#endif
}

void MonitorServer::Serve()
{
	while (m_Running)
	{
		int ready = Ready(m_Listener, false);
		if (ready == SocketError)
		{
			printf("Monitor: listener failed (%d).\n", LastSocketError());
			m_Running = false;
			return;
		}
		if (!ready)
			continue;
		Socket client = accept(static_cast<Socket>(m_Listener), nullptr, nullptr);
		if (client == InvalidSocket)
			continue;
		if (Nonblocking(client))
		{
#ifdef SO_NOSIGPIPE
			int enabled = 1;
			setsockopt(client, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled));
#endif
			Respond(client);
		}
		CloseSocket(client);
	}
}

void MonitorServer::Respond(std::uintptr_t client)
{
	Socket socket = static_cast<Socket>(client);
	using Clock = std::chrono::steady_clock;
	auto deadline = Clock::now() + std::chrono::seconds(1);
	std::string request;
	char buffer[2048];
	while (m_Running && Clock::now() < deadline && request.size() < 8192 &&
	       request.find("\r\n\r\n") == std::string::npos)
	{
		int ready = Ready(client, false);
		if (ready == SocketError)
			return;
		if (!ready)
			continue;
		int received = static_cast<int>(recv(socket, buffer, sizeof(buffer), 0));
		if (received == SocketError && WouldBlock())
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
		if (ready == SocketError)
			return;
		if (!ready)
			continue;
		int count = static_cast<int>(send(socket,
		                                  response.data() + sent,
		                                  static_cast<int>(response.size() - sent),
#ifdef MSG_NOSIGNAL
		                                  MSG_NOSIGNAL
#else
		                                  0
#endif
		                                  ));
		if (count == SocketError && WouldBlock())
			continue;
		if (count <= 0)
			return;
		sent += count;
	}
}
