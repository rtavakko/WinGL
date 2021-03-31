#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>

#include <openglrendersurface.h>
#include <openglnativerenderwindow.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0,
                        QScreen* outputScreen = nullptr,
                        OpenGLRenderer::OpenGLRenderSpecs specs = OpenGLRenderer::OpenGLRenderSpecs{
            OpenGLRenderer::OpenGLTextureSpecs{
            640,
            360,
            4,
            GL_TEXTURE_2D,
            GL_RGBA8,
            GL_RGBA,
            GL_UNSIGNED_BYTE
            },
            60.0
            },
            unsigned int numDisplayWindows = 1);

    ~MainWindow();

signals:
    void setRenderFPS(double fps);

    void showNativeDisplay();

private:
    bool initialize();

    Ui::MainWindow *ui;

    //Render specs
    OpenGLRenderer::OpenGLRenderSpecs videoSpecs;
    QScreen* mainOutputScreen;

    //Renders a texture on a separate thread
    OpenGLRenderSurface* textureRenderer;

    QThread* renderThread;

    //Displays the texture rendered by textureRenderer on the GUI thread
    unsigned int numDisplays;
    std::vector<OpenGLNativeRenderWindow*> textureDisplay;

    //Used for timing
    std::chrono::time_point<std::chrono::high_resolution_clock> t_startRender;
    std::chrono::time_point<std::chrono::high_resolution_clock> t_endRender;

    std::chrono::duration<double,std::milli> t_deltaRender;

    double textRefreshTime;
};

#endif // MAINWINDOW_H
