#pragma once
#include <chrono>

namespace flower
{
    class Timer
    {
        friend class Launcher;
    private:
        std::chrono::system_clock::time_point m_initTimePoint{ };
        std::chrono::duration<float> m_deltaTime { 0.0f };

        std::chrono::system_clock::time_point m_gameTimePoint{ };
        std::chrono::duration<float> m_gameDeltaTime { 0.0f };

        std::chrono::system_clock::time_point m_frameTimePoint{ };
        std::chrono::duration<float> m_frameDeltaTime { 0.0f };

        uint64_t m_frameCount = 0;

        bool  m_bGamePause = true;
        float m_currentFps {0.0f};
        float m_currentSmoothFps { 0.0f };

    public:
        float getCurrentFps() const { return m_currentFps;}
        float getCurrentSmoothFps() const { return m_currentSmoothFps;}

        void setCurrentFps(float fps) { m_currentFps = fps; }
        void setCurrentSmoothFps(float fps) { m_currentSmoothFps = fps; }

        Timer(){ }
        ~Timer() { }

        void globalTimeInit()
        {
            m_initTimePoint = std::chrono::system_clock::now();
        }

        float globalPassTime()
        {
            m_deltaTime = std::chrono::system_clock::now() - m_initTimePoint;
            return m_deltaTime.count();
        }

        bool isGamePause() const
        {
            return m_bGamePause;
        }

        void gameStart()
        {
            m_gameTimePoint = std::chrono::system_clock::now();
            m_gameDeltaTime = std::chrono::duration<float>(0.0f);
            m_bGamePause = false;
        }

        auto getFrameCount() const
        {
            return m_frameCount;
        }

        void frameCountAdd()
        {
            m_frameCount++;
        }

        float gameTime()
        {
            if(m_bGamePause)
            {
                return m_gameDeltaTime.count();
            }
            else
            {
                auto nowTime = std::chrono::system_clock::now();
                m_gameDeltaTime += nowTime - m_gameTimePoint;
                m_gameTimePoint = nowTime;

                return m_gameDeltaTime.count();
            }
        }

        void gamePause()
        {
            if(!m_bGamePause)
            {
                auto nowTime = std::chrono::system_clock::now();
                m_gameDeltaTime += nowTime - m_gameTimePoint;

                m_gameTimePoint = nowTime;
                m_bGamePause = true;
            }
        }

        void gameContinue()
        {
            if(m_bGamePause)
            {
                m_gameTimePoint = std::chrono::system_clock::now();
                m_bGamePause = false;
            }
        }

        void gameStop()
        {
            m_gameTimePoint = std::chrono::system_clock::now();
            m_gameDeltaTime = std::chrono::duration<float>(0.0f);

            m_bGamePause = true;
        }

        void frameTimeInit()
        {
            m_frameTimePoint = std::chrono::system_clock::now();
        }

        void frameTick()
        {
            m_frameDeltaTime = std::chrono::system_clock::now() - m_frameTimePoint;
        }

        float frameDeltaTime() const
        {
            return m_frameDeltaTime.count();
        }

        void frameReset()
        {
            m_frameTimePoint = std::chrono::system_clock::now();
        }
    };

    extern Timer GTimer;
}