#pragma once


class Framebuffer
{
public:
	GLuint colorAttachmentHandle;
	GLuint positionAttachmentHandle;
	GLuint normalAttachmentHandle;
	GLuint depthAttachmentHandle;
	GLuint framebufferHandle;
	GLenum framebufferStatus;
};