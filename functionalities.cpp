#include <stdlib.h>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <fstream>
using namespace std;

#define dummyChatRoom "uninitializedChatRoom"

unordered_map<string, set<string>> chatRoomTouser;
unordered_map<string, pair<pair<string, string>, string>> userToIPAndChatRoom;
unordered_set<string> ipInUse;

string createChatRoom(string chatRoomName, string user, string ip, string port) {
	string retSt = "";
	auto tempItr = chatRoomTouser.find(chatRoomName);
	if (tempItr != chatRoomTouser.end())
		retSt = "A Chat Room having this name already exists!";
	else
	{
		set<string> tempSet;
		tempSet.insert(user);

		chatRoomTouser.insert(make_pair(chatRoomName, tempSet));
		auto it = userToIPAndChatRoom.find(user);
		it->second.second = chatRoomName;
		// printMaps();
		retSt = "Chatroom created!";
	}
	return retSt;
}
string listFunctions(string specification, string user, string ip, string port) {
	string temp, retSt = "";
	transform(specification.begin(), specification.end(), std::back_inserter(temp), ::toupper);
	if (temp == "CHATROOMS")
		for (auto it : chatRoomTouser)
			retSt += it.first + "\n";
	else if (temp == "USERS")
	{
		auto itr = userToIPAndChatRoom.find(user);
		string chatRoom = itr->second.second;
		if (chatRoom == dummyChatRoom)
			retSt = "You do not belong to any chat room!";
		else
		{
			auto tempItr = chatRoomTouser.find(chatRoom);
			for (auto it = tempItr->second.begin(); it != tempItr->second.end(); it++)
				retSt += *it;
		}
	}
	return retSt;
}
string leaveChatRoom(string user, string ip, string port) {
	string retSt = "";
	auto itr = userToIPAndChatRoom.find(user);
	if (itr->second.second != dummyChatRoom)
	{
		string existingChatRoom = itr->second.second;
		itr->second.second = dummyChatRoom;
		auto tempItr = chatRoomTouser.find(existingChatRoom);
		tempItr->second.erase(user);
		// printMaps();
		retSt = "Deregistered from the chatroom!";
	}
	else
		retSt = "You do not belong to any chatroom!";
	
	return retSt;
}
string joinChatroom(string chatRoomName, string user, string ip, string port) {
	string retSt = "";
	auto it = chatRoomTouser.find(chatRoomName);
	if (it == chatRoomTouser.end())
		retSt = "No existent chatroom!";
	else
	{
		
		it->second.insert(user);
		auto itr1 = userToIPAndChatRoom.find(user);
		cout << "join else " << itr1->second.second << endl;
		string currentChatRoom = itr1->second.second;
		auto itr2 = chatRoomTouser.find(currentChatRoom);
		if (itr2 != chatRoomTouser.end())
			retSt = "User already in another chatroom!\n";
		
		else
		{
			itr1->second.second = chatRoomName;
			// printMaps();
			retSt = "Joined in chatroom!";
		}
	}
	return retSt;
}
string addUserToChatRoom(string chatRoomName, string userToBeAdded, string ip, string port) {
	string retSt = "";
	auto it = userToIPAndChatRoom.find(userToBeAdded);
	if (it == userToIPAndChatRoom.end())
		retSt = "Invalid user name!";
	else
		if (it->second.second != dummyChatRoom)
			retSt = "Requested user is in another chatroom!";
		else
		{
			it->second.second = chatRoomName;
			auto itr = chatRoomTouser.find(chatRoomName);
			itr->second.insert(userToBeAdded);
			retSt = "User successfully added to the Chatroom!";
			// printMaps();
		}
	return retSt;
}
string addNewUser(string userName, string user, string ip, string port) {
	string retSt = "";
	auto it = userToIPAndChatRoom.find(userName);
	if (it != userToIPAndChatRoom.end() || ipInUse.find(ip) != ipInUse.end())
		retSt = "User name or IP already in use. Please use other user credentials!!\n";
	else
	{
		pair<pair<string, string>, string> p;
		p.first.first = ip;
		ipInUse.insert(ip);
		p.first.second = port;
		p.second = dummyChatRoom;
		userToIPAndChatRoom.insert(make_pair(userName, p));
		retSt = "New user successfully added!!\n";
		// printMaps();
	}
	return retSt;
}
vector<pair<string, string>> replyMessage(string user, string ip, string port) {
	vector<pair<string, string>> membersOfChatRoom;
	auto it = userToIPAndChatRoom.find(user);
	if (it->second.second == dummyChatRoom)
		cout << "User doesn't belong to any chatroom!!\n";
	else
	{
		auto it1 = chatRoomTouser.find(it->second.second);
		// cout << "people in same room are\n";
		for (auto itr = it1->second.begin(); itr != it1->second.end(); itr++)
		{
			if (*itr != user)
			{
				// cout << *itr << "->";
				auto tempit = userToIPAndChatRoom.find(*itr);
				// cout << tempit->second.first.first << " " << tempit->second.first.second << endl;
				membersOfChatRoom.push_back(tempit->second.first);
			}
		}
	}
	return membersOfChatRoom;
}