
#ifndef _SIMPLEWINDOW_HEADER_H_
#define _SIMPLEWINDOW_HEADER_H_

#include "opencv2/opencv.hpp"
#include <memory>
#include <thread>
#include <mutex>

#ifdef _DEBUG
#pragma comment(lib, "opencv_core320d.lib")
#pragma comment(lib, "opencv_highgui320d.lib")
#pragma comment(lib, "opencv_imgproc320d.lib")
#else
#pragma comment(lib, "opencv_core320.lib")
#pragma comment(lib, "opencv_highgui320.lib")
#pragma comment(lib, "opencv_imgproc320.lib")
#endif

class SimpleWindowManager
{
#define AUTOLOCK(lk) std::lock_guard<std::mutex> lg(lk)

private:
    class SimpleWindow
    {
    public:
        explicit SimpleWindow(const std::string& winName)
            : _winName(winName), _locker(), _painter(), _mat()
        {
            _painter = std::thread(&SimpleWindow::Paint, this);
        }

        void Show(cv::Mat& mat)
        {
            AUTOLOCK(_locker);
            _mat = mat.clone();
        }

    private:
        void Paint()
        {
            cv::namedWindow(_winName);
            while (true)
            {
                {
                    AUTOLOCK(_locker);
                    if (!_mat.empty())
                    {
                        cv::imshow(_winName, _mat);
                    }
                }
                cv::waitKey(10);
            }
            cv::destroyWindow(_winName);
        }

    private:
        std::string _winName;

        std::mutex _locker;
        std::thread _painter;

        cv::Mat _mat;
    };

public:
    static void Show(const std::string& winName, cv::Mat& mat)
    {
        if (winName.empty() || mat.empty())
        {
            return;
        }

        std::shared_ptr<SimpleWindow> swp(nullptr);
        {
            AUTOLOCK(_locker);
            std::map<std::string, std::shared_ptr<SimpleWindow>>::iterator it = _windows.find(winName);
            if (it != _windows.cend())
            {
                swp = it->second;
            }
            else
            {
                swp = std::shared_ptr<SimpleWindow>(new SimpleWindow(winName));
                _windows.insert(std::make_pair(winName, swp));
            }
        }
        if (swp)
        {
            swp->Show(mat);
        }
    }

    static void Clear()
    {
        AUTOLOCK(_locker);
        _windows.clear();
    }

private:
    static std::mutex _locker;
    static std::map<std::string, std::shared_ptr<SimpleWindow>> _windows;

private:
    SimpleWindowManager();
    SimpleWindowManager(const SimpleWindowManager&);
    SimpleWindowManager& operator=(const SimpleWindowManager&);
};

std::mutex SimpleWindowManager::_locker;
std::map<std::string, std::shared_ptr<SimpleWindowManager::SimpleWindow>> SimpleWindowManager::_windows;

#endif
