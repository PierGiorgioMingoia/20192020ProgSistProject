/* client watcher: si collega a un server per mantenere aggiornato il contenuto di una cartella su entrambe le macchine*/

#include "FileWatcher.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>
#include <fstream>
#include <string>
#include <iomanip>

#define BUFFER_SIZE 5                                                           // è il numero di comandi che può mandare prima di dover attendere una risposta dal server

using boost::asio::ip::tcp;

//-------------------------------definizione funzioni---------------------------------------------

void receive(tcp::socket* s);                                   
void reconnect(tcp::socket* s, tcp::resolver::results_type* endpoints);
void send_file(std::string path, tcp::socket& s);

//-------------------------------variabili globali------------------------------------------------
enum { max_length = 1024 };                                                     //dimensione massima dei messaggi mandabili e ricevibili

std::condition_variable buffer_var;                                             // serve a dire che il buffer dei messaggi è nuovamente non pieno
std::map<std::string,std::string> msg_buffer;                                   // buffer dei comandi; massima dimensione BUFFER_SIZE

std::condition_variable connect_var;                                            // serve a passare da modalità "working" a "tentativo di riconnessione" 
int DISCONNECTED = 0;                                                           // indica se attualmente il client è connesso al server

std::mutex flag_access;                                                         // mutex per l'accesso a DISCONNECTED
std::mutex cout_access;                                                         // mutex per l'accesso alla stampa, è utile per non mischiare i messaggi
std::mutex buffer_access;                                                       // mutex per l'accesso al buffer dei comandi

//-----------------------------------funzioni------------------------------------------------------

void send(std::string msg,tcp::socket &s)                                           // serve a mandare un solo messaggio sul socket s
{
    using namespace std; // For strlen.
    char request[max_length];
    size_t request_length;

    try {
        strcpy_s(request, msg.c_str());                                             // trasformo il messaggio in vettore di char
        request_length = strlen(request);                                           // leggo la lunghezza

        cout_access.lock();                                                         //stampa per debug
        std::cout << "sent message: "<< request;                                    //stampa per debug
        cout_access.unlock();                                                       //stampa per debug

        boost::asio::write(s, boost::asio::buffer(request, request_length));        // scrittura sul socket, è qui che può partire l'eccezione da catturare
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in send: " << e.what() << "\n";                     // solo per debug
        std::unique_lock<std::mutex> lk_dc(flag_access);                            // dico che client e server non sono più connessi
        DISCONNECTED = 1;                                                           // quindi devo passare alla modalità "riconnessione"
        lk_dc.unlock();                                                             //
        connect_var.notify_all();                                                   // qui sveglio il thread che se ne occupa
    }
}

void receive(tcp::socket* s)                                                        // funzione thread che controlla continuamente l'arrivo di nuovi messaggi
{
    using namespace std;                                                            // For strlen. NON NE HO IDEA era nella versione base da cui abbiamo iniziato ma non da fastidio quindi...
    char reply[max_length];                                                         //    
    size_t reply_length;                                                            //
                                                                                    //
    while (1)                                                                       //
    {                                                                               //
        try                                                                         //
        {                                                                           //
            std::unique_lock<std::mutex> lk_dc(flag_access);                        //
            while (DISCONNECTED) connect_var.wait(lk_dc);                           // controllo se sono disconnesso: se sì aspetto di essere svegliato, altrimenti continuo
            lk_dc.unlock();                                                         //
            reply_length = s->read_some(boost::asio::buffer(reply, max_length));    // legge il socket. è qui che possono scattare gli errori (dato che la read è sempre attiva è più probabile che sia la read la prima funzione ad accorgersi che il server non comunica più)
                                                                                    //
            //case multiple lines are read as a single message                      // è possibile che più comandi vengano letti in una sola lettura, 
            std::string reply_str(reply,reply_length),token;                        // in quel caso spezzo la stringa ottenuta e leggo i singoli comandi
            std::stringstream ss(reply_str);                                        //
            std::vector<std::string> replies;                                       //
            while (std::getline(ss, token, '\n'))                                   //
            {                                                                       //
                if (token[0] == 'F')                                                // se il comando è una risposta a un comando F(che sta per Fine, cioè l'ultimo di un determinato file)
                {                                                                   // allora lo considero un acknowledge per quel comando
                    std::unique_lock<std::mutex> lk(buffer_access);                 //
                    //msg_buffer.find(std::string(reply + 3*sizeof(char)));         // stesso significato della riga successiva
                    msg_buffer.erase(token.substr(3));                              // elimino il comando dal buffer dato che ho ricevuto l'ack
                    if (msg_buffer.size() ==4 ) buffer_var.notify_all();            // se prima il buffer era pieno(e di conseguenza ora non lo è più) noticfico che posso riprendere a mandare messaggi
                    lk.unlock();                                                    //
                                                                                    //
                    cout_access.lock();                                             // stampa er debug
                    std::cout << "Reply is: ";                                      // stampa er debug
                    std::cout << token;                                             // stampa er debug
                    std::cout << "\n";                                              // stampa er debug
                    cout_access.unlock();                                           // stampa er debug
                }                                                                   //
            }                                                                       //
        }                                                                           //
                                                                                    //
        catch (std::exception& e)                                                   //
        {                                                                           // 
            std::cerr << "Exception in receive: " << e.what() << "\n";              // solo per debug
            std::unique_lock<std::mutex> lk_dc(flag_access);                        // dico che client e server non sono più connessi
            DISCONNECTED = 1;                                                       // quindi devo passare alla modalità "riconnessione"
            lk_dc.unlock();                                                         // 
            connect_var.notify_all();                                               // qui sveglio il thread che se ne occupa
        }                                                                           //
    }                                                                               //
                                                                                    //
}                                                                                   //
                                                                                    //
                                                                                    //
void reconnect(tcp::socket* s, tcp::resolver::results_type* endpoints)              // thread sempre attivo che però è in sleep quando tutto va bene
{                                                                                   //
    std::string msg;                                                                //
    while (1)                                                                       //
    {                                                                               //
        std::unique_lock<std::mutex> lk(flag_access);                               //
        while (!DISCONNECTED) connect_var.wait(lk);                                 // attende di essere svegliato quando qualcuno si accorge della disconnessine
        std::chrono::milliseconds delay(50);                                        // ogni 50 ms riprova a connettersi
        std::this_thread::sleep_for(delay);                                         //
        try                                                                         //
        {                                                                           //
            boost::asio::connect(*s, *endpoints);                                   // prova a connettersi, tutto ciò che c'è dopo viene saltato in caso di fallimento perchè si salta al catch dell'errore
            DISCONNECTED = 0;                                                       // se la connessione è riuscita reiposto il flag
                                                                                    //
            // re-send non acknoledged commands                                     // prima di ritornare alla normalità devo svuotare il buffer, dato che contiene i comandi elaborati (in realtà un comando 
                                                                                    // viene inserito nel buffer subito prima dell'invio, quindi è possibile che il comando sia nel buffer ma non mandato 
                                                                                    // se si verifica un errore in fase di send) e che il watcher non ci tornerà più quando riprenderà l'attività normale
            std::unique_lock<std::mutex> lk_buf(buffer_access);                     //
            for (std::map<std::string, std::string>::iterator it = msg_buffer.begin();it!=msg_buffer.end();it++ )
            {                                                                       // per tutti i comandi nel buffer
                msg.append(it->second + ": " + it->first + '\n');                   
                send(msg, *s);                                                      // li rimando
                send_file(it->first, *s);                                           // rimando il contenuto del file. da notare che se faccio il send di un file eliminato mando solo il comando F: (spiegazione nel corpo della funzione)
            }                                                                       //
            lk_buf.unlock();                                                        //
            lk.unlock();                                                            // terminata la funzione di riconnessione
            connect_var.notify_all();                                               // notifico gli altri thread che possono ripartire
            cout_access.lock();                                                     // stampa di debug
            std::cout << "CLIENT ONLINE" << std::endl;                              // stampa di debug
            cout_access.unlock();                                                   // stampa di debug
        }                                                                           //
        catch (std::exception& e)                                                   //
        {                                                                           //
            std::cerr << "Exception in reconnect: " << e.what() << "\n";            // finchè non riesce a riconnettersi continua a mostrare il messaggio di errore
        }                                                                           //
                                                                                    //
    }                                                                               //
}                                                                                   //
                                                                                    //
void send_file(std::string path, tcp::socket &s)                                    //
{                                                                                   //
    std::ifstream iFile(path);                                                      // ifile è un file stream in imput
    std::string msg;                                                                //
    if (iFile.is_open())                                                            // se il file non esiste ifile non lo crea perchè di default può solo ricevere (almeno mi sembra)
    {                                                                               //
        while (getline(iFile, msg))                                                 // mando riga per riga, può essere ottimizzato
        {                                                                           //
            send(std::string("L: ").append(msg).append("\n"), s);                   //
        }                                                                           //
                              //
    }   
    send(std::string("F: ").append(path).append("\n"), s);                          // quindi mando solo il comando F
}                                                                                   //
                                                                                    //
                                                                                    //
int main(int argc, char* argv[]) {                                                  // main
                                                                                    //
    try                                                                             //
    {                                                                               //
        if (argc != 3)                                                              //controllo sugli argomenti
        {                                                                           //
            std::cerr << "Usage: blocking_tcp_echo_client <host> <port>\n";         //
            return 1;                                                               //
        }                                                                           //
                                                                                    //
        boost::asio::io_context io_context;                                         // roba di boost
                                                                                    //
        tcp::resolver resolver(io_context);                                         //
        tcp::resolver::results_type endpoints =                                     //
            resolver.resolve(tcp::v4(), argv[1], argv[2]);                          //
                                                                                    //
        tcp::socket s(io_context);                                                  //
        boost::asio::connect(s, endpoints);                                         // prima connessione, se questa non va per ora il programma quitta, si può decidere di mandare il problema alla reconnect
                                                                                    //


        std::cout << "CLIENT ONLINE" << std::endl;

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

            std::unique_lock<std::mutex> lk_dc(flag_access);                        //
            while (DISCONNECTED) connect_var.wait(lk_dc);                           // controlla che il client sia connesso , altrimenti aspetta
            lk_dc.unlock();                                                         //
                                                                                    //
            std::unique_lock<std::mutex> lk(buffer_access);                         //
            while (msg_buffer.size()>=BUFFER_SIZE) buffer_var.wait(lk);             // controlla che il buffer non sia pieno, altrimenti aspetta
            lk.unlock();                                                            //
                                                                                    //
	    	switch (status) {                                                       //
                                                                                    //
	    	case FileStatus::created:                                               //
                                                                                    //
	    		//std::cout << "C: " << path_to_watch << '\n';                      //
                msg.append("C: " + path_to_watch + '\n');                           // comando C
                lk.lock();                                                          //
                msg_buffer.insert_or_assign(path_to_watch, std::string("C"));       // da notare che se modifico 2 volte lo stesso file mantengo solo un comando nel buffer, 
                                                                                    //  tanto in caso ci sia bisogno di rimandarlo posso mandare semplicemente l'ultima versione
                                                                                    // es prima C poi M -> mando solo M e il server se vede un nome di un file che non ha lo crea dal nulla 
                                                                                    // es prima C poi E -> dico al server di eliminare un file che non ha, il server può ignorare il messaggio
                                                                                    // es prima M poi E -> perchè rimandare delle modifiche di un file che poi dovrò cancellare?
                                                                                    // es prima M poi M -> mando direttamente l'ultima versione e risparmio tempo
                lk.unlock();                                                        //
                send(msg, s);                                                       //
                send_file(path_to_watch, s);                                        //
	    		break;                                                              //
                                                                                    //
                                                                                    //
	    	case FileStatus::modified:                                              //
	    		//std::cout << "M: " << path_to_watch << '\n';                      //
                msg.append("M: " + path_to_watch + '\n');                           // comando M
                lk.lock();                                                          //
                msg_buffer.insert_or_assign(path_to_watch, std::string("M"));       //
                lk.unlock();                                                        //
                send(msg, s);                                                       //
                send_file(path_to_watch, s);                                        //
	    		break;                                                              //
                                                                                    //
                                                                                    //
	    	case FileStatus::erased:                                                //
	    		//std::cout << "E: " << path_to_watch << '\n';                      //
                msg.append("E: " + path_to_watch + '\n');                           // comando E
                lk.lock();                                                          //
                msg_buffer.insert_or_assign(path_to_watch, std::string("E"));       //
                lk.unlock();                                                        //
                send(msg, s);                                                       //
                send(std::string("F: ").append(path_to_watch).append("\n"), s);     //
                                                                                    //
	    		break;                                                              //
	    	default:                                                                //
	    		std::cout << "Error! Unknown file status.\n";                       //
	    	}                                                                       //
                                                                                    //
		});                                                                         //
    }                                                                               //
    catch (std::exception& e)                                                       //
    {                                                                               //
        std::cerr << "Exception general: " << e.what() << "\n";                     //
    }

}


