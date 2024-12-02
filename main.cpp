#define STB_IMAGE_IMPLEMENTATION
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <stb/stb_image.h>

static void mouse_callback(GLFWwindow* window, double xpos, double ypos);

class HikingSimulator {
private:
    GLFWwindow* window;
    std::vector<glm::vec3> hikingPath;
    GLuint terrainVAO, terrainVBO;
    GLuint pathVAO, pathVBO;
    GLuint shaderProgram;
    GLuint terrainTexture;

    

    // Camera variables
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;

    struct TerrainData {
        std::vector<float> heights;
        int width;
        int height;
    } terrain;

    std::vector<GLfloat> terrainVertices;
    std::vector<GLuint> terrainIndices;

    // Mouse control variables
    double lastX = 400, lastY = 300;
    float yaw = -45.0f;
    float pitch = -35.0f;
    bool firstMouse = true;

    // Hiker variables
    float hikerProgress = 0.0f;
    GLuint hikerVAO, hikerVBO;
    float animationSpeed = 0.0001f;

public:
    
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
        void* ptr = glfwGetWindowUserPointer(window);
        if (ptr) {
            HikingSimulator* simulator = static_cast<HikingSimulator*>(ptr);
            simulator->processMouseMovement(xpos, ypos);
        }
    }

    void createHikerMarker() {
        // Simple pyramid to represent the hiker
        std::vector<GLfloat> hikerVertices = {
            // Base
            -1.0f, 0.0f, -1.0f,
             1.0f, 0.0f, -1.0f,
             1.0f, 0.0f,  1.0f,
            -1.0f, 0.0f,  1.0f,
            // Top
             0.0f, 2.0f,  0.0f
        };

        std::vector<GLuint> hikerIndices = {
            // Base
            0, 1, 2,
            0, 2, 3,
            // Sides
            0, 4, 1,
            1, 4, 2,
            2, 4, 3,
            3, 4, 0
        };

        glGenVertexArrays(1, &hikerVAO);
        glGenBuffers(1, &hikerVBO);
        GLuint hikerEBO;
        glGenBuffers(1, &hikerEBO);

        glBindVertexArray(hikerVAO);

        glBindBuffer(GL_ARRAY_BUFFER, hikerVBO);
        glBufferData(GL_ARRAY_BUFFER, hikerVertices.size() * sizeof(GLfloat),
            hikerVertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hikerEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, hikerIndices.size() * sizeof(GLuint),
            hikerIndices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0);
    }

    glm::vec3 getHikerPosition() {
        if (hikingPath.size() < 2) return glm::vec3(0.0f);

        float totalLength = hikingPath.size() - 1;
        float currentSegment = hikerProgress * totalLength;
        int currentIndex = static_cast<int>(currentSegment);
        float t = currentSegment - currentIndex;

        if (currentIndex >= hikingPath.size() - 1) {
            return hikingPath.back();
        }

        return glm::mix(hikingPath[currentIndex], hikingPath[currentIndex + 1], t);
    }

    HikingSimulator() {
        initGLFW();
        initOpenGL();
        loadTerrain("E:/VR_Project/terrain and hiking data/høydedata.png");
        loadHikingPath("E:/VR_Project/terrain and hiking data/hiker_path.txt");
        setupShaders();
        createHikerMarker();
    }
    void processMouseMovement(double xpos, double ypos) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;

        float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(direction);
    }

    void run() {
        while (!glfwWindowShouldClose(window)) {
            processInput();
            render();
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    void processInput() {
        // Add mouse button check at the beginning
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
            return;
        }

        float cameraSpeed = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            cameraPos += cameraUp * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            cameraPos -= cameraUp * cameraSpeed;
        // Control animation speed
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            animationSpeed *= 0.01f;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            animationSpeed *= 0.01f;

        // Reset animation
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
            hikerProgress = 0.0f;
    }

        
    
    

    void initGLFW() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(1024, 768, "Hiking Simulator", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(window);
        glfwSetWindowUserPointer(window, this);
        glfwSetCursorPosCallback(window, mouse_callback);  
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    

    void initOpenGL() {
        if (glewInit() != GLEW_OK) {
            throw std::runtime_error("Failed to initialize GLEW");
        }

        glEnable(GL_DEPTH_TEST);
        setupCamera();
    }

    void setupCamera() {
        cameraPos = glm::vec3(0.0f, 150.0f, 150.0f);
        cameraFront = glm::vec3(0.0f, -0.5f, -0.5f);
        cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    void loadHikingPath(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open hiking path file");
        }

        float x, y, z;
        while (file >> x >> y >> z) {
            hikingPath.push_back(glm::vec3(x, y, z));
        }

        // Create and bind path VAO/VBO
        glGenVertexArrays(1, &pathVAO);
        glGenBuffers(1, &pathVBO);

        glBindVertexArray(pathVAO);
        glBindBuffer(GL_ARRAY_BUFFER, pathVBO);
        glBufferData(GL_ARRAY_BUFFER, hikingPath.size() * sizeof(glm::vec3),
            hikingPath.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
    }

    void loadTerrain(const std::string& heightmapFile) {
        int channels;
        unsigned char* data = stbi_load(heightmapFile.c_str(),
            &terrain.width, &terrain.height,
            &channels, STBI_grey);

        if (!data) {
            throw std::runtime_error("Failed to load terrain heightmap");
        }

        // Convert heightmap data to float values (0.0 to 1.0)
        terrain.heights.resize(terrain.width * terrain.height);
        for (int i = 0; i < terrain.width * terrain.height; i++) {
            terrain.heights[i] = data[i] / 255.0f;
        }

        stbi_image_free(data);
        generateTerrainMesh();
    }

    void generateTerrainMesh() {
        // Adjust scale factors for more mountainous appearance
        const float scaleY = 100.0f;    // Height scale
        const float scaleXZ = 0.5f;     // Horizontal scale - make terrain more compact

        // Clear previous data
        terrainVertices.clear();
        terrainIndices.clear();

        // Generate vertices and texture coordinates
        for (int z = 0; z < terrain.height; z++) {
            for (int x = 0; x < terrain.width; x++) {
                float height = terrain.heights[z * terrain.width + x];

                // Normalize coordinates to center the terrain
                float normalizedX = (x - terrain.width / 2.0f) * scaleXZ;
                float normalizedZ = (z - terrain.height / 2.0f) * scaleXZ;

                // Vertex position
                terrainVertices.push_back(normalizedX);           // x
                terrainVertices.push_back(height * scaleY);       // y
                terrainVertices.push_back(normalizedZ);           // z

                // Texture coordinates (used for height-based coloring)
                terrainVertices.push_back((float)x / terrain.width);
                terrainVertices.push_back(height);
            }
        }

        // Generate indices
        for (int z = 0; z < terrain.height - 1; z++) {
            for (int x = 0; x < terrain.width - 1; x++) {
                int topLeft = z * terrain.width + x;
                int topRight = topLeft + 1;
                int bottomLeft = (z + 1) * terrain.width + x;
                int bottomRight = bottomLeft + 1;

                // First triangle
                terrainIndices.push_back(topLeft);
                terrainIndices.push_back(bottomLeft);
                terrainIndices.push_back(topRight);

                // Second triangle
                terrainIndices.push_back(topRight);
                terrainIndices.push_back(bottomLeft);
                terrainIndices.push_back(bottomRight);
            }
        }

        // Create and bind terrain VAO/VBO/EBO
        glGenVertexArrays(1, &terrainVAO);
        glGenBuffers(1, &terrainVBO);
        GLuint terrainEBO;
        glGenBuffers(1, &terrainEBO);

        glBindVertexArray(terrainVAO);

        glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
        glBufferData(GL_ARRAY_BUFFER, terrainVertices.size() * sizeof(GLfloat),
            terrainVertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, terrainIndices.size() * sizeof(GLuint),
            terrainIndices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
            (void*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
    }

   

    std::string loadShaderSource(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open shader file: " + filename);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    GLuint compileShader(const std::string& source, GLenum shaderType) {
        GLuint shader = glCreateShader(shaderType);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        // Check for compilation errors
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            throw std::runtime_error("Shader compilation failed: " + std::string(infoLog));
        }

        return shader;
    }

    void setupShaders() {
        // Load and compile vertex shader
        std::string vertexSource = loadShaderSource("vertex_shader.glsl");
        GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);

        // Load and compile fragment shader
        std::string fragmentSource = loadShaderSource("fragment_shader.glsl");
        GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);

        // Create and link shader program
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        // Check for linking errors
        GLint success;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetProgramInfoLog(shaderProgram, sizeof(infoLog), nullptr, infoLog);
            throw std::runtime_error("Shader program linking failed: " + std::string(infoLog));
        }

        // Clean up individual shaders
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void renderTerrain(const glm::mat4& view, const glm::mat4& projection) {
        glUseProgram(shaderProgram);

        // Set uniforms
        glm::mat4 model = glm::mat4(1.0f);
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw terrain
        glBindVertexArray(terrainVAO);
        glDrawElements(GL_TRIANGLES, terrainIndices.size(), GL_UNSIGNED_INT, 0);
    }

    void renderPath(const glm::mat4& view, const glm::mat4& projection) {
        glUseProgram(shaderProgram);

        // Set uniforms
        glm::mat4 model = glm::mat4(1.0f);
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw hiking path
        glBindVertexArray(pathVAO);
        glDrawArrays(GL_LINE_STRIP, 0, hikingPath.size());
    }

    void renderHiker(const glm::mat4& view, const glm::mat4& projection) {
        glUseProgram(shaderProgram);

        glm::vec3 hikerPos = getHikerPosition();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, hikerPos);
        model = glm::scale(model, glm::vec3(2.0f)); // Scale the hiker marker

        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw hiker marker
        glBindVertexArray(hikerVAO);
        glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);
    }

    void render() {
        glClearColor(0.529f, 0.808f, 0.922f, 1.0f); // outside the terrain area sky blue

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update hiker position
        hikerProgress += animationSpeed;
        if (hikerProgress > 1.0f) hikerProgress = 0.0f;

        // Get hiker position for camera following
        glm::vec3 hikerPos = getHikerPosition();

        // Update camera to follow hiker
        glm::vec3 cameraOffset = glm::vec3(50.0f, 50.0f, 50.0f);
        cameraPos = hikerPos + cameraOffset;
        cameraFront = glm::normalize(hikerPos - cameraPos);

        glm::mat4 view = glm::lookAt(cameraPos, hikerPos, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
            1024.0f / 768.0f, 0.1f, 2000.0f);

        renderTerrain(view, projection);
        renderPath(view, projection);
        renderHiker(view, projection);
    }
};

int main() {
    try {
        HikingSimulator simulator;
        simulator.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}





