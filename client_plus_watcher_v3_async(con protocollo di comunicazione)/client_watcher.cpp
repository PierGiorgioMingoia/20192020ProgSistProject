// client_watcher.cpp : Questo file contiene la funzione 'main', in cui inizia e termina l'esecuzione del programma.
//

#include "FileWatcher.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>
#include <fstream>
#include <string>
#include <iomanip>

using boost::asio::ip::tcp;
void receive(tcp::socket* s);
std::mutex message_buffer_access;

enum { max_length = 1024 };

void send(std::string msg,tcp::socket &s)
{
    using namespace std; // For strlen.
    char request[max_length];
    //char reply[max_length];
    size_t request_length;

    strcpy_s(request, msg.c_str());
    request_length = strlen(request);

    message_buffer_access.lock();
    std::cout << "sent message: "<< request << '\n';
    message_buffer_access.unlock();

    boost::asio::write(s, boost::asio::buffer(request, request_length));
    //size_t reply_length = boost::asio::read(s,
        //boost::asio::buffer(reply, request_length));

    //std::cout << "Reply is: ";
    //std::cout.write(reply, reply_length);
    //std::cout << "\n";
}

void receive(tcp::socket* s)
{
    using namespace std; // For strlen.
    char reply[max_length];

    while (1)
    {
        size_t reply_length = s->read_some(boost::asio::buffer(reply, max_length));
        //size_t reply_length = boost::asio::read(s,
            //boost::asio::buffer(reply, max_length));

        message_buffer_access.lock();
        std::cout << "Reply is: ";
        std::cout.write(reply, reply_length);
        std::cout << "\n";
        message_buffer_access.unlock();
    }
}


int main(int argc, char* argv[]) {

    

    try
    {
        if (argc != 3)
        {
            std::cerr << "Usage: blocking_tcp_echo_client <host> <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;

        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints =
            resolver.resolve(tcp::v4(), argv[1], argv[2]);

        tcp::socket s(io_context);
        boost::asio::connect(s, endpoints);

        std::cout << "CLIENT ONLINE" << std::endl;

        using namespace std; // For strlen.
        char request[max_length];
        char reply[max_length];
        size_t request_length;
        //send(msg,s);
        std::thread listener (receive , &s);

	// Create a FileWatcher instance that will check the current folder for changes every 5 seconds
	FileWatcher fw{ "./", std::chrono::milliseconds(5000),s };

	// Start monitoring a folder for changes and (in case of changes)
	// run a user provided lambda function
	fw.start([](std::string path_to_watch, FileStatus status, tcp::socket& s) -> void {
		// Process only regular files, all other file types are ignored
		if (!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased) {
			return;
		}
        std::string msg;
        std::ifstream iFile(path_to_watch);

		switch (status) {
		case FileStatus::created:
			std::cout << "C: " << path_to_watch << '\n';
            msg.append("C: " + path_to_watch + '\n');
            send(msg, s);
            if (iFile.is_open())
            {
                while (getline(iFile, msg))
                {
                    send(std::string ("L: ").append(msg), s);
                }
                send(std::string("F: ").append(path_to_watch), s);
            }
            
			break;
		case FileStatus::modified:
			std::cout << "M: " << path_to_watch << '\n';
            msg.append("M: " + path_to_watch + '\n');
            send(msg, s);
            if (iFile.is_open())
            {
                while (getline(iFile, msg))
                    send(std::string("L: ").append(msg), s);
                send(std::string("F: ").append(path_to_watch), s);
            }
			break;
		case FileStatus::erased:
			std::cout << "E: " << path_to_watch << '\n';
            msg.append("E: " + path_to_watch + '\n');
            send(msg, s);
            send(std::string("F: ").append(path_to_watch), s);

			break;
		default:
			std::cout << "Error! Unknown file status.\n";
		}
		});
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

// Per eseguire il programma: CTRL+F5 oppure Debug > Avvia senza eseguire debug
// Per eseguire il debug del programma: F5 oppure Debug > Avvia debug

// Suggerimenti per iniziare: 
//   1. Usare la finestra Esplora soluzioni per aggiungere/gestire i file
//   2. Usare la finestra Team Explorer per connettersi al controllo del codice sorgente
//   3. Usare la finestra di output per visualizzare l'output di compilazione e altri messaggi
//   4. Usare la finestra Elenco errori per visualizzare gli errori
//   5. Passare a Progetto > Aggiungi nuovo elemento per creare nuovi file di codice oppure a Progetto > Aggiungi elemento esistente per aggiungere file di codice esistenti al progetto
//   6. Per aprire di nuovo questo progetto in futuro, passare a File > Apri > Progetto e selezionare il file con estensione sln



