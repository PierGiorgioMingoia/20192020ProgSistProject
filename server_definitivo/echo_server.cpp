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
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include "FileWatcher.h"


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
                    sync =true;
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
                delete this;
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
                delete this;
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

            std::string reply_str;
            std::string token;                                                   
            std::string file_name;
            std::filesystem::path p;

            while (1)
            {
                //comportamento a regime R -> azione (-> W) -> R -> azione (-> W) -> R ....
                reply_str = "";
                do
                {
                    reply_length = socket_.read_some(boost::asio::buffer(data_, max_length));
                    reply_str.append(data_, reply_length);

                } while (reply_length == max_length && data_[max_length - 1] != '\n');  //per evitare di spezzare messaggi


                //case multiple lines are read as a single message                      // è possibile che più comandi vengano letti in una sola lettura, 
                std::stringstream ss(reply_str);                // ho provato a inizializzarlo fuori dal while e poi assegnargli un valore in questo punto ma poi dava problemi alla getline seguente
                //token.resize(10000);
                while (std::getline(ss, token, '\n'))                                   // per ogni comando ottenuto
                {
                    switch (token[0])
                    {
                    case 'C':   //crea
                        file_name = "./" + user_ + "\\" + token.substr(5);
                        p = std::filesystem::path(file_name).remove_filename();
                        std::filesystem::create_directories(p);
                        Ofile.open(file_name);
                        if (Ofile.is_open())                        //tutte queste stampe poi vanno eliminate
                            std::cout << "file is open\n";          //tutte queste stampe poi vanno eliminate
                        else                                        //tutte queste stampe poi vanno eliminate
                            std::cout << "file is not open\n";      //tutte queste stampe poi vanno eliminate
                        break;
                    case 'M':   //modifica
                        // si può forse fare una copia di backup nel caso la modifica non vada a buon fine, una sorta di rollback
                        //...
                        file_name = "./" + user_ + "\\" + token.substr(5);
                        Ofile.open(file_name);                          //C ed M sono in pratica la stessa cosa
                        if (Ofile.is_open())
                            std::cout << "file is open\n";
                        else
                            std::cout << "file is not open\n";
                        break;
                    case 'E':   //elimina
                        file_name = "./" + user_ + "\\" + token.substr(5);
                        std::filesystem::remove_all(file_name);
                        break;
                    case 'L':   //linea
                        if (Ofile.is_open())
                            std::cout << "file is open\n";
                        else
                            std::cout << "file is not open\n";
                        Ofile.write(token.substr(3).c_str(), token.size() - 3);
                        Ofile.put('\n');
                        break;
                    case 'S': // ricevo la richiesta di sincronizzazione
                        {FileWatcher fw("./" + user_ + "/", std::chrono::milliseconds(5000), socket_);
                        std::string sync_data = fw.get_folder_data();
                        if (sync_data.size() == 0)  //se la cartella è vuota devo comunque mandare un carattere altrimenti blocco tutto
                            sync_data.append("|");
                        //faccio cose
                        //...
                        //
                        boost::asio::write(socket_, boost::asio::buffer(sync_data.c_str(), sync_data.size())); 
                        }
                        break;
                    case 'U':   //ultima modifica
                        Ofile.close();
                        boost::filesystem::last_write_time(file_name, std::stoi(token.substr(3)));
                        break;
                    case 'F':   //fine comando
                        //per ora metto il close qui
                        //Ofile.close();
                        // controllo qualunque cosa mi possa servire
                        //...
                        //mando l'ack
                        boost::asio::write(socket_, boost::asio::buffer(token.c_str(), token.size()));
                        break;
                    default:
                        break;
                    }
                }
            }

            std::chrono::seconds delay(15);
            std::this_thread::sleep_for(delay);
            socket_.write_some(boost::asio::buffer(str, sizeof(str)));

            delete this;
            //socket_.async_read_some(boost::asio::buffer(data_, max_length),
            //    boost::bind(&session::handle_read, this,
            //        boost::asio::placeholders::error,
            //        boost::asio::placeholders::bytes_transferred));

        }
        catch (std::exception e)
        {
            std::cerr << e.what();
            delete this;
        }
        
    }

private:
    void handle_client()
    {
        char str[] = "F: hello";
        socket_.write_some(boost::asio::buffer(str, sizeof(str)));
        std::chrono::seconds delay(15);
        std::this_thread::sleep_for(delay);
        socket_.write_some(boost::asio::buffer(str, sizeof(str)));
    }
    void handle_read(const boost::system::error_code& error,
        size_t bytes_transferred)
    {
        if (!error)
        {
            boost::asio::async_write(socket_,
                boost::asio::buffer(data_, bytes_transferred),
                boost::bind(&session::handle_write, this,
                    boost::asio::placeholders::error));
        }
        else
        {
            delete this;
        }
    }

    void handle_write(const boost::system::error_code& error)
    {
        if (!error)
        {
            socket_.async_read_some(boost::asio::buffer(data_, max_length),
                boost::bind(&session::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            delete this;
        }
    }

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
            connections_.push_back(std::thread (&server::handle_incoming_connection, this, new_session,ec));
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
        else
        {
            delete new_session;
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