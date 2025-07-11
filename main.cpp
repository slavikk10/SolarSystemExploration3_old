#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <tinyfiledialogs.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// define ImGui docking
#define IMGUI_ENABLE_DOCKING
// main ImGui file
#include <imgui/imgui.h>
// ImGui backend files for OpenGL and GLFW
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <shader.hpp>
#include <camera.hpp>
#include <model.hpp>
#include <json_parser.hpp>
#include <celestialbody.hpp>
#include <filesystem.hpp>
#include <socket.hpp>

#include <cstdlib>
#include <cctype>
#include <iostream>
#include <random>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <atomic>

std::mutex recvMutex;

std::vector<int> clientUIDs(1000000);

std::unordered_map<int, glm::vec3> playerPositions;
std::mutex playerPositionsMutex;

std::unordered_map<int, glm::vec3> planetPositions;
std::mutex planetPositionsMutex;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path, bool gamma_correction);
unsigned int loadCubemap(vector<std::string> faces, bool gamma_correction);
glm::vec3 floatToVec3(float v[3]);
glm::vec2 floatToVec2(float v[2]);
glm::vec4 floatToVec4(float v[4]);
void loadShaderUniforms(Shader shader);
std::string toString(const char* v);
char* stringToChar(std::string v);
void addToArr(const char* arr[], char* v);
void loadHDRtexture(const char* hdrPath, Shader& equirectangularShader, glm::mat4 captureProj, unsigned int &captureFBO, glm::mat4 captureViews[], unsigned int &envCubemap, unsigned int cubeVAO, unsigned int skyboxVAO, unsigned int &captureRBO, Shader& convShader, Shader& prefilterShader, unsigned int &irradianceMap, unsigned int &prefilterMap, unsigned int &brdfLUTTexture);

void renderQuad(float scale, float z_offset);
void renderSphere(bool patches);

// settings
unsigned int SCR_WIDTH = 1920;
unsigned int SCR_HEIGHT = 1040;

// camera
Camera camera(glm::vec3(8.241306702279538e+09, 3.218946189282089e+07, -1.526005896490690e+11)/10000.0f);
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// constants
const float sscale = 1.0f; // scale of the Solar System (1:1)

bool isServer;

int main(int argc, char* argv[]) {
    // check if server or client
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "server") {
            isServer = true;
            std::cout << "Initialized server context successfully.\n";
        } else if (arg1 == "client") {
            isServer = false;
            std::cout << "Initialized client context successfully.\n";
        } else {
            std::cout << "Unknown argument.\n";
        }
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Error: server socket creation failed\n";
        return 1;
    }

    int client_fd;
    if (!isServer) {
        client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (client_fd == -1) {
            std::cerr << "Error: client socket creation failed\n";
            return 1;
        }
    }

    if (isServer) {
        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(7070);

        if (::bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Error: bind failed\n";
            close(server_fd);
            return 1;
        }

        if (listen(server_fd, 10) < 0) {
            std::cerr << "Error: failed to listen\n";
            close(server_fd);
            return 1;
        }
    } else {
        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr("127.0.0.1");
        address.sin_port = htons(7070);

        if (connect(client_fd, (sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Error: failed to connect to server\n";
            return 1;
        }

        std::cout << "Connected to server.\n";
    }

    stbi_set_flip_vertically_on_load(true);
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, isServer ? "Solar System Exploration 3 (server)" : "Solar System Exploration 3", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    float start = glfwGetTime();

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    glPatchParameteri(GL_PATCH_VERTICES, 3);

    // build and compile shaders
    // -------------------------
    Shader shader(getFilePath("shaders/pbr/pbr.vert").c_str(), getFilePath("shaders/pbr/pbr.frag").c_str(), getFilePath("shaders/pbr/pbr.tesc").c_str(), getFilePath("shaders/pbr/pbr.tese").c_str());
    Shader shaderTex(getFilePath("shaders/pbr/pbr.vert").c_str(), getFilePath("shaders/pbr/pbr_textured.frag").c_str(), getFilePath("shaders/pbr/pbr.tesc").c_str(), getFilePath("shaders/pbr/pbr.tese").c_str());
    //Shader shaderTex(getFilePath("shaders/pbr/pbr.vert").c_str(), getFilePath("shaders/pbr/pbr_textured.frag").c_str(), getFilePath("shaders/pbr/pbr.geom").c_str());
    Shader lightShader(getFilePath("shaders/light/light.vert").c_str(), getFilePath("shaders/light/light.frag").c_str(), getFilePath("shaders/light/light.geom").c_str());
    Shader atmosphereShader(getFilePath("shaders/atmosphere/atmosphere.vert").c_str(), getFilePath("shaders/atmosphere/atmosphere.frag").c_str(), getFilePath("shaders/atmosphere/atmosphere.geom").c_str());
    Shader equirectangularShader(getFilePath("shaders/equirectangular/skybox.vert").c_str(), getFilePath("shaders/equirectangular/skybox.frag").c_str(), getFilePath("shaders/equirectangular/skybox.geom").c_str());
    Shader convShader(getFilePath("shaders/convolution/skybox copy.vert").c_str(), getFilePath("shaders/convolution/skybox copy.frag").c_str(), getFilePath("shaders/convolution/skybox copy.geom").c_str());
    Shader brdfShader(getFilePath("shaders/brdf/brdf.vert").c_str(), getFilePath("shaders/brdf/brdf.frag").c_str(), getFilePath("shaders/brdf/brdf.geom").c_str());
    Shader prefilterShader(getFilePath("shaders/pre-filter/pre-filter.vert").c_str(), getFilePath("shaders/pre-filter/pre-filter.frag").c_str(), getFilePath("shaders/pre-filter/pre-filter.geom").c_str());
    Shader skyboxShader(getFilePath("shaders/skybox/skybox.vert").c_str(), getFilePath("shaders/skybox/skybox.frag").c_str(), getFilePath("shaders/skybox/skybox.geom").c_str());
    Shader hdrShader(getFilePath("shaders/hdr/hdr.vert").c_str(), getFilePath("shaders/hdr/hdr.frag").c_str(), getFilePath("shaders/hdr/hdr.geom").c_str());

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float cubeVertices[] = {
        // positions          // normals
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    float cubeVertices2[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    float lightVertices[] = {
        // Front face
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,

        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        // Back face
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,

        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,

        // Left face
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,

        // Right face
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,

        // Top face
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,

        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        // Bottom face
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,

        1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f
    };

    float floorVertices[] = {
      // position             normal             tex coords
        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,   0.0f, 0.0f, 1.0f,  1.0f, 1.0f,

        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
         1.0f,  1.0f, 0.0f,   0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f,   0.0f, 0.0f, 1.0f,  0.0f, 1.0f
    };

    /*float floorVertices[] = {
      // position            normal              tex coords
        -1.0f, 0.0f,  1.0f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        -1.0f, 0.0f, -1.0f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
         1.0f, 0.0f, -1.0f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f,

        -1.0f, 0.0f,  1.0f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
         1.0f, 0.0f, -1.0f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
         1.0f, 0.0f,  1.0f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f
    };*/

    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    // cube VAO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices2), &cubeVertices2, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    // floor VAO
    unsigned int floorVAO, floorVBO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), &floorVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3  * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6  * sizeof(float)));
    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    // light VAO
    unsigned int lightVAO, lightVBO;
    glGenVertexArrays(1, &lightVAO);
    glGenBuffers(1, &lightVBO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lightVertices), &lightVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // generate depth map framebuffer
    /*unsigned int depthCubemapFBO;
    glGenFramebuffers(1, &depthCubemapFBO);

    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

    unsigned int depthCubemap;
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, depthCubemapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);*/

    unsigned int fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    unsigned int colorBuffer;
    glGenTextures(1, &colorBuffer);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned int depthBuffer;
    glGenTextures(1, &depthBuffer);
    glBindTexture(GL_TEXTURE_2D, depthBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // attach buffers
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffer, 0);


    unsigned int captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 1024, 1024);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
    GL_RENDERBUFFER, captureRBO);

    unsigned int envCubemap;
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 1024, 1024, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(false);

    // load planet textures
    // -----------------
    // Mercury
    unsigned int mercurySurfaceTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/mercury/mercury_surface.jpg", false);

    // Venus
    unsigned int venusAtmoTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/venus/venus_atmo.jpg", false);

    // Earth
    unsigned int earthSurfaceTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/earth/earth_surface.png", false);
    unsigned int earthMetallicTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/earth/earth_metallic.jpg", false);
    unsigned int earthRoughnessTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/earth/earth_metallic.jpg", false);
    unsigned int earthHeightTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/earth/earth_height.jpg", false);
    unsigned int earthNormalTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/earth/earth_normal.png", false);

    // Earth/Moon
    unsigned int moonSurfaceTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/earth/moon/moon_surface.png", false);
    unsigned int moonHeightTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/earth/moon/moon_surface.png", false);
    unsigned int moonNormalTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/earth/moon/moon_normal.png", false);

    // Mars
    unsigned int marsSurfaceTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/mars/mars_surface.jpg", false);

    // Jupiter
    unsigned int jupiterAtmoTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/jupiter/jupiter_atmo.png", false);

    // Saturn
    unsigned int saturnAtmoTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/saturn/saturn_atmo.jpg", false);

    // Uranus
    unsigned int uranusAtmoTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/uranus/uranus_atmo.jpg", false);

    // Neptune
    unsigned int neptuneAtmoTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/neptune/neptune_atmo.jpg", false);

    // Moon textures
    // -----------------------------------------------------------------------------------------------------------------------------------

    // Jupiter/Io
    unsigned int ioSurfaceTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/jupiter/moons/io/io_surface.png", false);

    // Jupiter/Europa
    unsigned int europaSurfaceTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/jupiter/moons/io/io_surface.png", false);
    unsigned int europaHeightTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/jupiter/moons/europa/europa_height.png", false);

    // Jupiter/Ganymede
    unsigned int ganymedeSurfaceTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/jupiter/moons/ganymede/ganymede_surface.jpg", false);

    // Jupiter/Callisto
    unsigned int callistoSurfaceTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/jupiter/moons/io/io_surface.png", false);

    // load HDR texture
    // ----------------
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    const char* oldHdrPath = "/Users/vyacheslav/SSE3_OpenGL/resources/textures/HDR/space.hdr";
    const char* hdrPath = "/Users/vyacheslav/SSE3_OpenGL/resources/textures/HDR/space.hdr";
    float *data = stbi_loadf(hdrPath, &width, &height, &nrComponents, 0);
    unsigned int hdrTexture;
    if (data) {
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        stbi_image_free(data);
    }
    stbi_set_flip_vertically_on_load(false);

    // shader configuration
    // --------------------
    hdrShader.use();
    hdrShader.setInt("hdrTex", 0);

    shader.use();
    shader.setInt("irradianceMap", 3);
    shader.setInt("prefilterMap", 4);
    shader.setInt("brdfLUT", 5);

    shaderTex.use();
    shaderTex.setInt("albedoMap", 0);
    shaderTex.setInt("metallicMap", 1);
    shaderTex.setInt("roughnessMap", 2);
    shaderTex.setInt("irradianceMap", 3);
    shaderTex.setInt("prefilterMap", 4);
    shaderTex.setInt("brdfLUT", 5);
    shaderTex.setInt("heightMap", 6);
    shaderTex.setInt("normalMap", 7);

    atmosphereShader.use();
    atmosphereShader.setInt("depthTex", 0);
    atmosphereShader.setInt("colorTex", 1);

    float lightPos[] = { 0.0f, 0.0f, 0.0f };
    float lightColor[] = { 200000000000000000000000.0f, 200000000000000000000000.0f, 200000000000000000000000.0f };

    glm::mat4 captureProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f, 0.0f, 0.0f),  glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),  glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 1.0f, 0.0f),  glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 0.0f, 1.0f),  glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    equirectangularShader.use();
    equirectangularShader.setInt("equirectangularMap", 0);
    equirectangularShader.setMat4("projection", captureProj);
    equirectangularShader.setFloat("exposure", 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, 1024, 1024);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i) {
        equirectangularShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);


    unsigned int irradianceMap;
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 32, 32);

    convShader.use();
    convShader.setInt("environmentMap", 0);
    convShader.setMat4("projection", captureProj);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glViewport(0, 0, 32, 32);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i) {
        convShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    
    unsigned int prefilterMap;
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    prefilterShader.use();
    prefilterShader.setInt("environmentMap", 0);
    prefilterShader.setMat4("projection", captureProj);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        unsigned int mipWidth = 128 * std::pow(0.5, mip);
        unsigned int mipHeight = 128 * std::pow(0.5, mip);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        prefilterShader.setFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i) {
            prefilterShader.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glBindVertexArray(skyboxVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    unsigned int brdfLUTTexture;
    glGenTextures(1, &brdfLUTTexture);

    // pre-allocate enough memory for the LUT texture.
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    float end = glfwGetTime();

    std::vector<CelestialBody> bodies;
    if (isServer) {
        bodies = {
            CelestialBody(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 0.0), 695700000, 695508000, 695800000, 274.049), // Sun
            //CelestialBody(glm::vec3(-3.010549851451788E+07, -6.359103389001439E+07, -2.405822593778040E+06), glm::vec3(3.430790710871147E+01, -1.832212359355950E+01, -4.642893227687938E+00), 2439700.00, 2440500.0, 2438500.0, 3.700), // Mercury
            //CelestialBody(glm::vec3( 9.524255103572875e+10, -6.213717611494824e+09, -5.199647206619611e+10), glm::vec3( 1.629745416122789e+4, -5.174244527849687e-4,  3.074604261038377e+4), 6051800.00, 8.870), // Venus
            parsePlanetJSON("resources/planets/earth.json"),
            parsePlanetJSON("resources/planets/moon.json"),
            //CelestialBody(glm::vec3(4.554264437870046E+07, -1.461045291504693E+08, -9.824045699685812E+02), glm::vec3(2.889013299101454E+01,  9.091279156494144E+00,  2.875594629304423E-02), 1737400.00, 1738100.0, 1736000.0, 1.620), // Earth/Moon
            /*CelestialBody(glm::vec3(-2.447261099970919e+11,  5.416688334250521e+09, -2.904527272051912e+10), glm::vec3( 3.702006156517989e+3, -5.516948900142156e-4, -2.200086665127839e+4), 3389500.00, 3.730), // Mars
            CelestialBody(glm::vec3(-4.155962876379675e+10, -2.252909919162691e+09,  7.674747027447987e+11), glm::vec3(-1.319807488636713e+4, -5.516948900142156e-4, -8.347507826614081e-5), 71492000.0, 24.79), // Jupiter
            CelestialBody(glm::vec3( 1.426000189864588e+12, -5.468390271451207e+10, -1.203550185774606e+11), glm::vec3( 2.775345470427069e-4, -1.774901403126501e-4,  9.603681056096420e+3), 58232000.0, 10.44), // Saturn
            CelestialBody(glm::vec3( 1.573711616842715e+12, -1.125450741055036e+10,  2.459155382854037e+12), glm::vec3(-5.786230412012094e+3,  8.734538058380403e-6,  3.353286525767898e+3), 25362000.0, 8.690), // Uranus
            CelestialBody(glm::vec3( 4.469572046319349e+12, -1.027391845496875e+11, -1.294629107594498e+10), glm::vec3(-1.931667661979989e-6, -1.123826516130224e-4,  5.467910945075801e+3), 24622000.0, 11.15), // Neptune
            // Moons (excluding Luna)
            CelestialBody(glm::vec3(-4.197883892169575e+10, -2.260962042046607e+09,  7.674149175417285e+11), glm::vec3(-1.074655400734931e+4, -2.774086178816164e-4, -1.716350727666113e+4), 1821600.00, 1.796), // Jupiter/Io
            CelestialBody(glm::vec3(-4.162413575114931e+10, -2.233047647732496e+09,  7.681422995544446e+11), glm::vec3(-2.686161798983449e+4, -4.447906515638533e-5, -1.522091185973320e+0), 1560800.00, 1.315), // Jupiter/Europa
            CelestialBody(glm::vec3(-4.239282399846242e+10, -2.239100603450358e+09,  7.681503440723845e+11), glm::vec3(-2.004451065563605e+3, -1.239077410929723e+2, -8.499809670836234e+3), 2634100.00, 1.428), // Jupiter/Ganymede
            CelestialBody(glm::vec3(-4.197883892169575e+10, -2.260962042046607e+09,  7.674149175417285e+11), glm::vec3(-1.074655400734931e+4, -2.774086178816164e-4, -1.716350727666113e+4), 2410300.00, 1.235), // Jupiter/Callisto
            CelestialBody(glm::vec3(-4.197883892169575e+10, -2.260962042046607e+09,  7.674149175417285e+11), glm::vec3(-1.074655400734931e+4, -2.774086178816164e-4, -1.716350727666113e+4), 1821600.00, 1.796), // Saturn/Io
            CelestialBody(glm::vec3(-4.162413575114931e+10, -2.233047647732496e+09,  7.681422995544446e+11), glm::vec3(-2.686161798983449e+4, -4.447906515638533e-5, -1.522091185973320e+0), 1560800.00, 1.315), // Saturn/Europa
            CelestialBody(glm::vec3(-4.239282399846242e+10, -2.239100603450358e+09,  7.681503440723845e+11), glm::vec3(-2.004451065563605e+3, -1.239077410929723e+2, -8.499809670836234e+3), 2634100.00, 1.428), // Saturn/Ganymede
            CelestialBody(glm::vec3(-4.197883892169575e+10, -2.260962042046607e+09,  7.674149175417285e+11), glm::vec3(-1.074655400734931e+4, -2.774086178816164e-4, -1.716350727666113e+4), 2410300.00, 1.235), // Saturn/Callisto
            // Miscallenous objects
            CelestialBody(glm::vec3(-2.447261099970919e+11,  5.416688334250521e+09+3389500.0+10000.0, -2.904527272051912e+10), glm::vec3( 3.702006156517989e+3, -5.516948900142156e-4, -2.200086665127839e+4), 1.81, 0.0000000014)*/ // Player
        };
    }

    std::vector<glm::vec4> bodyData = {
        glm::vec4(695700000.0, 695508000, 695800000, 274.049),
        glm::vec4(6371000.0, 6378137.0, 6356752.0, 9.81),
        glm::vec4(1737400.00, 1738100.0, 1736000.0, 1.620)
    };

    std::thread([&]() {
        if (isServer) {
            while (!glfwWindowShouldClose(window)) {
                sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                int accept_client_fd = accept(server_fd, (sockaddr*)&client_addr, &addr_len);
                if (accept_client_fd < 0) {
                    std::cerr << "Error: client accept failed\n";
                    continue;
                }

                // generate unique ID for the player
                int uid = rand() % 1000000;
                // send the UID to each connected client once
                std::string packet = "UID;" + std::to_string(uid) + ";";
                send(accept_client_fd, packet.c_str(), packet.length(), 0);

                std::cout << "Player " << uid << " joined.\n";

                // send planet positions
                packet = "SUN;" + std::to_string(bodies[0].position.x) + ";" + std::to_string(bodies[0].position.y) + ";" + std::to_string(bodies[0].position.z) + ";";
                send(accept_client_fd, packet.c_str(), packet.length(), 0);

                packet = "EARTH;" + std::to_string(bodies[1].position.x) + ";" + std::to_string(bodies[1].position.y) + ";" + std::to_string(bodies[1].position.z) + ";";
                send(accept_client_fd, packet.c_str(), packet.length(), 0);

                packet = "MOON;" + std::to_string(bodies[2].position.x) + ";" + std::to_string(bodies[2].position.y) + ";" + std::to_string(bodies[2].position.z) + ";";
                send(accept_client_fd, packet.c_str(), packet.length(), 0);

                if (accept_client_fd >= 0)
                    std::thread(handleClient, accept_client_fd, uid).detach();
            }
        }
    }).detach();

    int uid;
    std::thread([&]() {
        if (!isServer) {
            char buffer[8192];
            std::lock_guard<std::mutex> lock(recvMutex);
            int bytes = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes > 0) {
                std::string msg(buffer, bytes);
                std::cout << "RECEIVED MESSAGE: " << msg << std::endl;

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
            }

            handleClient(client_fd, uid);
            std::cout << ":|\n";
        }
    }).detach();

    if (!isServer) {
        while (planetPositions.size() != 3) {
            std::cout << "Waiting... (current size: " << planetPositions.size() << ")\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        for (unsigned int i = 0; i < planetPositions.size(); i++) {
            bodies[i].position            = planetPositions[i];
            bodies[i].averageRadius       = bodyData[i].x;
            bodies[i].equatorialRadius    = bodyData[i].y;
            bodies[i].polarRadius         = bodyData[i].z;
            bodies[i].gravityAcceleration = bodyData[i].w;
        }

        std::cout << "Set bodies to size of " << bodies.size() << " (planetPositions size: " << planetPositions.size() << ").\n";
    }

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        if (lastFrame == 0)
            deltaTime = currentFrame - (end - start);
        else
            deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        std::cout << "FPS: " << 1.0f / deltaTime << std::endl;


        for (auto &body : bodies) {
            body.updateVelocity(bodies, deltaTime);
        }

        for (auto &body : bodies) {
            body.updatePosition(deltaTime);
        }

        camera.Position = glm::vec3(bodies[2].position.x/sscale, bodies[2].position.y/sscale+6378237.0, bodies[2].position.z/sscale);
        //camera.Position = glm::vec3(bodies[14].position.x/sscale, bodies[14].position.y/sscale, bodies[14].position.z/sscale);

        // input
        // -----
        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
            glEnable(GL_DEPTH_TEST);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100000000000000000.0f); // 100 quintillion meters far plane (1e17, or 100 trillion (1e14) km)
            glm::mat4 view = camera.GetViewMatrix();

            shader.use();
            shader.setMat4("projection", projection);
            shader.setMat4("view", view);

            lightShader.use();
            lightShader.setMat4("projection", projection);
            lightShader.setMat4("view", view);

            shaderTex.use();
            shaderTex.setMat4("projection", projection);
            shaderTex.setMat4("view", view);

            atmosphereShader.use();
            atmosphereShader.setMat4("projection", projection);
            atmosphereShader.setMat4("view", view);

            shader.use();

            shader.setVec3("albedo", glm::vec3(1.0f, 0.0f, 0.0f));
            shader.setFloat("ao", 1.0f);
            shader.setVec3("camPos", glm::vec3(0.0f, 0.0f, 0.0f));
            for (unsigned int i = 0; i < 1; i++) {
                shader.setVec3("lightPositions[" + std::to_string(i) + "]", glm::vec3(lightPos[0] - camera.Position.x, lightPos[1] - camera.Position.y, lightPos[2] - camera.Position.z));
                shader.setVec3("lightColors[" + std::to_string(i) + "]", glm::vec3(lightColor[0], lightColor[1], lightColor[2]));
            }

            atmosphereShader.use();
            atmosphereShader.setVec3("camPos", glm::vec3(0.0f, 0.0f, 0.0f));
            atmosphereShader.setVec3("lightPos", glm::vec3(lightPos[0] - camera.Position.x, lightPos[1] - camera.Position.y, lightPos[2] - camera.Position.z));
            
            shaderTex.use();

            shaderTex.setVec3("camPos", glm::vec3(0.0f, 0.0f, 0.0f));
            for (unsigned int i = 0; i < 1; i++) {
                shaderTex.setVec3("lightPositions[" + std::to_string(i) + "]", glm::vec3(lightPos[0] - camera.Position.x, lightPos[1] - camera.Position.y, lightPos[2] - camera.Position.z));
                shaderTex.setVec3("lightColors[" + std::to_string(i) + "]", glm::vec3(lightColor[0], lightColor[1], lightColor[2]));
            }

            // bind PBR textures

            // bind irradiance map
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
            // bind pre-filter map
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
            // bind BRDF LUT texture
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);

            // bind Earth textures

            // bind surface tex
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, earthSurfaceTex);
            // bind metallic map
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, earthMetallicTex);
            // bind roughness map
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, earthRoughnessTex);
            // bind height map
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, earthHeightTex);
            // bind normal map
            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_2D, earthNormalTex);

            lightShader.use();

            std::cout << "TEST 11\n";

            // -----------------------------------------------------------------------------------------------------------------------------
            // Sun
            glm::dmat4 model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[0].position.x/sscale - camera.Position.x, bodies[0].position.y/sscale - camera.Position.y, bodies[0].position.z/sscale - camera.Position.z));
            model = glm::scale(model, glm::dvec3(bodies[0].averageRadius/sscale));

            lightShader.setMat4("model", model);
            lightShader.setMat4("view", view);
            lightShader.setMat4("projection", projection);
            renderSphere(false);
            // -----------------------------------------------------------------------------------------------------------------------------

            std::cout << "TEST 12\n";

            shaderTex.use();

            // -----------------------------------------------------------------------------------------------------------------------------
            // Earth
            static double rotationAngle = 0.0;
            rotationAngle += glm::radians(0.0041667) * (deltaTime * 100);

            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[1].position.x/sscale - camera.Position.x, bodies[1].position.y/sscale - camera.Position.y, bodies[1].position.z/sscale - camera.Position.z));
            model = glm::scale(model, glm::dvec3(bodies[1].averageRadius/sscale));
            //model = glm::rotate(model, glm::radians(23.44), glm::dvec3(1.0, 0.0, 0.0)); // axial tilt
            model = glm::rotate(model, rotationAngle,       glm::dvec3(0.0, 1.0, 0.0)); // rotation around axis

            shaderTex.setBool("flipHor", true);
            shaderTex.setBool("skipRM", false);
            shaderTex.setMat4("model", model);

            //glm::mat3 rotationMatrix = glm::mat3(glm::vec3(glm::cos(rotationAngle), 0.0f, glm::sin(rotationAngle)), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-glm::sin(rotationAngle), 0.0f, glm::cos(rotationAngle)));
            glm::mat3 rotationMatrix = glm::mat3(glm::vec3(glm::cos(rotationAngle), 0.0f, glm::sin(rotationAngle)), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-glm::sin(rotationAngle), 0.0f, glm::cos(rotationAngle))); // rotation matrix with axial tilt

            shaderTex.setMat3("rotationMatrix", rotationMatrix);

            renderSphere(true);
            // -----------------------------------------------------------------------------------------------------------------------------

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mercurySurfaceTex);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Mercury
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[1].position.x/sscale - camera.Position.x, bodies[1].position.y/sscale - camera.Position.y, bodies[1].position.z/sscale - camera.Position.z));
            //model = glm::scale(model, glm::dvec3(bodies[1].radius/sscale));

            shaderTex.setBool("skipRM", true);
            shaderTex.setMat4("model", model);
            shaderTex.setMat3("rotationMatrix", glm::mat3(1.0f));
            //renderSphere(true);
            // -----------------------------------------------------------------------------------------------------------------------------

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, venusAtmoTex);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Venus
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[2].position.x/sscale - camera.Position.x, bodies[2].position.y/sscale - camera.Position.y, bodies[2].position.z/sscale - camera.Position.z));
            //model = glm::scale(model, glm::dvec3(bodies[2].radius/sscale));

            shaderTex.setMat4("model", model);
            renderSphere(true);
            // -----------------------------------------------------------------------------------------------------------------------------

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, moonSurfaceTex);
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, moonHeightTex);
            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_2D, moonNormalTex);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Moon
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[2].position.x/sscale - camera.Position.x, bodies[2].position.z/sscale - camera.Position.z, bodies[2].position.y/sscale - camera.Position.y));
            model = glm::scale(model, glm::dvec3(bodies[2].averageRadius/sscale));

            shaderTex.setMat4("model", model);
            renderSphere(true);
            // -----------------------------------------------------------------------------------------------------------------------------

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, marsSurfaceTex);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Mars
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[5].position.x/sscale - camera.Position.x, bodies[5].position.y/sscale - camera.Position.y, bodies[5].position.z/sscale - camera.Position.z));
            
            //model = glm::scale(model, glm::dvec3(bodies[5].radius/sscale));

            shaderTex.setMat4("model", model);
            //renderSphere(true);
            // -----------------------------------------------------------------------------------------------------------------------------

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, jupiterAtmoTex);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Jupiter
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[6].position.x/sscale - camera.Position.x, bodies[6].position.y/sscale - camera.Position.y, bodies[6].position.z/sscale - camera.Position.z));
            
            //model = glm::scale(model, glm::dvec3(bodies[6].radius/sscale, 66854000/sscale, bodies[6].radius/sscale));

            shaderTex.setMat4("model", model);
            //renderSphere(true);
            // -----------------------------------------------------------------------------------------------------------------------------

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, saturnAtmoTex);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Saturn
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[7].position.x/sscale - camera.Position.x, bodies[7].position.y/sscale - camera.Position.y, bodies[7].position.z/sscale - camera.Position.z));
            
            //model = glm::scale(model, glm::dvec3(bodies[7].radius/sscale));

            shaderTex.setMat4("model", model);
            //renderSphere(true);
            // -----------------------------------------------------------------------------------------------------------------------------

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, uranusAtmoTex);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Uranus
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[8].position.x/sscale - camera.Position.x, bodies[8].position.y/sscale - camera.Position.y, bodies[8].position.z/sscale - camera.Position.z));
            
            //model = glm::scale(model, glm::dvec3(bodies[8].radius/sscale));

            shaderTex.setMat4("model", model);
            //renderSphere(true);
            // -----------------------------------------------------------------------------------------------------------------------------

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, neptuneAtmoTex);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Neptune
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[9].position.x/sscale - camera.Position.x, bodies[9].position.y/sscale - camera.Position.y, bodies[9].position.z/sscale - camera.Position.z));
            
            //model = glm::scale(model, glm::dvec3(bodies[9].radius/sscale));

            shaderTex.setMat4("model", model);
            //renderSphere(true);
            // -----------------------------------------------------------------------------------------------------------------------------

            // MOONS (excluding Luna)

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ioSurfaceTex);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Jupiter/Io
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[10].position.x/sscale - camera.Position.x, bodies[10].position.y/sscale - camera.Position.y, bodies[10].position.z/sscale - camera.Position.z));
            
            //model = glm::scale(model, glm::dvec3(bodies[10].radius/sscale));

            shaderTex.setMat4("model", model);
            //renderSphere(true);
            // -----------------------------------------------------------------------------------------------------------------------------

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, europaSurfaceTex);
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, europaHeightTex);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Jupiter/Europa
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[11].position.x/sscale - camera.Position.x, bodies[11].position.y/sscale - camera.Position.y, bodies[11].position.z/sscale - camera.Position.z));
            
            //model = glm::scale(model, glm::dvec3(bodies[11].radius/sscale));

            shaderTex.setMat4("model", model);
            //renderSphere(true);
            // -----------------------------------------------------------------------------------------------------------------------------

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ganymedeSurfaceTex);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Jupiter/Ganymede
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[12].position.x/sscale - camera.Position.x, bodies[12].position.y/sscale - camera.Position.y, bodies[12].position.z/sscale - camera.Position.z));
            
            //model = glm::scale(model, glm::dvec3(bodies[12].radius/sscale));

            shaderTex.setMat4("model", model);
            //renderSphere(true);
            // -----------------------------------------------------------------------------------------------------------------------------

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ganymedeSurfaceTex);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Jupiter/Callisto
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[13].position.x/sscale - camera.Position.x, bodies[13].position.y/sscale - camera.Position.y, bodies[13].position.z/sscale - camera.Position.z));
            
            //model = glm::scale(model, glm::dvec3(bodies[13].radius/sscale));

            shaderTex.setMat4("model", model);
            //renderSphere(true);
            // -----------------------------------------------------------------------------------------------------------------------------

            // render skybox
            // set depth function to GL_LEQUAL so fragment passes if it is equal to previous value
            glDepthFunc(GL_LEQUAL);
            // setup matrices and other values
            skyboxShader.use();
            //skyboxShader.setFloat("exposure", 1.0);
            skyboxShader.setInt("environmentMap", 0);
            skyboxShader.setMat4("view", view);
            skyboxShader.setMat4("projection", projection);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
                
            glBindVertexArray(skyboxVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // set depth func to GL_LESS back
            glDepthFunc(GL_LESS);


            atmosphereShader.use();
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, depthBuffer);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, colorBuffer);

            
            // disable writing to depth buffer
            glDepthMask(GL_FALSE);

            atmosphereShader.setVec3("planetWorldPos", glm::vec3(bodies[1].position.x/sscale - camera.Position.x, bodies[1].position.y/sscale - camera.Position.y, bodies[1].position.z/sscale - camera.Position.z));
            atmosphereShader.setFloat("planetRadius", bodies[1].averageRadius/sscale);
            atmosphereShader.setVec3("wavelengths", glm::vec3(650.0f, 570.0f, 475.0f));
            atmosphereShader.setFloat("atmosphereHeight", 100000.0f);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Earth's atmosphere
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[1].position.x/sscale - camera.Position.x, bodies[1].position.y/sscale - camera.Position.y, bodies[1].position.z/sscale - camera.Position.z));
            
            model = glm::scale(model, glm::dvec3(bodies[1].averageRadius/sscale+1000000.0f));

            atmosphereShader.setMat4("model", model);
            renderSphere(false);
            // -----------------------------------------------------------------------------------------------------------------------------

            atmosphereShader.setVec3("planetWorldPos", glm::dvec3(bodies[5].position.x/sscale - camera.Position.x, bodies[5].position.y/sscale - camera.Position.y, bodies[5].position.z/sscale - camera.Position.z));
            atmosphereShader.setFloat("planetRadius", bodies[5].averageRadius/sscale);
            atmosphereShader.setVec3("wavelengths", glm::vec3(600.0f, 780.0f, 800.0f));
            atmosphereShader.setFloat("atmosphereHeight", 44000.0f);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Mars' atmosphere
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[5].position.x/sscale - camera.Position.x, bodies[5].position.y/sscale - camera.Position.y, bodies[5].position.z/sscale - camera.Position.z));
            
            model = glm::scale(model, glm::dvec3(bodies[5].averageRadius/sscale+44000.0f));

            atmosphereShader.setMat4("model", model);
            renderSphere(false);
            // -----------------------------------------------------------------------------------------------------------------------------

            glDepthMask(GL_TRUE); // enable writing back

            // render players
            // --------------
            std::cout << "Total players: " << playerPositions.size() << std::endl;
            lightShader.use();
            for (const auto& [clientUID, pos] : playerPositions) {
                if (clientUID == uid) continue;
                std::cout << "Player " << clientUID << " position: " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
                model = glm::dmat4(1.0);
                model = glm::translate(model, glm::dvec3(pos - camera.Position));
                model = glm::scale(model, glm::dvec3(1.81));
                lightShader.setMat4("model", model);
                renderSphere(false);
            }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        /*glDepthFunc(GL_LEQUAL);
        // setup matrices and other values
        skyboxShader.use();
        skyboxShader.setFloat("exposure", 1.0);
        // always put skybox on the player (camera) position
        view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            
        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glDepthFunc(GL_LESS);*/

        hdrShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        renderQuad(1.0f, -1.0f);

        //ImGui::Render();
        //ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // end ImGui session
    // -----------------
    //ImGui_ImplOpenGL3_Shutdown();
    //ImGui_ImplGlfw_Shutdown();
    //ImGui::DestroyContext();

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    //glDeleteVertexArrays(1, &floorVAO);
    glDeleteVertexArrays(1, &skyboxVAO);
    //glDeleteBuffers(1, &floorVBO);
    glDeleteBuffers(1, &skyboxVBO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    // close window if escape key is pressed
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // movement controls
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path, bool gamma_correction)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (gamma_correction) {
            if (nrComponents == 3)
                format = GL_SRGB;
            else if (nrComponents == 4)
                format = GL_SRGB_ALPHA;
        } else {
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        if (gamma_correction) {
            if (format == GL_SRGB_ALPHA)
                glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            else
                glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front) 
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemap(vector<std::string> faces, bool gamma_correction)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            if (gamma_correction) {
                if (nrChannels == 4) {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB_ALPHA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                } else if (nrChannels == 3) {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                }
            } else {
                if (nrChannels == 4) {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                } else if (nrChannels == 3) {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                }
            }
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void renderQuad(float scale, float z_offset) {
    float quadVertices[] = {
      // positions                                tex coords
        -1.0f*scale, -1.0f*scale, 0.0f+z_offset,  0.0f, 0.0f,
        -1.0f*scale,  1.0f*scale, 0.0f+z_offset,  0.0f, 1.0f,
         1.0f*scale, -1.0f*scale, 0.0f+z_offset,  1.0f, 0.0f,
         1.0f*scale,  1.0f*scale, 0.0f+z_offset,  1.0f, 1.0f
    };

    //glDisable(GL_DEPTH_TEST);
    //glDepthMask(GL_FALSE);

    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    //glEnable(GL_DEPTH_TEST);
    //glDepthMask(GL_TRUE);
}

unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere(bool patches)
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        /*bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y       * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y       * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }*/

        indices.clear();
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            for (unsigned int x = 0; x < X_SEGMENTS; ++x)
            {
                indices.push_back(y       * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back(y       * (X_SEGMENTS + 1) + x + 1);

                indices.push_back(y       * (X_SEGMENTS + 1) + x + 1);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
            }
        }
        indexCount = static_cast<unsigned int>(indices.size());

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);           
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);        
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));        
    }

    glBindVertexArray(sphereVAO);
    if (!patches)
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    else
        glDrawElements(GL_PATCHES, indexCount, GL_UNSIGNED_INT, 0);
}

void loadHDRtexture(const char* hdrPath, Shader& equirectangularShader, glm::mat4 captureProj, unsigned int &captureFBO, glm::mat4 captureViews[], unsigned int &envCubemap, unsigned int cubeVAO, unsigned int skyboxVAO, unsigned int &captureRBO, Shader& convShader, Shader& prefilterShader, unsigned int &irradianceMap, unsigned int &prefilterMap, unsigned int &brdfLUTTexture)
{
    stbi_set_flip_vertically_on_load(true);
                int width, height, nrComponents;
                float *data = stbi_loadf(hdrPath, &width, &height, &nrComponents, 0);
                unsigned int hdrTexture;
                if (data) {
                    glGenTextures(1, &hdrTexture);
                    glBindTexture(GL_TEXTURE_2D, hdrTexture);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    stbi_image_free(data);
                }
                stbi_set_flip_vertically_on_load(false);

                equirectangularShader.use();
                equirectangularShader.setInt("equirectangularMap", 0);
                equirectangularShader.setMat4("projection", captureProj);
                equirectangularShader.setFloat("exposure", 1.0f);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, hdrTexture);

                glViewport(0, 0, 512, 512);
                glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
                for (unsigned int i = 0; i < 6; ++i) {
                    equirectangularShader.setMat4("view", captureViews[i]);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    glBindVertexArray(cubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
                glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

                glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
                glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 32, 32);

                convShader.use();
                convShader.setInt("environmentMap", 0);
                convShader.setMat4("projection", captureProj);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

                glViewport(0, 0, 32, 32);
                glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
                for (unsigned int i = 0; i < 6; ++i) {
                    convShader.setMat4("view", captureViews[i]);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    glBindVertexArray(cubeVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

                prefilterShader.use();
                prefilterShader.setInt("environmentMap", 0);
                prefilterShader.setMat4("projection", captureProj);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

                glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
                unsigned int maxMipLevels = 5;
                for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
                    unsigned int mipWidth = 128 * std::pow(0.5, mip);
                    unsigned int mipHeight = 128 * std::pow(0.5, mip);
                    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
                    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, mipWidth, mipHeight);
                    glViewport(0, 0, mipWidth, mipHeight);

                    float roughness = (float)mip / (float)(maxMipLevels - 1);
                    prefilterShader.setFloat("roughness", roughness);
                    for (unsigned int i = 0; i < 6; ++i) {
                        prefilterShader.setMat4("view", captureViews[i]);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        glBindVertexArray(skyboxVAO);
                        glDrawArrays(GL_TRIANGLES, 0, 36);
                    }
                }
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
                glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 512, 512);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
