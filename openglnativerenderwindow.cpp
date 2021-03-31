#include "openglnativerenderwindow.h"

OpenGLNativeRenderWindow::OpenGLNativeRenderWindow(QScreen *outputScreen,
                                                   OpenGLRenderer::OpenGLRenderSpecs specs,
                                                   const QSurfaceFormat &surfaceFormat,
                                                   QOpenGLContext *sharedContext) :
    QOffscreenSurface(nullptr,nullptr),
    OpenGLRenderer(specs),
    openGLFormat(surfaceFormat),
    openGLContext(nullptr),
    sharedOpenGLContext(sharedContext),
    inputTextureID(0),
    visible(false)
{
    //Create offscreen surface
    setFormat(openGLFormat);
    create();
}

LRESULT OpenGLNativeRenderWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    HGLRC hglrc;
    PIXELFORMATDESCRIPTOR pixelFormatDesc;
    int pixelFormat;

    OpenGLNativeRenderWindow* renderWindow;
    switch (message)
    {
        case WM_CREATE:
        pixelFormatDesc =
        {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // Flags
            PFD_TYPE_RGBA,                                              // The kind of framebuffer. RGBA or palette.
            32,                                                         // Colordepth of the framebuffer.
            0, 0, 0, 0, 0, 0,
            0,
            0,
            0,
            0, 0, 0, 0,
            32,                                                         // Number of bits for the depthbuffer
            8,                                                          // Number of bits for the stencilbuffer
            0,                                                          // Number of Aux buffers in the framebuffer.
            PFD_MAIN_PLANE,
            0,
            0, 0, 0
        };

        hdc = GetDC(hwnd);

        pixelFormat = ChoosePixelFormat(hdc, &pixelFormatDesc);
        SetPixelFormat(hdc, pixelFormat, &pixelFormatDesc);

        hglrc = wglCreateContext(hdc);              //Create OpenGL context
        wglMakeCurrent(hdc,hglrc);                  //Make the OpenGL context current
        break;
        case WM_DESTROY:
        wglMakeCurrent(GetDC(hwnd),NULL);           //Deselect OpenGL context
        wglDeleteContext(wglGetCurrentContext());   //Delete OpenGL context
        PostQuitMessage(0);                         //Send wm_quit
        break;
        case WM_PAINT:
        PAINTSTRUCT ps;
        hdc = BeginPaint(hwnd, &ps);

        //Access render window instance
        renderWindow = reinterpret_cast<OpenGLNativeRenderWindow*>(GetWindowLongPtrW(hwnd,0));
        if(renderWindow)
            //Make sure the render window is created and visible on its own thread before resize / render
            if(renderWindow->isVisible())
                renderWindow->renderFrame();

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW+1));

        EndPaint(hwnd, &ps);
        break;
        case WM_SIZE:
        //Access render window instance
        renderWindow = reinterpret_cast<OpenGLNativeRenderWindow*>(GetWindowLongPtrW(hwnd,0));
        if(renderWindow)
            //Make sure the render window is created and visible on its own thread before resize / render
            if(renderWindow->isVisible())
                    renderWindow->resize(LOWORD(lParam),HIWORD(lParam));
        break;
    }

    return DefWindowProc(hwnd,message,wParam,lParam);
}

void OpenGLNativeRenderWindow::displayLastError()
{
    DWORD errorMessageID = GetLastError();

    LPSTR messageBuffer = nullptr;

    //Ask Win32 to give us the string version of that message ID
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be)
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL,
                                 errorMessageID,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                 (LPSTR)&messageBuffer,
                                 0,
                                 NULL);

    //Copy the error message into a std::string
    std::string message(messageBuffer, size);

    //Free the Win32's string's buffer
    LocalFree(messageBuffer);

    qDebug()<<message.c_str();
}

const QSurfaceFormat &OpenGLNativeRenderWindow::getOpenGLFormat()
{
    return openGLFormat;
}

QOpenGLContext *OpenGLNativeRenderWindow::getOpenGLContext()
{
    return openGLContext;
}

HWND OpenGLNativeRenderWindow::getWindowHandle() const
{
    return hwnd;
}

HDC OpenGLNativeRenderWindow::getWindowRenderContext() const
{
    return hdc;
}

HGLRC OpenGLNativeRenderWindow::getOpenGLContextHandle() const
{
    return hglrc;
}

bool OpenGLNativeRenderWindow::isVisible() const
{
    return visible;
}

void OpenGLNativeRenderWindow::createNative()
{
    //Register the window class
    const wchar_t CLASS_NAME[]  = L"Renderer";

    WNDCLASS wc = {};

    HINSTANCE hInstance = GetModuleHandle(NULL);

    wc.lpfnWndProc = (WNDPROC)(&OpenGLNativeRenderWindow::WindowProc);
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.style = CS_OWNDC;
    wc.cbWndExtra = sizeof(OpenGLNativeRenderWindow*);

    RegisterClass(&wc);

    int wWid = static_cast<int>(getSpecs().frameType.width);
    int wHei = static_cast<int>(getSpecs().frameType.height);

    //Create the window
    hwnd = CreateWindowEx(
                0,                      //Optional window styles
                CLASS_NAME,             //Window class
                L"Render Window",       //Window text
                WS_OVERLAPPEDWINDOW,    //Window style
                CW_USEDEFAULT,          //X
                CW_USEDEFAULT,          //Y
                wWid,                   //W
                wHei,                   //H
                NULL,                   //Parent window
                NULL,                   //Menu
                hInstance,              //Instance handle
                NULL                    //Additional application data
                );

    if(hwnd == NULL)
        return;

    hdc = GetDC(hwnd);                  //Get the device context for native window
    hglrc = wglGetCurrentContext();

    SetWindowLongPtrW(hwnd,
                      0,
                      reinterpret_cast<LONG_PTR>(this));
}

void OpenGLNativeRenderWindow::showNative()
{
    //NOTE: it is vital that this method is called via a signal after the render window is moved to its own thread; otherwise
    //the window procedure will be called from the GUI thread and not from its intended thread
    //Create native window and context
    createNative();

    //Allocate memory for the context object and prepare to create it
    openGLContext = new QOpenGLContext(this);

    QWGLNativeContext nativeContext(hglrc,hwnd);
    openGLContext->setNativeHandle(QVariant::fromValue(nativeContext));

    openGLContext->setFormat(openGLFormat);
    if(sharedOpenGLContext)
        openGLContext->setShareContext(sharedOpenGLContext);

    //Make sure the context is created & is sharing resources with the shared context
    bool contextCreated = openGLContext->create();
    assert(contextCreated);

    if(sharedOpenGLContext)
    {
        bool sharing = QOpenGLContext::areSharing(openGLContext,sharedOpenGLContext);
        assert(sharing);
    }

    //Resize / show window
    resize(renderSpecs.frameType.width,renderSpecs.frameType.height);
    visible = !ShowWindow(hwnd,SW_SHOWNORMAL);
}

void OpenGLNativeRenderWindow::resize(unsigned int w, unsigned int h)
{
    if(!makeContextCurrent())
        return;

    initialize();

    OpenGLRenderer::resize(w,h);

    swapSurfaceBuffers();
    doneContextCurrent();
}

void OpenGLNativeRenderWindow::updateSpecs(OpenGLRenderer::OpenGLRenderSpecs specs)
{
    if(!makeContextCurrent())
        return;

    initialize();

    OpenGLRenderer::updateSpecs(specs);

    swapSurfaceBuffers();
    doneContextCurrent();
}

void OpenGLNativeRenderWindow::setFrame(GLuint texID, unsigned int width, unsigned int height)
{
    inputTextureID = texID;

    RECT rect;
    GetWindowRect(hwnd,&rect);

    InvalidateRect(hwnd,&rect,true);
}

void OpenGLNativeRenderWindow::renderFrame()
{
    updateStartTime();

    RECT rect;
    GetWindowRect(hwnd,&rect);
    if(!InvalidateRect(hwnd,&rect,true))
        return;

    if(!makeContextCurrent())
        return;

    initialize();

    //Render to FBO
    glBindFramebuffer(GL_FRAMEBUFFER,fboID);
    glBindVertexArray(vaoID);

    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    shader->bind();

    //Update shader uniform values
    updateUniforms();

    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, inputTextureID);

    glViewport(0,0,renderSpecs.frameType.width,renderSpecs.frameType.height);

    glDrawBuffers(1, &GL_outputColorAttachment);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

    //Render to default FBO

    if(!makeContextCurrentNative())
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, outputTextureID);

    glViewport(0,0,renderSpecs.frameType.width,renderSpecs.frameType.height);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

    shader->release();

    glBindVertexArray(0);

    swapSurfaceBuffers();
    doneContextCurrent();

    swapSurfaceBuffersNative();
    doneContextCurrent();

    updateEndTime();

    double actualFPS = 1000.0f/t_delta.count();

    emit renderedFrame(actualFPS);
}

void OpenGLNativeRenderWindow::swapSurfaceBuffers()
{
    openGLContext->swapBuffers(this);
}

bool OpenGLNativeRenderWindow::makeContextCurrent()
{
    return openGLContext->makeCurrent(this);
}

void OpenGLNativeRenderWindow::doneContextCurrent()
{
    openGLContext->doneCurrent();
}

void OpenGLNativeRenderWindow::swapSurfaceBuffersNative()
{
    //SwapBuffers(hdc);
    wglSwapLayerBuffers(hdc,
                        WGL_SWAP_MAIN_PLANE);
}

bool OpenGLNativeRenderWindow::makeContextCurrentNative()
{
    return wglMakeCurrent(hdc,
                          hglrc);
}

void OpenGLNativeRenderWindow::doneContextCurrentNative()
{
    wglMakeCurrent(hdc,NULL);
}
