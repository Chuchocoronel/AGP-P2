//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "importer.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf_s(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
    app->programs.push_back(program);

    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
    Submesh& submesh = mesh.submeshes[submeshIndex];

    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
        if (submesh.vaos[i].programHandle == program.handle)
            return submesh.vaos[i].handle;

    GLuint vaoHandle = 0;

    // Create a new vao for this submesh/program
    {
        glGenVertexArrays(1, &vaoHandle);
        glBindVertexArray(vaoHandle);

        glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

        // Link all the vertex inputs attributes to attributes in de vertex buffer
        for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
        {
            bool attributeWasLinked = false;

            for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
            {
                if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
                {
                    const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                    const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
                    const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset;
                    const u32 stride = submesh.vertexBufferLayout.stride;
                    glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
                    glEnableVertexAttribArray(index);

                    attributeWasLinked = true;
                    break;
                }
            }

            assert(attributeWasLinked);
        }

        glBindVertexArray(0);
    }

    Vao vao = { vaoHandle,program.handle };
    submesh.vaos.push_back(vao);

    return vaoHandle;
}

bool IsPowerOf2(u32 value)
{
    return value && !(value & (value - 1));
}

u32 Align(u32 value, u32 alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

Buffer CreateBuffer(u32 size, GLenum type, GLenum usage)
{
    Buffer buffer = {};
    buffer.size = size;
    buffer.type = type;

    glGenBuffers(1, &buffer.handle);
    glBindBuffer(type, buffer.handle);
    glBufferData(type, buffer.size, NULL, usage);
    glBindBuffer(type, 0);

    return buffer;
}

#define CreateConstantBuffer(size) CreateBuffer(size, GL_UNIFORM_BUFFER, GL_STREAM_DRAW)
#define CreateStaticVertexBuffer(size) CreateBuffer(size, GL_ARRAY_BUFFER, GL_STATIC_DRAW)
#define CreateStaticIndexBuffer(size) CreateBuffer(size, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW)

void BindBuffer(const Buffer& buffer)
{
    glBindBuffer(buffer.type, buffer.handle);
}

void MapBuffer(Buffer& buffer, GLenum access)
{
    glBindBuffer(buffer.type, buffer.handle);
    buffer.data = (u8*)glMapBuffer(buffer.type, access);
    buffer.head = 0;
}

void UnmapBuffer(Buffer& buffer)
{
    glUnmapBuffer(buffer.type);
    glBindBuffer(buffer.type, 0);
}

void AlignHead(Buffer& buffer, u32 alignment)
{
    ASSERT(IsPowerOf2(alignment), "The alignment must be a power of 2");
    buffer.head = Align(buffer.head, alignment);
}

void PushAlignedData(Buffer& buffer, const void* data, u32 size, u32 alignment)
{
    ASSERT(buffer.data != NULL, "The buffer must be mapped first");
    AlignHead(buffer, alignment);
    memcpy((u8*)buffer.data + buffer.head, data, size);
    buffer.head += size;
}

#define PushData(buffer, data, size) PushAlignedData(buffer, data, size, 1)
#define PushUInt(buffer, value) { u32 v = value; PushAlignedData(buffer, &v, sizeof(v), 4); }
#define PushVec3(buffer, value) PushAlignedData(buffer, value_ptr(value), sizeof(value), sizeof(vec4))
#define PushVec4(buffer, value) PushAlignedData(buffer, value_ptr(value), sizeof(value), sizeof(vec4))
#define PushMat3(buffer, value) PushAlignedData(buffer, value_ptr(value), sizeof(value), sizeof(vec4))
#define PushMat4(buffer, value) PushAlignedData(buffer, value_ptr(value), sizeof(value), sizeof(vec4))

void CreateFramebuffer(Framebuffer &fb, ivec2 &display)
{
    glGenTextures(1, &fb.colorAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, fb.colorAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, display.x, display.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &fb.positionAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, fb.positionAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, display.x, display.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &fb.normalAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, fb.normalAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, display.x, display.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &fb.depthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, fb.depthAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, display.x, display.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &fb.framebufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, fb.framebufferHandle);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb.colorAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, fb.normalAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, fb.positionAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fb.depthAttachmentHandle, 0);

    fb.framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fb.framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (fb.framebufferStatus)
        {
        case GL_FRAMEBUFFER_UNDEFINED: ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED: ELOG("GL_FRAMEBUFFER_UNSUPPORTED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: ELOG("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
        default: ELOG("Unknown framebuffer status error"); break;
        }
    }

    GLuint drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, drawBuffers);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Init(App* app)
{
    app->cam = Camera(glm::vec3(0.0f, 0.0f, 10.0f));

    // TODO: Initialize your resources here!
    // - vertex buffers
    // - element/index buffers
    // - vaos
    // - programs (and retrieve uniform indices)
    // - textures

    app->mode = Mode_Model;

    //if (app->mode == Mode_Model)
    {

    //glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_CULL_FACE);

    //Geometry
    // Display Buffer
    glGenBuffers(1, &app->embeddedVertices);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &app->embeddedElements);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Attribute state
    glGenVertexArrays(1, &app->vao);
    glBindVertexArray(app->vao);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBindVertexArray(0);

    //Framebuffer
    app->fbuffer = Framebuffer();
    app->deferredFBuffer = Framebuffer();
    CreateFramebuffer(app->fbuffer, app->displaySize);
    CreateFramebuffer(app->deferredFBuffer, app->displaySize);

    // Initialization program
    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
    Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
    texturedMeshProgram.vertexInputLayout.attributes.push_back({ 0,3 });
    texturedMeshProgram.vertexInputLayout.attributes.push_back({ 1,3 });
    texturedMeshProgram.vertexInputLayout.attributes.push_back({ 2,2 });

    app->texturedGeometryProgramIdx4 = LoadProgram(app, "shadersLight.glsl", "TEXTURED_GEOMETRY");
    Program& texturedLightProgram = app->programs[app->texturedGeometryProgramIdx4];
    texturedLightProgram.vertexInputLayout.attributes.push_back({ 0,3 });
    texturedLightProgram.vertexInputLayout.attributes.push_back({ 1,3 });
    texturedLightProgram.vertexInputLayout.attributes.push_back({ 2,2 });

    app->entities.push_back(Entity(glm::vec3(0.0f, 0.0f, 0.0f), LoadModel(app, "Patrick/Patrick.obj")));
    app->entities.push_back(Entity(glm::vec3(7.0f, 0.0f, 0.0f), LoadModel(app, "Patrick/Patrick.obj")));
    app->entities.push_back(Entity(glm::vec3(-7.0f, 0.0f, 0.0f), LoadModel(app, "Patrick/Patrick.obj")));

    app->pointLightModel = LoadModel(app, "Patrick/PointLight.obj");
    app->directionalLightModel = LoadModel(app, "Patrick/DirectionalLight.obj");

    Light light = Light();
    light.color = glm::vec3(1.0, 1.0, 1.0);
    light.direction = glm::normalize(glm::vec3(1.0, 0.0, 0.0));
    light.position = glm::vec3(11.0, 0.0, 0.0);
    light.type = LightType_Directional;

    Light light2 = Light();
    light2.color = glm::vec3(0.0, 0.0, 1.0);
    light2.position = glm::vec3(0.0, 0.0, -1.0);
    light2.type = LightType_Point;

    Light light3 = Light();
    light3.color = glm::vec3(1.0, 0.0, 0.0);
    light3.direction = glm::normalize(glm::vec3(-1.0, -1.0, 0.0));
    light3.position = glm::vec3(-3.0, -3.0, -4.0);
    light3.type = LightType_Directional;

    Light light4 = Light();
    light4.color = glm::vec3(0.0, 1.0, 0.0);
    light4.position = glm::vec3(2.0, 0.0, 2.0);
    light4.type = LightType_Point;

    Light light5 = Light();
    light5.color = glm::vec3(0.0, 1.0, 1.0);
    light5.position = glm::vec3(-2.0, 2.0, 2.0);
    light5.type = LightType_Point;

    Light light6 = Light();
    light6.color = glm::vec3(1.0, 0.3, 1.0);
    light6.position = glm::vec3(7.0, 3.0, 2.0);
    light6.type = LightType_Point;

    Light light7 = Light();
    light7.color = glm::vec3(1.0, 1.0, 0.0);
    light7.position = glm::vec3(-7.0, -2.0, 2.0);
    light7.type = LightType_Point;

    app->lights.push_back(light);
    app->lights.push_back(light2);
    app->lights.push_back(light3);
    app->lights.push_back(light4);
    app->lights.push_back(light5);
    app->lights.push_back(light6);
    app->lights.push_back(light7);

    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAligment);

    app->buffer = CreateConstantBuffer(app->maxUniformBufferSize);
    app->bufferGlobals = CreateConstantBuffer(app->maxUniformBufferSize);

    }
    //else
    {
        glGenBuffers(1, &app->embeddedVertices2);
        glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices2);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &app->embeddedElements2);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements2);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glGenVertexArrays(1, &app->vao2);
        glBindVertexArray(app->vao2);
        glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices2);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements2);
        glBindVertexArray(0);

        app->texturedGeometryProgramIdx2 = LoadProgram(app, "shaders2.glsl", "TEXTURED_GEOMETRY");
        Program& texturedGeometryProgram2 = app->programs[app->texturedGeometryProgramIdx2];
        app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram2.handle, "uTexture");

        app->texturedGeometryProgramIdx3 = LoadProgram(app, "shaders3.glsl", "TEXTURED_GEOMETRY");
    }

    // Initialization texture
    app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

    // GL INFO
    app->glInfo.glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    app->glInfo.glRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    app->glInfo.glVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    app->glInfo.glShadingVersion = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));

    GLint numExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    for (GLint i = 0; i < numExtensions; ++i)
    {
        app->glInfo.glExtensions.push_back(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, GLuint(i))));
    }

    /*glGenBuffers(1, &app->buffer.handle);
    glBindBuffer(GL_UNIFORM_BUFFER, app->buffer.handle);
    glBufferData(GL_UNIFORM_BUFFER, app->maxUniformBufferSize, NULL, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);*/
}

void Gui(App* app)
{
    //ImGui::DockSpaceOverViewport();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Render Mode"))
        {
            const char* modes[] = { "Forward", "Deferred" };
            int currentMode = app->renderMode;

            ImGui::Text("Select Render:");

            if (ImGui::Combo("##combo", &currentMode, modes, 2))
            {
                app->renderMode = currentMode;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Render target"))
        {
            const char* renders[] = { "Albedo", "Normals" , "Position", "Depth" };
            int currentRender = app->renderTarget;

            ImGui::Text("Select Mode:");

            if (ImGui::Combo("##combo", &currentRender, renders, 4))
            {
                app->renderTarget = currentRender;
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f / app->deltaTime);
    ImGui::End();

    ImGui::Begin("OpenGL Info");
    ImGui::Text("Version: %s", app->glInfo.glVersion.c_str());
    ImGui::Text("Renderer: %s", app->glInfo.glRenderer.c_str());
    ImGui::Text("Vendor: %s", app->glInfo.glVendor.c_str());
    ImGui::Text("GLSL Version: %s", app->glInfo.glShadingVersion.c_str());

    ImGui::Separator();
    ImGui::Text("Extensions");
    for (size_t i = 0; i < app->glInfo.glExtensions.size(); ++i)
    {
        ImGui::Text("%s", app->glInfo.glVersion.c_str());
    }

    ImGui::End();
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here

    app->cam.CalculateProjection(app->displaySize.x, app->displaySize.y);

    if (app->input.keys[K_W] == BUTTON_PRESSED)
        app->cam.ProcessKeyboard(FORWARD, app->deltaTime);
    if (app->input.keys[K_A] == BUTTON_PRESSED)
        app->cam.ProcessKeyboard(LEFTE, app->deltaTime);
    if (app->input.keys[K_S] == BUTTON_PRESSED)
        app->cam.ProcessKeyboard(BACKWARD, app->deltaTime);
    if (app->input.keys[K_D] == BUTTON_PRESSED)
        app->cam.ProcessKeyboard(RIGHTE, app->deltaTime);

    if (app->input.mouseButtons[LEFT] == BUTTON_PRESSED)
    {
        app->cam.ProcessMouseMovement(app->input.mouseDelta.x, -app->input.mouseDelta.y);
    }

    /*glBindBuffer(GL_UNIFORM_BUFFER, app->buffer.handle);
    app->buffer.data = (u8*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    app->buffer.head = 0;*/

    if (app->mode == Mode_Model)
    {

    MapBuffer(app->bufferGlobals, GL_WRITE_ONLY);

    app->globalParamsOffset = app->bufferGlobals.head;

    PushVec3(app->bufferGlobals, app->cam.Position);

    PushUInt(app->bufferGlobals, app->lights.size());

    for (u32 i = 0; i < app->lights.size(); ++i)
    {
        AlignHead(app->bufferGlobals, sizeof(vec4));

        Light& light = app->lights[i];
        PushUInt(app->bufferGlobals, light.type);
        PushVec3(app->bufferGlobals, light.color);
        PushVec3(app->bufferGlobals, light.direction);
        PushVec3(app->bufferGlobals, light.position);
    }

    app->globalParamsSize = app->bufferGlobals.head - app->globalParamsOffset;

    UnmapBuffer(app->bufferGlobals);

    MapBuffer(app->buffer, GL_WRITE_ONLY);

    for (std::vector<Entity>::iterator it = app->entities.begin(); it < app->entities.end(); ++it)
    {
        AlignHead(app->buffer, app->uniformBlockAligment);

        glm::mat4 worldMatrix = glm::translate((*it).pos);
       // worldMatrix = glm::scale(worldMatrix, glm::vec3(0.9));
        glm::mat4 worldViewProjection = app->cam.GetProjectionMatrix() * app->cam.GetViewMatrix() * worldMatrix;

        (*it).localParamsOffset = app->buffer.head;

        PushMat4(app->buffer, worldMatrix);

        PushMat4(app->buffer, worldViewProjection);

        (*it).localParamsSize = app->buffer.head - (*it).localParamsOffset;
    }

    UnmapBuffer(app->buffer);
    }
}

void Render(App* app)
{
    switch (app->mode)
    {
        case Mode_TexturedQuad:
        {
            // TODO: Draw your textured quad here!
            // - clear the framebuffer
            // - set the viewport
            // - set the blending state
            // - bind the texture into unit 0
            // - bind the program 
            //   (...and make its texture sample from unit 0)
            // - bind the vao
            // - glDrawElements() !!!

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx2];
            glUseProgram(programTexturedGeometry.handle);
            glBindVertexArray(app->vao2);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUniform1i(app->programUniformTexture, 0);
            glActiveTexture(GL_TEXTURE0);
            GLuint textureHandle = app->textures[app->diceTexIdx].handle;
            glBindTexture(GL_TEXTURE_2D, textureHandle);

            glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(u16), GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);
            glUseProgram(0);
        }
        break;
        case Mode_Model:
        {
            #pragma region G Buffer Pass

            glBindFramebuffer(GL_FRAMEBUFFER, app->fbuffer.framebufferHandle);

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);

            /// ENTITIES /////////////////////////////////////////////////

            Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
            glUseProgram(texturedMeshProgram.handle);

            for (std::vector<Entity>::iterator it = app->entities.begin(); it < app->entities.end(); ++it)
            {
                Model& model = app->models[(*it).modelIdx];
                Mesh& mesh = app->meshes[model.meshIdx];

                glBindBufferRange(GL_UNIFORM_BUFFER, 0, app->bufferGlobals.handle, app->globalParamsOffset, app->globalParamsSize);
                glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->buffer.handle, (*it).localParamsOffset, (*it).localParamsSize);

                for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                {
                    GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
                    glBindVertexArray(vao);

                    u32 submeshMaterialIdx = model.materialIdx[i];
                    Material& submeshMaterial = app->materials[submeshMaterialIdx];

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
                    glUniform1i(app->texturedMeshProgram_uTexture, 0);
                    //glUniformMatrix4fv(glGetUniformLocation(texturedMeshProgram.handle, "view"), 1, GL_FALSE, &app->cam.GetViewMatrix()[0][0]);
                    //glUniformMatrix4fv(glGetUniformLocation(texturedMeshProgram.handle, "proj"), 1, GL_FALSE, &app->cam.GetProjectionMatrix()[0][0]);
                    //glUniform3fv(glGetUniformLocation(texturedMeshProgram.handle, "vViewDir"), 1, glm::value_ptr(app->cam.Front));

                    Submesh& submesh = mesh.submeshes[i];
                    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                }
            }

            /// LIGHTS /////////////////////////////////////////////////

            glEnable(GL_DEPTH_TEST);
            Program& texturedLightProgram = app->programs[app->texturedGeometryProgramIdx4];
            glUseProgram(texturedLightProgram.handle);

            for (std::vector<Light>::iterator it = app->lights.begin(); it < app->lights.end(); ++it)
            {
                Model& model = app->models[app->directionalLightModel];

                if ((*it).type == LightType_Directional)
                    model = app->models[app->directionalLightModel];
                if ((*it).type == LightType_Point)
                    model = app->models[app->pointLightModel];

                Mesh& mesh = app->meshes[model.meshIdx];

                for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                {
                    GLuint vao = FindVAO(mesh, i, texturedLightProgram);

                    //This is hardcoded because i don't know why the vao goes default to vao of model pointLight indeferent to what model is.
                    if ((*it).type == LightType_Directional)
                        vao = 21;
                    if ((*it).type == LightType_Point)
                        vao = 22;

                    glBindVertexArray(vao);

                    //u32 submeshMaterialIdx = model.materialIdx[i];
                    //Material& submeshMaterial = app->materials[submeshMaterialIdx];

                    glActiveTexture(GL_TEXTURE0);
                    //glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
                    glUniform3fv(glGetUniformLocation(texturedLightProgram.handle, "lightColor"), 1, glm::value_ptr((*it).color));
                    glUniformMatrix4fv(glGetUniformLocation(texturedLightProgram.handle, "model"), 1, GL_FALSE, glm::value_ptr(glm::translate(glm::mat4(1), (*it).position)));
                    glUniformMatrix4fv(glGetUniformLocation(texturedLightProgram.handle, "view"), 1, GL_FALSE, &app->cam.GetViewMatrix()[0][0]);
                    glUniformMatrix4fv(glGetUniformLocation(texturedLightProgram.handle, "proj"), 1, GL_FALSE, &app->cam.GetProjectionMatrix()[0][0]);

                    Submesh& submesh = mesh.submeshes[i];
                    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                }
            }
        

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            #pragma endregion

            #pragma region Deferred

            if (app->renderMode == 1)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, app->deferredFBuffer.framebufferHandle);

                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glViewport(0, 0, app->displaySize.x, app->displaySize.y);

                Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx3];
                glUseProgram(programTexturedGeometry.handle);
                glBindVertexArray(app->vao2);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, app->fbuffer.colorAttachmentHandle);
                glUniform1i(glGetUniformLocation(programTexturedGeometry.handle, "colColor"), 0);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, app->fbuffer.positionAttachmentHandle);
                glUniform1i(glGetUniformLocation(programTexturedGeometry.handle, "posColor"), 1);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, app->fbuffer.normalAttachmentHandle);
                glUniform1i(glGetUniformLocation(programTexturedGeometry.handle, "norColor"), 2);

                glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(u16), GL_UNSIGNED_SHORT, 0);

                glBindVertexArray(0);
                glUseProgram(0);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }

            #pragma endregion

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx2];
            glUseProgram(programTexturedGeometry.handle);
            glBindVertexArray(app->vao2);

            glDisable(GL_DEPTH_TEST);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            bool depth = false;

            glUniform1i(app->programUniformTexture, 0);
            glActiveTexture(GL_TEXTURE0);

            switch (app->renderTarget)
            {
            case 0:
                if (app->renderMode == 1)
                    glBindTexture(GL_TEXTURE_2D, app->deferredFBuffer.colorAttachmentHandle);
                else
                    glBindTexture(GL_TEXTURE_2D, app->fbuffer.colorAttachmentHandle);
                break;
            case 1:
                glBindTexture(GL_TEXTURE_2D, app->fbuffer.positionAttachmentHandle);
                break;
            case 2:
                glBindTexture(GL_TEXTURE_2D, app->fbuffer.normalAttachmentHandle);
                break;
            case 3:
                glBindTexture(GL_TEXTURE_2D, app->fbuffer.depthAttachmentHandle);
                depth = true;
                break;
            }

            glUniform1i(glGetUniformLocation(programTexturedGeometry.handle, "isDepth"), depth);
            
            glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(u16), GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);
            glUseProgram(0);
        }
        break;
        default:;
    }
}

