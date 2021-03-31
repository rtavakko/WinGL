#include "mainwindow.h"
#include <QApplication>

#include <QSurfaceFormat>

#define OPENGL_MAJOR_VERSION 4
#define OPENGL_MINOR_VERSION 1
#define OPENGL_SWAP_INTERVAL 1
#define OPENGL_SWAP_BEHAVIOUR 0
#define OPENGL_NUM_DISPLAY_WINDOWS 1

int main(int argc, char *argv[])
{
    //Create the global OpenGL surface / context
    QSurfaceFormat format;
    format.setDepthBufferSize(32);
    format.setStencilBufferSize(8);

    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    format.setAlphaBufferSize(8);

    format.setSwapBehavior(QSurfaceFormat::SwapBehavior(OPENGL_SWAP_BEHAVIOUR));

    format.setSwapInterval(OPENGL_SWAP_INTERVAL);  //Turn Vsync on (1) / off (0)

    format.setVersion(OPENGL_MAJOR_VERSION,
                      OPENGL_MINOR_VERSION);

    format.setProfile(QSurfaceFormat::CoreProfile);

    QSurfaceFormat::setDefaultFormat(format);

    //Make sure all OpenGL contexts share resources
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    const QSurfaceFormat& surfaceFormat = QSurfaceFormat::defaultFormat();

    //Render specifications
    OpenGLRenderer::OpenGLTextureSpecs textureSpecs = OpenGLRenderer::OpenGLTextureSpecs
    {
            640,
            360,
            4,
            GL_TEXTURE_2D,
            GL_RGBA8,
            GL_RGBA,
            GL_UNSIGNED_BYTE
    };

    OpenGLRenderer::OpenGLRenderSpecs videoSpecs = OpenGLRenderer::OpenGLRenderSpecs
    {
            textureSpecs,
            60.0
    };

    //Create application / main window
    QApplication a(argc, argv);
    MainWindow w(nullptr,
                 nullptr,
                 videoSpecs,
                 OPENGL_NUM_DISPLAY_WINDOWS);
    w.show();

    return a.exec();
}
