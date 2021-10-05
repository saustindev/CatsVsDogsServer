#include <SFML/Network.hpp>
#include <iostream>
#include <thread>
#include <fstream>
#include <mutex>
#define PORT 5555

typedef struct clientData {
	bool dog;
	int score;
} ClientData;

int score[2] = { 0 };
std::mutex m;

typedef struct clientTracker {
	bool open = true;
	sf::TcpSocket socket;
} ClientTracker;

std::vector<ClientTracker*> clients;

int firstOpen() {
	for (int i = 0; i < clients.size(); i++) {
		if (clients.at(i)->open) {
			return i;
		}
	}
	ClientTracker* cl = new ClientTracker();
	cl->open = true;
	clients.push_back(cl);
	return clients.size() - 1;
}

void clientThread(int num) {
	bool loop = true;
	while (loop) {
		ClientData data;
		std::size_t received;
		sf::Socket::Status result = clients.at(num)->socket.receive(&data, sizeof(ClientData), received);
		if (result != sf::Socket::Done)
		{
			if (result == sf::Socket::Disconnected) {
				loop = false;
				clients.at(num)->open = true;
				break;
			}
			else {
				// error...
				std::cout << "Failed to receive data" << std::endl;
			}

		}
		else {
			if (received == 1) {
				std::cout << "Received request for score" << std::endl;
				//Try to send the score
				if (clients.at(num)->socket.send(&score[data.dog ? 1 : 0], sizeof(int)) != sf::Socket::Done)
				{
					// error...
					std::cout << "Failed to send score" << std::endl;
				}
			}
			else if (received == sizeof(ClientData)) {
				std::cout << "Received client's score" << std::endl;
				m.lock();
				score[data.dog ? 1 : 0] += data.score;
				m.unlock();
			}
			else {
				std::cout << "Received packet of wrong size" << std::endl;
			}
		}
	}
	m.lock();
	std::ofstream ofs;
	ofs.open("score.dat", std::ofstream::out | std::ofstream::trunc);
	ofs << std::to_string(score[0]) << "\n" << std::to_string(score[1]) << "\n";
	ofs.close();
	m.unlock();
}



std::vector<std::thread*> threads;

int main() {
	sf::TcpListener listener;
	std::fstream f("score.dat");
	if (f.good()) {
		std::string line;
		std::getline(f, line);
		score[0] = std::atoi(line.c_str());
		std::getline(f, line);
		score[1] = std::atoi(line.c_str());
	}
	f.close();
	// bind the listener to a port
	if (listener.listen(PORT) != sf::Socket::Done)
	{
		// error...
		std::cout << "Failed to bind to port" << std::endl;
	}
	// accept a new connection
	while (1) {
		int num = firstOpen();
		if (listener.accept(clients.at(num)->socket) != sf::Socket::Done)
		{
			// error...
			std::cout << "Failed to accept client" << std::endl;

		}
		else {
			std::cout << "Got connection" << std::endl;
			clients.at(num)->open = false;
			threads.push_back(new std::thread(clientThread, num));
		}
		m.lock();
		std::ofstream ofs;
		ofs.open("score.dat", std::ofstream::out | std::ofstream::trunc);
		ofs << std::to_string(score[0]) << "\n" << std::to_string(score[1]) << "\n";
		ofs.close();
		m.unlock();
	}
	for (std::thread* t : threads) {
		t->join();
	}
	
	return 0;
}