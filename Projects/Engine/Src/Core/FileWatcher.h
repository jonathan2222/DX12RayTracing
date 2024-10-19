#pragma once

#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <unordered_set>

namespace RS
{
	class FileWatcher
	{
	public:
		using Path = std::filesystem::path;

		enum FileStatus
		{
			CREATED,
			MODIFIED,
			REMOVED
		};

		using Listener = std::function<void(Path, FileStatus)>;

		FileWatcher();
		~FileWatcher();

		void Init(const std::string& threadName);
		void Release();

		void SetDelay(uint milliseconds);

		// pathOnDisk can be a directory or a path to a file.
		void AddFileListener(const Path& pathOnDisk, Listener callback);
		bool HasExactListener(const Path& pathOnDisk);

		void Clear();

	private:
		void CallListeners(const Path& path, FileStatus status);

		void ThreadFunction(const std::string& threadName);

	private:
		std::thread* m_pThread;
		std::mutex m_FileMutex;
		std::unordered_map<Path, std::filesystem::file_time_type> m_Files;
		std::unordered_set<Path> m_RegistratedFiles;
		std::unordered_set<Path> m_RegistratedDirectories;
		std::unordered_map<Path, std::vector<Listener>> m_Listeners;

		std::chrono::duration<uint, std::milli> m_Delay;
		bool m_Running;
	};
}