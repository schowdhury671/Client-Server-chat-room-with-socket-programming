all:
	g++ -std=c++11 -c functionalities.cpp
	g++ -std=c++11 -o server server.cpp -lpthread
	g++ -std=c++11 -o client client.cpp -lpthread