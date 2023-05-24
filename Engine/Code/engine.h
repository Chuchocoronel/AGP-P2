//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>

#include "mesh.h"
#include "material.h"
#include "model.h"
#include "vertex.h"
#include "Camera.h"
#include "entity.h"
#include "buffer.h"
#include "Light.h"
#include "framebuffer.h"

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; // What is this for?
    VertexShaderLayout vertexInputLayout;
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_Model,
    Mode_Count
};

struct OpenGLInfo
{
    std::string glVersion;
    std::string glRenderer;
    std::string glVendor;
    std::string glShadingVersion;
    std::vector<std::string> glExtensions;
};

struct VertexV3V2
{
    glm::vec3 pos;
    glm::vec2 uv;
};

const VertexV3V2 vertices[] = {
    { glm::vec3(-1.0, -1.0, 0.0), glm::vec2(0.0, 0.0) }, // bottom-left vertex
    { glm::vec3(1.0, -1.0, 0.0), glm::vec2(1.0, 0.0) }, // bottom-right vertex
    { glm::vec3(1.0, 1.0, 0.0), glm::vec2(1.0, 1.0) }, // top-right vertex
    { glm::vec3(-1.0, 1.0, 0.0), glm::vec2(0.0, 1.0) }, // top-left vertex
};

const u16 indices[] = {
    0, 1, 2,
    0, 2, 3
};

struct App
{
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    std::vector<Texture>  textures;
    std::vector<Material> materials;
    std::vector<Mesh> meshes;
    std::vector<Model> models;
    std::vector<Program>  programs;

    u32 pointLightModel;
    u32 directionalLightModel;

    // program indices
    u32 texturedGeometryProgramIdx;
    u32 texturedGeometryProgramIdx2;
    u32 texturedGeometryProgramIdx3;
    u32 texturedGeometryProgramIdx4;
    u32 texturedMeshProgramIdx;
    
    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    std::vector<Entity> entities;

    // Mode
    Mode mode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    GLuint embeddedVertices2;
    GLuint embeddedElements2;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;
    GLuint texturedMeshProgram_uTexture;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    GLuint vao2;

    OpenGLInfo glInfo;

    Camera cam;

    std::vector<Light>lights;

    GLint maxUniformBufferSize;
    GLint uniformBlockAligment;

    Buffer buffer;
    Buffer bufferGlobals;

    u32 globalParamsOffset;
    u32 globalParamsSize;

    Framebuffer fbuffer;
    Framebuffer deferredFBuffer;

    int renderTarget;
    int renderMode;
};

u32 LoadTexture2D(App* app, const char* filepath);

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);