#pragma once
#include <fstream>
#include <stdint.h>

int checksum(std::string fileName) {

	std::ifstream file;

	file.open(fileName);

	int sum = 0;

	int word = 0;
	while (file.read(reinterpret_cast<char*>(&word), sizeof(word))) {
		sum += word;
		word = 0;
	}
	return sum;

}