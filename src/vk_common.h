#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <volk.h>

#define VK_LOGE(...)                                 \
    do                                               \
    {                                                \
        fprintf(stderr, "[VK_ERROR]: " __VA_ARGS__); \
        fflush(stderr);                              \
    } while (false)

#define VK_LOGW(...)                                \
    do                                              \
    {                                               \
        fprintf(stderr, "[VK_WARN]: " __VA_ARGS__); \
        fflush(stderr);                             \
    } while (false)

#define VK_LOGI(...)                                \
    do                                              \
    {                                               \
        fprintf(stderr, "[VK_INFO]: " __VA_ARGS__); \
        fflush(stderr);                             \
    } while (false)

#define VK_ASSERT(x)                                                 \
    do                                                               \
    {                                                                \
        if (x != VK_SUCCESS)                                         \
        {                                                            \
            VK_LOGE("Vulkan error at %s:%d.\n", __FILE__, __LINE__); \
            abort();                                                 \
        }                                                            \
    } while (0)
