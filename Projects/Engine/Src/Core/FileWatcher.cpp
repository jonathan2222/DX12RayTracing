#include "PreCompiled.h"
#include "FileWatcher.h"

#include <filesystem>
#include <thread>

#include "Core/CorePlatform.h"
#include <chrono>
#include <string>

RS::FileWatcher::FileWatcher()
    : m_pThread(nullptr)
    , m_Running(false)
    , m_Delay(1000)
{
}

RS::FileWatcher::~FileWatcher()
{
    Release();
}

void RS::FileWatcher::Init(const std::string& threadName)
{
    Release();
    m_Running = true;
    m_pThread = new std::thread(&RS::FileWatcher::ThreadFunction, this, threadName);
}

void RS::FileWatcher::Release()
{
    m_Running = false;
    if (m_pThread == nullptr)
        return;

    m_pThread->join();
    delete m_pThread;
    m_pThread = nullptr;
}

void RS::FileWatcher::SetDelay(uint milliseconds)
{
    m_Delay = milliseconds;
}

void RS::FileWatcher::AddFileListener(const std::filesystem::path& pathOnDisk, Listener callback)
{
    AddFileListener(pathOnDisk, 0u, callback);
}

void RS::FileWatcher::AddFileListener(const std::filesystem::path& pathOnDisk, uint64 userKey, Listener callback)
{
    ListenerKey key{ .m_Path = pathOnDisk, .m_UserKey = userKey };

    std::lock_guard<std::mutex> lock(m_FileMutex);

    auto it = m_Listeners.find(key);
    if (it != m_Listeners.end())
    {
        it->second.push_back(callback);
        return;
    }

    m_Listeners[key] = { callback };

    if (std::filesystem::is_directory(pathOnDisk))
    {
        // Dir
        if (!m_RegistratedDirectories.contains(pathOnDisk))
            m_RegistratedDirectories.insert(pathOnDisk);
    }
    else
    {
        // File
        if (!m_RegistratedFiles.contains(pathOnDisk))
            m_RegistratedFiles.insert(pathOnDisk);

        if (std::filesystem::exists(pathOnDisk))
            m_Files[pathOnDisk] = std::filesystem::last_write_time(pathOnDisk);
    }
}

bool RS::FileWatcher::HasExactListener(const Path& pathOnDisk)
{
    return HasExactListener(pathOnDisk, 0u);
}

bool RS::FileWatcher::HasExactListener(const Path& pathOnDisk, uint64 userKey)
{
    ListenerKey key{ .m_Path = pathOnDisk, .m_UserKey = userKey };

    std::lock_guard<std::mutex> lock(m_FileMutex);
    return m_Listeners.find(key) != m_Listeners.end();
}

void RS::FileWatcher::Clear()
{
    std::lock_guard<std::mutex> lock(m_FileMutex);
    m_Listeners.clear();
    m_RegistratedDirectories.clear();
    m_RegistratedFiles.clear();
}

void RS::FileWatcher::CallListeners(const Path& filePath, FileStatus status)
{
    // We find all listeners that might listen to this file path and call them.
    try
    {
        // The listeners' paths can be a file or directory. Thus we get the canonical path of both and compare.
        Path canonicalFilePath = std::filesystem::canonical(filePath);
        for (auto& entry : m_Listeners)
        {
            Path canonicalListenerPath = std::filesystem::canonical(entry.first.m_Path);

            if (canonicalListenerPath.string() == canonicalFilePath.string())
            {
                for (Listener& listener : entry.second)
                    listener(filePath, entry.first.m_UserKey, status);
            }
            else
            // Because the canonical path is basically a kind of absolute path, we know that if one is part of the other,
            //  the one that is part of it should be smaller in size.
            if (canonicalListenerPath.string().size() > canonicalFilePath.string().size())
            {
                // Only succeed if the smaller one can be found from the start of the path.
                if (canonicalListenerPath.string().find(canonicalFilePath.string()) == 0)
                {
                    for (Listener& listener : entry.second)
                        listener(filePath, entry.first.m_UserKey, status);
                }
            }
            else if (canonicalListenerPath.string().size() < canonicalFilePath.string().size())
            {
                // Only succeed if the smaller one can be found from the start of the path.
                if (canonicalFilePath.string().find(canonicalListenerPath.string()) == 0)
                {
                    for (Listener& listener : entry.second)
                        listener(filePath, entry.first.m_UserKey, status);
                }
            }
        }
    }
    catch (std::exception e)
    {

    }
}

void RS::FileWatcher::ThreadFunction(const std::string& threadName)
{
    CorePlatform::SetCurrentThreadName(threadName);

    while (m_Running)
    {
        CorePlatform::ThreadSleep(m_Delay);

        std::lock_guard<std::mutex> lock(m_FileMutex);

        // Check if any file was removed
        auto it = m_Files.begin();
        while (it != m_Files.end())
        {
            if (!std::filesystem::exists(it->first))
            {
                // If so, remove it and call its listeners.
                CallListeners(it->first, FileStatus::REMOVED);
                it = m_Files.erase(it);
            }
            else
                it++;
        }

        for (const Path& dir : m_RegistratedDirectories)
        {
            for (auto& file : std::filesystem::recursive_directory_iterator(dir))
            {
                auto currentFileLastWriteTime = std::filesystem::last_write_time(file);

                if (!m_Files.contains(file.path()))
                {
                    m_Files[file.path()] = currentFileLastWriteTime;
                    CallListeners(file.path(), FileStatus::CREATED);
                }
                else
                {
                    if (m_Files[file.path()] != currentFileLastWriteTime)
                    {
                        m_Files[file.path()] = currentFileLastWriteTime;
                        CallListeners(file.path(), FileStatus::MODIFIED);
                    }
                }
            }
        }

        for (const Path& file : m_RegistratedFiles)
        {
            if (!std::filesystem::exists(file)) continue;

            auto currentFileLastWriteTime = std::filesystem::last_write_time(file);

            if (!m_Files.contains(file))
            {
                m_Files[file] = currentFileLastWriteTime;
                CallListeners(file, FileStatus::CREATED);
            }
            else
            {
                if (m_Files[file] != currentFileLastWriteTime)
                {
                    m_Files[file] = currentFileLastWriteTime;
                    CallListeners(file, FileStatus::MODIFIED);
                }
            }
        }
    }
}
