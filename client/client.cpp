#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <bits/stdc++.h>
#include <fcntl.h>
#include <openssl/sha.h>

using namespace std;

#define SERVERPORT 8989
#define BUFSIZE 4096
#define SOCKETERROR (-1)
#define SERVER_BACKLOG 100

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

void *handle_connection(void *client_socket);
void *handle_peer_connection(void *client_listen_port);
void check(int exp, const char *msg);

unordered_map<string, string> file_path; // filename - it's path

vector<string> tokenize(string command)
{
    vector<string> v;
    string temp = "";
    for (auto a : command)
    {
        if (a == ' ')
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

string calcSHA(string file_buffer_string)
{
    string hash;
    unsigned char md[20];
    if (!SHA1(reinterpret_cast<const unsigned char *>(&file_buffer_string[0]), file_buffer_string.length(), md))
    {
        printf("Error in hashing\n");
    }
    else
    {
        for (int i = 0; i < 20; i++)
        {
            char buf[3];
            sprintf(buf, "%02x", md[i] & 0xff);
            hash += string(buf);
        }
    }

    string sha;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
        sha += hash[i];

    return sha;
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

void send_file(int client_sd, string f_path)
{

    int fd_from = open(f_path.c_str(), O_RDONLY);

    char buffer[4096];
    bzero(buffer, BUFSIZE);

    while (1)
    {
        int n = read(fd_from, buffer, BUFSIZE);
        if (n <= 0)
            break;
        int r = send(client_sd, buffer, n, 0);
        bzero(buffer, BUFSIZE);

        if (r <= 0)
            break;
    }

    // cout << "File sent successfully" << endl;
    // cout << "Peer: ";
}

bool receive_file(string fil_name, string d_path, int client_socket)
{
    string f_path = d_path + "/" + fil_name;
    int fd_to = open(f_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd_to < 0)
    {
        cout << "Error in opening file" << endl;
        return false;
    }
    char buffer[BUFSIZE];
    bzero(buffer, BUFSIZE);
    cout << "Downloading file..." << endl;
    int total_bytes = 0;

    while (1)
    {
        int n = recv(client_socket, buffer, BUFSIZE, 0);
        if (n < BUFSIZE)
        {
            if (n <= 0)
                break;
            else
            {
                write(fd_to, buffer, n);
                total_bytes += n;
                break;
            }
        }
        total_bytes += n;

        int r = write(fd_to, buffer, n);
        if (r < 0)
        {
            cout << "Error in writing file \n\n";
            return false;
        }
        bzero(buffer, BUFSIZE);
    }
    cout << "Downloaded file successfully! Total bytes = " << total_bytes << "\n\n";
    return true;
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

unordered_map<int, vector<string>> chunk_details; // chunk number - user id's which have the chunk
string piece_select(int chunk_number)
{
    string u_id;
    sort(chunk_details.begin(), chunk_details.end());
    for (auto a : chunk_details)
    {
        if (a.first == chunk_number)
        {
            int k = a.second.size();
            u_id = a.second[k / 2];
        }
    }
    return u_id;
}

int main(int argc, const char *argv[])
{

    int client_socket;
    check(client_socket = socket(AF_INET, SOCK_STREAM, 0),
          "Failed to create socket");

    string client_ip_port = argv[1];
    int pos = client_ip_port.find(":");
    string client_port = client_ip_port.substr(pos + 1);
    int listen_port = atoi(client_port.data());

    pthread_t t;
    pthread_create(&t, NULL, handle_peer_connection, &listen_port);

    vector<string> tracker_info;
    string tracker_info_path = argv[2];
    extract_port(tracker_info, tracker_info_path);

    string first_tracker = tracker_info[0];
    int pos1 = first_tracker.find(":");
    string server_port = first_tracker.substr(pos1 + 1);
    int port_no = atoi(server_port.data());

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_no);
    server_address.sin_addr.s_addr = INADDR_ANY;

    int connection_status = connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    if (connection_status == -1)
    {
        printf("There was an error making a connection to the remote socket \n\n");
        exit(0);
    }

    char server_response[2000];
    string client_message;
    unordered_map<string, vector<string>> download_info; // groupid , (file's which have been downloaded from this group)

    while (1)
    {
        cout << "Peer: ";
        getline(cin, client_message);
        vector<string> client_command = tokenize(client_message);
        bool flag = 1;

        if (client_command[0] == "login")
            client_message += (" " + client_port);
        else if (client_command[0] == "upload_file" && client_command.size() == 3)
        {
            string file_name = getFileName(client_command[1]);
            client_message = "";
            client_message = client_command[0] + " " + file_name + " " + client_command[2];
            client_message += (" " + calcSHA(client_command[1]));
            if (calcSHA(client_command[1]) == "-1")
                flag = 0;
            else
                file_path[file_name] = client_command[1];
        }

        if (!flag)
        {
            cout << "Invalid file path. \n\n";
            continue;
        }

        send(client_socket, client_message.data(), 2000, 0);
        bzero(server_response, 2000);
        recv(client_socket, server_response, 2000, 0);

        if (client_command[0] == "download_file" &&
            (string(server_response) != "Enter valid command") &&
            (string(server_response) != "Please login first") &&
            (string(server_response) != "Group does not exist") &&
            (string(server_response) != "You are not present in this group") &&
            (string(server_response) != "This file does not exist in the group"))
        {
            if (!strlen(server_response))
                cout << "No clients available to share the file. \n\n";
            else
            {
                vector<string> ports_to_connect = tokenize(server_response);
                if (ports_to_connect[0] != "This" || ports_to_connect[0] != "Group")
                {
                    for (auto a : ports_to_connect)
                    {
                        // cout << "Connecting to: " << a << endl;
                        int client_server_socket;
                        client_server_socket = socket(AF_INET, SOCK_STREAM, 0);

                        int port_no = atoi(a.data());

                        struct sockaddr_in server_address;
                        server_address.sin_family = AF_INET;
                        server_address.sin_port = htons(port_no);
                        server_address.sin_addr.s_addr = INADDR_ANY;

                        bool flag1 = 0;

                        int connected_peer_server = connect(client_server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
                        if (connected_peer_server != -1)
                        {
                            send(client_server_socket, client_command[2].data(), 2000, 0); // send client_command[2] i.e. file name
                            if (receive_file(client_command[2], client_command[3], client_server_socket))
                            {
                                download_info[client_command[1]].push_back(client_command[2]);
                                file_path[client_command[2]] = client_command[3] + "/" + client_command[2];
                                string success = "downloaded " + client_command[1] + " " + client_command[2];
                                send(client_socket, success.data(), 2000, 0);
                                bzero(server_response, 2000);
                                recv(client_socket, server_response, 2000, 0);
                                // cout << server_response << "\n\n";
                            }
                            else
                                cout << "There was some error in downloading file \n\n";
                            flag1 = 1;
                            close(client_server_socket);
                        }
                        if (flag1)
                            break;
                    }
                }
                else
                    cout << server_response << "\n\n";
            }
        }
        else if (client_command[0] == "show_downloads" && (string(server_response) != "Please login first"))
        {
            if (!download_info.size())
                cout << "No download info currently! \n\n";
            else
            {
                for (auto a : download_info)
                {
                    cout << "[D] " << a.first << endl;
                    cout << "[C] " << a.first;
                    for (auto b : a.second)
                        cout << " " << b;
                    cout << "\n\n";
                }
            }
        }
        else
            cout << server_response << "\n\n";
        if (strcmp(client_message.data(), "quit") == 0)
            break;
    }

    close(client_socket);

    return 0;
}

void *handle_peer_connection(void *client_listen_port)
{
    int listen_port = *((int *)client_listen_port);

    int server_socket, client_socket, addr_size;
    SA_IN server_addr, client_addr;

    check((server_socket = socket(AF_INET, SOCK_STREAM, 0)),
          "Failed to create socket");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(listen_port);

    check((bind(server_socket, (SA *)&server_addr, sizeof(server_addr))),
          "Bind failed!");
    check((listen(server_socket, SERVER_BACKLOG)),
          "Listen failed!");
    pthread_t id[100];
    int i = 0;
    while (1)
    {
        // cout << "Waiting for Clients..." << endl;

        addr_size = sizeof(SA_IN);
        client_socket = accept(server_socket, (SA *)&client_addr, (socklen_t *)&addr_size);

        // cout << "Connected with client!" << endl;

        pthread_create(&id[i++], NULL, handle_connection, &client_socket);
    }
    for (int j = 0; j < i; j++)
    {
        pthread_join(id[j], NULL);
    }
}

void *handle_connection(void *p_client_socket)
{

    int client_socket = *((int *)p_client_socket);

    string server_response, user_id = "";

    char client_message[2000];
    bzero(client_message, 2000);
    recv(client_socket, client_message, 2000, 0);

    send_file(client_socket, file_path[client_message]);
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