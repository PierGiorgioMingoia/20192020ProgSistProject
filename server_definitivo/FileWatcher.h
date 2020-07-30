#pragma once

#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

using boost::asio::ip::tcp;

// Define available file changes
enum class FileStatus { created, modified, erased };

class FileWatcher {
public:
	std::string path_to_watch;
	// Time interval at which we check the base folder for changes
	std::chrono::duration<int, std::milli> delay;
	tcp::socket *s;

	// Keep a record of files from the base directory and their last modification time
	FileWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay,tcp::socket &s) : 
		path_to_watch{ path_to_watch }, delay{ delay }, s{ &s } 
	{
		for (auto& file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
			paths_[file.path().string()] = boost::filesystem::last_write_time(file.path().string());

		}
	}
	// Costruttore da usare per forzare il file watcher ad avere una certa struttura dati iniziale, senza costruirla guardando la cartella co
	FileWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay, tcp::socket& s, std::string serialized_data_struct) :
		path_to_watch{ path_to_watch }, delay{ delay }, s{ &s } 
	{
		deserialize_map(paths_, serialized_data_struct);
	}

	// Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
	void start(const std::function<void(std::string, FileStatus, tcp::socket&)>& action) {
		while (running_) {
			// Wait for "delay" milliseconds
			std::this_thread::sleep_for(delay);

			auto it = paths_.begin();
			while (it != paths_.end()) {
				if (!std::filesystem::exists(it->first)) {
					action(it->first, FileStatus::erased,*s);
					it = paths_.erase(it);
				}
				else {
					it++;
				}
			}

			// Check if a file was created or modified
			for (auto& file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
				auto current_file_last_write_time = boost::filesystem::last_write_time(file.path().string());

				// File creation
				if (!contains(file.path().string())) {
					paths_[file.path().string()] = current_file_last_write_time;
					action(file.path().string(), FileStatus::created,*s);
					// File modification
				}
				else {
					if (paths_[file.path().string()] != current_file_last_write_time) {
						paths_[file.path().string()] = current_file_last_write_time;
						action(file.path().string(), FileStatus::modified,*s);
					}
				}
			}
		}
	}
	std::string get_folder_data()
	{
		std::string str;
		serialize_map(paths_, str);
		return str;
	}


private:
	std::unordered_map<std::string, std::time_t> paths_;
	bool running_ = true;

	// Check if "paths_" contains a given key
	// If your compiler supports C++20 use paths_.contains(key) instead of this function
	bool contains(const std::string& key) {
		auto el = paths_.find(key);
		return el != paths_.end();
	}

	int serialize_map(std::unordered_map<std::string, std::time_t>& mymap, std::string& str)
	{
		for (std::unordered_map<std::string, std::time_t>::iterator it = mymap.begin(); it != mymap.end(); ++it)
		{
			str.append("./" + it->first.substr(path_to_watch.size()) + ":" + std::to_string(it->second) + "|");
		}
		return str.length();
	}

	int deserialize_map(std::unordered_map<std::string, std::time_t>& mymap, std::string& str)
	{
		std::string pair, token;
		std::stringstream ss(str);
		std::size_t pos;
		int i = 0;
		if (str.size() == 1)
			return 0;
		while (std::getline(ss, pair, '|'))
		{
			pos = pair.find(":");
			mymap.insert({ std::string(pair, 0, pos), std::stoi(std::string(pair, pos + 1)) });
			i++;
		}
		return i;
	}
};