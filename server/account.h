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

		if (it->second == computeHashPassword(password)) {
			std::cout << name << " ha eseguito l'accesso con successo" << std::endl;
			return true;
		}
		else
		{
			std::cout << "Tentativo di accesso di " << name << " con password errata" << std::endl;
			return false;

		}
	}
	else
	{
		std::cout << " Profilo " << name << " non presente nel file accounts" << std::endl;
		return false;
	}
}


bool createNewAccount(std::string name, std::string password, std::map<std::string, int>& accounts, std::string filePath) {
	auto it = accounts.find(name);
	if (it != accounts.end()) {
		return false;
	}
	else {
		int hashPassword = computeHashPassword(password);
		accounts.insert(std::pair<std::string, int>(name, hashPassword));
		std::ofstream accountsFile(filePath, std::ios_base::app | std::ios_base::out);
		if (accountsFile.is_open()) {
			accountsFile << name << " " << hashPassword << "\n";
			accountsFile.close();
			return true;
		}
		else
			return false;
	}
}

bool registrationOfUser(std::string userAndPassword, std::map<std::string, int>& accounts, std::string filePath) {
	std::string username, password;
	std::string delimiter = "|";
	username = userAndPassword.substr(0, userAndPassword.find(delimiter));
	password = userAndPassword.substr(userAndPassword.find(delimiter)+1, userAndPassword.length());
	return createNewAccount(username, password, accounts, filePath);
}