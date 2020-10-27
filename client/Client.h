#pragma once

#include "FileWatcher.h"
#include <boost/asio.hpp>
#include <string>
#include <iostream>

using boost::asio::ip::tcp;

#define BUFFER_SIZE 5                                                           // è il numero di comandi che può mandare prima di dover attendere una risposta dal server

//-------------------------------definizione funzioni---------------------------------------------

void receive(tcp::socket* s);           
void reconnect(tcp::socket* s, tcp::resolver::results_type* endpoints, std::string* user, std::string* pw);
void send_file(std::string path, tcp::socket& s);
int login(tcp::socket& s, std::string& user, std::string& pw, bool first_time = false);
std::string syncronize(tcp::socket& s);
bool backup(tcp::socket& s,std::string user);
std::string backup_or_sync(tcp::socket& s,std::string user);
void send(std::string msg, tcp::socket& s);
void send(const char* msg, int size, tcp::socket& s);

//-------------------------------variabili globali------------------------------------------------
enum { max_length = 1024 };                                                     //dimensione massima dei messaggi mandabili e ricevibili

std::condition_variable buffer_var;                                             // serve a dire che il buffer dei messaggi è nuovamente non pieno
std::map<std::string, std::string> msg_buffer;                                   // buffer dei comandi; massima dimensione BUFFER_SIZE

std::condition_variable connect_var;                                            // serve a passare da modalità "working" a "tentativo di riconnessione" 
int DISCONNECTED = 0;                                                           // indica se attualmente il client è connesso al server
int REPORTED = 0;                                                               // indicca se un errore della connessione è già stato riportato da qualcuno

std::mutex flag_access;                                                         // mutex per l'accesso a DISCONNECTED
std::mutex cout_access;                                                         // mutex per l'accesso alla stampa, è utile per non mischiare i messaggi
std::mutex buffer_access;                                                       // mutex per l'accesso al buffer dei comandi



class Client
{
private:
    std::string ip_addr, port;
    boost::asio::io_context io_context;
    bool cmd_args;

    void get_args()
    {
        bool correct = false;
        while (!correct)
        {
            std::string tmp;
            std::cout << "inserire indirizzo IPv4 server: ";
            std::cin >> tmp;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); //flush di cin

            boost::system::error_code ec;
            boost::asio::ip::address::from_string(tmp, ec);
            if (ec)
            {
                std::cout << std::endl << "indirizzo IPv4 non valido [xxx.xxx.xxx.xxx]" << std::endl;
                continue;
            }
            correct = true;
            ip_addr = tmp;
        }

        correct = false;
        while (!correct)
        {
            std::string tmp;
            std::cout << "inserire porta server: ";
            std::cin >> tmp;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); //flush di cin

            if (std::stoi(tmp)<1 || std::stoi(tmp)>65535)
            {
                std::cout << std::endl << "porta non valida [1-65535]" << std::endl;
                continue;
            }
            correct = true;
            port = tmp;
        }

    }

public:
    Client() { cmd_args = false; }
    Client(char* ip, char* port) : ip_addr(ip), port(port)
    {
        cmd_args = true;
        //magari mettere controlli se ip e porta sono validi 
    }
    void run()
    {
        if (!cmd_args)
        {
            get_args();
        }
        // creazione connessione
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve(tcp::v4(), ip_addr, port);
        tcp::socket s(io_context);
        boost::asio::connect(s, endpoints);

        std::cout << "Connessione avvenuta con successo" << std::endl;

        std::string user, pw; // in realtà poi user e pw vengono chiesti 
        // possibile sostituire con shared pointer? avrebbe qualche vantaggio?


        int log = login(s, user, pw, true);
        std::filesystem::create_directories("./" + user);
        std::string data;
        if (log == 1)                //utente già esistente
            data = backup_or_sync(s,user);// syncronize(s);
        if (log == 2)               //utente nuovo
            data = "|";

        std::thread listener(receive, &s);
        std::thread reconnecter(reconnect, &s, &endpoints, &user, &pw);

        // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
        FileWatcher fw{ "./" + user + "/", std::chrono::milliseconds(5000),s,data };
        // Start monitoring a folder for changes and (in case of changes)
        // run a user provided lambda function
        fw.start([](std::string path_to_watch, std::string root_folder, FileStatus status, tcp::socket& s) -> void {
            // Process only regular files, all other file types are ignored
            try
            {
                bool dir=false;

                std::string msg;
                std::string relative_path = "./" + path_to_watch.substr(root_folder.size());

                std::unique_lock<std::mutex> lk_dc(flag_access);                        //
                while (DISCONNECTED) connect_var.wait(lk_dc);                           // controlla che il client sia connesso , altrimenti aspetta
                lk_dc.unlock();                                                         //
                                                                                        //
                std::unique_lock<std::mutex> lk(buffer_access);                         //
                while (msg_buffer.size() >= BUFFER_SIZE) buffer_var.wait(lk);             // controlla che il buffer non sia pieno, altrimenti aspetta
                lk.unlock();                                                            //
                                                                                        //
                switch (status) {                                                       //
                                                                                        //
                case FileStatus::created:                                               //
                                                                                        //
                    if (std::filesystem::is_directory(std::filesystem::path(path_to_watch))) 
                    {
                        if (std::filesystem::is_empty(std::filesystem::path(path_to_watch)))
                        {
                            msg.append("D: " + relative_path + '\n');                           // comando D
                            /*lk.lock();                                                          //
                            msg_buffer.insert_or_assign(relative_path, std::string("D"));         // è necessario? insomma una cartella vuota non ha molta utilità
                            lk.unlock();        */                                                //
                            send(msg, s);                                                       //                                    //
                            send(std::string("U: ").append(std::to_string(boost::filesystem::last_write_time(path_to_watch))).append("\n"), s);
                            send(std::string("F: ").append(relative_path).append("\n"), s);     // 
                        }
                        return;
                    }
                    //std::cout << "C: " << path_to_watch << '\n';                      //
                    msg.append("C: " + relative_path + '\n');                           // comando C
                    lk.lock();                                                          //
                    msg_buffer.insert_or_assign(relative_path, std::string("C"));       // da notare che se modifico 2 volte lo stesso file mantengo solo un comando nel buffer, 
                                                                                        // tanto in caso ci sia bisogno di rimandarlo posso mandare semplicemente l'ultima versione
                                                                                        // es prima C poi M -> mando solo M e il server se vede un nome di un file che non ha lo crea dal nulla 
                                                                                        // es prima C poi E -> dico al server di eliminare un file che non ha, il server può ignorare il messaggio
                                                                                        // es prima M poi E -> perchè rimandare delle modifiche di un file che poi dovrò cancellare?
                                                                                        // es prima M poi M -> mando direttamente l'ultima versione e risparmio tempo
                    lk.unlock();                                                        //
                    send(msg, s);                                                       //
                    send_file(path_to_watch, s);                                        //
                    send(std::string("U: ").append(std::to_string(boost::filesystem::last_write_time(path_to_watch))).append("\n"), s);
                    send(std::string("F: ").append(relative_path).append("\n"), s);     // 
                    break;                                                              //
                                                                                        //
                                                                                        //
                case FileStatus::modified:                                              //
                    //std::cout << "M: " << path_to_watch << '\n';                      //
                    if (std::filesystem::is_directory(std::filesystem::path(path_to_watch)))
                    {
                        return;
                    }
                    msg.append("M: " + relative_path + '\n');                           // comando M
                    lk.lock();                                                          //
                    msg_buffer.insert_or_assign(relative_path, std::string("M"));       //
                    lk.unlock();                                                        //
                    send(msg, s);                                                       //
                    send_file(path_to_watch, s);                                        //
                    send(std::string("U: ").append(std::to_string(boost::filesystem::last_write_time(path_to_watch))).append("\n"), s);
                    send(std::string("F: ").append(relative_path).append("\n"), s);     // 
                    break;                                                              //
                                                                                        //
                                                                                        //
                case FileStatus::erased:                                                //
                    //std::cout << "E: " << path_to_watch << '\n';                      //
                    msg.append("X: " + relative_path + '\n');                           // comando E
                    lk.lock();                                                          //
                    msg_buffer.insert_or_assign(relative_path, std::string("E"));       //
                    lk.unlock();                                                        //
                    send(msg, s);                                                       //
                    send(std::string("F: ").append(relative_path).append("\n"), s);     //
                                                                                        //
                    break;                                                              //
                default:                                                                //
                    std::cout << "Error! Unknown file status.\n";                       //
                }                                                                       //
                                                                                        //
            }
            catch (std::exception e)
            {
                int first_to_report = 0;
                std::unique_lock<std::mutex> lk_re(flag_access);
                if (!REPORTED)
                {
                    REPORTED = 1;
                    first_to_report = 1;
                }
                lk_re.unlock();

                if (first_to_report)
                {
                    std::unique_lock<std::mutex> lk_dc(flag_access);                            // dico che client e server non sono più connessi
                    DISCONNECTED = 1;                                                           // quindi devo passare alla modalità "riconnessione"
                    lk_dc.unlock();                                                             //
                    connect_var.notify_all();                                                   // qui sveglio il thread che se ne occupa
                }
            }
            });
    }


};



using boost::asio::ip::tcp;


//-----------------------------------funzioni------------------------------------------------------
void send(const char* msg, int size, tcp::socket& s)
{
    try 
    {
        boost::asio::write(s, boost::asio::buffer(msg, size));  
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in send: " << e.what() << "\n";                     // solo per debug
        throw;
    }
}


void send(std::string msg, tcp::socket& s)                                           // serve a mandare un solo messaggio sul socket s
{
    try 
    {
        send(msg.c_str(), (int)msg.size(), s);
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in send: " << e.what() << "\n";                     // solo per debug
        throw;
    }
}

void receive(tcp::socket* s)                                                        // funzione thread che controlla continuamente l'arrivo di nuovi messaggi
{

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
            std::string reply_str(reply, reply_length), token;                        // in quel caso spezzo la stringa ottenuta e leggo i singoli comandi
            std::stringstream ss(reply_str);                                        //
            while (std::getline(ss, token, '\n'))                                   //
            {                                                                       //
                if (token[0] == 'F')                                                // se il comando è una risposta a un comando F(che sta per Fine, cioè l'ultimo di un determinato file)
                {                                                                   // allora lo considero un acknowledge per quel comando
                    std::unique_lock<std::mutex> lk(buffer_access);                 //
                    msg_buffer.erase(token.substr(3));                              // elimino il comando dal buffer dato che ho ricevuto l'ack
                    if (msg_buffer.size() == 4) buffer_var.notify_all();            // se prima il buffer era pieno(e di conseguenza ora non lo è più) noticfico che posso riprendere a mandare messaggi
                    lk.unlock();                                                    //
                                                                                    //
                    std::unique_lock<std::mutex> lk_out(cout_access);                   // stampa per debug
                    std::cout << "Reply is: ";                                      // stampa per debug
                    std::cout << token;                                             // stampa per debug
                    std::cout << "\n";                                              // stampa per debug
                }
                else if (token[0] == 'X')                                                
                {                                                               
                    std::unique_lock<std::mutex> lk_out(cout_access);               // stampa per debug
                    std::cout << token;                                             // stampa per debug
                    std::cout << "\n";                                              // stampa per debug
                }//
            }                                                                       //
        }                                                                           //
                                                                                    //
        catch (std::exception& e)                                                   //
        {
            int first_to_report = 0;                                                         // 
            std::cerr << "Exception in receive: " << e.what() << "\n";              // solo per debug

            std::unique_lock<std::mutex> lk_re(flag_access);
            if (!REPORTED)
            {
                REPORTED = 1;
                first_to_report = 1;
            }
            lk_re.unlock();

            if (first_to_report)
            {
                std::unique_lock<std::mutex> lk_dc(flag_access);                        // dico che client e server non sono più connessi
                DISCONNECTED = 1;                                                       // quindi devo passare alla modalità "riconnessione"
                lk_dc.unlock();                                                         // 
                connect_var.notify_all();                                               // qui sveglio il thread che se ne occupa
            }
        }                                                                           //
    }                                                                               //
                                                                                    //
}                                                                                   //
                                                                                    //


                                                                                    //
void reconnect(tcp::socket* s, tcp::resolver::results_type* endpoints, std::string* user, std::string* pw)
{                                                                                   //
    try
    {
        std::string msg;                                                                //
        while (1)                                                                       //
        {                                                                               //
            std::unique_lock<std::mutex> lk(flag_access);                               //
            while (!DISCONNECTED) connect_var.wait(lk);                                 // attende di essere svegliato quando qualcuno si accorge della disconnessione
            lk.unlock();
            try                                                                         //
            {                                                                           //
                boost::asio::connect(*s, *endpoints);                                   // prova a connettersi, tutto ciò che c'è dopo viene saltato in caso di fallimento perchè si salta al catch dell'errore
                std::unique_lock<std::mutex> lk_out(cout_access);                       // stampa di debug
                std::cout << "Riconnessione avvenuta con successo" << std::endl;          // stampa di debug
                lk_out.unlock();                                                        // stampa di debug
                login(*s, *user, *pw);

                //check se il login è andato bene
                //...
                                                                                        //
                // re-send non acknoledged commands                                     // prima di ritornare alla normalità devo svuotare il buffer, dato che contiene i comandi elaborati (in realtà un comando 
                                                                                        // viene inserito nel buffer subito prima dell'invio, quindi è possibile che il comando sia nel buffer ma non mandato 
                                                                                        // se si verifica un errore in fase di send) e che il watcher non ci tornerà più quando riprenderà l'attività normale
                std::unique_lock<std::mutex> lk_buf(buffer_access);                     //
                for (std::map<std::string, std::string>::iterator it = msg_buffer.begin(); it != msg_buffer.end(); it++)
                {                                                                       // per tutti i comandi nel buffer
                    msg.append(it->second + ": " + it->first + '\n');
                    send(msg, *s);                                                      // li rimando
                    send_file("./" + *user + it->first.substr(1), *s);                  // rimando il contenuto del file. da notare che se faccio il send di un file eliminato non mando nulla
                    if (it->second.compare("E") != 0)
                        send(std::string("U: ").append(std::to_string(boost::filesystem::last_write_time("./" + *user + it->first.substr(1)))).append("\n"), *s);
                    send(std::string("F: ").append(it->first).append("\n"), *s);
                }                                                                       //
                lk_buf.unlock();                                                        //
                                                                                        // terminata la funzione di riconnessione


                std::unique_lock<std::mutex> lk_re(flag_access);
                REPORTED = 0;
                lk_re.unlock();


                lk.lock();                              //
                DISCONNECTED = 0;
                lk.unlock();
                connect_var.notify_all();                                               // notifico gli altri thread che possono ripartire
            }                                                                           //
            catch (std::exception& e)                                                   //
            {                                                                           //
                std::cerr << "Exception in reconnect: " << e.what() << "\n";            // finchè non riesce a riconnettersi continua a mostrare il messaggio di errore
            }                                                                           //
                                                                                        //
        }                                                                               //

    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in reconnect: " << e.what() << "\n";
    }

}                                                                                   //

        //send file binary
void send_file(std::string path, tcp::socket& s)                                    //
{
    try
    {
        std::ifstream iFile(path, std::ifstream::binary);                                                      // ifile è un file stream in imput
        char buff[1008];

        std::string msg;                                                                //
        if (iFile.is_open())                                                            // se il file non esiste ifile non lo crea perchè di default può solo ricevere (almeno mi sembra)
        {                                                                               //
            while (iFile)                                                               // mando riga per riga, può essere ottimizzato
            {                                                                           //
                iFile.read(buff + 8, sizeof(buff) - 8);
                int n = iFile.gcount();
                buff[0] = 'L';
                buff[1] = ':';
                buff[2] = ' ';
                buff[3] = n / 1000 + 48;
                buff[4] = n % 1000 / 100 + 48;
                buff[5] = n % 100 / 10 + 48;
                buff[6] = n % 10 + 48;
                buff[7] = '|';

                send(buff, iFile.gcount() + 8, s);                   //
            }                                                                           //
                                                                                        //
        }
        iFile.close();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in send file: " << e.what() << "\n";
        throw;
    }
}


int login(tcp::socket& s, std::string& user, std::string& pw, bool first_time)
{
    char buffer[max_length];
    int logged = 0;
    char c;
    try
    {
        while (!logged)
        {
            if (first_time) //solo al primo login, per la riconnesione il protocollo non richiede inserimento di user e pw in quanto riprova con gli ultimi inseriti
            {
                c = ' ';
                // decido se registrare o fare direttamente il login
                //
                // if registrare
                    //
                    // chiedo nome e pw      <--|
                    // li invio                 |
                    // if risposta positiva     |
                        // esci                 |                        
                    // else --------------------|

                std::cout << "Esegui accesso o crea un nuovo profilo [l/r] ?:";
                std::cin >> c;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); //flush di cin
                if (c != 'l' && c != 'r')                                                           // input errato ricomincia da capo
                {
                    std::cout << std::endl << "l -> login, r -> registrazione";
                    continue;
                }

                std::cout << "Username:";
                std::cin >> user;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); //flush di cin
                std::cout << "Password:";
                std::cin >> pw;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); //flush di cin
                // presi nome e pw

                if (c == 'r')
                {


                    send("RGS: " + user + '|' + pw + '\n', s);
                    size_t reply_length = s.read_some(boost::asio::buffer(buffer, max_length));
                    // in realtà il confronto andrebbe fatto col caso giust, non con l'errore
                    if (buffer[0] == 'E') // errore 
                    {
                        std::cout << std::string(buffer, 3, reply_length - 3);
                        continue;
                    }
                    else
                    {
                        std::cout << "Registrazione avvenuta con successo\n";
                    }
                }
            }

            send("I: " + user + "\n", s);
            size_t reply_length = s.read_some(boost::asio::buffer(buffer, max_length));
            std::unique_lock<std::mutex> lk_out(cout_access);
            std::cout << std::string(buffer, reply_length) << std::endl;
            lk_out.unlock();
            if (buffer[0] == 'R')
            {
                send("P: " + pw + "\n", s);
                reply_length = s.read_some(boost::asio::buffer(buffer, max_length));
                if (buffer[0] == 'R') {
                    lk_out.lock();
                    std::cout << std::string(buffer, 3, reply_length - 3);
                    lk_out.unlock();
                    return 1;   //identificazione avvenuta con successo, utente già esistente (sync necessaria)
                }
                else if (buffer[0] == 'N') {
                    lk_out.lock();
                    std::cout << std::string(buffer, 3, reply_length - 3);
                    lk_out.unlock();
                    return 2;   //identificazione avvenuta con successo, utente nouvo (no sync)
                }
                else if (buffer[0] == 'E')
                {
                    lk_out.lock();
                    std::cout << std::string(buffer, 3, reply_length - 3);
                    lk_out.unlock();
                    continue;  //password sbagliata o errore generico, accesso negato
                }
            }
            else //risposta non conforme al protocollo, riiniziare procedura login
            {
                lk_out.lock();
                std::cout << std::string(buffer, 0, reply_length);
                lk_out.unlock();
                continue;   //errore di comunicazione, probabilmente un bug nel server
            }
        }
    }
    catch (std::exception& e)                                                   //
    {                                                                           //
        std::cerr << "Exception in login: " << e.what() << "\n";
        throw;
    }
}

std::string syncronize(tcp::socket& s)
{
    try
    {

        std::string data("");
        char buffer[max_length];
        send("S: \n", s);

        for (size_t reply_length = max_length;
            reply_length == max_length;
            reply_length = s.read_some(boost::asio::buffer(buffer, max_length)), data.append(std::string(buffer, reply_length)));
        return data;
    }
    catch (std::exception& e)                                                   //
    {                                                                           //
        std::cerr << "Exception in syncronize: " << e.what() << "\n";
        throw;
    }
}


std::string backup_or_sync(tcp::socket& s,std::string user)
{
    using namespace std;
    bool correct = false;
    char c;
    string data;

    try
    {

        while (!correct)
        {
            cout << "effettuare il download del contenuto corrente? [s/n]: ";
            std::cin >> c;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); //flush di cin
            if (c != 's' && c != 'n')                                                           // input errato ricomincia da capo
            {
                std::cout << std::endl << "s -> download, n -> continua senza download";
                continue;
            }
            else
                correct = true;
        }
        if (c == 's')
        {
            bool done = false;
            int n = 0;

            while (!done && n++ < 10)
                done = backup(s,user);
            data = "-";
            if (!done)
                throw std::exception("impossibile ottenere copia di backup dal server");
        }
        else
            data = syncronize(s);
        return data;
    }
    catch (std::exception e)
    {
        throw e;
    }
}

bool backup(tcp::socket& s,std::string user)
{
    std::string reply_str = "";
    std::filesystem::path p;
    bool cont = true,end=false;
    int reply_length;
    char data[max_length];
    std::string file_name;
    std::ofstream Ofile;


    try
    {
        send("B: \n", s);

        while (!end)
        {
            do
            {
                reply_length = s.read_some(boost::asio::buffer(data, max_length));
                reply_str.append(data, reply_length);

            } while (reply_length == max_length && reply_str.length() < 2048);  //leggo fino a 2048 byte dal socket, se ce n'è meno proseguo con solo quelli letti senza aspettare 
            cont = true;

            while (reply_str.length() > 0 && cont)                                   // ogni stringa reply_str può contenere 1 o più messaggi o parti di messaggio, ma il primo byte è sempre anche il primo byte di un messaggio
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
                    file_name = reply_str.substr(3, pos - 3);
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
                    file_name = reply_str.substr(3, pos - 3);
                    p = std::filesystem::path(file_name);
                    std::filesystem::create_directories(p);
                    Ofile.open(file_name, std::ofstream::binary);

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
                    boost::filesystem::last_write_time(file_name, std::stoi(reply_str.substr(3, pos - 3)));
                    if (pos < reply_str.length() - 1)
                        reply_str = std::string(reply_str, pos + 1);
                    else
                        reply_str = "";
                }
                break;
                case 'F':   //fine comando
                {
                    end = true;
                    cont = false;
                }
                break;
                default: // comando non prestabilito
                {
                    std::string str("E: errore in formato messaggio\n");
                    //boost::asio::write(s, boost::asio::buffer(str.c_str(), str.length()));
                    return false; // o "return" tanto probabilmente è causato da un bug 
                }
                }
            }
        }
        return true;
    }
    catch (std::exception e)
    {
        throw e;
    }
}