#pragma once

class Buffer
{
public:
    u32 size;
    GLenum type;
    GLuint handle;
    u8* data;
    u32 head;
};