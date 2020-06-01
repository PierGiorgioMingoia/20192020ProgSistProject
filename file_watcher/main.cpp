// ConsoleApplication1.cpp : Questo file contiene la funzione 'main', in cui inizia e termina l'esecuzione del programma.
//

#include <iostream>
#include "FileWatcher.h"


int main() {
	std::cout << "MIAO" << std::endl;
	// Create a FileWatcher instance that will check the current folder for changes every 5 seconds
	FileWatcher fw{ "./", std::chrono::milliseconds(5000) };

	// Start monitoring a folder for changes and (in case of changes)
	// run a user provided lambda function
	fw.start([](std::string path_to_watch, FileStatus status) -> void {
		// Process only regular files, all other file types are ignored
		if (!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased) {
			return;
		}

		switch (status) {
		case FileStatus::created:
			std::cout << "File created: " << path_to_watch << '\n';
			break;
		case FileStatus::modified:
			std::cout << "File modified: " << path_to_watch << '\n';
			break;
		case FileStatus::erased:
			std::cout << "File erased: " << path_to_watch << '\n';
			break;
		default:
			std::cout << "Error! Unknown file status.\n";
		}
		});
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
