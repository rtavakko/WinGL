#include "openglrenderer.h"

OpenGLRenderer::OpenGLRenderer(OpenGLRenderer::OpenGLRenderSpecs specs) :
    initialized(false),
    renderSpecs(specs),
    shader(nullptr),
    vertexAttributeLocation(0),
    texCoordAttributeLocation(0),
    textureUniformLocation(0),
    vaoID(0),
    vboID(0),
    fboID(0),
    textureUnit(0),
    outputTextureID(0)
{

}

OpenGLRenderer::~OpenGLRenderer()
{
    if(shader)
        delete shader;

    shader = nullptr;
}

GLuint OpenGLRenderer::getTextureID() const
{
    return outputTextureID;
}

OpenGLRenderer::OpenGLRenderSpecs OpenGLRenderer::getSpecs() const
{
    return renderSpecs;
}

void OpenGLRenderer::initialize()
{
    if(initialized)
        return;

    initializeOpenGLFunctions();

    initializeShaderProgram();
    initializeVertexBuffers();
    initializeFBO();
    initializeUniforms();

    initialized = true;
}

void OpenGLRenderer::resize(unsigned int w, unsigned int h)
{
    if(w == 0 || h == 0)
        return;

    if(renderSpecs.frameType.width != w || renderSpecs.frameType.height != h)
    {
        renderSpecs.frameType.width = w;
        renderSpecs.frameType.height = h;

        resizeFBO();
    }
}

void OpenGLRenderer::updateSpecs(OpenGLRenderer::OpenGLRenderSpecs specs)
{
    if(renderSpecs.frameType.width == 0 || renderSpecs.frameType.height == 0 || renderSpecs.frameType.channels == 0)
        return;

    if(
            renderSpecs.frameType.width != specs.frameType.width ||
            renderSpecs.frameType.height != specs.frameType.height ||
            renderSpecs.frameType.channels != specs.frameType.channels ||
            renderSpecs.frameRate != specs.frameRate ||
            renderSpecs.frameType.internalFormat != specs.frameType.internalFormat ||
            renderSpecs.frameType.format != specs.frameType.format ||
            renderSpecs.frameType.dataType != specs.frameType.dataType)
    {
        renderSpecs = specs;

        resizeFBO();
    }
}

void OpenGLRenderer::setFrameRate(float fps)
{
    if(fps <= 0.0f)
        return;

    renderSpecs.frameRate = fps;
}

void OpenGLRenderer::start()
{
}

void OpenGLRenderer::stop()
{
}

void OpenGLRenderer::initializeFBO()
{
    //Generate output FBO and texture
    glGenFramebuffers(1, &fboID);
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);

    glGenTextures(1, &outputTextureID);

    glBindTexture(GL_TEXTURE_2D, outputTextureID);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, renderSpecs.frameType.internalFormat, renderSpecs.frameType.width, renderSpecs.frameType.height, 0, renderSpecs.frameType.format, renderSpecs.frameType.dataType, (const GLvoid*)(nullptr));
    glGenerateMipmap(GL_TEXTURE_2D);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_outputColorAttachment, GL_TEXTURE_2D, outputTextureID, 0);

    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRenderer::initializeShaderProgram()
{
    //Create main OpenGL program; add vertex and fragment shaders, link and bind
    shader = new QOpenGLShaderProgram();
    shader -> addShaderFromSourceFile(QOpenGLShader::Vertex, QString(":/GLSL/passVertex.glsl"));
    shader -> addShaderFromSourceFile(QOpenGLShader::Fragment, QString(":/GLSL/passFragment.glsl"));

    shader -> link();
    shader -> bind();

    //Get locations of vertex shader attributes
    vertexAttributeLocation = glGetAttribLocation(shader -> programId(), "vertex");
    texCoordAttributeLocation = glGetAttribLocation(shader -> programId(), "texCoord");

    textureUniformLocation = glGetUniformLocation(shader -> programId(), "texture");

    glGetUniformiv(shader->programId(), textureUniformLocation, &textureUnit);   //Set the value of the texture unit (GL_TEXTUREX) so it can be used in glActiveTexture

    shader->release();
}

void OpenGLRenderer::initializeVertexBuffers()
{
    //Vertex and texture positions of image quad
    static const GLfloat vertexData[6][4] = {{-1.0f,-1.0f,0.0f,0.0f},{1.0f,-1.0f,1.0f,0.0f},{1.0f,1.0f,1.0f,1.0f},
                                             {-1.0f,-1.0f,0.0f,0.0f},{1.0f,1.0f,1.0f,1.0f},{-1.0f,1.0f,0.0f,1.0f}};
    //Generate and bind VAO
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);

    //Generate and bind VBO
    glGenBuffers(1, &vboID);
    glBindBuffer(GL_ARRAY_BUFFER, vboID);
    glBufferData(GL_ARRAY_BUFFER, 24*sizeof(GLfloat), vertexData, GL_STATIC_DRAW);

    glEnableVertexAttribArray(vertexAttributeLocation);
    glEnableVertexAttribArray(texCoordAttributeLocation);

    glVertexAttribPointer(vertexAttributeLocation, 2, GL_FLOAT, GL_TRUE, 4*sizeof(GLfloat), (const void*)(0));
    glVertexAttribPointer(texCoordAttributeLocation, 2, GL_FLOAT, GL_TRUE, 4*sizeof(GLfloat), (const void*)(2*sizeof(GLfloat)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER,0);
}

void OpenGLRenderer::initializeUniforms()
{
}

void OpenGLRenderer::resizeFBO()
{
    //Resize output texture
    glBindTexture(GL_TEXTURE_2D, outputTextureID);

        glTexImage2D(GL_TEXTURE_2D, 0, renderSpecs.frameType.internalFormat, renderSpecs.frameType.width, renderSpecs.frameType.height, 0, renderSpecs.frameType.format, renderSpecs.frameType.dataType, (const GLvoid*)(nullptr));

    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLRenderer::updateUniforms()
{
}

void OpenGLRenderer::updateStartTime()
{
    t_start = std::chrono::high_resolution_clock::now();
}

void OpenGLRenderer::updateEndTime()
{
    t_end = std::chrono::high_resolution_clock::now();
    t_delta = t_end - t_start;
}
