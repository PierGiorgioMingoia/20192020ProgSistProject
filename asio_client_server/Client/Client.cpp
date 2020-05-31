#include <iostream>
#include <boost/asio.hpp>

using namespace boost::asio;
using ip::tcp;
using std::string;
using std::cout;
using std::endl;

int main() {
    boost::asio::io_service io_service;
    //socket creation
    tcp::socket socket(io_service);
    //connection
    socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 1234));
    // request/message from client
    const string msg = "Hello from Client!\n";
    boost::system::error_code error;
    boost::asio::write(socket, boost::asio::buffer(msg), error);
    if (!error) {
        cout << "Client sent hello message!" << endl;
    }
    else {
        cout << "send failed: " << error.message() << endl;
    }
    // getting response from server
    boost::asio::streambuf receive_buffer;
    boost::asio::read(socket, receive_buffer, boost::asio::transfer_all(), error);
    if (error && error != boost::asio::error::eof) {
        cout << "receive failed: " << error.message() << endl;
    }
    else {
        const char* data = boost::asio::buffer_cast<const char*>(receive_buffer.data());
        cout << data << endl;
    }
    return 0;
}
/*#include <iostream>
#include <boost/asio.hpp>

using namespace boost::asio;
using ip::tcp;
using std::string;
using std::cout;
using std::endl;

enum file_type {dir,file};

class system_file
{
    public:
        string name;
        file_type type;

        system_file(const string name, file_type type) :
            name(name), type(type) {};
};

class system_buff_fifo
{
    public:
        int size = 3;
        int lenght = 0;
        system_file* buff[3];

        int push(system_file* syst_file)
        {
            if (lenght == size)
                return 1;
            else
            {
                buff[lenght] = syst_file;
                lenght++;
                return 0;
            }
        }

        system_file* pop()
        {
            system_file* tmp;
            if (lenght == 0)
                return NULL;
            else
            {
                tmp = buff[0];
                lenght--;
                buff[0] = buff[1];
                buff[1] = buff[2];
                return tmp;
            }
        }
};


int main() {
    boost::asio::io_service io_service;
    //socket creation
    tcp::socket socket(io_service);
    //connection
    socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 1234));
    // request/message from client

    system_file *a = new system_file("a", dir);
    system_file *b = new system_file("b", dir);
    system_file *testo = new system_file("testo.txt", file);

    system_buff_fifo buffer;
    buffer.push(a);
    buffer.push(b);
    buffer.push(testo);

    int IDm = 1;
    boost::system::error_code error;

    while (system_file* i = buffer.pop())
    {
        switch (i->type)
        {
        case 0:
        {
            string msg = std::to_string(IDm) + ",1,0," + i->name + '\n';//crea cartella
            boost::asio::write(socket, boost::asio::buffer(msg), error);
            if (!error) {
                cout << msg << endl;
            }
            else {
                cout << "send failed: " << error.message() << endl;
            }
            break;
        }
        case 1:
        {
            string msg = std::to_string(IDm) + ",2,0," + i->name  + '\n';//crea file
            boost::asio::write(socket, boost::asio::buffer(msg), error);
            if (!error) {
                cout << msg << endl;
            }
            else {
                cout << "send failed: " << error.message() << endl;
            }
            break;
        }
        default:
            break;
        }

        
        IDm++;
    }


    // getting response from server
    boost::asio::streambuf receive_buffer;
    boost::asio::read(socket, receive_buffer, boost::asio::transfer_all(), error);
    if (error && error != boost::asio::error::eof) {
        cout << "receive failed: " << error.message() << endl;
    }
    else {
        const char* data = boost::asio::buffer_cast<const char*>(receive_buffer.data());
        cout << data << endl;
    }
    return 0;
}**/