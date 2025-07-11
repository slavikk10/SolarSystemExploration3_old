#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <unordered_map>

#include <camera.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

extern std::vector<int> clientUIDs;

std::vector<int> clients;
std::mutex clients_mutex;

extern Camera camera;

extern std::unordered_map<int, glm::vec3> playerPositions;
extern std::mutex playerPositionsMutex;

extern std::unordered_map<int, glm::vec3> planetPositions;
extern std::mutex planetPositionsMutex;

// add client to the list that stores their sockets and to the list that stores their UID
void addClient(int client_fd, int clientUID) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.push_back(client_fd);
    //std::lock_guard<std::mutex> lockUID(clientUIDs_mutex);
    //clientUIDs.push_back(clientUID);
}

// remove client from the list that stores their sockets and from the list that stores their UID
void removeClient(int client_fd, int clientUID) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(std::remove(clients.begin(), clients.end(), client_fd), clients.end());
    //std::lock_guard<std::mutex> lockUID(clientUIDs_mutex);
    //clientUIDs.erase(std::remove(clientUIDs.begin(), clientUIDs.end(), clientUID), clientUIDs.end());
}

// send message to all clients
void broadcastMessage(const std::string& message, int sender_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int client_fd : clients) {
        if (client_fd != sender_fd) {
            send(client_fd, message.c_str(), message.length(), 0);
        }
    }
}

int containsPlanetData(std::string msg) {
    if (msg.rfind("SUN;",   0) == 0) return 0;
    if (msg.rfind("EARTH;", 0) == 0) return 1;
    if (msg.rfind("MOON;",  0) == 0) return 2;
    return -1;
}

inline void handleClient(int client_fd, int uid=-1) {
    bool clientRunning = true;
    int clientUID = uid;

    addClient(client_fd, clientUID);
    std::thread t1([=]() mutable {
        char buffer[8192];
        while (clientRunning) {
            int bytes = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes > 0) {
                std::string msg(buffer, bytes);

                std::vector<std::string> tokens;
                std::stringstream ss(msg);
                std::string item;

                while (std::getline(ss, item, ';')) {
                    if (!item.empty())
                        tokens.push_back(item);
                }

                for (size_t i = 0; i < tokens.size();) {
                    const std::string& token = tokens[i];

                    if (token == "UID" && i + 1 < tokens.size()) {
                        uid = std::stoi(tokens[i + 1]);
                        clientUIDs[uid] = uid;
                        std::cout << "my UID is " << uid << " bruh\n";
                        i += 2;
                    } else if ((token == "SUN" || token == "EARTH" || token == "MOON") && i + 3 < tokens.size()) {
                        int planetIndex = containsPlanetData(token + ";");
                        std::cout << "Parsing planet position for " << token << " (planet index: " << planetIndex << ")\n";

                        float x = std::stof(tokens[i + 1]);
                        float y = std::stof(tokens[i + 2]);
                        float z = std::stof(tokens[i + 3]);

                        std::cout << "Setting position (" << x << ", " << y << ", " << z << ") for " << token << std::endl;

                        std::lock_guard<std::mutex> lock(planetPositionsMutex);
                        planetPositions[planetIndex] = glm::vec3(x, y, z);

                        i += 4;
                    }
                    else {
                        std::cerr << "Error: unknown argument: " << token << "\n";
                        i++;
                    }
                }

                broadcastMessage(msg, client_fd);
            } else if (bytes == 0) {
                clientRunning = false;
                std::cout << "Player " << clientUID << " disconnected.\n";
                removeClient(client_fd, clientUID);
            }
        }
        std::cout << "YOOOO 1\n";
    });

    std::thread t2([&]{
        while (clientRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            int currentUID = clientUID;

            std::string msg = std::to_string(currentUID) + ";" + std::to_string(camera.Position.x) + ";" + std::to_string(camera.Position.y) + ";" + std::to_string(camera.Position.z) + ";";
            broadcastMessage(msg, client_fd);
        }
    });

    t1.join();
    t2.join();

    removeClient(client_fd, clientUID);
    close(client_fd);
}