
#ifndef _FINDER_HEADER_H_
#define _FINDER_HEADER_H_

#include <map>

#include <thread>
#include <mutex>

#include <vector>

#include <chrono>

template<typename KeyType, typename ValueType, typename GroupType>
class BestFinder
{
public:
    typedef void(*CallbackForOne)(void*, ValueType&);
    typedef void(*CallbackForMultiple)(void*, std::list<ValueType>&);
    typedef void(*ResetVaue)(ValueType&);

private:
    static long long Now()
    {
        typedef std::chrono::milliseconds time_precision;
        typedef std::chrono::time_point<std::chrono::system_clock, time_precision> now_time_point;
        now_time_point tp = std::chrono::time_point_cast<time_precision>(std::chrono::steady_clock::now());
        return tp.time_since_epoch().count();
    }

    struct StatInfo 
    {
        ValueType value;
        long long entertimeout;
        long long interval;
        long long leavetimeout;
        long long accesstime;
        long long timestamp;
    };

    typedef std::map<KeyType, StatInfo> MapType;
    typedef std::map<GroupType, MapType> GroupMapType;

public:
    BestFinder(int interval, CallbackForOne callbackforone, CallbackForMultiple callbackformultiple, void* context = nullptr, ResetVaue resetValue = nullptr) :
        _checking(false), _checker()
        , _locker(), _group_mapper()
        , _interval(interval)
        , callback_one(callbackforone), callback_multiple(callbackformultiple), _context(context)
        , _reset(resetValue)
    {
        if (_interval > 0)
        {
            _checking = true;
            _checker = std::thread(&BestFinder::check_timeout, this);
        }
#ifdef BESTFINDER_STATISTIC
        _statistic = 0;
#endif
    }

    bool Find(const KeyType& key, ValueType& value, const GroupType& groupValue)
    {
        std::lock_guard<std::mutex> lg(_locker);
        GroupMapType::iterator git = _group_mapper.find(groupValue);
        if (git != _group_mapper.end())
        {
            MapType::iterator it = git->second.find(key);
            if (it != git->second.end())
            {
                value = it->second.value;
                it->second.accesstime = Now();
                return true;
            }
        }
        return false;
    }

    void Add(const KeyType& key, const ValueType& value, long long interval, const GroupType& groupValue, long long enterTimeout = 0, long long leaveTimeout = 0)
    {
        std::lock_guard<std::mutex> lg(_locker);
        GroupMapType::iterator git = _group_mapper.find(groupValue);
        if (git == _group_mapper.end())
        {
            MapType mapnode;
            long long timeNow = Now();
            mapnode.insert(std::make_pair(key, StatInfo{ value, enterTimeout, interval, leaveTimeout, timeNow, timeNow }));
            _group_mapper.insert(std::make_pair(groupValue, mapnode));
#ifdef BESTFINDER_STATISTIC
            ++_statistic;
#endif
        }
        else
        {
            long long timeNow = Now();
            MapType::iterator it = git->second.find(key);
            if (it == git->second.end())
            {
                git->second.insert(std::make_pair(key, StatInfo{ value, enterTimeout, interval, leaveTimeout, timeNow, timeNow }));
#ifdef BESTFINDER_STATISTIC
                ++_statistic;
#endif
            }
            else
            {
                StatInfo& info = it->second;
                info.value = value;
                info.entertimeout = enterTimeout;
                info.interval = interval;
                info.leavetimeout = leaveTimeout;
                info.accesstime = timeNow;
                info.timestamp = info.accesstime;
            }
        }

#ifdef BESTFINDER_STATISTIC
        if (_statistic > 20)
        {
            printf("%s %d elements were in storage\n", __FUNCTION__, _statistic);
        }
#endif
    }

    void Update(const KeyType& key, const ValueType& value, const GroupType& groupValue, bool updateTimeStamp = false)
    {
        std::lock_guard<std::mutex> lg(_locker);
        GroupMapType::iterator git = _group_mapper.find(groupValue);
        if (git != _group_mapper.end())
        {
            MapType::iterator it = git->second.find(key);
            if (it != git->second.end())
            {
                it->second.value = value;
                it->second.accesstime = Now();
                if (updateTimeStamp)
                {
                    it->second.timestamp = it->second.accesstime;
                }
            }
        }
    }

    void Find(const std::vector<KeyType>& keys, std::vector<size_t>& founds, std::vector<size_t>& notfounds, const GroupType& groupValue)
    {
        std::lock_guard<std::mutex> lg(_locker);
        GroupMapType::iterator git = _group_mapper.find(groupValue);
        if (git != _group_mapper.end())
        {
            MapType& mapnode = git->second;
            long long timeNow = Now();
            for (size_t idx = 0; idx < keys.size(); ++idx)
            {
                MapType::iterator it = mapnode.find(key);
                if (it != mapnode.end())
                {
                    it->second.accesstime = timeNow;
                    founds.push_back(idx);
                }
                else
                {
                    notfounds.push_back(idx);
                }
            }
        }
        else
        {
            for (size_t idx = 0; idx < keys.size(); ++idx)
            {
                notfounds.push_back(idx);
            }
        }
    }

    void FindIn(const std::vector<KeyType>& keys, std::vector<size_t>& founds, const GroupType& groupValue)
    {
        std::lock_guard<std::mutex> lg(_locker);
        GroupMapType::iterator git = _group_mapper.find(groupValue);
        if (git != _group_mapper.end())
        {
            MapType& mapnode = git->second;
            long long timeNow = Now();
            for (size_t idx = 0; idx < keys.size(); ++idx)
            {
                MapType::iterator it = mapnode.find(key);
                if (it != mapnode.end())
                {
                    it->second.accesstime = timeNow;
                    founds.push_back(idx);
                }
            }
        }
    }

    void FindNotIn(const std::vector<KeyType>& keys, std::vector<size_t>& notfounds, const GroupType& groupValue)
    {
        std::lock_guard<std::mutex> lg(_locker);
        GroupMapType::iterator git = _group_mapper.find(groupValue);
        if (git != _group_mapper.end())
        {
            MapType& mapnode = git->second;
            for (size_t idx = 0; idx < keys.size(); ++idx)
            {
                MapType::iterator it = mapnode.find(keys[idx]);
                if (it == mapnode.end())
                {
                    notfounds.push_back(idx);
                }
            }
        }
        else
        {
            for (size_t idx = 0; idx < keys.size(); ++idx)
            {
                notfounds.push_back(idx);
            }
        }
    }

    void Add(const std::vector<KeyType>& keys, const std::vector<ValueType>& values, long long interval, const GroupType& groupValue, long long enterTimeout = 0, long long leaveTimeout= 0)
    {
        std::lock_guard<std::mutex> lg(_locker);
        GroupMapType::iterator git = _group_mapper.find(groupValue);
        if (git != _group_mapper.end())
        {
            MapType& mapnode = git->second;

            long long timeNow = Now();
            for (size_t idx = 0; idx < keys.size(); ++idx)
            {
                MapType::iterator it = mapnode.find(keys[idx]);
                if (it == mapnode.end())
                {
                    mapnode.insert(std::make_pair(keys[idx], StatInfo{ values[idx], enterTimeout, interval, timeNow, timeNow }));
#ifdef BESTFINDER_STATISTIC
                    ++_statistic;
#endif
                }
                else
                {
                    StatInfo& info = it->second;
                    info.value = values[idx];
                    info.entertimeout = enterTimeout;
                    info.interval = interval;
                    info.leavetimeout = leaveTimeout;
                    info.accesstime = timeNow;
                    info.timestamp = timeNow;
                }
            }
        }
        else
        {
            MapType mapnode;
            for (size_t idx = 0; idx < keys.size(); ++idx)
            {
                mapnode.insert(std::make_pair(keys[idx], StatInfo{ values[idx], enterTimeout, interval, timeNow, timeNow }));
#ifdef BESTFINDER_STATISTIC
                ++_statistic;
#endif
            }
            _group_mapper.insert(std::make_pair(groupValue, mapnode));
        }
    }

    void Update(const std::vector<KeyType>& keys, const std::vector<ValueType>& values, const GroupType& groupValue, bool updateTimeStamp = false)
    {
        std::lock_guard<std::mutex> lg(_locker);
        GroupMapType::iterator git = _group_mapper.find(groupValue);
        if (git != _group_mapper.end())
        {
            MapType& mapnode = git->second;
            long long timeNow = Now();
            for (size_t idx = 0; idx < keys.size(); ++idx)
            {
                MapType::iterator it = mapnode.find(keys[idx]);
                if (it != mapnode.end())
                {
                    it->second.value = values[idx];
                    it->second.accesstime = timeNow;
                    if (updateTimeStamp)
                    {
                        it->second.timestamp = timeNow;
                    }
                }
            }
        }
    }

    ~BestFinder()
    {
        if (_checking)
        {
            _checking = false;
            if (_checker.joinable())
            {
                _checker.join();
            }
        }
    }

private:
    void check_timeout()
    {
        while (_checking)
        {
            std::list<ValueType> values;
            size_t valuesSize = 0;
            {
                std::lock_guard<std::mutex> lg(_locker);
                for (GroupMapType::iterator git = _group_mapper.begin(); git != _group_mapper.end(); ++git)
                {
                    MapType& mapnode = git->second;
                    MapType::iterator it = mapnode.begin();
                    long long timeNow = Now();
                    while (it != mapnode.end())
                    {
                        StatInfo& info = it->second;

                        // check enter timeout
                        if (info.entertimeout > 0)
                        {
                            if (info.timestamp + info.entertimeout <= timeNow)
                            {
                                if (callback_one)
                                {
                                    callback_one(_context, info.value);
                                }
                                if (callback_multiple)
                                {
                                    values.push_back(info.value);
                                    valuesSize++;
                                }

                                // reset timestamp and enter timeout
                                info.entertimeout = 0;
                                info.timestamp = timeNow;

                                // reset value
                                if (_reset)
                                {
                                    _reset(info.value);
                                }
                            }
                            ++it;
                        } 
                        else
                        {
                            // check leave timeout
                            if (info.leavetimeout > 0)
                            {
                                if (info.accesstime + info.leavetimeout <= timeNow)
                                {
                                    if (callback_one)
                                    {
                                        callback_one(_context, info.value);
                                    }
                                    if (callback_multiple)
                                    {
                                        values.push_back(info.value);
                                        valuesSize++;
                                    }
                                    it = mapnode.erase(it);
#ifdef BESTFINDER_STATISTIC
                                    --_statistic;
#endif
                                }
                                else
                                {
                                    if (info.interval > 0)
                                    {
                                        if (info.timestamp + info.interval <= timeNow)
                                        {
                                            if (callback_one)
                                            {
                                                callback_one(_context, info.value);
                                            }
                                            if (callback_multiple)
                                            {
                                                values.push_back(info.value);
                                                valuesSize++;
                                            }
                                            info.timestamp = timeNow;

                                            if (_reset)
                                            {
                                                _reset(info.value);
                                            }
                                        }
                                    }
                                    ++it;
                                }
                            } 
                            else
                            {
                                if (info.interval > 0)
                                {
                                    if (info.timestamp + info.interval <= timeNow)
                                    {
                                        if (callback_one)
                                        {
                                            callback_one(_context, info.value);
                                        }
                                        if (callback_multiple)
                                        {
                                            values.push_back(info.value);
                                            valuesSize++;
                                        }
                                        info.timestamp = timeNow;

                                        if (_reset)
                                        {
                                            _reset(info.value);
                                        }
                                    }
                                }
                                ++it;
                            }
                        }
                    }
                }
            }

            if (callback_multiple && valuesSize > 0)
            {
                callback_multiple(_context, values);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(_interval));
        }
    }

private:
    bool _checking;

    std::thread _checker;

    std::mutex _locker;
    GroupMapType _group_mapper;

#ifdef BESTFINDER_STATISTIC
    int _statistic;
#endif

    int _interval;
    
    CallbackForOne callback_one;
    CallbackForMultiple callback_multiple;
    void* _context;

    ResetVaue _reset;

private:
    BestFinder(const BestFinder&);
    BestFinder& operator=(const BestFinder&);
};

#endif

