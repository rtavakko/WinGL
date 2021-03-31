#ifndef OPENGLNATIVERENDERWINDOW_H
#define OPENGLNATIVERENDERWINDOW_H

#include <openglrenderer.h>

#include <QOffScreenSurface>
#include <QOpenGLContext>

#include <QtPlatformHeaders/QWGLNativeContext>
#include <WinUser.h>
#include <wingdi.h>
#include <windef.h>

#include <errhandlingapi.h>

class OpenGLNativeRenderWindow : public QOffscreenSurface, public OpenGLRenderer
{
    Q_OBJECT
public:
    OpenGLNativeRenderWindow(QScreen* outputScreen,
                             OpenGLRenderer::OpenGLRenderSpecs specs,
                             const QSurfaceFormat& surfaceFormat,
                             QOpenGLContext* sharedContext);

    //Class window procedure
    static LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void displayLastError();

    //QT OpenGL resource access
    const QSurfaceFormat& getOpenGLFormat();
    QOpenGLContext* getOpenGLContext();

    //WGL resource access
    HWND getWindowHandle() const;
    HDC getWindowRenderContext() const;
    HGLRC getOpenGLContextHandle() const;

    bool isVisible() const;

public slots:

    //Called once the render window is moved to a thread to create / show a native window
    virtual void createNative();
    virtual void showNative();

    //Used to update render specs
    virtual void resize(unsigned int w, unsigned int h) override;
    virtual void updateSpecs(OpenGLRenderer::OpenGLRenderSpecs specs) override;

    //Render methods
    void setFrame(GLuint texID, unsigned int width, unsigned int height);
    virtual void renderFrame() override;

signals:
    void renderedFrame(double actualFPS);

protected:
    //QT context methods
    void swapSurfaceBuffers();

    bool makeContextCurrent();
    void doneContextCurrent();

    //WGL context methods
    void swapSurfaceBuffersNative();

    bool makeContextCurrentNative();
    void doneContextCurrentNative();

    //QT OpenGL resources
    QSurfaceFormat openGLFormat;
    QOpenGLContext* openGLContext;

    QOpenGLContext* sharedOpenGLContext;

    //WGL resources
    HWND hwnd;
    HDC hdc;
    HGLRC hglrc;

    GLuint inputTextureID;

    bool visible;
};

#endif // OPENGLNATIVERENDERWINDOW_H
