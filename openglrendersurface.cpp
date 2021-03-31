#include "openglrendersurface.h"

OpenGLRenderSurface::OpenGLRenderSurface(QScreen *outputScreen,
                                         QObject *parent,
                                         OpenGLRenderer::OpenGLRenderSpecs specs,
                                         const QSurfaceFormat &surfaceFormat,
                                         QOpenGLContext *sharedContext) :
    QOffscreenSurface(outputScreen,parent),
    OpenGLRenderer(specs),
    openGLFormat(surfaceFormat),
    openGLContext(nullptr),
    depthrenderbuffer(0),
    trianglePositionAttributeLocation(0),
    triangleColorAttributeLocation(0),
    triangleMatrixUniformLocation(0),
    triangleAngle(0.0f)
{
    //Create offscreen surface
    setFormat(openGLFormat);
    create();

    //Allocate memory for the context object and prepare to create it
    openGLContext = new QOpenGLContext(this);
    openGLContext->setFormat(openGLFormat);
    if(sharedContext)
        openGLContext->setShareContext(sharedContext);

    //Make sure the context is created & is sharing resources with the shared context
    bool contextCreated = openGLContext->create();
    assert(contextCreated);

    if(sharedContext)
    {
        bool sharing = QOpenGLContext::areSharing(openGLContext,sharedContext);
        assert(sharing);
    }

    //Initialize sync timer
    initializeTimer();
    qRegisterMetaType<GLuint>("GLuint");
}

OpenGLRenderSurface::~OpenGLRenderSurface()
{

}

const QSurfaceFormat &OpenGLRenderSurface::getOpenGLFormat()
{
    return openGLFormat;
}

QOpenGLContext *OpenGLRenderSurface::getOpenGLContext()
{
    return openGLContext;
}

void OpenGLRenderSurface::setFrameRate(float fps)
{
    OpenGLRenderer::setFrameRate(fps);
}

void OpenGLRenderSurface::start()
{
    float timeOut = 1000.0f/renderSpecs.frameRate;
    syncTimer->start(timeOut);
}

void OpenGLRenderSurface::stop()
{
    syncTimer->stop();
}

void OpenGLRenderSurface::renderFrame()
{
    updateStartTime();

    //Render to FBO
    if (!makeContextCurrent())
        return;

    initialize();

    glBindFramebuffer(GL_FRAMEBUFFER, fboID);
    glBindVertexArray(vaoID);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    updateUniforms();
    shader->bind();

    glViewport(0, 0, renderSpecs.frameType.width, renderSpecs.frameType.height);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    shader->release();

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER,0);

    swapSurfaceBuffers();
    doneContextCurrent();

    updateEndTime();

    emit renderedFrame(1000.0f/t_delta.count());

    emit frameReady(outputTextureID,
                    renderSpecs.frameType.width,
                    renderSpecs.frameType.height);
}

void OpenGLRenderSurface::initializeFBO()
{
    //Generate output FBO and texture
    glGenFramebuffers(1, &fboID);
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);

    glGenTextures(1, &outputTextureID);

    glBindTexture(GL_TEXTURE_2D, outputTextureID);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, renderSpecs.frameType.internalFormat, renderSpecs.frameType.width, renderSpecs.frameType.height, 0, renderSpecs.frameType.format, renderSpecs.frameType.dataType, (const GLvoid*)(nullptr));
    glGenerateMipmap(GL_TEXTURE_2D);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_outputColorAttachment, GL_TEXTURE_2D, outputTextureID, 0);

    //Depth render buffer
    glGenRenderbuffers(1, &depthrenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, renderSpecs.frameType.width, renderSpecs.frameType.height);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRenderSurface::initializeShaderProgram()
{
    //Create debug triangle OpenGL program; add vertex and fragment shaders, link and bind
    shader = new QOpenGLShaderProgram();
    shader -> addShaderFromSourceFile(QOpenGLShader::Vertex, QString(":/GLSL/triangleVertex.glsl"));
    shader -> addShaderFromSourceFile(QOpenGLShader::Fragment, QString(":/GLSL/triangleFragment.glsl"));

    shader -> link();
    shader -> bind();

    //Get locations of vertex shader attributes
    trianglePositionAttributeLocation = glGetAttribLocation(shader -> programId(), "positionAttribute");
    triangleColorAttributeLocation = glGetAttribLocation(shader -> programId(), "colorAttribute");

    triangleMatrixUniformLocation = glGetUniformLocation(shader -> programId(), "matrix");

    shader->release();
}

void OpenGLRenderSurface::initializeVertexBuffers()
{
    static GLfloat triangleData[3][5] = {{0.0f, 0.707f, 1.0f, 0.0f, 0.0f},
                                         {-0.5f, -0.5f, 0.0f, 1.0f, 0.0f},
                                         {0.5f, -0.5f, 0.0f, 0.0f, 1.0f}
    };

    //Generate and bind triangle VAO
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);

    //Generate and bind VBO
    glGenBuffers(1, &vboID);
    glBindBuffer(GL_ARRAY_BUFFER, vboID);
    glBufferData(GL_ARRAY_BUFFER, 15*sizeof(GLfloat), triangleData, GL_STATIC_DRAW);

    glEnableVertexAttribArray(trianglePositionAttributeLocation);
    glEnableVertexAttribArray(triangleColorAttributeLocation);

    glVertexAttribPointer(trianglePositionAttributeLocation, 2, GL_FLOAT, GL_TRUE, 5*sizeof(GLfloat), (const void*)(0));
    glVertexAttribPointer(triangleColorAttributeLocation, 3, GL_FLOAT, GL_TRUE, 5*sizeof(GLfloat), (const void*)(2*sizeof(GLfloat)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER,0);
}

void OpenGLRenderSurface::initializeUniforms()
{

}

void OpenGLRenderSurface::updateUniforms()
{
    shader->bind();

    triangleAngle += 1.0f;
    triangleAngle = (triangleAngle>360.0f)?(0.0f):(triangleAngle);

    QMatrix4x4 matrix;
    matrix.perspective(60.0f, 4.0f / 3.0f, 0.1f, 100.0f);
    matrix.translate(0.0f, 0.0f, -2.0f);
    matrix.rotate(triangleAngle, 0.0f, 1.0f, 0.0f);

    shader->setUniformValue("matrix", matrix);

    shader->release();
}

void OpenGLRenderSurface::initializeTimer()
{
    syncTimer = new QTimer(this);
    syncTimer->setTimerType(Qt::PreciseTimer);

    QObject::connect(syncTimer,&QTimer::timeout,this,&OpenGLRenderSurface::renderFrame);
}

void OpenGLRenderSurface::swapSurfaceBuffers()
{
    openGLContext->swapBuffers(this);
}

bool OpenGLRenderSurface::makeContextCurrent()
{
    return openGLContext->makeCurrent(this);
}

void OpenGLRenderSurface::doneContextCurrent()
{
    openGLContext->doneCurrent();
}
