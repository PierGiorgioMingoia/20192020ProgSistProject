/* client watcher: si collega a un server per mantenere aggiornato il contenuto di una cartella su entrambe le macchine*/

#include <iostream>
#include "Client.h"



                                                                                    //
                                                                                    //
int main(int argc, char* argv[]) {                                                  // main
                                                                                    //
    try                                                                             //
    {                                                                               //
        if (argc !=3)                                                              //controllo sugli argomenti
        {                                                                           //
            std::cerr << "Usage: client_watcher <host> <port>\n";         //
            return 1;                                                               //
        }                                                                           //

        Client c(argv[1], argv[2]);
        c.run();
    }                                                                               //
    catch (std::exception& e)                                                       //
    {                                                                               //
        std::cerr << "Exception general: " << e.what() << "\n";                     //
    }

}


