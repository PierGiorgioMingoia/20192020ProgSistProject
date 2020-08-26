//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <map>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include "FileWatcher.h"
#include "backup.h"


using boost::asio::ip::tcp;

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
		// hello di debug
		char str[] = "F: hello\n";
		size_t reply_length;
		bool sync = false;
		std::ofstream Ofile;
		std::string file_name;
		std::map<std::string, std::string> openBackUpFiles;
		try
		{
			//socket_.write_some(boost::asio::buffer(str, sizeof(str)));

			//login, il primo messaggio è del client
			reply_length = socket_.read_some(boost::asio::buffer(data_, max_length));
			if (data_[0] == 'I') // ricevo l'ID
			{
				user_ = std::string(data_, reply_length - 1).substr(3);
				//cerca se l'utente è già registrato
				//...
				//
				if (std::filesystem::exists("./" + user_))// dovrebbe anche avere una mappa<string,string> di user-password e controllare il match
				{
					//caso utente già presente
					//...
					sync = true;
					std::string ret_user = "R: hello\n";
					socket_.write_some(boost::asio::buffer(ret_user.c_str(), ret_user.size()));
				}
				else
				{
					//caso nuovo utente
					//...
					sync = false;
					std::string new_user = "N: hello\n";
					std::filesystem::create_directory("./" + user_);
					socket_.write_some(boost::asio::buffer(new_user.c_str(), new_user.size()));
				}



			}
			else
			{
				std::cerr << "login failed";
				return;
			}

			//ricevo la pw
			reply_length = socket_.read_some(boost::asio::buffer(data_, max_length));
			if (data_[0] == 'P') // ricevo la pw
			{
				//controllo se la pw è giusta
				//...
				//per ora qualunque password è accettata
				std::string new_user = "I: hello\n"; // completata identificazione
				socket_.write_some(boost::asio::buffer(new_user.c_str(), new_user.size()));
			}
			else
			{
				std::cerr << "login failed";
				return;
			}


			//sincronizzazione (eventuale)
			/*if (sync) // spostato dentro lo switch case
			{
				reply_length = socket_.read_some(boost::asio::buffer(data_, max_length));
				if (data_[0] == 'S') // ricevo la richiesta di sincronizzazione
				{
					FileWatcher fw("./" + user_+"/", std::chrono::milliseconds(5000), socket_);
					std::string sync_data = fw.get_folder_data();
					if (sync_data.size() == 0)
						sync_data.append("|");
					//faccio cose
					//...
					//
					boost::asio::write(socket_, boost::asio::buffer(sync_data.c_str(), sync_data.size()));
				}
				else
				{
					std::cerr << "sync failed";
					delete this;
				}
			}*/
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

				} while (reply_length == max_length && reply_str.length() < 2048);//&& data_[max_length - 1] != '\n');  //per evitare di spezzare messaggi
				cont = true;

				//case multiple lines are read as a single message                      // è possibile che più comandi vengano letti in una sola lettura, 
				//std::stringstream ss(reply_str);                // ho provato a inizializzarlo fuori dal while e poi assegnargli un valore in questo punto ma poi dava problemi alla getline seguente
				//token.resize(10000);
				while (reply_str.length() > 0 && cont)                                   // per ogni comando ottenuto
				{
					switch (reply_str[0])
					{
					case 'C':   //crea
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
						if (Ofile.is_open())                        //tutte queste stampe poi vanno eliminate
							std::cout << "file is open C\n";          //tutte queste stampe poi vanno eliminate
						else                                        //tutte queste stampe poi vanno eliminate
							std::cout << "file is not open\n";      //tutte queste stampe poi vanno eliminate
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
						openBackUpFiles.insert(std::pair<std::string, std::string>(file_name, createBackUpFile(user_, file_name, realFileName)));


						// si può forse fare una copia di backup nel caso la modifica non vada a buon fine, una sorta di rollback
						//...
						Ofile.open(file_name, std::ofstream::binary);                          //C ed M sono in pratica la stessa cosa
						if (Ofile.is_open())
							std::cout << "file is open M\n";
						else
							std::cout << "file is not open\n";
						if (pos < reply_str.length() - 1)
							reply_str = std::string(reply_str, pos + 1);
						else
							reply_str = "";
					}
					break;
					case 'E':   //elimina
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
					case 'L':   //linea
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
						if (Ofile.is_open())
							std::cout << "file is open L\n";
						else
							std::cout << "file is not open\n";
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
						Ofile.close();
						boost::filesystem::last_write_time(file_name, std::stoi(reply_str.substr(3, pos - 3)));
						if (pos < reply_str.length() - 1)
							reply_str = std::string(reply_str, pos + 1);
						else
							reply_str = "";

						deleteBackUpfile(openBackUpFiles, file_name);

					}
					break;
					case 'F':   //fine comando
					{
						//per ora metto il close qui
						//Ofile.close();
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
						std::cout << "F" << file_name << std::endl;
					}
					break;
					default:
						break;
					}
				}
			}
			//socket_.async_read_some(boost::asio::buffer(data_, max_length),
			//    boost::bind(&session::handle_read, this,
			//        boost::asio::placeholders::error,
			//        boost::asio::placeholders::bytes_transferred));

		}
		catch (std::exception e)
		{
			//TODO Gestione errore rollback to Backup
			while (!openBackUpFiles.empty())
			{
				std::map<std::string, std::string>::iterator it = openBackUpFiles.begin();
				overWriteFileBackup(it->second, it->first);
				openBackUpFiles.erase(it);

			}

			std::cerr << e.what();
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
	void start_accept()
	{
		session* new_session = new session(io_context_);
		acceptor_.async_accept(new_session->socket(),
			boost::bind(&server::handle_accept, this, new_session,
				boost::asio::placeholders::error));
	}

	void handle_accept(session* new_session,
		const boost::system::error_code& error)
	{
		if (!error)
		{
			new_session->start();
		}
		else
		{
			delete new_session;
		}

		start_accept();
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
		s.run();
		//io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}