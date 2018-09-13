
#ifndef _XMATPOOL_HEADER_H_
#define _XMATPOOL_HEADER_H_

#include <map>
#include <queue>
#include <mutex>

template<typename MatType, int reservedSize = 10, int deviceCount = 16>
class XMatPool
{
public:
    static bool Alloc(MatType& src, MatType& dst, int deviceIndex = 0)
    {
        if (deviceIndex >= deviceCount)
        {
            return false;
        }

        int resolutionType = (src.cols << 14) + src.rows;

        std::lock_guard<std::mutex> lg(_locker[deviceIndex]);
        std::map<int, std::queue<MatType>>::iterator it = _pool[deviceIndex].find(resolutionType);
        if (it != _pool[deviceIndex].end())
        {
            std::queue<MatType>& alias = it->second;
            if (alias.size() > 0)
            {
                dst = alias.front();
                alias.pop();

#ifdef MATPOOL_STATISTIC
                _pool_element_count--;
#endif

                return true;
            }
        }
        return false;
    }

    static void Free(MatType& mat, int deviceIndex = 0)
    {
        if (deviceIndex >= deviceCount)
        {
            return;
        }

        if (!mat.empty())
        {
            int resolutionType = (mat.cols << 14) + mat.rows;

            std::lock_guard<std::mutex> lg(_locker[deviceIndex]);
            std::map<int, std::queue<MatType>>::iterator it = _pool[deviceIndex].find(resolutionType);
            if (it != _pool[deviceIndex].end())
            {
                std::queue<MatType>& que = it->second;
                if (que.size() > reservedSize)
                {
                    while (que.size() > reservedSize)
                    {
#ifdef MATPOOL_STATISTIC
                        _pool_element_count--;
#endif
                        que.pop();
                    }
                }
                else
                {
#ifdef MATPOOL_STATISTIC
                    _pool_element_count++;
#endif
                    que.push(mat);
                }
            }
            else
            {
                std::queue<MatType> que;
                que.push(mat);
                _pool[deviceIndex].insert(std::make_pair(resolutionType, que));
#ifdef MATPOOL_STATISTIC
                _pool_element_count++;
#endif
            }

#ifdef MATPOOL_STATISTIC
            if (_pool_element_count > 5)
            {
                printf("MatPool element count: %d\n", _pool_element_count);
            }
#endif
        }
    }

    static void Reset(int gpuIndex)
    {
        if (gpuIndex < deviceCount && gpuIndex >= 0)
        {
            _pool[gpuIndex].clear();
        }
    }

    static void Clear()
    {
        for (int i = 0; i < deviceCount; ++i)
        {
            _pool[i].clear();
        }
    }

private:
    static std::mutex _locker[deviceCount];
    static std::map<int, std::queue<MatType>> _pool[deviceCount];

#ifdef MATPOOL_STATISTIC
    static int _pool_element_count;
#endif
};

template<typename MatType, int reservedSize /*= 10*/, int deviceCount /*= 16*/>
std::mutex XMatPool<MatType, reservedSize, deviceCount>::_locker[deviceCount];

template<typename MatType, int reservedSize /*= 10*/, int deviceCount /*= 16*/>
std::map<int, std::queue<MatType>> XMatPool<MatType, reservedSize, deviceCount>::_pool[deviceCount];

#ifdef MATPOOL_STATISTIC
template<typename MatType, int reservedSize /*= 10*/, int deviceCount /*= 16*/>
int XMatPool<MatType, reservedSize, deviceCount>::_pool_element_count = 0;
#endif

#endif

