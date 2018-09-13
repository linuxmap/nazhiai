
#ifndef _XMEMPOOL_HEADER_H_
#define _XMEMPOOL_HEADER_H_

#include <queue>
#include <map>
#include <mutex>

template<typename El, typename GroupType>
class XMemPool
{
public:
    typedef std::queue<El*> Pool;
    typedef std::map<GroupType, Pool> Pools;

public:
    static El* Alloc(const GroupType& groupValue)
    {
        El* pEl = nullptr;
        {
            std::lock_guard<std::mutex> lg(_locker);
            Pools::iterator psItr = _pools.find(groupValue);
            if (psItr != _pools.end())
            {
                Pool& alias = psItr->second;
                if (alias.size() > 0)
                {
                    pEl = alias.front();
                    alias.pop();
                }
            }
        }

        if (!pEl)
        {
            pEl = new El();
        }

        return pEl;
    }

    static void Free(const GroupType& groupValue, El* pEl)
    {
        if (pEl)
        {
            std::lock_guard<std::mutex> lg(_locker);
            Pools::iterator psItr = _pools.find(groupValue);
            if (psItr != _pools.end())
            {
                psItr->second.push(pEl);
            }
            else
            {
                Pool pool;
                pool.push(pEl);
                _pools.insert(std::make_pair(groupValue, pool));
            }
        }
    }

private:
    static std::mutex _locker;
    static Pools _pools;
};

template<typename El>
std::mutex XMemPool<El, GroupType>::_locker;

template<typename El, typename GroupType>
Pools XMemPool<El, GroupType>::_pools;

#endif

