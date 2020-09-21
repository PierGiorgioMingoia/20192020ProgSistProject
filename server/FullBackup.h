#pragma once
#include <string>
#include <iostream>
#include <filesystem>
#include <boost/filesystem.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;


void sendEntireUserFolder(std::string user, tcp::socket& s);
void sendFile(std::filesystem::path file, tcp::socket& s);
void sendString(std::string msg, tcp::socket& s);
void sendMessageOverTheSocket(const char* msg, int size, tcp::socket& s);
void initialMessageForEachFile(std::string file, tcp::socket& s);
void lastMessageForEachFile(std::string file, tcp::socket& s);
void send_directory(std::filesystem::path file, tcp::socket& s);


void sendEntireUserFolder(std::string user, tcp::socket& s) {
	try {
		for (auto& file : std::filesystem::recursive_directory_iterator("./" + user)) {
			if (file.is_directory())
			{
				if (std::filesystem::is_empty(file.path()))
					send_directory(file.path().string(),s);
			}
			else
				sendFile(file.path(), s);
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception in send file: " << e.what() << "\n";
		throw;
	}
}

void send_directory(std::filesystem::path file, tcp::socket& s)
{
	sendString(std::string("D: ").append(file.string()).append("\n"), s);
	return;
}

void sendFile(std::filesystem::path file, tcp::socket& s) {
	std::string fileStringName = file.string();
	try {
		std::ifstream iFile(fileStringName, std::ifstream::binary);
		char buff[1008];

		initialMessageForEachFile(fileStringName, s);
		if (iFile.is_open()) {
			while (iFile)
			{
				iFile.read(buff + 8, sizeof(buff) - 8);
				int n = iFile.gcount();
				buff[0] = 'L';
				buff[1] = ':';
				buff[2] = ' ';
				buff[3] = n / 1000 + 48;
				buff[4] = n % 1000 / 100 + 48;
				buff[5] = n % 100 / 10 + 48;
				buff[6] = n % 10 + 48;
				buff[7] = '|';
				sendMessageOverTheSocket(buff, iFile.gcount() + 8, s);
			}
		}
		iFile.close();
		lastMessageForEachFile(fileStringName, s);
	}
	catch (std::exception& e) {
		std::cerr << "Exception in send file: " << e.what() << "\n";
		throw;
	}
}



void sendString(std::string msg, tcp::socket& s) {
	try
	{
		sendMessageOverTheSocket(msg.c_str(), (int)msg.size(), s);
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception in send: " << e.what() << "\n";
		throw;
	}
}

void sendMessageOverTheSocket(const char* msg, int size, tcp::socket& s) {
	try
	{
		boost::asio::write(s, boost::asio::buffer(msg, size));
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception in send: " << e.what() << "\n";
		throw;
	}
}

void initialMessageForEachFile(std::string file, tcp::socket& s) {

	sendString(std::string("C: ").append(file).append("\n"), s);

}

void lastMessageForEachFile(std::string file, tcp::socket& s) {
	sendString(std::string("U: ").append(std::to_string(boost::filesystem::last_write_time(file))).append("\n"), s);
}