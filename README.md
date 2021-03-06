﻿# 20192020ProgSistProject





## Server

### Files

- **echo_server.cpp** description
    ##### Classes
    - session
    - server
    ##### functions
    - int *main*(int argc, char* argv[])
    
- FileWatcher.h
    ##### Classes
    - FileWatcher
- account.h
    ##### functions
    - std::map<std::string, int> *readAndStoreAccounts*(std::string filePath)
    - int *computeHashPassword*(std::string password)
    - bool *checkNameAndPassword*(std::string name, std::string password, const std::map<std::string, int>& accounts)
    - bool *createNewAccount*(std::string name, std::string password, std::map<std::string, int>& accounts, std::string filePath)
    - bool *registrationOfUser*(std::string userAndPassword, std::map<std::string, int>& accounts, std::string filePath)
    - bool *checkIfAlreadyLoggedIn*(std::string user, std::vector<std::string>& activeAccounts)
    - void *debugPrintAllActiveAccount*(std::vector<std::string> path)
    - void *insertInActiveAccounts*(std::string user, std::vector<std::string>& activeAccounts)
    - void *removeFromActiveAccounts*(std::string user, std::vector<std::string>& activeAccounts) 
- backup.h
    ##### Classes
    - BackUpFile
    ##### functions
    - void *overWriteFileBackup*(BackUpFile backUpPath, std::string filePath)
    - void *deleteBackUpFile*(std::map<std::string, BackUpFile>& backUpFiles, std::string filePath)
- FullBackup.h
    ##### functions
    - void *sendEntireUserFolder*(std::string user, tcp::socket& s);
    - void *sendFile*(std::filesystem::path file, tcp::socket& s);
    - void *sendString*(std::string msg, tcp::socket& s);
    - void *sendMessageOverTheSocket*(const char* msg, int size, tcp::socket& s);
    - void *initialMessageForEachFile*(std::string file, tcp::socket& s);
    - void *lastMessageForEachFile*(std::string file, tcp::socket& s);
    - void *send_directory*(std::filesystem::path file, tcp::socket& s);
- errorMsg.h
- accounts.txt

### Run
    ./echo_server.exe 1234

## Client

### Files

- client_watcher.cpp
    - int *main*(int argc, char* argv[])
- Client.h
    ##### Classes
    - Client
    ##### functions
    - void *receive*(tcp::socket* s);           
    - void *reconnect*(tcp::socket* s, tcp::resolver::results_type* endpoints, std::string* user, std::string* pw);
    - void *send_file*(std::string path, tcp::socket& s);
    - int *login*(tcp::socket& s, std::string& user, std::string& pw, bool first_time = false);
    - std::string *syncronize*(tcp::socket& s);
    - bool *backup*(tcp::socket& s,std::string user);
    - std::string *backup_or_sync*(tcp::socket& s,std::string user);
    - void *send*(std::string msg, tcp::socket& s);
    - void *send*(const char* msg, int size, tcp::socket& s);
- FileWatcher.h
    ##### Classes
    - FileWatcher
### Run
    .\client_watcher.exe 127.0.0.1 1234
