#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <cstring>

// Helper functions for float-uint conversion (outside the class)
uint32_t floatBitsToUint(float f) {
    union FloatUint { float f; uint32_t u; };
    FloatUint converter;
    converter.f = f;
    return converter.u;
}

float uintBitsToFloat(uint32_t u) {
    union FloatUint { float f; uint32_t u; };
    FloatUint converter;
    converter.u = u;
    return converter.f;
}

class HeightMapReducer {
private:
    GLuint computeProgram;
    GLuint heightTexture;
    GLuint resultSSBO;
    int textureWidth, textureHeight;

    std::string loadShaderSource(const std::string& filename) {
        std::ifstream file(filename);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
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

    void createComputeShader() {
        std::string computeShaderSource = R"(
#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, r32f) uniform readonly image2D heightMap;

layout(std430, binding = 0) buffer ResultBuffer {
    float results[];
} resultBuffer;

// Shared memory for local reduction
shared float localMin[256];
shared float localMax[256];

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(heightMap);

    uint localIndex = gl_LocalInvocationIndex;
    uint workGroupIndex = gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x;

    // Initialize shared memory
    localMin[localIndex] = 1e30;  // Very large number
    localMax[localIndex] = -1e30; // Very small number

    // Load height value if within bounds
    if (coord.x < imageSize.x && coord.y < imageSize.y) {
        float height = imageLoad(heightMap, coord).r;
        localMin[localIndex] = height;
        localMax[localIndex] = height;
    }

    barrier();

    // Parallel reduction in shared memory
    for (uint stride = 128; stride > 0; stride >>= 1) {
        if (localIndex < stride) {
            localMin[localIndex] = min(localMin[localIndex], localMin[localIndex + stride]);
            localMax[localIndex] = max(localMax[localIndex], localMax[localIndex + stride]);
        }
        barrier();
    }

    // Write result from first thread in workgroup
    if (localIndex == 0) {
        resultBuffer.results[workGroupIndex * 2] = localMin[0];
        resultBuffer.results[workGroupIndex * 2 + 1] = localMax[0];
    }
}
)";

        GLuint computeShader = compileShader(computeShaderSource, GL_COMPUTE_SHADER);

        computeProgram = glCreateProgram();
        glAttachShader(computeProgram, computeShader);
        glLinkProgram(computeProgram);

        GLint success;
        glGetProgramiv(computeProgram, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(computeProgram, 512, nullptr, infoLog);
            std::cerr << "Program linking failed: " << infoLog << std::endl;
        }

        glDeleteShader(computeShader);
    }

    void createHeightTexture(const std::vector<float>& heightData, int width, int height) {
        textureWidth = width;
        textureHeight = height;

        glGenTextures(1, &heightTexture);
        glBindTexture(GL_TEXTURE_2D, heightTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, heightData.data());

        glBindImageTexture(0, heightTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    }

    void createResultBuffer() {
        // Calculate number of workgroups
        int groupsX = (textureWidth + 15) / 16;
        int groupsY = (textureHeight + 15) / 16;
        int totalGroups = groupsX * groupsY;

        // Each workgroup writes 2 floats (min, max)
        size_t bufferSize = totalGroups * 2 * sizeof(float);

        glGenBuffers(1, &resultSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, resultSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, nullptr, GL_DYNAMIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, resultSSBO);
    }

public:
    HeightMapReducer() : computeProgram(0), heightTexture(0), resultSSBO(0) {}

    ~HeightMapReducer() {
        cleanup();
    }

    bool initialize() {
        createComputeShader();
        return true;
    }

    void setHeightData(const std::vector<float>& heightData, int width, int height) {
        createHeightTexture(heightData, width, height);
        createResultBuffer(); // Create buffer after we know the texture size
    }

    std::pair<float, float> findMinMax() {
        // Dispatch compute shader
        glUseProgram(computeProgram);

        int groupsX = (textureWidth + 15) / 16;
        int groupsY = (textureHeight + 15) / 16;

        glDispatchCompute(groupsX, groupsY, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Read back results
        int totalGroups = groupsX * groupsY;
        std::vector<float> results(totalGroups * 2);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, resultSSBO);
        void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        if (ptr) {
            memcpy(results.data(), ptr, totalGroups * 2 * sizeof(float));
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }

        // Find global min/max from workgroup results
        float globalMin = 1e30f;
        float globalMax = -1e30f;

        for (int i = 0; i < totalGroups; ++i) {
            float workgroupMin = results[i * 2];
            float workgroupMax = results[i * 2 + 1];

            // Skip uninitialized values (from workgroups that didn't process any pixels)
            if (workgroupMin < 1e29f) {
                globalMin = std::min(globalMin, workgroupMin);
                globalMax = std::max(globalMax, workgroupMax);
            }
        }

        return std::make_pair(globalMin, globalMax);
    }

    void cleanup() {
        if (computeProgram) glDeleteProgram(computeProgram);
        if (heightTexture) glDeleteTextures(1, &heightTexture);
        if (resultSSBO) glDeleteBuffers(1, &resultSSBO);
    }
};

// Helper function to generate sample height data
std::vector<float> generateSampleHeightMap(int width, int height) {
    std::vector<float> heights(width * height);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Generate some interesting height data (sine waves)
            float fx = static_cast<float>(x) / width;
            float fy = static_cast<float>(y) / height;

            float height1 = sin(fx * 10.0f) * cos(fy * 8.0f);
            float height2 = sin(fx * 20.0f + fy * 15.0f) * 0.5f;

            heights[y * width + x] = height1 + height2 + 2.0f; // Offset to make positive
        }
    }

    return heights;
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create OpenGL context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window for compute-only

    GLFWwindow* window = glfwCreateWindow(1, 1, "Height Map Reducer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Check for compute shader support
    if (!GLEW_ARB_compute_shader) {
        std::cerr << "Compute shaders not supported" << std::endl;
        return -1;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // Create height map reducer
    HeightMapReducer reducer;
    if (!reducer.initialize()) {
        std::cerr << "Failed to initialize height map reducer" << std::endl;
        return -1;
    }

    // Generate sample height data
    const int WIDTH = 512;
    const int HEIGHT = 512;
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

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
