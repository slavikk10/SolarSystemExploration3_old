#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <celestialbody.hpp>
#include <filesystem.hpp>

CelestialBody parsePlanetJSON(const char* path) {
    std::string json;
    std::ifstream jsonFile;
    std::stringstream jsonStream;

    jsonFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    // load JSON file by given path
    try {
        jsonFile.open(getFilePath(path).c_str());
        jsonStream << jsonFile.rdbuf();
        jsonFile.close();
        json = jsonStream.str();
    }
    catch(std::ifstream::failure e) {
        std::cout << "Error reading JSON file at (relative) path: " << path << std::endl;
    }

    // parse the loaded JSON file line-by-line
    std::string line;
    std::string name;
    std::string s_g_accel;
    std::string s_avg_radius;
    std::string s_eq_radius;
    std::string s_polar_radius;
    glm::vec3 position;
    glm::vec3 velocity;
    std::string x;
    std::string y;
    std::string z;
    std::string vel_x;
    std::string vel_y;
    std::string vel_z;

    float g_accel;
    float avg_radius;
    float eq_radius;
    float polar_radius;

    unsigned int err = 11;
    while (std::getline(jsonStream, line)) {
        size_t name_pos = line.find("name");
        if (name_pos != std::string::npos) {
            err -= 1;
            // check if there is space between : and key value or not
            size_t start;
            if (line[name_pos+6] == '\"')
                start = name_pos+7;
            else
                start = name_pos+8;

            size_t lastQuote = line.find('\"', start + 1);
            name = line.substr(start, lastQuote - start);
            std::cout << "name: " << name << std::endl;
        }

        size_t s_g_accel_pos = line.find("gravitationalAcceleration");
        if (s_g_accel_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[s_g_accel_pos+27] == ' ')
                start = s_g_accel_pos+28;
            else
                start = s_g_accel_pos+27;

            size_t lastComma = line.find(',', start + 1);
            s_g_accel = line.substr(start, lastComma - start);
            std::cout << "gravitational acceleration: " << s_g_accel << std::endl;
        }

        size_t s_avg_radius_pos = line.find("averageRadius");
        if (s_avg_radius_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[s_avg_radius_pos+15] == ' ')
                start = s_avg_radius_pos+16;
            else
                start = s_avg_radius_pos+15;

            size_t lastComma = line.find(',', start + 1);
            s_avg_radius = line.substr(start, lastComma - start);
            std::cout << "average radius: " << s_avg_radius << std::endl;
        }

        // equatorial radius (float, used for rendering)
        size_t s_eq_radius_pos = line.find("equatorialRadius");
        if (s_eq_radius_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[s_eq_radius_pos+18] == ' ')
                start = s_eq_radius_pos+19;
            else
                start = s_eq_radius_pos+18;

            size_t lastComma = line.find(',', start + 1);
            s_eq_radius = line.substr(start, lastComma - start);
            std::cout << "equatorial radius: " << s_eq_radius << std::endl;
        }

        // polar radius (float, used for rendering)
        size_t s_polar_radius_pos = line.find("polarRadius");
        if (s_polar_radius_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[s_polar_radius_pos+13] == ' ')
                start = s_polar_radius_pos+14;
            else
                start = s_polar_radius_pos+13;

            size_t lastComma = line.find(',', start + 1);
            s_polar_radius = line.substr(start, lastComma - start);
            std::cout << "polar radius: " << s_polar_radius << std::endl;
        }

        // position (vec3)
        size_t x_pos = line.find("\"x");
        size_t y_pos = line.find("\"y");
        size_t z_pos = line.find("\"z");
        if (x_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[x_pos+4] == ' ')
                start = x_pos+5;
            else
                start = x_pos+4;

            size_t lastComma = line.find(',', start);
            x = line.substr(start, lastComma - start);
            position.x = std::stof(x);
        }
        if (y_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[y_pos+4] == ' ')
                start = y_pos+5;
            else
                start = y_pos+4;

            size_t lastComma = line.find(',', start + 1);
            y = line.substr(start, lastComma - start);
            position.y = std::stof(y);
        }
        if (z_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[z_pos+4] == ' ')
                start = z_pos+5;
            else
                start = z_pos+4;

            size_t lastComma = line.find(',', start + 1);
            z = line.substr(start, lastComma - start);
            position.z = std::stof(z);
        }

        // velocity (vec3)
        size_t vel_x_pos = line.find("velocityX");
        size_t vel_y_pos = line.find("velocityY");
        size_t vel_z_pos = line.find("velocityZ");
        if (vel_x_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[vel_x_pos+11] == ' ')
                start = vel_x_pos+12;
            else
                start = vel_x_pos+11;

            size_t lastComma = line.find(',', start);
            vel_x = line.substr(start, lastComma - start);
            velocity.x = std::stof(vel_x);
        }
        if (vel_y_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[vel_y_pos+11] == ' ')
                start = vel_y_pos+12;
            else
                start = vel_y_pos+11;

            size_t lastComma = line.find(',', start + 1);
            vel_y = line.substr(start, lastComma - start);
            velocity.y = std::stof(vel_y);
        }
        if (vel_z_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[vel_z_pos+11] == ' ')
                start = vel_z_pos+12;
            else
                start = vel_z_pos+11;

            size_t lastComma = line.find(',', start + 1);
            vel_z = line.substr(start, lastComma - start);
            velocity.z = std::stof(vel_z);
        }
    }

    if (err != 0) {
        std::cout << "Error: JSON file error at path: " << getFilePath(path) << std::endl;
    }

    g_accel      = std::stof(s_g_accel);
    avg_radius   = std::stof(s_avg_radius);
    eq_radius    = std::stof(s_eq_radius);
    polar_radius = std::stof(s_polar_radius);

    CelestialBody body(position, velocity, avg_radius, eq_radius, polar_radius, g_accel);

    return body;
}