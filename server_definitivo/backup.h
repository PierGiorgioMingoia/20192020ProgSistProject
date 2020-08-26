#include <iostream>
#include <fstream>
#include <map>
#include <boost/filesystem.hpp>

std::string createBackUpFile(std::string user, std::string filePath, std::string fileName) {
	static int count = 0;

	if (count == 0)
		boost::filesystem::create_directory("Backup");

	std::string dstPath = "./Backup\\" + user + fileName;
	/*std::ifstream src(filePath, std::ios::binary);
	std::ofstream dst(dstPath, std::ios::binary);*/

	boost::filesystem::copy_file(filePath, dstPath);
	/*
	dst << src.rdbuf();
	src.close();
	dst.close();
	*/
	return dstPath;
}

void deleteBackUpFile(std::map<std::string, std::string> & backUpFiles, std::string filePath) {
	std::map<std::string, std::string>::iterator  it = backUpFiles.find(filePath);

	if (it == backUpFiles.end())
		return;
	else {
		boost::filesystem::remove(it->second);
		std::cout <<it->first << " => " << it->second << '\n';
		backUpFiles.erase(it);

	}

}

void overWriteFileBackup(std::string backUpPath, std::string filePath) {
	boost::filesystem::copy_file(backUpPath, filePath, boost::filesystem::copy_option::overwrite_if_exists);
}