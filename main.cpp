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

#include <freetype2/ft2build.h>
#include <freetype2/freetype/freetype.h>

std::vector<int> clientUIDs(1000000);

std::unordered_map<int, glm::dvec3> playerPositions;
std::mutex playerPositionsMutex;

std::vector<glm::dvec3> planetPositions(10);
std::vector<glm::dvec3> planetVelocities(10);

struct Image {
    unsigned int ImageID;
    glm::ivec2 Size;
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_click_callback(GLFWwindow* window, int button, int action);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path, bool gamma_correction);
Image loadImage(char const * path, bool gamma_correction);
unsigned int loadCubemap(vector<std::string> faces, bool gamma_correction);
glm::vec3 floatToVec3(float v[3]);
glm::vec2 floatToVec2(float v[2]);
glm::vec4 floatToVec4(float v[4]);
void loadShaderUniforms(Shader shader);
std::string toString(const char* v);
char* stringToChar(std::string v);
void addToArr(const char* arr[], char* v);
void renderQuad(float scale, float z_offset);
void renderSphere(bool patches, glm::dmat4 &model);
void RenderLine(glm::vec3 pos1, glm::vec3 pos2, const glm::mat4 &view, const glm::mat4 &projection);
void RenderText(Shader &s, std::string text, float x, float y, float scale, glm::vec3 color, bool centered);
glm::vec2 convert3Dto2D(glm::vec3 position, const glm::mat4 &viewMatrix, const glm::mat4 &projectionMatrix);
void RenderCenteredImage(Shader &s, Image image, float x, float y, float scale);
void Button(Shader &s, Image image, float x, float y, float scale, void (*action)());
void HoverButton(Shader &s, Image image, Image hoverImage, float x, float y, float scale, void (*action)());
void QuitButton(Shader &s, Image image, Image hoverImage, float x, float y, float scale, GLFWwindow* window);
void startCallback();
void optionsCallback();
void optionsCloseCallback();

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
float lastFrame = glfwGetTime();

// constants
const float sscale = 1.0f; // scale of the Solar System (1:1)

bool isServer;
int timestamp;

float pitch, yaw, roll;
glm::vec3 ndv;

float throttle = 0.0f;
bool enginesOn = false;
float fuelConsumption; // liters/s
float fuel = 3000000.0f; // in liters


struct Character {
    unsigned int TextureID;  // ID handle of the glyph texture
    glm::ivec2   Size;       // Size of glyph
    glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
    unsigned int Advance;    // Offset to advance to next glyph
};

std::map<char, Character> Characters;

// text VBO and VAO
unsigned int textVBO, textVAO;

// image VBO and VAO
unsigned int imageVBO, imageVAO;

// mouse cursor position
double mouseX;
double mouseY;

bool lmbPressed = false;
bool mmbPressed = false;
bool rmbPressed = false;

// main menu vars
bool started = false;
float transparency = 1.0f;
bool options = false;

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

    isServer = true;

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
    glfwSetMouseButtonCallback(window, (GLFWmousebuttonfun)mouse_click_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // initialize freetype
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    FT_Face face;
    if (FT_New_Face(ft, getFilePath("resources/fonts/sans-serif.ttf").c_str(), 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;  
        return -1;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
  
    for (unsigned char c = 0; c < 128; c++)
    {
        // load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character = {
            texture, 
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


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

    Shader textShader(getFilePath("shaders/text/text.vert").c_str(), getFilePath("shaders/text/text.frag").c_str());
    Shader imageShader(getFilePath("shaders/image/image.vert").c_str(), getFilePath("shaders/image/image.frag").c_str());
    Shader lineShader(getFilePath("shaders/line/line.vert").c_str(), getFilePath("shaders/line/line.frag").c_str());

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


    // text VAO and VBO
    glGenBuffers(1, &textVBO);
    glGenVertexArrays(1, &textVAO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // image VAO and VBO
    glGenBuffers(1, &imageVBO);
    glGenVertexArrays(1, &imageVAO);
    glBindVertexArray(imageVAO);
    glBindBuffer(GL_ARRAY_BUFFER, imageVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), NULL, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

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
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, 4096, 4096);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
    GL_RENDERBUFFER, captureRBO);

    unsigned int envCubemap;
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 4096, 4096, 0, GL_RGB, GL_FLOAT, nullptr);
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
    //unsigned int marsSurfaceTex = loadTexture("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/mars/mars_surface.jpg", false);
    Image marsSurfaceTex = loadImage("/Users/vyacheslav/SSE3_OpenGL/resources/textures/planets/mars/mars_surface.jpg", false);

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

    // Load UI textures
    // ----------------
    Image objectNavImg   = loadImage(getFilePath("resources/textures/UI/ objectnav.png").c_str(),               false);
    Image mmtl           = loadImage(getFilePath("resources/textures/UI/main_menu/mmtl_sse3.png").c_str(),      false);
    Image start_button   = loadImage(getFilePath("resources/textures/UI/main_menu/start_button.png").c_str(),   false);
    Image options_button = loadImage(getFilePath("resources/textures/UI/main_menu/options_button.png").c_str(), false);
    Image quit_button    = loadImage(getFilePath("resources/textures/UI/main_menu/quit_button.png").c_str(),    false);
    Image start_hover    = loadImage(getFilePath("resources/textures/UI/main_menu/start_hover.png").c_str(),    false);
    Image options_hover  = loadImage(getFilePath("resources/textures/UI/main_menu/options_hover.png").c_str(),  false);
    Image quit_hover     = loadImage(getFilePath("resources/textures/UI/main_menu/quit_hover.png").c_str(),     false);
    Image options_panel  = loadImage(getFilePath("resources/textures/UI/main_menu/options_panel.png").c_str(),  false);
    Image options_close  = loadImage(getFilePath("resources/textures/UI/main_menu/options_close.png").c_str(),  false);

    // load HDR texture
    // ----------------
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    const char* oldHdrPath = "/Users/vyacheslav/SSE3_OpenGL/resources/textures/HDR/space.jpg";
    const char* hdrPath = "/Users/vyacheslav/SSE3_OpenGL/resources/textures/HDR/space.jpg";
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

    glViewport(0, 0, 4096, 4096);
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

    std::vector<CelestialBody> bodies(3);
    if (isServer) {
        bodies = {
            CelestialBody(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 0.0), 695700000, 695508000, 695800000, 274.049, 0.0, 0.0), // Sun
            parsePlanetJSON("resources/planets/mercury.json"),
            parsePlanetJSON("resources/planets/venus.json"),
            parsePlanetJSON("resources/planets/earth.json"),
            parsePlanetJSON("resources/planets/moon.json"),
            parsePlanetJSON("resources/planets/mars.json"),
            //CelestialBody(glm::vec3(1.325642404204230E+08, 2.593482662561163E+04, -7.248338623823936E+07+6378.137+1000.0), glm::vec3(1.364897762669466E+01, -7.501512110188457E-04, 2.612917605635382E+01), 1.81, 1.81, 1.81, 0.0000000014, 0.0, 0.0), // Player
            CelestialBody(glm::vec3(1.325642404204230E+08-6371.0f-70.0f, 2.593482662561163E+04, -7.248338623823936E+07), glm::vec3(1.364897762669466E+01, -7.501512110188457E-04, 2.612917605635382E+01), 1.81, 1.81, 1.81, 0.0000000014, 0.0, 0.0), // Player
            CelestialBody(glm::vec3(-4.155962876379675e+10, -2.252909919162691e+09,  7.674747027447987e+11), glm::vec3(-1.319807488636713e+4, -5.516948900142156e-4, -8.347507826614081e-5), 69911000.0, 71492000.0, 66854000.0, 24.79, 0.0, 0.0), // Jupiter
            CelestialBody(glm::vec3( 1.426000189864588e+12, -5.468390271451207e+10, -1.203550185774606e+11), glm::vec3( 2.775345470427069e-4, -1.774901403126501e-4,  9.603681056096420e+3), 58232000.0, 60268000.0, 54364000.0, 10.44, 0.0, 0.0), // Saturn
            CelestialBody(glm::vec3( 1.543100133303414E+09, -1.079295296547151E+07,  2.476645699940574E+09), glm::vec3(-5.829935196501141E+00, 8.764722226773869E-02, 3.283761162053807E+0), 25362000.0, 25559000.0, 24973000.0, 8.690, 0.0, 0.0), // Uranus
            CelestialBody(glm::vec3( 4.469373923682106E+09, -1.033281370518243E+08,  1.586954226509337E+07), glm::vec3(-5.559040099903875E-2, -1.115959486943088E-01, 5.466737189466951E+0), 24624000.0, 24766000.0, 24342000.0, 11.15, 0.0, 0.0), // Neptune
            // Moons (excluding Luna)
            CelestialBody(glm::vec3(-1.954828792932118E+08, 1.957055843438603E+06, -1.364900477469241E+08), glm::vec3(1.299942898733208E+01,  2.312948473565051E-01, -1.713550929627585E+01), 11.08, 11.08, 11.08, 0.0057, 0.0, 0.0), // Mars/Phobos
            CelestialBody(glm::vec3(-1.954645686512529E+08, 1.948474304707609E+06, -1.365008465245946E+08), glm::vec3(1.490155449375842E+01, -7.012061170424948E-01, -1.647023847564131E+01), 6.2,   6.2,   6.2,   0.003,  0.0, 0.0), // Mars/Deimos
        };
    }

    std::vector<glm::dvec4> bodyData = {
        glm::dvec4(695700000.0, 695508000, 695800000, 274.049),
        glm::dvec4(6371000.0, 6378137.0, 6356752.0, 9.81),
        glm::dvec4(1737400.00, 1738100.0, 1736000.0, 1.620),
        glm::dvec4(0.0, 0.0, 0.0, 0.0),
        glm::dvec4(0.0, 0.0, 0.0, 0.0),
        glm::dvec4(0.0, 0.0, 0.0, 0.0),
        glm::dvec4(0.0, 0.0, 0.0, 0.0),
        glm::dvec4(0.0, 0.0, 0.0, 0.0),
        glm::dvec4(0.0, 0.0, 0.0, 0.0),
        glm::dvec4(0.0, 0.0, 0.0, 0.0)
    };

    std::vector<std::string> bodyNames = {
        "Sun",
        "Mercury",
        "Venus",
        "Earth",
        "Moon",
        "Mars",
        "that one mf",
        "Jupiter",
        "Saturn",
        "Uranus",
        "Neptune",
        "Phobos",
        "Deimos",
        "Io",
        "Europa",
        "Ganymede",
        "Callisto",
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

                // send current server timestamp
                packet = "TIMESTAMP;" + std::to_string(timestamp) + ";";
                send(accept_client_fd, packet.c_str(), packet.length(), 0);

                // send planet positions and velocities
                packet = "SUN;" + std::to_string(bodies[0].position.x) + ";" + std::to_string(bodies[0].position.y) + ";" + std::to_string(bodies[0].position.z) + ";" + std::to_string(bodies[0].velocity.x) + ";" + std::to_string(bodies[0].velocity.y) + ";" + std::to_string(bodies[0].velocity.z) + ";";
                send(accept_client_fd, packet.c_str(), packet.length(), 0);

                packet = "EARTH;" + std::to_string(bodies[1].position.x) + ";" + std::to_string(bodies[1].position.y) + ";" + std::to_string(bodies[1].position.z) + ";" + std::to_string(bodies[1].velocity.x) + ";" + std::to_string(bodies[1].velocity.y) + ";" + std::to_string(bodies[1].velocity.z) + ";";
                send(accept_client_fd, packet.c_str(), packet.length(), 0);

                packet = "MOON;" + std::to_string(bodies[2].position.x) + ";" + std::to_string(bodies[2].position.y) + ";" + std::to_string(bodies[2].position.z) + ";" + std::to_string(bodies[2].velocity.x) + ";" + std::to_string(bodies[2].velocity.y) + ";" + std::to_string(bodies[2].velocity.z) + ";";
                send(accept_client_fd, packet.c_str(), packet.length(), 0);

                if (accept_client_fd >= 0)
                    std::thread(handleClient, accept_client_fd, uid).detach();
            }
        }
    }).detach();

    int uid;
    std::thread([&]() {
        if (!isServer) {
            handleClient(client_fd, uid);
        }
    }).detach();

    if (!isServer) {
        while (planetPositions[3] == glm::dvec3(0.0f)) {
            std::cout << "Waiting...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        for (unsigned int i = 0; i < 3; i++) {
            bodies[i].position            = planetPositions[i];
            bodies[i].averageRadius       = bodyData[i].x;
            bodies[i].equatorialRadius    = bodyData[i].y;
            bodies[i].polarRadius         = bodyData[i].z;
            bodies[i].gravityAcceleration = bodyData[i].w;
        }
    }

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        timestamp += 1;

        // get cursor position
        // -------------------
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        if (timestamp == 1 && deltaTime > 0.1f)
            deltaTime = 0.1f;            
        lastFrame = currentFrame;

        float fps = 1.0f / deltaTime;

        for (auto &body : bodies)
            body.updateVelocity(bodies, deltaTime*1);

        // calculate NDV
        ndv.x = -camera.GetViewMatrix()[0][2];
        ndv.y = -camera.GetViewMatrix()[1][2];
        ndv.z = -camera.GetViewMatrix()[2][2];

        ndv = glm::normalize(ndv);

        if (!started)
            camera.Position = glm::dvec3(bodies[3].position.x/sscale + 10000000.0f, bodies[3].position.y/sscale + 7000000.0f, bodies[3].position.z/sscale + 20000000.0f);
        else
            camera.Position = glm::vec3(bodies[6].position.x/sscale, bodies[6].position.y/sscale, bodies[6].position.z/sscale);

        // input
        // -----
        processInput(window);

        // calculate thrust acceleration of the rocket and calculate total acceleration
        glm::vec3 thrustAccel = 500.0f * ndv;
        glm::vec3 totalAccel = bodies[6].totalAcceleration + thrustAccel;

        bodies[6].velocity += throttle/100 * (totalAccel * (deltaTime*100));

        for (auto &body : bodies)
            body.updatePosition(deltaTime*1);

        //moonPositions.push_back(bodies[4].position);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
            glEnable(GL_DEPTH_TEST);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100000000000000000.0f); // 100 quintillion meters far plane (1e17, or 100 trillion (1e14) km)
            glm::mat4 orthoProjection = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);
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

            // -----------------------------------------------------------------------------------------------------------------------------
            // Sun
            glm::dmat4 model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[0].position.x/sscale - camera.Position.x, bodies[0].position.y/sscale - camera.Position.y, bodies[0].position.z/sscale - camera.Position.z));
            model = glm::scale(model, glm::dvec3(bodies[0].averageRadius/sscale));

            lightShader.setMat4("model", model);
            lightShader.setMat4("view", view);
            lightShader.setMat4("projection", projection);
            renderSphere(false, model);
            // -----------------------------------------------------------------------------------------------------------------------------

            shaderTex.use();

            // -----------------------------------------------------------------------------------------------------------------------------
            // Earth
            float rotationSpeed = glm::radians(bodies[3].rotationSpeed);
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[3].position.x/sscale - camera.Position.x, bodies[3].position.y/sscale - camera.Position.y, bodies[3].position.z/sscale - camera.Position.z));
            model = glm::scale(model, glm::dvec3(bodies[3].equatorialRadius/sscale, bodies[3].polarRadius/sscale, bodies[3].equatorialRadius/sscale));
            model = glm::rotate(model, (double)glm::radians(bodies[3].axialTilt),     glm::dvec3(0.0, 0.0, 1.0)); // axial tilt
            model = glm::rotate(model, (double)rotationSpeed*timestamp*deltaTime,     glm::dvec3(0.0, 1.0, 0.0)); // rotation around axis

            shaderTex.setBool("flipHor", true);
            shaderTex.setBool("skipRM", false);
            shaderTex.setMat4("model", model);

            glm::mat3 rotationMatrixY = glm::mat3(glm::vec3(glm::cos(rotationSpeed*timestamp*deltaTime), 0.0f, glm::sin(rotationSpeed*timestamp*deltaTime)), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-glm::sin(rotationSpeed*timestamp*deltaTime), 0.0f, glm::cos(rotationSpeed*timestamp*deltaTime)));         // rotation matrix for Y axis
            glm::mat3 rotationMatrixX = glm::mat3(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, glm::cos(90.0f+bodies[3].axialTilt), -glm::sin(90.0f+bodies[3].axialTilt)), glm::vec3(0.0f, glm::sin(90.0f+bodies[3].axialTilt), glm::cos(90.0f+bodies[3].axialTilt))); // rotation matrix for X axis
            glm::mat3 rotationMatrixZ = glm::mat3(glm::vec3(glm::cos(90.0f+bodies[3].axialTilt), -glm::sin(90.0f+bodies[3].axialTilt), 0.0f), glm::vec3(glm::sin(90.0f+bodies[3].axialTilt), glm::cos(90.0f+bodies[3].axialTilt), 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // rotation matrix for Z axis
            glm::mat3 rotationMatrix = rotationMatrixY * rotationMatrixZ;
            
            shaderTex.setMat3("rotationMatrix", rotationMatrix);

            renderSphere(true, model);
            // -----------------------------------------------------------------------------------------------------------------------------

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mercurySurfaceTex);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Mercury
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[1].position.x/sscale - camera.Position.x, bodies[1].position.y/sscale - camera.Position.y, bodies[1].position.z/sscale - camera.Position.z));
            model = glm::scale(model, glm::dvec3(bodies[1].equatorialRadius/sscale, bodies[1].polarRadius/sscale, bodies[1].equatorialRadius/sscale));

            shaderTex.setBool("skipRM", true);
            shaderTex.setMat4("model", model);
            shaderTex.setMat3("rotationMatrix", glm::mat3(1.0f));
        
            renderSphere(true, model);
            // -----------------------------------------------------------------------------------------------------------------------------

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, venusAtmoTex);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Venus
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[2].position.x/sscale - camera.Position.x, bodies[2].position.y/sscale - camera.Position.y, bodies[2].position.z/sscale - camera.Position.z));
            model = glm::scale(model, glm::dvec3(bodies[2].equatorialRadius/sscale, bodies[2].polarRadius/sscale, bodies[2].equatorialRadius/sscale));

            shaderTex.setMat4("model", model);
            renderSphere(true, model);
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
            model = glm::translate(model, glm::dvec3(bodies[4].position.x/sscale - camera.Position.x, bodies[4].position.y/sscale - camera.Position.y, bodies[4].position.z/sscale - camera.Position.z));
            model = glm::scale(model, glm::dvec3(bodies[4].equatorialRadius/sscale, bodies[4].polarRadius/sscale, bodies[4].equatorialRadius/sscale));

            shaderTex.setMat4("model", model);
            renderSphere(true, model);
            // -----------------------------------------------------------------------------------------------------------------------------

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, marsSurfaceTex.ImageID);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Mars
            rotationSpeed = glm::radians(bodies[5].rotationSpeed);
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[5].position.x/sscale - camera.Position.x, bodies[5].position.y/sscale - camera.Position.y, bodies[5].position.z/sscale - camera.Position.z));
            model = glm::scale(model, glm::dvec3(bodies[5].equatorialRadius/sscale, bodies[5].polarRadius/sscale, bodies[5].equatorialRadius/sscale));
            model = glm::rotate(model, (double)glm::radians(bodies[5].axialTilt), glm::dvec3(1.0, 0.0, 0.0)); // axial tilt
            model = glm::rotate(model, 0.0,       glm::dvec3(0.0, 1.0, 0.0)); // rotation around axis

            shaderTex.setMat4("model", model);
            renderSphere(true, model);
            // -----------------------------------------------------------------------------------------------------------------------------

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, jupiterAtmoTex);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Jupiter
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[7].position.x/sscale - camera.Position.x, bodies[7].position.y/sscale - camera.Position.y, bodies[7].position.z/sscale - camera.Position.z));
            
            model = glm::scale(model, glm::dvec3(bodies[7].equatorialRadius/sscale, bodies[7].polarRadius/sscale, bodies[7].equatorialRadius/sscale));

            shaderTex.setMat4("model", model);
            renderSphere(true, model);
            // -----------------------------------------------------------------------------------------------------------------------------

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, saturnAtmoTex);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Saturn
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[8].position.x/sscale - camera.Position.x, bodies[8].position.y/sscale - camera.Position.y, bodies[8].position.z/sscale - camera.Position.z));
            
            model = glm::scale(model, glm::dvec3(bodies[8].averageRadius/sscale));

            shaderTex.setMat4("model", model);
            renderSphere(true, model);
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

            // -----------------------------------------------------------------------------------------------------------------------------
            // Mars/Phobos
            rotationSpeed = glm::radians(bodies[11].rotationSpeed);
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[11].position.x/sscale - camera.Position.x, bodies[11].position.y/sscale - camera.Position.y, bodies[11].position.z/sscale - camera.Position.z));
            model = glm::scale(model, glm::dvec3(bodies[11].equatorialRadius/sscale, bodies[11].polarRadius/sscale, bodies[11].equatorialRadius/sscale));
            model = glm::rotate(model, (double)glm::radians(bodies[11].axialTilt), glm::dvec3(1.0, 0.0, 0.0)); // axial tilt
            model = glm::rotate(model, 0.0,       glm::dvec3(0.0, 1.0, 0.0)); // rotation around axis

            shaderTex.setMat4("model", model);
            renderSphere(true, model);
            // -----------------------------------------------------------------------------------------------------------------------------

            // -----------------------------------------------------------------------------------------------------------------------------
            // Mars/Deimos
            rotationSpeed = glm::radians(bodies[12].rotationSpeed);
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[12].position.x/sscale - camera.Position.x, bodies[12].position.y/sscale - camera.Position.y, bodies[12].position.z/sscale - camera.Position.z));
            model = glm::scale(model, glm::dvec3(bodies[12].equatorialRadius/sscale, bodies[12].polarRadius/sscale, bodies[12].equatorialRadius/sscale));
            model = glm::rotate(model, (double)glm::radians(bodies[12].axialTilt), glm::dvec3(1.0, 0.0, 0.0)); // axial tilt
            model = glm::rotate(model, 0.0,       glm::dvec3(0.0, 1.0, 0.0)); // rotation around axis

            shaderTex.setMat4("model", model);
            renderSphere(true, model);
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

            atmosphereShader.setVec3("planetWorldPos", glm::vec3(bodies[3].position.x/sscale - camera.Position.x, bodies[3].position.y/sscale - camera.Position.y, bodies[3].position.z/sscale - camera.Position.z));
            atmosphereShader.setFloat("planetRadius", bodies[3].averageRadius/sscale);
            atmosphereShader.setVec3("wavelengths", glm::vec3(650.0f, 570.0f, 475.0f));
            atmosphereShader.setFloat("atmosphereHeight", 100000.0f);

            // -----------------------------------------------------------------------------------------------------------------------------
            // Earth's atmosphere
            model = glm::dmat4(1.0f);
            model = glm::translate(model, glm::dvec3(bodies[3].position.x/sscale - camera.Position.x, bodies[3].position.y/sscale - camera.Position.y, bodies[3].position.z/sscale - camera.Position.z));
            
            model = glm::scale(model, glm::dvec3(bodies[3].averageRadius/sscale+100000.0f));

            atmosphereShader.setMat4("model", model);
            renderSphere(false, model);
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
            renderSphere(false, model);
            // -----------------------------------------------------------------------------------------------------------------------------

            glDepthMask(GL_TRUE); // enable writing back

            // render players
            // --------------
            //std::cout << "Total players: " << playerPositions.size() << std::endl;
            lightShader.use();
            for (const auto& [clientUID, pos] : playerPositions) {
                if (clientUID == uid) continue;
                std::cout << "Player " << clientUID << " position: " << pos.x << ", " << pos.y << ", " << pos.z << " at my timestamp " << timestamp << std::endl;
                glm::dvec3 posDiff = pos - glm::dvec3(camera.Position);
                std::cout << "Position difference: " << posDiff.x << ", " << posDiff.y << ", " << posDiff.z << std::endl;
                model = glm::dmat4(1.0);
                model = glm::translate(model, glm::dvec3(pos - glm::dvec3(camera.Position)));
                model = glm::scale(model, glm::dvec3(1.81));
                lightShader.setMat4("model", model);
                renderSphere(false, model);
            }

            // render Moon's path
            /*lightShader.use();
            for (unsigned int i = 0; i < moonPositions.size() - 1; i++)
            {
                model = glm::dmat4(1.0);
                model = glm::translate(model, glm::dvec3(moonPositions[i]));
                RenderLine(moonPositions[i], moonPositions[i+1]);
            }*/

            // enable blending
            // ---------------
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // calculate fuel consumption and update fuel left
            // -----------------------------------------------
            fuelConsumption = throttle/100 * 12890.0f;
            if (enginesOn)
            {
                fuel -= fuelConsumption * deltaTime;
                if (fuel <= 0.0f)
                {
                    fuel = 0.0f;
                    enginesOn = false; 
                    throttle = 0.0f;
                }
            }

            // render text
            // -----------
            std::string fps_s = "FPS: " + std::to_string(fps);
            std::string throttle_s = "Throttle: " + std::to_string(throttle) + "%";
            std::string fuel_s = "Fuel: " + std::to_string(fuel / 3000000.0f * 100.0f) + "%"; // divide the left fuel by original fuel and multiply by 100 to get in percents
            textShader.use();
            textShader.setMat4("projection", orthoProjection); // switch to orthographic projection for rending text
            imageShader.use();
            imageShader.setMat4("projection", orthoProjection);
            imageShader.setFloat("transparency", 1.0f);
            if (started)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                RenderText(textShader, fps_s, 5.0f, 950.0f, 0.3f, glm::vec3(1.0f), false); // render at top-left corner at 0.3 size with white color
                RenderText(textShader, throttle_s, SCR_WIDTH/3, 10.0f, 0.5f, glm::vec3(1.0f), false);
                RenderText(textShader, enginesOn ? "Engines: on" : "Engines: off", SCR_WIDTH/2, 10.0f, 0.5f, enginesOn ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f), false);
                RenderText(textShader, fuel_s, SCR_WIDTH/2+SCR_WIDTH/3, 10.0f, 0.5f, glm::vec3(1.0f), false);
                RenderText(textShader, std::to_string(mouseX) + ", " + std::to_string(mouseY), SCR_WIDTH/2, 950, 0.3f, glm::vec3(1.0f), false);

                if (transparency != 0.0f && transparency > 0.0f)
                    transparency -= deltaTime / 5.0f;
                else
                    transparency = 0.0f;
            }

            if (options)
            {
                HoverButton(imageShader, options_close, options_close, SCR_WIDTH - SCR_WIDTH / 20.0f, SCR_HEIGHT - SCR_HEIGHT / 10.0f, 0.2f, optionsCloseCallback);
                RenderCenteredImage(imageShader, options_panel, SCR_WIDTH / 2, SCR_HEIGHT / 2 - 50.0f, 0.85f);
            }
            
            // main menu
            // ---------
            if (!started && !options)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                RenderCenteredImage(imageShader, mmtl, SCR_WIDTH / 4.5, SCR_HEIGHT - 250, 1.0f);
                HoverButton(imageShader, start_button,   start_hover,   300, 500, 0.2f, startCallback);
                HoverButton(imageShader, options_button, options_hover, 300, 420, 0.2f, optionsCallback);
                QuitButton( imageShader, quit_button,    quit_hover,    300, 340, 0.2f, window);
            }
            
            if (started)
            {
                for (unsigned int i = 0; i < 13; i++)
                {
                    if (glm::dot(glm::normalize(bodies[i].position - camera.Position), -camera.Right) > 0.0) // check if the target is in front of the camera
                    {
                        RenderCenteredImage(imageShader, objectNavImg, convert3Dto2D(camera.Position - bodies[i].position, view, projection).x, convert3Dto2D(camera.Position - bodies[i].position, view, projection).y, 0.15f);
                        RenderText(textShader, bodyNames[i], convert3Dto2D(camera.Position - bodies[i].position, view, projection).x, convert3Dto2D(camera.Position - bodies[i].position, view, projection).y + 35, 0.4f, glm::vec3(1.0f), true);
                    }
                }
            }

            // BETA: render line
            lineShader.use();
            lineShader.setMat4("projection", orthoProjection);
            RenderLine(bodies[0].position, bodies[1].position, view, projection);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        hdrShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        renderQuad(1.0f, -1.0f);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // de-allocate all resources once they've outlived their purpose
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &skyboxVAO);
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

    // rocket controls
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
        enginesOn = !enginesOn;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && throttle != 100.0f)
    {
        throttle += 50.0f * deltaTime;
        if (throttle > 100.0f)
            throttle = 100.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && throttle != 0.0f)
    {
        throttle -= 50.0f * deltaTime;
        if (throttle < 0.0f)
            throttle = 0.0f;
    }
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
    if (started)
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
}

// glfw: whenever the mouse click is detected, this callback is called
// -------------------------------------------------------------------
void mouse_click_callback(GLFWwindow* window, int button, int action)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        lmbPressed = true;
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        lmbPressed = false;
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

Image loadImage(char const * path, bool gamma_correction)
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

    Image image;
    image.ImageID = textureID;
    image.Size.x = width;
    image.Size.y = height;

    return image;
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
void renderSphere(bool patches, glm::dmat4 &model)
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

        glm::vec3 modelPos = glm::vec3(model[3][0], model[3][1], model[3][2]);
        float distance = glm::length(-camera.Position - modelPos);

        std::cout << modelPos.x << std::endl;
        std::cout << modelPos.y << std::endl;
        std::cout << modelPos.z << std::endl;

        unsigned int X_SEGMENTS;
        unsigned int Y_SEGMENTS;
        std::cout << "Distance: " << distance << std::endl;
        std::cout << "Cam pos: " << camera.Position.x << ", " << camera.Position.y << ", " << camera.Position.z << std::endl;
        if (distance < 100.0f)
        {
            X_SEGMENTS = 128.0f; 
            Y_SEGMENTS = 128.0f;
        }
        else
        {
            X_SEGMENTS = 16.0f;  
            Y_SEGMENTS = 16.0f;
        }

        X_SEGMENTS = 64.0f;
        Y_SEGMENTS = 64.0f;

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

                glm::vec3 pos = glm::vec3(xPos, yPos, zPos);
                positions.push_back(pos);
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(pos);
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

unsigned int lineVAO = 0;
void RenderLine(glm::vec3 pos1, glm::vec3 pos2, const glm::mat4 &view, const glm::mat4 &projection)
{
    if (lineVAO == 0)
    {
        glGenVertexArrays(1, &lineVAO);

        unsigned int vbo;
        glGenBuffers(1, &vbo);

        std::vector<glm::vec2> data;
        data.push_back(convert3Dto2D(camera.Position - pos1, view, projection));
        data.push_back(convert3Dto2D(camera.Position - pos2, view, projection));

        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 2 * data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        unsigned int stride = 2 * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);      
    }

    glBindVertexArray(lineVAO);
    glDrawArrays(GL_LINES, 0, 2);
}

void RenderText(Shader &s, std::string text, float x, float y, float scale, glm::vec3 color, bool centered)
{
    s.use();
    s.setVec3("textColor", color);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    if (centered)
    {
        float totalWidth = 0;

        std::string::const_iterator c2;
        for (c2 = text.begin(); c2 != text.end(); c2++)
            totalWidth += Characters[*c2].Size.x * scale;

        x = x - totalWidth / 2;
    }

    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += (ch.Advance >> 6) * scale;
    }
}

glm::vec2 convert3Dto2D(glm::vec3 position, const glm::mat4 &view, const glm::mat4 &projection)
{
    glm::vec4 projected = projection * view * glm::vec4(position, 1.0f);
    glm::vec3 newPosition = glm::vec3(projected.x/projected.w, projected.y/projected.w, projected.z/projected.w);
    newPosition.x = newPosition.x / 2.0f + 0.5f;
    newPosition.y = newPosition.y / 2.0f + 0.5f;
    return glm::vec2(newPosition.x*SCR_WIDTH, newPosition.y*SCR_HEIGHT);
}

void RenderCenteredImage(Shader &s, Image image, float x, float y, float scale)
{
    s.use();
    glBindVertexArray(imageVAO);

    float width  = image.Size.x * scale;
    float height = image.Size.y * scale;

    float vertices[6][4] = {
        {x - width / 2, y + height / 2, 0.0f, 0.0f},
        {x - width / 2, y - height / 2, 0.0f, 1.0f},
        {x + width / 2, y - height / 2, 1.0f, 1.0f},

        {x - width / 2, y + height / 2, 0.0f, 0.0f},
        {x + width / 2, y - height / 2, 1.0f, 1.0f},
        {x + width / 2, y + height / 2, 1.0f, 0.0f}
    };

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, image.ImageID);
    glBindBuffer(GL_ARRAY_BUFFER, imageVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Button(Shader &s, Image image, float x, float y, float scale, void (*action)())
{
    RenderCenteredImage(s, image, x, y, scale);
    if (mouseX > x - (image.Size.x * scale / 2) && mouseX < x + (image.Size.x * scale / 2)) // check if cursor is inside the button hitbox
    {
        if (mouseY > y + (image.Size.y * scale / 2) && mouseY < y - (image.Size.y * scale / 2))
        {
            if (lmbPressed)
                action();
        }
    }
}

void HoverButton(Shader &s, Image image, Image hoverImage, float x, float y, float scale, void (*action)())
{
    if (mouseX > x - (image.Size.x * scale / 2) && mouseX < x + (image.Size.x * scale / 2)) // check if cursor is inside the button hitbox
    {
        if ((mouseY + 80.0f) > (1040.0f - y) - (image.Size.y * scale / 2) && (mouseY + 80.0f) < (1040.0f - y) + (image.Size.y * scale / 2))
        {
            RenderCenteredImage(s, hoverImage, x, y, scale);
            if (lmbPressed)
                action();
        }
        else
        {
            RenderCenteredImage(s, image, x, y, scale);
        }
    }
    else
    {
        RenderCenteredImage(s, image, x, y, scale);
    }
}

void QuitButton(Shader &s, Image image, Image hoverImage, float x, float y, float scale, GLFWwindow* window)
{
    if (mouseX > x - (image.Size.x * scale / 2) && mouseX < x + (image.Size.x * scale / 2)) // check if cursor is inside the button hitbox
    {
        if ((960 - mouseY) > y - (image.Size.y * scale / 2) && (960 - mouseY) < y + (image.Size.y * scale / 2))
        {
            RenderCenteredImage(s, hoverImage, x, y, scale);
            if (lmbPressed)
                glfwSetWindowShouldClose(window, true);
        }
        else
        {
            RenderCenteredImage(s, image, x, y, scale);
        }
    }
    else
    {
        RenderCenteredImage(s, image, x, y, scale);
    }
}

void startCallback()
{
    started = true;
}

void optionsCallback()
{
    options = true;
}

void optionsCloseCallback()
{
    options = false;
}
