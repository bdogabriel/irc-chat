#include <iostream>
#include <thread>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 6667

void receive_messages(int client_socket)
{
    char buffer[1024];
    while (true)
    {
        int bytes_received = recv(client_socket, buffer, 1024, 0);
        if (bytes_received <= 0)
        {
            std::cerr << "Connection closed by server\n";
            close(client_socket);
            exit(1);
        }
        buffer[bytes_received] = '\0';
        std::cout << "Server: " << buffer << std::endl;
    }
}

int main()
{
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "Connection to server failed\n";
        return 1;
    }

    std::cout << "Connected to server\n";

    std::thread(receive_messages, client_socket).detach();

    char message[1024];
    while (true)
    {
        std::cin.getline(message, 1024);
        send(client_socket, message, strlen(message), 0);
    }

    close(client_socket);
    return 0;
}
