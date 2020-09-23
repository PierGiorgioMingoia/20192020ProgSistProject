/* client watcher: si collega a un server per mantenere aggiornato il contenuto di una cartella su entrambe le macchine*/

#include <iostream>
#include "Client.h"



                                                                                    //
                                                                                    //
int main(int argc, char* argv[]) {                                                  // main
                                                                                    //
    Client* c = nullptr;
    try                                                                             //
    {    
        if (argc == 1)
            c = new Client();
        else if (argc == 3)                                                              //controllo sugli argomenti                                                                        //
            c = new Client(argv[1], argv[2]); 
        else
        {
            std::cout << "wrong number of input arguments " << std::endl << "correct usage: [client.exe <ip_addr> <port>] or [client.exe]" << std::endl;
            return 1;
        }

        c->run();
    }                                                                               //
    catch (std::exception& e)                                                       //
    {                                                                               //
        std::cerr << "Exception general: " << e.what() << "\n";                     //
        if (c != nullptr)
            delete c;
    }

}


