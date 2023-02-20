#include "ez_vulkan.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* glfw_window = glfwCreateWindow(800, 600, "gltf_sdf_test", nullptr, nullptr);
    ez_init();
    EzSwapchain swapchain = VK_NULL_HANDLE;
    ez_create_swapchain(glfwGetWin32Window(glfw_window), swapchain);

    while (!glfwWindowShouldClose(glfw_window))
    {
        glfwPollEvents();

        EzSwapchainStatus swapchain_status = ez_update_swapchain(swapchain);

        if (swapchain_status == EzSwapchainStatus::NotReady)
            continue;

        if (swapchain_status == EzSwapchainStatus::Resized)
        {
        }

        ez_acquire_next_image(swapchain);

        VkImageMemoryBarrier2 present_barrier = ez_image_barrier(swapchain->images[swapchain->image_index],
                                                              0, 0, VK_IMAGE_LAYOUT_UNDEFINED,
                                                              0, 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                              VK_IMAGE_ASPECT_COLOR_BIT);
        ez_pipeline_barrier(VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &present_barrier);

        ez_present(swapchain);

        ez_submit();
    }

    ez_wait_idle();
    ez_destroy_swapchain(swapchain);
    ez_terminate();
    glfwDestroyWindow(glfw_window);
    glfwTerminate();
    return 0;
}