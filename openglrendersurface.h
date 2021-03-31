#ifndef OPENGLRENDERSURFACE_H
#define OPENGLRENDERSURFACE_H

#include <openglrenderer.h>

#include <QOffscreenSurface>
#include <QOpenGLContext>

class OpenGLRenderSurface : public QOffscreenSurface, public OpenGLRenderer
{
    Q_OBJECT
public:    
    OpenGLRenderSurface(QScreen* outputScreen,
                        QObject* parent,
                        OpenGLRenderer::OpenGLRenderSpecs specs,
                        const QSurfaceFormat& surfaceFormat,
                        QOpenGLContext* sharedContext);

    virtual ~OpenGLRenderSurface();

    const QSurfaceFormat& getOpenGLFormat();
    QOpenGLContext* getOpenGLContext();

public slots:    

    virtual void setFrameRate(float fps) override;

    virtual void start() override;
    virtual void stop() override;

    virtual void renderFrame() override;

signals:
    void frameReady(GLuint texID, unsigned int width, unsigned int height);
    void renderedFrame(double actualFPS);

protected:

    virtual void initializeFBO() override;
    virtual void initializeShaderProgram() override;
    virtual void initializeVertexBuffers() override;
    virtual void initializeUniforms() override;

    virtual void updateUniforms() override;

    virtual void initializeTimer();

    void swapSurfaceBuffers();

    bool makeContextCurrent();
    void doneContextCurrent();

    //Sync timer
    QTimer* syncTimer;

    QSurfaceFormat openGLFormat;
    QOpenGLContext* openGLContext;

    //For rendering a debug triangle
    GLuint depthrenderbuffer;

    GLint trianglePositionAttributeLocation;
    GLint triangleColorAttributeLocation;
    GLint triangleMatrixUniformLocation;

    float triangleAngle;
};

#endif // OPENGLRENDERSURFACE_H
