#pragma once
#include <iostream>
#include <fstream>
#include <map>
#include <boost/filesystem.hpp>
#include <filesystem>


class BackUpFile {
	std::string path;
	std::filesystem::file_time_type lastModificationTime;
public:
	BackUpFile(std::string path, std::filesystem::file_time_type lastModificationTime) :path(path), lastModificationTime(lastModificationTime) {}

	std::string getPath() {
		return path;
	}
	std::filesystem::file_time_type getLastModificationTime() {
		return lastModificationTime;
	}
};

BackUpFile createBackUpFile(std::string user, std::string filePath, std::string fileName, std::filesystem::file_time_type lastMod) {
	static int count = 0;

	if (count == 0) {
		boost::filesystem::create_directory("Backup");
		count++;
	}

	std::string dstPath = "./Backup\\" + user+ fileName; 
	boost::filesystem::copy_file(filePath, dstPath);
	return BackUpFile(dstPath, lastMod );
}

void deleteBackUpFile(std::map<std::string, BackUpFile>& backUpFiles, std::string filePath) {
	std::map<std::string, BackUpFile>::iterator  it = backUpFiles.find(filePath);

	if (it == backUpFiles.end())
		return;
	else {
		boost::filesystem::remove(it->second.getPath());
		backUpFiles.erase(it);
	}

}

void overWriteFileBackup(BackUpFile backUpPath, std::string filePath) {
	boost::filesystem::copy_file(backUpPath.getPath(), filePath, boost::filesystem::copy_option::overwrite_if_exists);
	std::filesystem::last_write_time(filePath, backUpPath.getLastModificationTime());
	std::filesystem::remove(backUpPath.getPath());// bisognerebbe anche toglierla dalla mappa ma tanto questa funzione viene chiamata prima della return quindi la mappa verrà cancellata a breve.
}