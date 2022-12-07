# Mini-BitTorrent

This is a group based file sharing system where users can share, download files from the group they belong to. 
Download happens parallely with multiple pieces from multiple peers. It also supports multithreaded client/server and tracker.

## Tracker:
- Maintains information of clients with their files(shared by client) to assist the clients for the communication between peers.
- tracker_info.txt - Contains ip, port details of all the trackers

## Client:
- Downloads files from multiple peers and makes itself available as a leacher as soon as it downloads any file.

### Prerequisites
1. G++ compiler
   * ```sudo apt-get install g++```

#### To run the Tracker 

```
cd tracker
g++ tracker.cpp -lcrypto -Wall -pthread -o tracker 
./tracker tracker_info.txt tracker_no 

eg : ./tracker tracker_info.txt 1
```

#### To run the Client

```
cd client
g++ client.cpp -lcrypto -Wall -pthread -o client
./client ./client <IP>:<PORT> tracker_info.txt

eg : ./client 127.0.0.1:7600 tracker_info.txt
```
![Screenshot from 2022-12-07 14-34-00](https://user-images.githubusercontent.com/53634655/206136630-cac6172c-b24d-44d7-94a6-d2020e3fe4b0.png)

#### Working

1. At Least one tracker will always be online.
2. Client needs to create an account (userid and password) in order to be part of the
network.
3. Client can create any number of groups(groupid should be different) and hence will
be owner of those groups
4. Client needs to be part of the group from which it wants to download the file
5. Client will send join request to join a group
6. Owner Client Will Accept/Reject the request
7. After joining group ,client can see list of all the shareable files in the group
8. Client can share file in any group (note: file will not get uploaded to tracker but only
the <ip>:<port> of the client for that file)
9. Client can send the download command to tracker with the group name and
filename and tracker will send the details of the group members which are currently
sharing that particular file
10. After fetching the peer info from the tracker, client will communicate with peers about
the portions of the file they contain and hence accordingly decide which part of data
to take from which peer (You need to design your own Piece Selection Algorithm)
11. As soon as a piece of file gets downloaded it should be available for sharing
12. After logout, the client should temporarily stop sharing the currently shared files till
the next login
13. All trackers need to be in sync with each other

#### Functionality for client

* Create User Account: `create_user <user_id> <passwd>`
* Login: `login  <user_id> <passwd>`
* Create Group: `create_group <group_id>`
* Join Group: `join_group <group_id>`
* Leave Group: `leave_group <group_id>`
* List pending join: `list_requests <group_id>`
* Accept Group Joining Request: `accept_request <group_id> <user_id>`
* List All Group In Network: `list_groups`
* List All sharable Files In Group: `list_files <group_id>`
* Upload File: `upload_file <file_path> <group_id>`
* Download File: `download_file <group_id> <file_name> <destination_path>`
* Logout: `logout`
* Show_downloads: `show_downloads`
  * Output format:
    * [D] [grp_id] filename
    * [C] [grp_id] filename
    * D(Downloading), C(Complete)
* Stop sharing: `stop_share <group_id> <file_name>`

![image](https://user-images.githubusercontent.com/53634655/206136526-7890c6ac-78dc-44ef-af04-19f6c279a54c.png)
