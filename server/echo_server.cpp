//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
//
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <map>
#include <filesystem>
#include <chrono>
#include <vector>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include "FileWatcher.h"
#include "backup.h"
#include "account.h"
#include "errorMsg.h"
#include "FullBackup.h"


using boost::asio::ip::tcp;

std::string accountsFilePaths = "accounts.txt";
std::map<std::string, int> accountsMapNamePassword = readAndStoreAccounts(accountsFilePaths);
std::vector<std::string> activeAccounts;

class session
{
public:
	session(boost::asio::io_context& io_context)
		: socket_(io_context)
	{
	}

	tcp::socket& socket()
	{
		return socket_;
	}

	void start()
	{
		size_t reply_length;
		std::ofstream Ofile;
		std::string file_name;
		std::map<std::string, BackUpFile> openBackUpFiles;
		bool login = false;
		try
		{

			while (!login) //senza login non si passa alla fase successiva
			{
				//login o registrazione, il primo messaggio è del client

				reply_length = socket_.read_some(boost::asio::buffer(data_, max_length));
				if (convertSocketMsgToString(data_).rfind("RGS", 0) == 0) {					// registrazione nuovo utente
					std::string userAndPassword = std::string(data_, reply_length - 1).substr(5);
					if (registrationOfUser(userAndPassword, accountsMapNamePassword, accountsFilePaths)) {
						std::string ret_user = "R: Registrazione avvenuta con successo\n";
						socket_.write_some(boost::asio::buffer(ret_user.c_str(), ret_user.size()));
					}
					else {
						std::string ret_user = "E: Account esistente\n";
						socket_.write_some(boost::asio::buffer(ret_user.c_str(), ret_user.size()));
					}
					continue; // finita la registrazione (bene o male) attendo di nuovo un messaggio (RGS o I) dal client
				}

				else if (data_[0] == 'I') // utente già registrato
				{
					user_ = std::string(data_, reply_length - 1).substr(3);

					std::string ret_user = "R: Login in corso\n";
					socket_.write_some(boost::asio::buffer(ret_user.c_str(), ret_user.size()));

				}
				else
				{
					std::cerr << "login failed";
					std::string ret_user = "E: messaggio ricevuto non conforme allo standard\n";
					socket_.write_some(boost::asio::buffer(ret_user.c_str(), ret_user.size()));
					continue; // qui è a scelta, se è arrivato un messaggio codificato male è colpa di un bug nel client probabilmente
								// quindi anche ripetendo il login è probabile si verifichi all'infinito lo stesso errore, si può mettere un "return"
				}

				//ricevo la pw
				reply_length = socket_.read_some(boost::asio::buffer(data_, max_length));
				if (data_[0] == 'P') // ricevuta la pw
				{
					//controllo se la pw è giusta
					std::string password = std::string(data_, reply_length - 1).substr(3);
					if (checkNameAndPassword(user_, password, accountsMapNamePassword) && !checkIfAlreadyLoggedIn(user_, activeAccounts)) {	//se la pw è corretta

						if (std::filesystem::exists("./" + user_))
						{
							//caso cartella utente già presente
							//...
							std::string ret_user = "R: Login effettuato con successo, la tua cartella ti aspetta\n";
							socket_.write_some(boost::asio::buffer(ret_user.c_str(), ret_user.size()));
						}
						else
						{
							//caso utente registrato ma al primo accesso
							//...
							std::string new_user = "N: Login effettuato, creazione della cartella \n";
							std::filesystem::create_directory("./" + user_);
							socket_.write_some(boost::asio::buffer(new_user.c_str(), new_user.size()));
						}
						insertInActiveAccounts(user_, activeAccounts);
						login = true; // fase login terminata, si può accedere alla fase successiva 
					}
					else // pw errata
					{
						std::string loginError = createLoginError();
						socket_.write_some(boost::asio::buffer(loginError.c_str(), loginError.size()));
						continue; // password errata probabilmente, aspetto che il client riprovi con nuovi dati
					}
				}
				else
				{
					std::cerr << "login failed";
					std::string ret_user = "E: messaggio ricevuto non conforme allo standard\n";
					socket_.write_some(boost::asio::buffer(ret_user.c_str(), ret_user.size()));
					continue; // come prima qui si puù mettere anche un "return" tanto probabilmente un errore nel formato del messaggio è dovuto a un bug nel client
				}

			}


			// questi tre si possono mettere all'inizio, sono qui perchè sono più vicini a dove vengono utilizzati
			std::string reply_str = "";
			std::filesystem::path p;
			bool cont = true;

			while (1)
			{
				//comportamento a regime R -> azione (-> W) -> R -> azione (-> W) -> R ....
				do
				{
					reply_length = socket_.read_some(boost::asio::buffer(data_, max_length));
					reply_str.append(data_, reply_length);

				} while (reply_length == max_length && reply_str.length() < 2048);  //leggo fino a 2048 byte dal socket, se ce n'è meno proseguo con solo quelli letti senza aspettare 
				cont = true;

				while (reply_str.length() > 0 && cont)                                   // ogni stringa reply_str può contenere 1 o più messaggi o parti di messaggio, ma il primo byte è sempre anche il primo byte di un messaggio
				{
					switch (reply_str[0])
					{
					case 'C':   //crea file
					{
						int pos = reply_str.find('\n');
						if (pos == std::string::npos)
						{
							cont = false;
							break;
						}
						file_name = "./" + user_ + "\\" + reply_str.substr(5, pos - 5);
						p = std::filesystem::path(file_name).remove_filename();
						std::filesystem::create_directories(p);
						Ofile.open(file_name, std::ofstream::binary);

						if (pos < reply_str.length() - 1)
							reply_str = std::string(reply_str, pos + 1);
						else
							reply_str = "";
					}
					break;
					case 'D':   //crea cartella vuota
					{
						int pos = reply_str.find('\n');
						if (pos == std::string::npos)
						{
							cont = false;
							break;
						}
						file_name = "./" + user_ + "\\" + reply_str.substr(5, pos - 5);
						p = std::filesystem::path(file_name);
						std::filesystem::create_directories(p);

						if (pos < reply_str.length() - 1)
							reply_str = std::string(reply_str, pos + 1);
						else
							reply_str = "";
					}
					break;
					case 'M':   //modifica
					{
						int pos = reply_str.find('\n');
						if (pos == std::string::npos)
						{
							cont = false;
							break;
						}
						file_name = "./" + user_ + "\\" + reply_str.substr(5, pos - 5);

						std::string realFileName = reply_str.substr(5, pos - 5);

						// copia di backup
						if (boost::filesystem::exists(file_name)) // per un errore il server potrebbe non avere la copia originale
							openBackUpFiles.insert(std::pair<std::string, BackUpFile>(file_name, createBackUpFile(user_, file_name, realFileName, std::filesystem::last_write_time(file_name))));

						Ofile.open(file_name, std::ofstream::binary);                          //C ed M sono in pratica la stessa cosa

						if (pos < reply_str.length() - 1)
							reply_str = std::string(reply_str, pos + 1);
						else
							reply_str = "";
					}
					break;
					case 'X':   //elimina
					{
						int pos = reply_str.find('\n');
						if (pos == std::string::npos)
						{
							cont = false;
							break;
						}
						file_name = "./" + user_ + "\\" + reply_str.substr(5, pos - 5);
						std::filesystem::remove_all(file_name);
						if (pos < reply_str.length() - 1)
							reply_str = std::string(reply_str, pos + 1);
						else
							reply_str = "";
					}
					break;
					case 'L':   //linea da inserire in append a un file
					{
						if (reply_str.length() < 8)
						{
							cont = false;
							break;
						}
						int n = std::stoi(reply_str.substr(3, 4));
						if (reply_str.length() < n + 8)
						{
							cont = false;
							break;
						}

						if (n != 0)
							Ofile.write(reply_str.c_str() + 8, n);
						if (n == reply_str.length() - 8)
							reply_str = "";
						else
							reply_str = std::string(reply_str, n + 8);
					}
					break;
					case 'S': // ricevo la richiesta di sincronizzazione
					{
						int pos = reply_str.find('\n');
						if (pos == std::string::npos)
						{
							cont = false;
							break;
						}
						FileWatcher fw("./" + user_ + "/", std::chrono::milliseconds(5000), socket_);
						std::string sync_data = fw.get_folder_data();
						if (sync_data.size() == 0)  //se la cartella è vuota devo comunque mandare un carattere altrimenti blocco tutto
							sync_data.append("|");
						//faccio cose
						//...
						//
						boost::asio::write(socket_, boost::asio::buffer(sync_data.c_str(), sync_data.size()));

						if (pos < reply_str.length() - 1)
							reply_str = std::string(reply_str, pos + 1);
						else
							reply_str = "";
					}
					break;
					case 'U':   //ultima modifica
					{
						int pos = reply_str.find('\n');
						if (pos == std::string::npos)
						{
							cont = false;
							break;
						}
						if (Ofile.is_open())
							Ofile.close();
						if (boost::filesystem::exists(file_name))
							boost::filesystem::last_write_time(file_name, std::stoi(reply_str.substr(3, pos - 3)));
						else
						{
							std::string err_msg = "E: file non presente " + file_name + "\n";
							socket_.write_some(boost::asio::buffer(err_msg.c_str(), err_msg.size()));
						}
						if (pos < reply_str.length() - 1)
							reply_str = std::string(reply_str, pos + 1);
						else
							reply_str = "";

						deleteBackUpFile(openBackUpFiles, file_name); // finita la modifica elimino la copia di backup ( in caso di file creato non avviene nulla in quanto non esiste la copia da eliminare

					}
					break;
					case 'F':   //fine comando
					{
						// controllo qualunque cosa mi possa servire
						//...
						//mando l'ack
						int pos = reply_str.find('\n');
						if (pos == std::string::npos)
						{
							cont = false;
							break;
						}
						boost::asio::write(socket_, boost::asio::buffer(reply_str.c_str(), pos));
						if (pos < reply_str.length() - 1)
							reply_str = std::string(reply_str, pos + 1);
						else
							reply_str = "";
						std::cout << "Operazione terminata su F" << file_name << std::endl;
					}
					break;
					case 'B':
					{
						int pos = reply_str.find('\n');
						if (pos == std::string::npos)
						{
							cont = false;
							break;
						}
						sendEntireUserFolder(user_, socket_);
						boost::asio::write(socket_, boost::asio::buffer("F: \n", 4));
						if (pos < reply_str.length() - 1)
							reply_str = std::string(reply_str, pos + 1);
						else
							reply_str = "";
					}
					break;
					default: // comando non prestabilito
					{
						std::string str("E: errore in formato messaggio\n");
						boost::asio::write(socket_, boost::asio::buffer(str.c_str(), str.length()));
						break; // o "return" tanto probabilmente è causato da un bug 
					}
					}
				}
			}

		}
		catch (std::exception e)
		{
			//TODO Gestione errore rollback to Backup
			if (Ofile.is_open())
			{
				Ofile.close();
				std::filesystem::remove(file_name);
			}
			while (!openBackUpFiles.empty())
			{
				std::map<std::string, BackUpFile>::iterator it = openBackUpFiles.begin();
				overWriteFileBackup(it->second, it->first);
				openBackUpFiles.erase(it);

			}

			//std::cerr << e.what();
			removeFromActiveAccounts(user_, activeAccounts);
			std::cout << user_ << " ha effettuato la disconnessione" << std::endl;
			return;
		}

	}

private:


	tcp::socket socket_;
	enum { max_length = 1024 };
	char data_[max_length];
	std::string user_;
};

class server
{
public:
	server(boost::asio::io_context& io_context, short port)
		: io_context_(io_context),
		acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
	{
		//start_accept();
	}

	void run()
	{
		boost::system::error_code ec;
		while (!ec)
		{
			session* new_session = new session(io_context_);
			acceptor_.accept(new_session->socket(), ec);
			connections_.push_back(std::thread(&server::handle_incoming_connection, this, new_session, ec));
		}
		for (auto& t : connections_)
			t.join();
	}

private:
	void handle_incoming_connection(session* new_session, const boost::system::error_code& error)
	{
		if (!error)
		{
			new_session->start();
		}

		delete new_session;
	}

	boost::asio::io_context& io_context_;
	tcp::acceptor acceptor_;
	std::vector<std::thread> connections_;
};

int main(int argc, char* argv[])
{
	try
	{
		if (argc != 2)
		{
			std::cerr << "Usage: async_tcp_echo_server <port>\n";
			return 1;
		}

		boost::asio::io_context io_context;

		using namespace std; // For atoi.
		server s(io_context, atoi(argv[1]));
		std::cout << "Server di backup avviato" << std::endl;
		s.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}