#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 6667
#define MAX_CLIENTS 10

std::vector<int> clients;

void handle_client(int client_socket)
{
    char buffer[1024];
    while (true)
    {
        int bytes_received = recv(client_socket, buffer, 1024, 0);
        if (bytes_received <= 0)
        {
            close(client_socket);
            clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
            std::cout << "Client disconnected\n";
            break;
        }
        buffer[bytes_received] = '\0';
        std::cout << "Received: " << buffer << std::endl;
        for (int client : clients)
        {
            if (client != client_socket)
            {
                send(client, buffer, bytes_received, 0);
            }
        }
    }
}

int main()
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "Bind failed\n";
        return 1;
    }

    if (listen(server_socket, MAX_CLIENTS) < 0)
    {
        std::cerr << "Listen failed\n";
        return 1;
    }

    std::cout << "Server started on port " << PORT << std::endl;

    while (true)
    {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0)
        {
            std::cerr << "Client accept failed\n";
            continue;
        }

        clients.push_back(client_socket);
        std::cout << "New client connected\n";
        std::thread(handle_client, client_socket).detach();
    }

    close(server_socket);
    return 0;
}
