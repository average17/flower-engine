#include "Launcher.h"
#include "../Core/WindowData.h"
#include "../Core/UtilsPath.h"
#include "../Core/Timer.h"
#include "../RHI/RHI.h"
#include "../Engine.h"

#include <stb/stb_image.h>

namespace flower
{
    static AutoCVarInt32 cVarWindowDefaultShowMode(
        "r.Window.ShowMode",
        "Window display mode. 0 is full screen without tile, 1 is full screen with tile, 2 is custom size by r.Window.Width & .Height",
        "Window",
        1,
        CVarFlags::ReadOnly | CVarFlags::InitOnce
    );

    static AutoCVarInt32 cVarWindowDefaultWidth(
        "r.Window.Width",
        "Window default width which only work when r.Window.ShowMode equal 2.",
        "Window",
        1920,
        CVarFlags::ReadOnly | CVarFlags::InitOnce
    );

    static AutoCVarInt32 cVarWindowDefaultHeight(
        "r.Window.Height",
        "Window default height which only work when r.Window.ShowMode equal 2.",
        "Window",
        1080,
        CVarFlags::ReadOnly | CVarFlags::InitOnce
    );

    static AutoCVarString cVarTileName(
        "r.Window.TileName",
        "Window tile name.",
        "Window",
        "flower engine - ver 0.0.1 - vulkan 1.2",
        CVarFlags::ReadAndWrite
    );
    
    enum class EWindowShowMode : int32_t
    {
        Min = -1,

        FullScreenWithoutTile = 0,
        FullScreenWithTile = 1,
        Free = 2,

        Max,
    };

    EWindowShowMode getWindowShowMode()
    {
        int32_t type = glm::clamp(
            cVarWindowDefaultShowMode.get(), 
            (int32_t)EWindowShowMode::Min + 1,
            (int32_t)EWindowShowMode::Max - 1
        );

        return EWindowShowMode(type);
    }

    static void resizeCallBack(GLFWwindow* window, int width, int height)
    {
        auto app = reinterpret_cast<GLFWWindowData*>(glfwGetWindowUserPointer(window));
        app->callbackOnResize(width,height);
    }

    static void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
    {
        auto app = reinterpret_cast<GLFWWindowData*>(glfwGetWindowUserPointer(window));
        app->callbackOnMouseMove(xpos,ypos);
    }

    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        auto app = reinterpret_cast<GLFWWindowData*>(glfwGetWindowUserPointer(window));
        app->callbackOnMouseButton(button,action,mods);
    }

    static void scrollCallback(GLFWwindow* window,double xoffset,double yoffset)
    {
        auto app = reinterpret_cast<GLFWWindowData*>(glfwGetWindowUserPointer(window));
        app->callbackOnScroll(xoffset,yoffset);
    }

    static void windowFocusCallBack(GLFWwindow* window, int focused)
    {
        auto app = reinterpret_cast<GLFWWindowData*>(glfwGetWindowUserPointer(window));
        app->callbackOnSetFoucus((bool)focused);
    }

    void GLFWInit(const LauncherInfo& info)
    {
        GLFWWindowData* windowData = GLFWWindowData::get();

        const bool bValidInSize = info.width.has_value() && info.height.has_value();
        const bool bValidTile = info.name.has_value();

        std::string finalTileName = bValidTile ? info.name.value() : cVarTileName.get();

        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API);

        if(info.resizeable)
        {
            glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
        }
        else
        {
            glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
        }

        EWindowShowMode showMode = getWindowShowMode();

        if(bValidInSize)
        {
            showMode = EWindowShowMode::Free;
        }

        uint32_t width;
        uint32_t height;
        if(showMode == EWindowShowMode::FullScreenWithoutTile)
        {
            const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

            width  = mode->width;
            height = mode->height;

            glfwWindowHint(GLFW_RED_BITS,     mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS,   mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS,    mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

            windowData->m_window = glfwCreateWindow(width, height, finalTileName.c_str(),glfwGetPrimaryMonitor(),nullptr);

            glfwSetWindowPos(windowData->m_window, (mode->width - width) / 2, (mode->height - height) / 2);
        }
        else if(showMode == EWindowShowMode::FullScreenWithTile)
        {
            const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
            width = mode->width;
            height = mode->height;
            glfwWindowHint(GLFW_MAXIMIZED, GL_TRUE);
            windowData->m_window = glfwCreateWindow(width, height, finalTileName.c_str(),nullptr,nullptr);
        }
        else if(showMode == EWindowShowMode::Free)
        {
            if(bValidInSize)
            {
                width  = std::max(10, (int32_t)info.width.value());
                height = std::max(10, (int32_t)info.height.value());
            }
            else
            {
                width  = std::max(10,  cVarWindowDefaultWidth.get());
                height = std::max(10, cVarWindowDefaultHeight.get());
            }

            const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

            windowData->m_window = glfwCreateWindow(width, height, finalTileName.c_str(),nullptr,nullptr);
            glfwSetWindowPos(windowData->m_window, (mode->width - width) / 2, (mode->height - height) / 2);
        }
        else
        {
            LOG_FATAL("Unknown windowShowMode: {0}.",(int32_t)showMode);
        }
        LOG_INFO("Create window size: ({0},{1}).", width, height);

        // Register callback functions.
        glfwSetWindowUserPointer(windowData->m_window,       windowData);
        glfwSetFramebufferSizeCallback(windowData->m_window, resizeCallBack);
        glfwSetMouseButtonCallback(windowData->m_window,     mouseButtonCallback);
        glfwSetCursorPosCallback(windowData->m_window,       mouseMoveCallback);
        glfwSetScrollCallback(windowData->m_window,          scrollCallback);
        glfwSetWindowFocusCallback(windowData->m_window,     windowFocusCallBack);

        // Set application icon.
        GLFWimage images[1]; 
        images[0].pixels = stbi_load(
            UtilsPath::getUtilsPath()->getEngineIconFilePath().c_str(),
            &images[0].width,
            &images[0].height,
            0, 4
         );
        glfwSetWindowIcon(windowData->m_window, 1, images); 
        stbi_image_free(images[0].pixels);
    }

    void GLFWRelease()
    {
        glfwDestroyWindow(GLFWWindowData::get()->getWindow());
        glfwTerminate();
    }

    bool Launcher::preInit(const LauncherInfo& info)
    {
        GLFWInit(info);
        RHI::get()->init(GLFWWindowData::get()->getWindow());
        GEngine.beforeInit();

        return true;
    }

    bool Launcher::init()
    {
        GEngine.init();
        GTimer.globalTimeInit();

        return true;
    }

    // use lerp function to smooth time dt.
    float computeSmoothDt(float oldDt,float dt)
    {
        constexpr double frames_to_accumulate = 5.0;
        constexpr double delta_feedback       = 1.0 / frames_to_accumulate;

        constexpr double fps_min = 30.0;
        constexpr double delta_max = 1.0f / fps_min;

        const double delta_clamped  = dt > delta_max ? delta_max : dt;
        return (float)(oldDt * (1.0 - delta_feedback) + delta_clamped * delta_feedback);
    }

    void Launcher::guardedMain()
    {
        GTimer.frameTimeInit();
        float dt = 0.0166f;

        uint32_t frameCount = 0;
        uint32_t passFrame = 0;
        float smoothDt = 0.0f;
        float passTime = 0.0f;

        while(!glfwWindowShouldClose(GLFWWindowData::get()->getWindow()) && GLFWWindowData::get()->shouldRun())
        {
            glfwPollEvents();

            GTimer.frameTick();

            GTimer.frameReset();
            dt = frameCount == 0 ? dt : GTimer.frameDeltaTime();
            smoothDt = computeSmoothDt(smoothDt, dt);

            const bool bMinimized = GLFWWindowData::get()->getWidth() == 0 || GLFWWindowData::get()->getHeight() == 0;

            TickData tickData{};
            tickData.bLoseFocus = !GLFWWindowData::get()->isFoucus();
            tickData.deltaTime = dt;
            tickData.smoothDeltaTime = smoothDt;
            tickData.bIsMinimized = bMinimized;

            GEngine.tick(tickData);
            GTimer.frameCountAdd();

            frameCount ++;
            passFrame++;
            passTime += dt;
            if(passTime >= 1.0f)
            {
                GTimer.setCurrentSmoothFps(passFrame / passTime);
                passTime = 0;
                passFrame = 0;
            }

            GTimer.setCurrentFps(1.0f / dt);
        }

        RHI::get()->waitIdle();

        // sleep leave time for other thread end.
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    void Launcher::release()
    {
        GEngine.release();
        RHI::get()->release();
        GLFWRelease();
    }
}