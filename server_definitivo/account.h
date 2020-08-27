#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <boost/functional/hash.hpp>


std::map<std::string, int> readAndStoreAccounts(std::string filePath) {
	std::map<std::string, int> accounts;
	std::ifstream accountsFile(filePath);
	if (accountsFile.is_open()) {
		std::string fileLine;
		while (std::getline(accountsFile, fileLine))
		{
			std::istringstream pair(fileLine);
			std::string accountName;
			int accountPassword;

			if (!(pair >> accountName >> accountPassword)) { break; }

			accounts.insert(std::pair<std::string, int>(accountName, accountPassword));
		}
		accountsFile.close();
	}

	return accounts;

}

int computeHashPassword(std::string password) {
	std::hash <std::string> hashPassword;
	return hashPassword(password);
}

bool checkNameAndPassword(std::string name, std::string password, const std::map<std::string, int>& accounts) {
	auto it = accounts.find(name);
	if (it != accounts.end()) {

		if (it->second == computeHashPassword(password))
			return true;
		else
		{
			std::cout << "Tentativo di accesso di " << name << "con password errata" << std::endl;
			return false;

		}
	}
	else
	{
		std::cout << " Profilo " << name << " non presente nel file accounts" << std::endl;
		return false;
	}
}
