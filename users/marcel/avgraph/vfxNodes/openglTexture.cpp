#include "framework.h"
#include "openglTexture.h"

OpenglTexture::OpenglTexture()
	: id(0)
	, sx(0)
	, sy(0)
	, internalFormat(0)
{
}

OpenglTexture::~OpenglTexture()
{
	free();
}

void OpenglTexture::allocate(const int _sx, const int _sy, const int _internalFormat)
{
	free();

	// capture current OpenGL states before we change them

	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();

	//

	sx = _sx;
	sy = _sy;
	internalFormat = _internalFormat;
	
	//

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, sx, sy);
	checkErrorGL();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	checkErrorGL();
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	checkErrorGL();

	// restore previous OpenGL states

	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
}

void OpenglTexture::free()
{
	if (id != 0)
	{
		glDeleteTextures(1, &id);
		checkErrorGL();
		
		id = 0;
		sx = 0;
		sy = 0;
		internalFormat = 0;
	}
}

bool OpenglTexture::isChanged(const int _sx, const int _sy, const int _internalFormat) const
{
	return _sx != sx || _sy != sy || _internalFormat != internalFormat;
}

void OpenglTexture::setSwizzle(const int r, const int g, const int b, const int a)
{
	Assert(id != 0);
	if (id == 0)
		return;
	
	// capture current OpenGL states before we change them

	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();

	//

	glBindTexture(GL_TEXTURE_2D, id);
	GLint swizzleMask[4] = { r, g, b, a };
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	checkErrorGL();

	// restore previous OpenGL states

	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
}

void OpenglTexture::upload(const void * src, const int srcAlignment, const int srcPitch, const int uploadFormat, const int uploadElementType)
{
	Assert(id != 0);
	if (id == 0)
		return;
	
	// capture current OpenGL states before we change them

	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	GLint restoreUnpack;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpack);
	GLint restorePitch;
	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &restorePitch);
	checkErrorGL();

	//

	glBindTexture(GL_TEXTURE_2D, id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, std::min(8, srcAlignment));
	glPixelStorei(GL_UNPACK_ROW_LENGTH, srcPitch);
	glTexSubImage2D(
		GL_TEXTURE_2D,
		0, 0, 0,
		sx, sy,
		uploadFormat,
		uploadElementType,
		src);
	checkErrorGL();

	// restore previous OpenGL states

	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, restorePitch);
	checkErrorGL();
}
