#pragma once

#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include "checksum.h"

using boost::asio::ip::tcp;

// Define available file changes
enum class FileStatus { created, modified, erased };

class FileWatcher {
public:
	std::string path_to_watch;
	// Time interval at which we check the base folder for changes
	std::chrono::duration<int, std::milli> delay;
	tcp::socket* s;

	// Keep a record of files from the base directory and their last modification time
	FileWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay, tcp::socket& s) :
		path_to_watch{ path_to_watch }, delay{ delay }, s{ &s }
	{
		for (auto& file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
			paths_[file.path().string()].first = boost::filesystem::last_write_time(file.path().string());
			paths_[file.path().string()].second = checksum(file.path().string());

		}
	}
	// Costruttore da usare per forzare il file watcher ad avere una certa struttura dati iniziale, senza costruirla guardando la cartella co
	FileWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay, tcp::socket& s, std::string serialized_data_struct) :
		path_to_watch{ path_to_watch }, delay{ delay }, s{ &s }
	{
		if (serialized_data_struct.size() == 1)
		{
			if (serialized_data_struct == std::string("-"))
			{
				for (auto& file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
					paths_[file.path().string()].first = boost::filesystem::last_write_time(file.path().string());
					paths_[file.path().string()].second = checksum(file.path().string());
				}
			}
		}
		else
			deserialize_map(paths_, serialized_data_struct);
	}

	// Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
	void start(const std::function<void(std::string, std::string, FileStatus, tcp::socket&)>& action) {
		int checkSumCounter = 0;
		while (running_) {
			// Wait for "delay" milliseconds
			std::this_thread::sleep_for(delay);

			auto it = paths_.begin();
			while (it != paths_.end()) {
				if (!std::filesystem::exists(it->first)) {
					action(it->first, path_to_watch, FileStatus::erased, *s);
					it = paths_.erase(it);
				}
				else {
					it++;
				}
			}

			// Check if a file was created or modified
			for (auto& file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
				int fileCheckSum = 0;
				auto current_file_last_write_time = boost::filesystem::last_write_time(file.path().string());
				

				// File creation
				if (!contains(file.path().string())) {
					paths_[file.path().string()].first = current_file_last_write_time;
					paths_[file.path().string()].second =  0;
					action(file.path().string(), path_to_watch, FileStatus::created, *s);
					// File modification
				}
				else {
					if (checkSumCounter == 0) {
						fileCheckSum = checksum(file.path().string());
					}

					if (checkSumCounter == 0) {
						if (paths_[file.path().string()].first != current_file_last_write_time || paths_[file.path().string()].second != fileCheckSum) {
							paths_[file.path().string()].first = current_file_last_write_time;
							paths_[file.path().string()].second = fileCheckSum;
							action(file.path().string(), path_to_watch, FileStatus::modified, *s);
						}
					}
					else if(paths_[file.path().string()].first != current_file_last_write_time && checkSumCounter == 1) {
						paths_[file.path().string()].first = current_file_last_write_time;
						action(file.path().string(), path_to_watch, FileStatus::modified, *s);
					}
				}
			}
			checkSumCounter = 1;
		}
	}
	std::string get_folder_data()
	{
		std::string str;
		serialize_map(paths_, str);
		return str;
	}


private:
	std::unordered_map<std::string, std::pair<std::time_t, int>> paths_;
	bool running_ = true;

	// Check if "paths_" contains a given key
	// If your compiler supports C++20 use paths_.contains(key) instead of this function
	bool contains(const std::string& key) {
		auto el = paths_.find(key);
		return el != paths_.end();
	}

	int serialize_map(std::unordered_map<std::string, std::pair<std::time_t, int>>& mymap, std::string& str)
	{
		for (std::unordered_map<std::string, std::pair<std::time_t, int>>::iterator it = mymap.begin(); it != mymap.end(); ++it)
		{
			str.append("./" + it->first.substr(path_to_watch.size()) + ":" + std::to_string(it->second.first) + "," + std::to_string(it->second.second) + "|");
		}
		return str.length();
	}

	int deserialize_map(std::unordered_map<std::string, std::pair<std::time_t, int>>& mymap, std::string& str)
	{
		std::string pair, token;
		std::stringstream ss(str);
		std::size_t pos;
		std::size_t comma_pos;
		int i = 0;
		if (str.size() == 1)
			return 0;
		while (std::getline(ss, pair, '|'))
		{
			pos = pair.find(":");
			comma_pos = pair.find(",");
			mymap.insert({ std::string(path_to_watch).append(std::string(pair, 0, pos).substr(2)), std::pair<std::time_t, int>(std::stoi(std::string(pair, pos + 1,comma_pos)),std::stoi(std::string(pair,comma_pos + 1))) });
			i++;
		}
		return i;
	}
};