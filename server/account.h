#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
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
	long long sum = 0;
	int div = pow(2, 16) - 1;
	int size = password.size();
	const char* buf = password.c_str();
	char c;
	int vett[] = { 17,349,1153,2081,3361,4517,5573,6733,7411,16127,37199,93911,199933 };
	int n = password.c_str()[size / 2] % 13;
	for (int i = 0; i < size; i++)
	{
		c = buf[i];
		sum += sum * vett[n] + c * vett[(n * size) % 13] + (n + size % (i + 1)) * vett[c % 13];
		n = (n + c + sum) % 13;
		sum = sum * 999983 + c * 37199; //totalmente a caso, sono 2 numeri primi random che danno una buona distribuzione dei risultati
	}
	sum = abs(sum); // voglio solo
	sum = sum % div;//
	return int(sum);
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
	password = userAndPassword.substr(userAndPassword.find(delimiter) + 1, userAndPassword.length());
	return createNewAccount(username, password, accounts, filePath);
}

bool checkIfAlreadyLoggedIn(std::string user, std::vector<std::string>& activeAccounts) {
	auto it = std::find(activeAccounts.begin(), activeAccounts.end(), user);
	if (it != activeAccounts.end())
		return true;
	else
		return false;
}

void debugPrintAllActiveAccount(std::vector<std::string> path) {
	for (std::vector<std::string>::const_iterator i = path.begin(); i != path.end(); ++i)
		std::cout << *i << ' ';
}

void insertInActiveAccounts(std::string user, std::vector<std::string>& activeAccounts) {
	activeAccounts.push_back(user);
}

void removeFromActiveAccounts(std::string user, std::vector<std::string>& activeAccounts) {
	auto it = std::find(activeAccounts.begin(), activeAccounts.end(), user);
	if (it != activeAccounts.end())
		activeAccounts.erase(it);
}


