#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <array>

class MonitorServer
{
  public:
	~MonitorServer();
	bool Start();
	void Stop();
	void Publish(std::string json);
	void OpenBrowser() const;

	bool IsRunning() const
	{
		return m_Running;
	}

  private:
	void Serve();
	void Respond(std::uintptr_t client);
	std::uintptr_t m_Listener = static_cast<std::uintptr_t>(-1);
	std::atomic<bool> m_Running{false};
	std::thread m_Thread;
	std::mutex m_Mutex;
	std::string m_Json;
	std::array<std::string, 3> m_Assets;
};
