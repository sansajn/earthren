#include <SDL2/SDL.h>
#include <GLES3/gl32.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <cstring>

class HeightMapReducerES {
private:
    // SDL context
    SDL_Window* window;
    SDL_GLContext glContext;

    // Shader programs
    GLuint copyProgram;      // Copy height data to first texture
    GLuint reductionProgram; // Reduction shader

    // Textures and framebuffers
    GLuint heightTexture;    // Original height data
    GLuint pingTexture, pongTexture; // Ping-pong textures for reduction
    GLuint pingFBO, pongFBO; // Framebuffer objects

    // Geometry
    GLuint quadVAO, quadVBO;

    int textureWidth, textureHeight;
    int currentWidth, currentHeight;

    bool initializeSDL() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
            return false;
        }

        // Set OpenGL ES attributes
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

        // Create window (can be hidden for compute-only operations)
        window = SDL_CreateWindow(
            "Height Map Reducer ES",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            1, 1, // Minimal size since we don't need to display anything
            SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN
        );

        if (!window) {
            std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
            return false;
        }

        glContext = SDL_GL_CreateContext(window);
        if (!glContext) {
            std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
            return false;
        }

        // Enable VSync (optional)
        SDL_GL_SetSwapInterval(1);

        return true;
    }

    GLuint compileShader(const std::string& source, GLenum shaderType) {
        GLuint shader = glCreateShader(shaderType);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader compilation failed: " << infoLog << std::endl;
        }
        return shader;
    }

    GLuint createProgram(const std::string& vertexSource, const std::string& fragmentSource) {
        GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
        GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);

        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            std::cerr << "Program linking failed: " << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return program;
    }

    void createShaders() {
        // Vertex shader (same for both programs)
        std::string vertexShaderSource = R"(
#version 320 es
precision highp float;

layout(location = 0) in vec2 aPosition;
out vec2 vTexCoord;

void main() {
    gl_Position = vec4(aPosition, 0.0, 1.0);
    vTexCoord = aPosition * 0.5 + 0.5;
}
)";

        // Copy fragment shader - copies height data to texture
        std::string copyFragmentSource = R"(
#version 320 es
precision highp float;

uniform sampler2D uHeightTexture;
in vec2 vTexCoord;
out vec4 fragColor;

void main() {
    float height = texture(uHeightTexture, vTexCoord).r;
    // Store min in R, max in G (initially same value)
    fragColor = vec4(height, height, 0.0, 1.0);
}
)";

        // Reduction fragment shader
        std::string reductionFragmentSource = R"(
#version 320 es
precision highp float;

uniform sampler2D uInputTexture;
uniform vec2 uTexelSize;
in vec2 vTexCoord;
out vec4 fragColor;

void main() {
    vec2 texelSize = uTexelSize;

    // Sample 4 neighboring pixels (2x2 reduction)
    vec4 sample1 = texture(uInputTexture, vTexCoord + vec2(-texelSize.x * 0.5, -texelSize.y * 0.5));
    vec4 sample2 = texture(uInputTexture, vTexCoord + vec2( texelSize.x * 0.5, -texelSize.y * 0.5));
    vec4 sample3 = texture(uInputTexture, vTexCoord + vec2(-texelSize.x * 0.5,  texelSize.y * 0.5));
    vec4 sample4 = texture(uInputTexture, vTexCoord + vec2( texelSize.x * 0.5,  texelSize.y * 0.5));

    // Find min and max from the 4 samples
    float minVal = min(min(sample1.r, sample2.r), min(sample3.r, sample4.r));
    float maxVal = max(max(sample1.g, sample2.g), max(sample3.g, sample4.g));

    // Output min in R, max in G
    fragColor = vec4(minVal, maxVal, 0.0, 1.0);
}
)";

        copyProgram = createProgram(vertexShaderSource, copyFragmentSource);
        reductionProgram = createProgram(vertexShaderSource, reductionFragmentSource);
    }

    void createQuad() {
        // Full-screen quad vertices
        float quadVertices[] = {
            -1.0f, -1.0f,
             1.0f, -1.0f,
            -1.0f,  1.0f,
             1.0f,  1.0f
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    void createTextures(int width, int height) {
        currentWidth = width;
        currentHeight = height;

        // Clean up existing textures if any
        if (pingTexture) glDeleteTextures(1, &pingTexture);
        if (pongTexture) glDeleteTextures(1, &pongTexture);
        if (pingFBO) glDeleteFramebuffers(1, &pingFBO);
        if (pongFBO) glDeleteFramebuffers(1, &pongFBO);

        // Create ping-pong textures
        glGenTextures(1, &pingTexture);
        glGenTextures(1, &pongTexture);

        for (GLuint tex : {pingTexture, pongTexture}) {
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        // Create framebuffers
        glGenFramebuffers(1, &pingFBO);
        glGenFramebuffers(1, &pongFBO);

        glBindFramebuffer(GL_FRAMEBUFFER, pingFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingTexture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Ping framebuffer not complete!" << std::endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, pongFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pongTexture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Pong framebuffer not complete!" << std::endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void createHeightTexture(const std::vector<float>& heightData, int width, int height) {
        textureWidth = width;
        textureHeight = height;

        if (heightTexture) glDeleteTextures(1, &heightTexture);

        glGenTextures(1, &heightTexture);
        glBindTexture(GL_TEXTURE_2D, heightTexture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, heightData.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    void renderQuad() {
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

    void checkGLError(const std::string& operation) {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error after " << operation << ": 0x" << std::hex << error << std::endl;
        }
    }

public:
    HeightMapReducerES() : window(nullptr), glContext(nullptr),
                          copyProgram(0), reductionProgram(0), heightTexture(0),
                          pingTexture(0), pongTexture(0), pingFBO(0), pongFBO(0),
                          quadVAO(0), quadVBO(0) {}

    ~HeightMapReducerES() {
        cleanup();
    }

    bool initialize() {
        if (!initializeSDL()) {
            return false;
        }

        std::cout << "OpenGL ES Version: " << glGetString(GL_VERSION) << std::endl;
        std::cout << "GLSL ES Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

        createShaders();
        createQuad();

        checkGLError("initialization");

        return true;
    }

    void setHeightData(const std::vector<float>& heightData, int width, int height) {
        createHeightTexture(heightData, width, height);

        // Use power-of-2 dimensions for efficient reduction
        int reductionWidth = 1;
        int reductionHeight = 1;
        while (reductionWidth < width) reductionWidth *= 2;
        while (reductionHeight < height) reductionHeight *= 2;

        // Limit maximum size for memory efficiency
        reductionWidth = std::min(reductionWidth, 1024);
        reductionHeight = std::min(reductionHeight, 1024);

        createTextures(reductionWidth, reductionHeight);

        checkGLError("setHeightData");
    }

    std::pair<float, float> findMinMax() {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        // Step 1: Copy height data to ping texture
        glBindFramebuffer(GL_FRAMEBUFFER, pingFBO);
        glViewport(0, 0, currentWidth, currentHeight);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(copyProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, heightTexture);
        glUniform1i(glGetUniformLocation(copyProgram, "uHeightTexture"), 0);

        renderQuad();
        checkGLError("copy pass");

        // Step 2: Iterative reduction
        bool usePing = false; // Start with pong as target (ping has initial data)
        int width = currentWidth;
        int height = currentHeight;

        glUseProgram(reductionProgram);
        GLint texelSizeLocation = glGetUniformLocation(reductionProgram, "uTexelSize");
        GLint inputTextureLocation = glGetUniformLocation(reductionProgram, "uInputTexture");

        int passCount = 0;
        while (width > 1 || height > 1) {
            int newWidth = std::max(1, width / 2);
            int newHeight = std::max(1, height / 2);

            // Set up framebuffer and viewport
            glBindFramebuffer(GL_FRAMEBUFFER, usePing ? pingFBO : pongFBO);
            glViewport(0, 0, newWidth, newHeight);
            glClear(GL_COLOR_BUFFER_BIT);

            // Set up input texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, usePing ? pongTexture : pingTexture);
            glUniform1i(inputTextureLocation, 0);

            // Set texel size for sampling
            glUniform2f(texelSizeLocation, 2.0f / width, 2.0f / height);

            renderQuad();

            std::cout << "Reduction pass " << passCount++ << ": " << width << "x" << height
                      << " -> " << newWidth << "x" << newHeight << std::endl;

            // Swap buffers
            usePing = !usePing;
            width = newWidth;
            height = newHeight;

            checkGLError("reduction pass");
        }

        // Read final result (1x1 pixel)
        glBindFramebuffer(GL_FRAMEBUFFER, usePing ? pongFBO : pingFBO);

        float result[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, result);

        checkGLError("final read");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        std::cout << "Final result: min=" << result[0] << ", max=" << result[1] << std::endl;

        return std::make_pair(result[0], result[1]); // min, max
    }

    void cleanup() {
        if (copyProgram) glDeleteProgram(copyProgram);
        if (reductionProgram) glDeleteProgram(reductionProgram);
        if (heightTexture) glDeleteTextures(1, &heightTexture);
        if (pingTexture) glDeleteTextures(1, &pingTexture);
        if (pongTexture) glDeleteTextures(1, &pongTexture);
        if (pingFBO) glDeleteFramebuffers(1, &pingFBO);
        if (pongFBO) glDeleteFramebuffers(1, &pongFBO);
        if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
        if (quadVBO) glDeleteBuffers(1, &quadVBO);

        if (glContext) {
            SDL_GL_DeleteContext(glContext);
        }
        if (window) {
            SDL_DestroyWindow(window);
        }
        SDL_Quit();
    }
};

// Helper function to generate sample height data
std::vector<float> generateSampleHeightMap(int width, int height) {
    std::vector<float> heights(width * height);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float fx = static_cast<float>(x) / width;
            float fy = static_cast<float>(y) / height;

            float height1 = sin(fx * 10.0f) * cos(fy * 8.0f);
            float height2 = sin(fx * 20.0f + fy * 15.0f) * 0.5f;

            heights[y * width + x] = height1 + height2 + 2.0f;
        }
    }

    return heights;
}

int main() {
    HeightMapReducerES reducer;

    if (!reducer.initialize()) {
        std::cerr << "Failed to initialize height map reducer" << std::endl;
        return -1;
    }

    // Generate sample height data
    const int WIDTH = 256;
    const int HEIGHT = 256;
    std::vector<float> heightData = generateSampleHeightMap(WIDTH, HEIGHT);

    // Find CPU min/max for verification
    auto cpuMinMax = std::minmax_element(heightData.begin(), heightData.end());
    float cpuMin = *cpuMinMax.first;
    float cpuMax = *cpuMinMax.second;

    std::cout << "\nCPU Results:" << std::endl;
    std::cout << "Min height: " << cpuMin << std::endl;
    std::cout << "Max height: " << cpuMax << std::endl;

    // Set height data and find GPU min/max
    reducer.setHeightData(heightData, WIDTH, HEIGHT);
    auto gpuResult = reducer.findMinMax();

    std::cout << "\nGPU Results:" << std::endl;
    std::cout << "Min height: " << gpuResult.first << std::endl;
    std::cout << "Max height: " << gpuResult.second << std::endl;

    // Verify results
    float tolerance = 1e-5f;
    bool minMatch = std::abs(cpuMin - gpuResult.first) < tolerance;
    bool maxMatch = std::abs(cpuMax - gpuResult.second) < tolerance;

    std::cout << "\nVerification:" << std::endl;
    std::cout << "Min values match: " << (minMatch ? "YES" : "NO") << std::endl;
    std::cout << "Max values match: " << (maxMatch ? "YES" : "NO") << std::endl;

    if (minMatch && maxMatch) {
        std::cout << "✓ GPU reduction successful!" << std::endl;
    } else {
        std::cout << "✗ GPU reduction failed!" << std::endl;
        std::cout << "Difference - Min: " << std::abs(cpuMin - gpuResult.first)
                  << ", Max: " << std::abs(cpuMax - gpuResult.second) << std::endl;
    }

    return 0;
}
