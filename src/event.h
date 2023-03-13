#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <vector>

#define CALLBACK_0(func, object, ...) std::bind(&func, object, ##__VA_ARGS__)

#define CALLBACK_1(func, object, ...) std::bind(&func, object, std::placeholders::_1, ##__VA_ARGS__)

#define CALLBACK_2(func, object, ...) std::bind(&func, object, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)

#define CALLBACK_3(func, object, ...) std::bind(&func, object, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, ##__VA_ARGS__)

#define CALLBACK_4(func, object, ...) std::bind(&func, object, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, ##__VA_ARGS__)

typedef uint32_t EventHandle;

template<class RetType, class... Args>
class Event
{
public:
    struct EventData
    {
        EventHandle handle = 0;
        std::function<RetType(Args...)> func;
    };

    EventHandle bind(std::function<RetType(Args...)> func)
    {
        EventData data;
        data.handle = ++_next_handle;
        data.func = func;
        _event_datas.push_back(data);
        return data.handle;
    }

    void unbind(EventHandle handle)
    {
        for (typename std::vector<EventData>::iterator iter = _event_datas.begin(); iter != _event_datas.end(); iter++)
        {
            EventData& data = *iter;
            if (data.handle == handle)
            {
                _event_datas.erase(iter);
                break;
            }
        }
    }

    void dispatch(Args... args)
    {
        if (_event_datas.size() <= 0)
            return;

        for (auto event_data : _event_datas)
            event_data.func(std::forward<Args>(args)...);
    }

private:
    EventHandle _next_handle = 0;
    std::vector<EventData> _event_datas;
};