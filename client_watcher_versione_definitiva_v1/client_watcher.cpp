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

#define BUFFER_SIZE 5

using boost::asio::ip::tcp;

void receive(tcp::socket* s);
void reconnect(tcp::socket* s, tcp::resolver::results_type* endpoints);
void send_file(std::string path, tcp::socket& s);


std::condition_variable buffer_var; 
std::condition_variable connect_var;
std::map<std::string,std::string> msg_buffer; 
int DISCONNECTED = 0;
std::mutex flag_access;
std::mutex cout_access;
std::mutex buffer_access;

enum { max_length = 1024 };

void send(std::string msg,tcp::socket &s)
{
    using namespace std; // For strlen.
    char request[max_length];
    size_t request_length;

    try {
        strcpy_s(request, msg.c_str());
        request_length = strlen(request);

        cout_access.lock();
        std::cout << "sent message: "<< request;
        cout_access.unlock();

        boost::asio::write(s, boost::asio::buffer(request, request_length));
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in send: " << e.what() << "\n";
        std::unique_lock<std::mutex> lk_dc(flag_access);
        DISCONNECTED = 1;
        lk_dc.unlock();
        connect_var.notify_all();
    }
}

void receive(tcp::socket* s)
{
    using namespace std; // For strlen.
    char reply[max_length];
    size_t reply_length;
    
    while (1)
    {
        try
        {
            std::unique_lock<std::mutex> lk_dc(flag_access);
            while (DISCONNECTED) connect_var.wait(lk_dc);
            lk_dc.unlock();
            reply_length = s->read_some(boost::asio::buffer(reply, max_length));

            //case multiple lines are read as a single message
            std::string reply_str(reply,reply_length),token;
            std::stringstream ss(reply_str);
            std::vector<std::string> replies;
            while (std::getline(ss, token, '\n'))
            {
                if (token[0] == 'F')
                {
                    std::unique_lock<std::mutex> lk(buffer_access);
                    //msg_buffer.find(std::string(reply + 3*sizeof(char)));
                    msg_buffer.erase(token.substr(3));
                    if (msg_buffer.size() ==4 ) buffer_var.notify_all();
                    lk.unlock();

                    cout_access.lock();
                    std::cout << "Reply is: ";
                    std::cout << token;
                    std::cout << "\n";
                    cout_access.unlock();
                }
            }
        }

        catch (std::exception& e)
        {
            std::cerr << "Exception in receive: " << e.what() << "\n";
            std::unique_lock<std::mutex> lk_dc(flag_access);
            DISCONNECTED = 1;
            lk_dc.unlock();
            connect_var.notify_all();
        }
    }
    
}


void reconnect(tcp::socket* s, tcp::resolver::results_type* endpoints)
{
    std::string msg;
    while (1)
    {
        std::unique_lock<std::mutex> lk(flag_access);
        while (!DISCONNECTED) connect_var.wait(lk);
        std::chrono::milliseconds delay(50);
        std::this_thread::sleep_for(delay);
        try
        {
            boost::asio::connect(*s, *endpoints); 
            DISCONNECTED = 0;

            // re-send non acknoledged commands
            std::unique_lock<std::mutex> lk_buf(buffer_access);
            for (std::map<std::string, std::string>::iterator it = msg_buffer.begin();it!=msg_buffer.end();it++ )
            {
                msg.append(it->second + ": " + it->first + '\n');
                send(msg, *s);
                send_file(it->first, *s);
            }
            lk_buf.unlock();
            lk.unlock();
            connect_var.notify_all();
            cout_access.lock();
            std::cout << "CLIENT ONLINE" << std::endl;
            cout_access.unlock();
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception in reconnect: " << e.what() << "\n";
        }
        
    }
}

void send_file(std::string path, tcp::socket &s)
{
    std::ifstream iFile(path);
    std::string msg;
    if (iFile.is_open())
    {
        while (getline(iFile, msg))
        {
            send(std::string("L: ").append(msg).append("\n"), s);
        }
        send(std::string("F: ").append(path).append("\n"), s);
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

        //reconnect(&s,&endpoints);

        std::cout << "CLIENT ONLINE" << std::endl;
        using namespace std; // For strlen.
        char request[max_length];
        char reply[max_length];
        size_t request_length;
        //send(msg,s);
        std::thread listener (receive , &s);
        std::thread reconnecter(reconnect, &s, &endpoints);
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

        std::unique_lock<std::mutex> lk_dc(flag_access);
        while (DISCONNECTED) connect_var.wait(lk_dc);
        lk_dc.unlock();

        std::unique_lock<std::mutex> lk(buffer_access);
        while (msg_buffer.size()>=BUFFER_SIZE) buffer_var.wait(lk);
        lk.unlock();

		switch (status) {

		case FileStatus::created:

			//std::cout << "C: " << path_to_watch << '\n';
            msg.append("C: " + path_to_watch + '\n');
            lk.lock();
            msg_buffer.insert_or_assign(path_to_watch, std::string("C"));
            lk.unlock();
            send(msg, s);
            send_file(path_to_watch, s);
			break;
            
            
		case FileStatus::modified:
			//std::cout << "M: " << path_to_watch << '\n';
            msg.append("M: " + path_to_watch + '\n');
            lk.lock();
            msg_buffer.insert_or_assign(path_to_watch, std::string("M"));
            lk.unlock();
            send(msg, s);
            send_file(path_to_watch, s);
			break;


		case FileStatus::erased:
			//std::cout << "E: " << path_to_watch << '\n';
            msg.append("E: " + path_to_watch + '\n');
            lk.lock();
            msg_buffer.insert_or_assign(path_to_watch, std::string("E"));
            lk.unlock();
            send(msg, s);
            send(std::string("F: ").append(path_to_watch).append("\n"), s);

			break;
		default:
			std::cout << "Error! Unknown file status.\n";
		}

		});
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception general: " << e.what() << "\n";
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



