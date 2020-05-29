#include <iostream>
#include <string>
#include <ws2tcpip.h>

using  namespace std;
// Need to link with Ws2_32.lib
int inet_pton(int af, const char *src, void *dst)
{
    struct sockaddr_storage ss;
    int size = sizeof(ss);
    char src_copy[INET6_ADDRSTRLEN+1];

    ZeroMemory(&ss, sizeof(ss));
    /* stupid non-const API */
    strncpy (src_copy, src, INET6_ADDRSTRLEN+1);
    src_copy[INET6_ADDRSTRLEN] = 0;

    if (WSAStringToAddress(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
        switch(af) {
            case AF_INET:
                *(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
                return 1;
            case AF_INET6:
                *(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
                return 1;
        }
    }
    return 0;
}

int main() {
    std::cout << "Hello, World!" << std::endl;

    string ipAddress = "127.0.0.1"; //IP address of the server
    int port = 54000;  //Listening port of the server

    //Inizialize winSock
    WSAData data ;
    WORD ver = MAKEWORD(2,2);
    int wsResult = WSAStartup(ver,&data);
    if(wsResult != 0){
        cerr<<"Can't start Winsock, Err#" <<wsResult<<endl;
        return 1;
    }
    //create Socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == INVALID_SOCKET){
        cerr<<"Can't create socket, Err#"<<WSAGetLastError()<<endl;
        WSACleanup();
        return 1;
    }
    //fill in
    sockaddr_in hint; //API version
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

    //Connect to the server
    int connResult = connect(sock,(sockaddr*)&hint, sizeof(hint));
    if(connResult == SOCKET_ERROR){
        cerr<<"Can't connect to server, Err #"<<WSAGetLastError()<<endl;
        closesocket(sock);
        WSACleanup();
        return  1;
    }
    //Do-While
    char buf[4096];
    string userInput;
    do{
        //prompt the user some text
        cout<<"> ";
        getline(cin, userInput);
        if(userInput.size()>0){


            //send the text
            int sendResult = send(sock, userInput.c_str(), userInput.size()+1 ,0);
            if(sendResult != SOCKET_ERROR){
                //wait for response
                ZeroMemory(buf,4096);
                int bytesRecevied = recv(sock, buf, 4096, 0);
                if(bytesRecevied > 0){
                    //Echo response
                    cout<<"Server> "<< string(buf,0,bytesRecevied)<<endl;
                }
            }


        }
    }while(userInput.size()>0);

    // Gracefully close down everything
    closesocket(sock);
    WSACleanup();

    return 0;
}
