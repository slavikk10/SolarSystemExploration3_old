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

extern int timestamp;

extern std::unordered_map<int, glm::dvec3> playerPositions;
extern std::mutex playerPositionsMutex;

extern std::vector<glm::dvec3> planetPositions;
extern std::vector<glm::dvec3> planetVelocities;

// add client to the list that stores their sockets and to the list that stores their UID
void addClient(int client_fd, int clientUID) 
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.push_back(client_fd);
    //std::lock_guard<std::mutex> lockUID(clientUIDs_mutex);
    //clientUIDs.push_back(clientUID);
}

// remove client from the list that stores their sockets and from the list that stores their UID
void removeClient(int client_fd, int clientUID) 
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(std::remove(clients.begin(), clients.end(), client_fd), clients.end());
    //std::lock_guard<std::mutex> lockUID(clientUIDs_mutex);
    //clientUIDs.erase(std::remove(clientUIDs.begin(), clientUIDs.end(), clientUID), clientUIDs.end());
}

// send message to all clients
void broadcastMessage(const std::string& message, int sender_fd) 
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int client_fd : clients) 
    {
        if (client_fd != sender_fd) 
        {
            send(client_fd, message.c_str(), message.length(), 0);
        }
    }
}

int containsPlanetData(std::string msg) 
{
    if (msg.rfind("SUN;",   0) == 0) return 0;
    if (msg.rfind("EARTH;", 0) == 0) return 1;
    if (msg.rfind("MOON;",  0) == 0) return 2;
    return -1;
}

void handleClient(int client_fd, int uid=-1) 
{
    std::atomic<bool> clientRunning{true};
    std::atomic<int> clientUID{uid};

    addClient(client_fd, clientUID);
    std::thread t1([&]() {
        char buffer[8192];
        while (clientRunning.load()) 
        {
            int bytes = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes > 0) 
            {
                std::string msg(buffer, bytes);

                std::vector<std::string> tokens;
                std::stringstream ss(msg);
                std::string item;

                int senderUID;
                double px, py, pz;

                while (std::getline(ss, item, ';')) 
                {
                    if (!item.empty())
                        tokens.push_back(item);
                }

                for (size_t i = 0; i < tokens.size();) 
                {
                    const std::string& token = tokens[i];

                    if (token == "UID" && i + 1 < tokens.size()) 
                    {
                        clientUID = std::stoi(tokens[i + 1]);
                        clientUIDs[clientUID] = clientUID;
                        std::cout << "my UID is " << clientUID << "\n";
                        i += 2;
                    } 
                    else if ((token == "SUN" || token == "EARTH" || token == "MOON") && i + 6 <= tokens.size()) 
                    {
                        int planetIndex = containsPlanetData(token + ";");

                        double x = std::stof(tokens[i + 1]);
                        double y = std::stof(tokens[i + 2]);
                        double z = std::stof(tokens[i + 3]);
                        double vx = std::stof(tokens[i + 4]);
                        double vy = std::stof(tokens[i + 5]);
                        double vz = std::stof(tokens[i + 6]);

                        glm::dvec3 position = glm::dvec3(x, y, z);
                        glm::dvec3 velocity = glm::dvec3(vx, vy, vz);

                        std::cout << "Setting the goofy position to " << position.x << ", " << position.y << ", " << position.z << std::endl;
                        std::cout << "Setting the goofy velocity to " << velocity.x << ", " << velocity.y << ", " << velocity.z << std::endl;

                        planetPositions[planetIndex] = position;
                        planetVelocities[planetIndex] = velocity;

                        i += 7;
                    }
                    else if (token == "TIMESTAMP")
                    {
                        timestamp = std::stoi(tokens[i + 1]);
                        i += 2;
                    }
                    else if (isdigit(msg[0]))
                    {
                        if (sscanf(msg.c_str(), "%i;%d;%d;%d;", &senderUID, &px, &py, &pz))
                        {
                            std::cout << "SENDER UID: " << senderUID << std::endl;
                            std::lock_guard<std::mutex> lock(playerPositionsMutex);
                            std::cout << "Setting player position " << px << ", " << py << ", " << pz << std::endl;
                            playerPositions[senderUID] = glm::dvec3(px, py, pz);
                        }

                        i += 4;
                    }
                    else 
                    {
                        std::cerr << "Error: unknown argument: " << token << "\n";
                        i++;
                    }
                }

                broadcastMessage(msg, client_fd);
            } 
            else if (bytes == 0) 
            {
                clientRunning = false;
                std::cout << "Player " << clientUID << " disconnected.\n";
                removeClient(client_fd, clientUID);
            }
        }
    });

    std::thread t2([&]()
    {
        while (clientRunning.load()) 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            int currentUID = clientUID;

            glm::dvec3 position = (glm::dvec3)camera.Position;

            std::string msg = std::to_string(currentUID) + ";" + std::to_string(position.x) + ";" + std::to_string(position.y) + ";" + std::to_string(position.z) + ";";
            broadcastMessage(msg, client_fd);
        }
    });

    t1.join();
    t2.join();

    removeClient(client_fd, clientUID);
    close(client_fd);
}