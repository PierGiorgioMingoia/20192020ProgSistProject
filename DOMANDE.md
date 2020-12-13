# Elenco Possibili domande


## Server
- Gestione utenti
Creazione di una cartella per ciascun utente, solamente accedervi se il login viene fatto correttamente, dopo aver caricato il primo file viene creata
- Come vengono gestite la connessione al server, multithread... perchè questa scelta ?
 risposta: thread per ogni connessione o thread per diverse cose, thread per ogni connessione, per nostro servizio, non perdita de effcienza e relativa semplictità del codice. 
 Thread sempre in attesa sulla porta, quando riceve connessione genrea una classe session, all'interno da un nuovo thread. Primo thread resta in attesa sulla porta, e gli altri vengono generati ogni client. Protocollo Rai con contrazione dello Stack ed eliminazione dell'oggetto session e del suo thread. Lista dei puntatori dei thread creati. Sempre thread nuovi. 
- Come funziona la classe session?
Metodi socket() e start(), proprietà socket, buffer, utente. Metodo start(), try/catch, while(!login) se errore login viene rimandato al client ad un messaggio e resta in attesa di un login, tutto ciò che serve a login/registrazione,  /while, che resta in attesa di tutti i messaggi del client della sessione.
- Descrizione del protocollo
RGS->Registrazione, I->Login, P->password, C->creazione del file (file,dataUltimaModifica), D-> creazione cartella vuota, M ->modifica (file), L->numero di byte messaggio L che va ad aggiungere ad un file(bytes), X -> elminare file(file), S -> richiesta sincronizzazzione (server chiede client), B -> richiesta backup (client chiede server), U->manda (data ultima modifca e chiusura file),P -> probe per verificare il checksum, F -> termine comando (file) controllo.
Semplicità e comprensibile a noi nel momendo della progettazione.
- Gestione della Disconnessione
viene lanciata la catch della start(), chiudo file aperti, eseguo backup, rimuovo dagli account attivi.
- Funzionamento FileWatcher
Fa scansione nella sincronizzazzione, oggetto FileWatcher per la scansione e struttura dati e non serve più, client si occupa di trovare la differenza.
- Checksum + dataUltimaModifica
Data ultima modifica per distinguere se un file è stato modificato rispetto a quella salvata, ma per ulteriore controllo, checksum solo sulla prima scansione. Ogni modifica client viene riportata sul server immediatamente.
- Autenticazione e gestione account loggati
file contentete credenziali, mappa dove li conserviamo, lista account attivi, computeHashPassword() per non salvare in plain la password, funziona su tutti computer. Semirandom. Non fondamentale utilizzo database, semplicità 
- Backup dati
Fatto durante una modifica richiesta dal client, server si salva una copia del file e lo elimina se tutto va a termine altrimenti rimuove il fiale semi-modificato riptritstinado il bakcup.
- Download intera cartella su client
Mandare intero contenuto della cartella dell'user presente sul server
- Librerie utilizzate e perchè?
Boost grande numero funzionalità ad esempio socket, portabitlità e gestione file(come data ultima modifica)

## Client 
- Selezione server backup
C'è la possibilità a seconda dei argomenti passati la possibilità di selezionare un server di backup, indirizzo e porta, sfruttando i vari costruttori.
- Connessione al servizio
run(), boost per connettersi e poi ricconnettersi
- Utilizzo dei thread (listener, reconnecter)
thread recconnect sempre in attesa, Condition Variable, blocca tutti e sveglia reconnect, 3 thread principale FileWatcher, recconnecter, listener(ricezione messaggi), Thread main scrive sul buffer, listener rimuove dal buffer, first reporter è il primo thread che capisce che c'è stato un problema.
- Utilizzo lock
 Flag_acces, buffer_acces(solo tot messaggi al server prima di fermarmi altrimenti non mando niente ), mutex per accesso di scrittura sul prompt, avendo tre thread per scrivere sul cmd.
- Sincronizzazione con il Server
Decide se scaricare o confrontarsi e inviare le modifiche, contattando il server con S di mandare l'immagine della struttura che possiede, la confronta con la sua e decide cosa mandare al server, data_ultima e check non faniente per il resto manda le cose per modificare i file e renderli sincronizzati, dentro funzione login, in base alle informazioni ricevute dal srever sapendo se ho una cartella, mi viene chiesto se voglio fare backup or sync.
- Protocollo
Stesso del Server, riceve solamente errori e conferme, Se manda al server la richiesta di sync resta in attesa della stringa della mappa del filesystem del server. Comandi F rimuovo dal buffer, invece se ricevo E: stampo l'errore. Riavvio errore.
- Invio informazioni
formato binario, boost permette di metter opzione binary (per i file, non txt per esempio, è molto probabile che cisiano degli errori e interpretati con me fine stringa quindi optato per il binario)
- Ricezione informazioni
formato binario
- FileWatcher
data_ultima modifica e anche checksum uguale a quello del server, e decide le varie operazioni da eseguire com C, D, M, X.
- Riconnessione
Se disconnesso e mi riconnetto mando i messaggi che mie erano rimasti nel buffer