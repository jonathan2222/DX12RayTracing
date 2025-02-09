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

		using Listener = std::function<void(Path, uint64, FileStatus)>;

		FileWatcher();
		~FileWatcher();

		void Init(const std::string& threadName);
		void Release();

		void SetDelay(uint milliseconds);

		// pathOnDisk can be a directory or a path to a file.
		void AddFileListener(const Path& pathOnDisk, uint64 userKey, Listener callback);
		void AddFileListener(const Path& pathOnDisk, Listener callback);
		bool HasExactListener(const Path& pathOnDisk, uint64 userKey);
		bool HasExactListener(const Path& pathOnDisk);

		void Clear();

	private:
		void CallListeners(const Path& path, FileStatus status);

		void ThreadFunction(const std::string& threadName);

		struct ListenerKey
		{
			std::filesystem::path m_Path;
			uint64 m_UserKey;
			bool operator==(const ListenerKey&) const = default;
		};

		struct ListenerKeyHash
		{
			size_t operator()(const ListenerKey& key) const noexcept
			{
				return std::hash<std::filesystem::path>{}(key.m_Path) ^ std::hash<uint64>{}(key.m_UserKey);
			}
		};
	private:
		std::thread* m_pThread;
		std::mutex m_FileMutex;
		std::unordered_map<Path, std::filesystem::file_time_type> m_Files;
		std::unordered_set<Path> m_RegistratedFiles;
		std::unordered_set<Path> m_RegistratedDirectories;
		std::unordered_map<ListenerKey, std::vector<Listener>, ListenerKeyHash> m_Listeners;

		uint64 m_Delay;
		bool m_Running;
	};

}