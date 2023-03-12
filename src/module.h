#pragma once

template<class T>
class Module
{
public:
    static T* get()
    {
        static T _instance;
        return &_instance;
    }
};