#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define PORT 6667
#define MAX_CLIENTS 10
#define ADMIN_PASSWORD "adminpass"

using namespace std;

vector<int> clients;
map<int, string> client_names;
map<string, vector<int>> channels;
map<string, int> channel_admins;
map<int, string> client_ips;
map<string, set<int>> muted_clients;
set<int> admins;
map<int, string> client_channels;

void broadcast_message(const string &message, const string &channel, int exclude_fd = -1)
{
    for (int client : channels[channel])
    {
        if (client != exclude_fd)
        {
            send(client, message.c_str(), message.size(), 0);
        }
    }
}

void handle_client(int client_socket)
{
    char buffer[1024];
    string nickname = "";
    string current_channel = "";
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    getpeername(client_socket, (sockaddr *)&addr, &addr_len);
    client_ips[client_socket] = inet_ntoa(addr.sin_addr);

    while (true)
    {
        int bytes_received = recv(client_socket, buffer, 1024, 0);
        if (bytes_received <= 0)
        {
            close(client_socket);
            clients.erase(remove(clients.begin(), clients.end(), client_socket), clients.end());
            if (!nickname.empty() && !current_channel.empty())
            {
                broadcast_message(nickname + " has left the chat.\n", current_channel, client_socket);
                channels[current_channel].erase(remove(channels[current_channel].begin(), channels[current_channel].end(), client_socket), channels[current_channel].end());
            }
            break;
        }
        buffer[bytes_received] = '\0';
        string message(buffer);

        if (message[0] == '/')
        {
            istringstream iss(message.substr(1));
            string command;
            iss >> command;

            if (command == "nick")
            {
                iss >> nickname;
                client_names[client_socket] = nickname;
                string confirm_message = "Nickname changed to " + nickname + "\n";
                send(client_socket, confirm_message.c_str(), confirm_message.size(), 0);
            }
            else if (command == "join")
            {
                if (nickname.empty())
                {
                    send(client_socket, "You must set a nickname before joining a channel.\n", 51, 0);
                }
                else
                {
                    string channel;
                    iss >> channel;
                    if (channels.find(channel) != channels.end())
                    {
                        // Leave current channel if already in one
                        if (!current_channel.empty())
                        {
                            channels[current_channel].erase(remove(channels[current_channel].begin(), channels[current_channel].end(), client_socket), channels[current_channel].end());
                            broadcast_message(nickname + " has left " + current_channel + "\n", current_channel, client_socket);
                        }

                        current_channel = channel;
                        client_channels[client_socket] = channel;
                        channels[channel].push_back(client_socket);
                        string confirm_message = "You joined channel " + channel + "\n";
                        send(client_socket, confirm_message.c_str(), confirm_message.size(), 0);
                        broadcast_message(nickname + " has joined " + channel + "\n", channel, client_socket);
                    }
                    else
                    {
                        send(client_socket, "Channel does not exist. Please ask an admin to create it.\n", 58, 0);
                    }
                }
            }
            else if (command == "mute")
            {
                if (admins.find(client_socket) != admins.end())
                {
                    string user_to_mute;
                    iss >> user_to_mute;
                    int user_socket = -1;
                    for (const auto &pair : client_names)
                    {
                        if (pair.second == user_to_mute)
                        {
                            user_socket = pair.first;
                            break;
                        }
                    }
                    if (user_socket != -1)
                    {
                        muted_clients[current_channel].insert(user_socket);
                        string confirm_message = "User " + user_to_mute + " has been muted\n";
                        send(client_socket, confirm_message.c_str(), confirm_message.size(), 0);
                    }
                }
                else
                {
                    send(client_socket, "Only admins can mute users.\n", 28, 0);
                }
            }
            else if (command == "unmute")
            {
                if (admins.find(client_socket) != admins.end())
                {
                    string user_to_unmute;
                    iss >> user_to_unmute;
                    int user_socket = -1;
                    for (const auto &pair : client_names)
                    {
                        if (pair.second == user_to_unmute)
                        {
                            user_socket = pair.first;
                            break;
                        }
                    }
                    if (user_socket != -1)
                    {
                        muted_clients[current_channel].erase(user_socket);
                        string confirm_message = "User " + user_to_unmute + " has been unmuted\n";
                        send(client_socket, confirm_message.c_str(), confirm_message.size(), 0);
                    }
                }
                else
                {
                    send(client_socket, "Only admins can unmute users.\n", 30, 0);
                }
            }
            else if (command == "kick")
            {
                if (admins.find(client_socket) != admins.end())
                {
                    string user_to_kick;
                    iss >> user_to_kick;
                    int user_socket = -1;
                    for (const auto &pair : client_names)
                    {
                        if (pair.second == user_to_kick)
                        {
                            user_socket = pair.first;
                            break;
                        }
                    }
                    if (user_socket != -1)
                    {
                        string kick_message = "You have been kicked from the channel by an admin.\n";
                        send(user_socket, kick_message.c_str(), kick_message.size(), 0);
                        channels[current_channel].erase(remove(channels[current_channel].begin(), channels[current_channel].end(), user_socket), channels[current_channel].end());
                        broadcast_message(user_to_kick + " has been kicked from the channel by an admin.\n", current_channel, client_socket);
                        close(user_socket);
                    }
                }
                else
                {
                    send(client_socket, "Only admins can kick users.\n", 28, 0);
                }
            }
            else if (command == "login")
            {
                string password;
                iss >> password;
                if (password == ADMIN_PASSWORD)
                {
                    admins.insert(client_socket);
                    send(client_socket, "You are now an admin.\n", 22, 0);
                }
                else
                {
                    send(client_socket, "Invalid admin password.\n", 24, 0);
                }
            }
            else if (command == "create")
            {
                if (admins.find(client_socket) != admins.end())
                {
                    string channel;
                    iss >> channel;
                    channels[channel] = vector<int>();
                    channel_admins[channel] = client_socket;
                    string confirm_message = "Channel " + channel + " was created by " + nickname + "\n";
                    send(client_socket, confirm_message.c_str(), confirm_message.size(), 0);
                    broadcast_message("Channel " + channel + " was created by " + nickname + "\n", channel, client_socket);
                }
                else
                {
                    send(client_socket, "Only admins can create channels.\n", 33, 0);
                }
            }
            else if (command == "list")
            {
                stringstream response;
                response << "You are currently in channel: " << current_channel << "\n";
                response << "Available channels:\n";
                for (const auto &channel : channels)
                {
                    response << "- " << channel.first << ": ";
                    for (int user_socket : channel.second)
                    {
                        response << client_names[user_socket] << " ";
                    }
                    response << "\n";
                }
                send(client_socket, response.str().c_str(), response.str().size(), 0);
            }
        }
        else
        {
            if (nickname.empty())
            {
                send(client_socket, "You must set a nickname before sending messages.\n", 49, 0);
            }
            else if (current_channel.empty())
            {
                send(client_socket, "You must join a channel before sending messages.\n", 50, 0);
            }
            else
            {
                if (muted_clients[current_channel].find(client_socket) != muted_clients[current_channel].end())
                {
                    send(client_socket, "You are muted and cannot send messages in this channel.\n", 54, 0);
                }
                else
                {
                    broadcast_message(nickname + ": " + message + "\n", current_channel, client_socket);
                }
            }
        }
    }
}

int main()
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        cerr << "Socket creation failed\n";
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        cerr << "Bind failed: " << strerror(errno) << "\n";
        return 1;
    }

    if (listen(server_socket, MAX_CLIENTS) < 0)
    {
        cerr << "Listen failed\n";
        return 1;
    }

    cout << "Server started on port " << PORT << endl;

    while (true)
    {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0)
        {
            cerr << "Client accept failed\n";
            continue;
        }

        clients.push_back(client_socket);
        cout << "New client connected\n";
        thread(handle_client, client_socket).detach();
    }

    close(server_socket);
    return 0;
}
