#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <stdbool.h>
#include <limits.h>
#include <bits/stdc++.h>
#include <pthread.h>

#define SERVERPORT 8989
#define BUFSIZE 8000
#define SOCKETERROR (-1)
#define SERVER_BACKLOG 1 // server backlog - no. of client requests I am going to take at a time

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

using namespace std;

void *handle_connection(void *client_socket);
void check(int exp, const char *msg);
vector<string> tokenize(string command, char c);

// data structures being used
unordered_map<string, string> users;                                    // userid - password
unordered_map<string, string> userIpPort;                               // userid - IP,port
unordered_map<string, bool> loggedin;                                   // userid - true/false
unordered_map<string, vector<string>> group_members;                    // groupid - (user id's of the members)
unordered_map<string, vector<string>> group_requests;                   // groupid - (user id's of people who made requests)
unordered_map<string, vector<pair<string, string>>> group_files;        // groupid - (filename , uska SHA)
unordered_map<string, unordered_map<string, vector<string>>> file_info; // groupid - (filename , (user id's which have this file))

vector<string> tokenize(string command, char c)
{
    vector<string> v;
    string temp = "";
    for (auto a : command)
    {
        if (a == c)
        {
            v.push_back(temp);
            temp = "";
        }
        else
            temp += a;
    }
    v.push_back(temp);
    return v;
}

string getFileName(string path)
{
    string file_name;
    size_t found;
    found = path.find_last_of('/');
    if (found == string::npos)
        file_name = path;
    else
        file_name = path.substr(found + 1);
    return file_name;
}

void extract_port(vector<string> &tracker_info, string path)
{
    fstream file;
    file.open(path.c_str(), ios::in);
    string t;
    while (getline(file, t))
    {
        tracker_info.push_back(t);
    }
    file.close();
}

int main(int argc, const char *argv[])
{
    int server_socket, client_socket, addr_size;
    SA_IN server_addr, client_addr;

    check((server_socket = socket(AF_INET, SOCK_STREAM, 0)),
          "Failed to create socket");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;

    vector<string> tracker_info;
    string tracker_info_path = argv[1];
    extract_port(tracker_info, tracker_info_path);

    string first_tracker = tracker_info[0];
    int pos = first_tracker.find(":");
    string x = first_tracker.substr(pos + 1);
    int port_no = atoi(x.data());
    server_addr.sin_port = htons(port_no);

    check((bind(server_socket, (SA *)&server_addr, sizeof(server_addr))),
          "Bind failed!");
    check((listen(server_socket, SERVER_BACKLOG)),
          "Listen failed!");

    while (true)
    {
        cout << "Waiting for connections..." << endl;

        addr_size = sizeof(SA_IN);
        check((client_socket = accept(server_socket, (SA *)&client_addr, (socklen_t *)&addr_size)),
              "Accept connection failed!");

        cout << "Connected!" << endl;
        pthread_t t;
        pthread_create(&t, NULL, handle_connection, &client_socket);
    }

    close(server_socket);
    return 0;
}

bool validateCommand(string first, vector<string> command)
{

    if (first == "download_file" || first == "login" || first == "upload_file")
        return (command.size() == 4);
    else if (first == "create_user" || first == "accept_request" || first == "stop_share")
        return (command.size() == 3);
    else if (first == "create_group" || first == "join_group" || first == "leave_group" || first == "list_requests" || first == "list_files")
        return (command.size() == 2);
    else if (first == "list_groups" || first == "logout" || first == "show_downloads")
        return (command.size() == 1);
    else
        return 0;
}

void *handle_connection(void *p_client_socket)
{
    int client_socket = *((int *)p_client_socket);
    char buffer[BUFSIZE];
    string user_id = "";

    string server_response;
    char client_message[2000];

    while (1)
    {
        bzero(client_message, 2000);
        if (recv(client_socket, client_message, 2000, 0) <= 0)
            break;

        vector<string> command = tokenize(client_message, ' ');

        if (command[0] == "downloaded")
        {
            file_info[command[1]][command[2]].push_back(user_id);
            server_response = "Okay! Acknowledged that now you can also share";
            send(client_socket, server_response.data(), 2000, 0);
            continue;
        }
        cout << "Client: ";
        cout << client_message << endl;
        if (strcmp(client_message, "quit") == 0)
            server_response = "Okay byeee!";
        else
        {

            if (!validateCommand(command[0], command))
            {
                server_response = "Enter valid command";
                send(client_socket, server_response.data(), 2000, 0);
                continue;
            }

            bool flag = 0;
            if (command[0] != "login" && command[0] != "create_user")
            {
                if (user_id.size() == 0 || !loggedin[user_id])
                {
                    server_response = "Please login first";
                    send(client_socket, server_response.data(), 2000, 0);
                    flag = 1;
                }
            }
            if (flag)
                continue;
            if (command[0] == "create_user")
            {
                if (users.count(command[1]))
                    server_response = "User already exists!";
                else
                {
                    users[command[1]] = command[2];
                    server_response = "User created!";
                }
            }
            else if (command[0] == "login")
            {
                if (!users.count(command[1]))
                    server_response = "User does not exit";
                else
                {
                    if (loggedin[user_id])
                        server_response = "You are already logged in!";
                    else
                    {
                        server_response = "Logged in successfully";
                        user_id = command[1];
                        loggedin[user_id] = 1;
                        userIpPort[user_id] = command[command.size() - 1];
                    }
                }
            }
            else if (command[0] == "create_group")
            {
                if (group_members.find(command[1]) != group_members.end())
                    server_response = "Group already created!";
                else
                {
                    group_members[command[1]].push_back(user_id);
                    server_response = "Group created successfully";
                }
            }
            else if (command[0] == "join_group")
            {
                if (!group_members.count(command[1]))
                    server_response = "Group does not exist";
                else
                {
                    auto a = group_members[command[1]];
                    if (find(a.begin(), a.end(), user_id) != a.end())
                        server_response = "You are already present in the group!";
                    else
                    {
                        vector<string> curr_rqsts = group_requests[command[1]];
                        auto it = find(curr_rqsts.begin(), curr_rqsts.end(), user_id);
                        if (it != curr_rqsts.end())
                            server_response = "You have made a join request";
                        else
                        {
                            group_requests[command[1]].push_back(user_id);
                            server_response = "Join request made successfully";
                        }
                    }
                }
            }
            else if (command[0] == "leave_group")
            {
                string group_id = command[1];
                if (!group_members.count(group_id))
                    server_response = "Group does not exist";
                else
                {
                    auto a = group_members[group_id];
                    auto it = find(a.begin(), a.end(), user_id);
                    if (it == a.end())
                        server_response = "You are not present in this group";
                    else
                    {
                        if (group_members[group_id].size() == 1)
                        {
                            group_members.erase(group_id);
                            group_requests.erase(group_id);
                            group_files.erase(group_id);
                            file_info.erase(group_id);
                            server_response = "Group deleted as you were the only person in the group";
                        }
                        else
                        {
                            vector<string> temp;
                            for (auto a : group_members[group_id])
                                if (a != user_id)
                                    temp.push_back(a);

                            group_members[group_id] = temp;
                            auto file_owners = file_info[group_id];
                            for (auto a : file_owners)
                            {
                                vector<string> temp;
                                for (auto b : a.second) // user_id's which have the current file
                                    if (b != user_id)
                                        temp.push_back(b);
                                file_info[group_id][a.first] = temp;
                            }
                            server_response = "You left the group!";
                        }
                    }
                }
            }
            else if (command[0] == "list_requests")
            {
                if (!group_members.count(command[1]))
                    server_response = "Group does not exist";
                else
                {
                    auto a = group_members[command[1]];
                    if (a[0] != user_id)
                        server_response = "You are not the admin of this group";
                    else
                    {
                        if (!group_requests.count(command[1]) || group_requests[command[1]].size() == 0)
                            server_response = "No join requests at this time";
                        else
                        {
                            server_response = "";
                            for (auto a : group_requests[command[1]])
                                server_response += (a + " ");
                        }
                    }
                }
            }
            else if (command[0] == "accept_request")
            {
                if (!group_members.count(command[1]))
                    server_response = "Group does not exist";
                else
                {
                    auto a = group_members[command[1]];
                    if (a[0] != user_id)
                        server_response = "You are not the admin of this group";
                    else
                    {
                        if (!group_requests.count(command[1]))
                            server_response = "No pending join requests for this group";
                        else
                        {
                            auto it = find(group_requests[command[1]].begin(), group_requests[command[1]].end(), command[2]);
                            if (it == group_requests[command[1]].end())
                                server_response = "This user_id has not requested to join the group.";
                            else
                            {
                                group_members[command[1]].push_back(command[2]);
                                group_requests[command[1]].erase(it);
                                server_response = "User accepted";
                            }
                        }
                    }
                }
            }
            else if (command[0] == "list_groups")
            {
                if (group_members.size() == 0)
                    server_response = "No groups at this time";
                else
                {
                    server_response = "";
                    for (auto a : group_members)
                        server_response += (a.first + " ");
                }
            }
            else if (command[0] == "list_files")
            {
                if (!group_members.count(command[1]))
                    server_response = "Group does not exist";
                else
                {
                    auto a = group_members[command[1]];
                    auto it = find(a.begin(), a.end(), user_id);
                    if (it == a.end())
                        server_response = "You are not present in this group";
                    else
                    {
                        server_response = "";
                        if (!group_files.count(command[1]) || group_files[command[1]].size() == 0)
                            server_response = "No files in the group at this time";
                        else
                            for (auto a : group_files[command[1]])
                                server_response += (a.first + " ");
                    }
                }
            }
            else if (command[0] == "upload_file")
            {
                string group_id = command[2];

                if (!group_members.count(group_id))
                    server_response = "Group does not exist";
                else
                {
                    auto a = group_members[group_id];
                    auto it = find(a.begin(), a.end(), user_id);
                    if (it == a.end())
                        server_response = "You are not present in this group";
                    else
                    {
                        string file_name = getFileName(command[1]);
                        string sha = command[3];
                        group_files[group_id].push_back(make_pair(file_name, sha));
                        file_info[group_id][file_name].push_back(user_id);
                        server_response = "File uploaded successfully";
                    }
                }
            }
            else if (command[0] == "download_file")
            {
                string group_id = command[1], file_name = command[2];
                if (!group_members.count(group_id))
                    server_response = "Group does not exist";
                else
                {
                    auto a = group_members[group_id];
                    auto it = find(a.begin(), a.end(), user_id);
                    if (it == a.end())
                        server_response = "You are not present in this group";
                    else
                    {
                        bool flag = 0;
                        for (auto a : group_files[group_id])
                            if (a.first == file_name)
                                flag = 1;
                        if (!flag)
                            server_response = "This file does not exist in the group";
                        else
                        {
                            server_response = "";
                            vector<string> v = file_info[group_id][file_name];
                            for (auto u_id : v)
                            {
                                if (loggedin[u_id]) // give the port of only logged in users
                                    server_response += (userIpPort[u_id] + " ");
                            }
                        }
                    }
                }
            }
            else if (command[0] == "logout")
            {
                loggedin[user_id] = 0;
                userIpPort.erase(user_id);
                user_id = "";
                server_response = "Logged out successfully";
            }
            else if (command[0] == "show_downloads")
            {
                server_response = "Showing downloads:";
            }
            else // command[0] = stop_share
            {
                string group_id = command[1], file_name = command[2];
                if (!group_members.count(group_id))
                    server_response = "Group does not exist";
                else
                {
                    vector<pair<string, string>> v = group_files[group_id]; // filenames and their SHA
                    vector<pair<string, string>> temp;

                    for (auto a : v)
                        if (a.first != file_name)
                            temp.push_back(a);

                    if (temp == v)
                        server_response = "This file does not exist in the group";
                    else
                    {
                        vector<string> v = file_info[group_id][file_name];
                        vector<string> temp1;
                        for (auto a : v)
                            if (a != user_id)
                                temp1.push_back(a);

                        if (temp1 == v)
                            server_response = "You don't have this file";
                        else
                        {
                            if (v.size() == 1)
                            {
                                if (temp.size() == 1) // only one file was there in the group
                                    group_files.erase(group_id);
                                else
                                    group_files[group_id] = temp; // removing the file from the group_files if only one user has it
                                file_info[group_id].erase(file_name);
                            }
                            else
                                file_info[group_id][file_name] = temp1;
                            server_response = "Stopped sharing this file";
                        }
                    }
                }
            }
        }
        send(client_socket, server_response.data(), 2000, 0);
    }
    close(client_socket);
    return NULL;
}

void check(int exp, const char *msg)
{
    if (exp == SOCKETERROR)
    {
        perror(msg);
        exit(1);
    }
}
