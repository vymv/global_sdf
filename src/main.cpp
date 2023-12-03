#include "camera.h"
#include "camera_controller.h"
#include "ez_vulkan.h"
#include "filesystem.h"
#include "geometry_manager.h"
#include "input.h"
#include "renderer.h"
#include "scene.h"
#include "scene_importer.h"
#include "shader_manager.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

static void window_size_callback(GLFWwindow* window, int w, int h)
{
}

static void cursor_position_callback(GLFWwindow* window, double pos_x, double pos_y)
{
    Input::get()->set_mouse_position((float)pos_x, (float)pos_y);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    Input::get()->set_mouse_button(button, action);
}

static void mouse_scroll_callback(GLFWwindow* window, double offset_x, double offset_y)
{
    Input::get()->set_mouse_scroll((float)offset_y);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Input::get()->set_keyboard(key, action);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* glfw_window = glfwCreateWindow(800, 600, "gltf_sdf_test", nullptr, nullptr);
    glfwSetFramebufferSizeCallback(glfw_window, window_size_callback);
    glfwSetCursorPosCallback(glfw_window, cursor_position_callback);
    glfwSetMouseButtonCallback(glfw_window, mouse_button_callback);
    glfwSetScrollCallback(glfw_window, mouse_scroll_callback);
    glfwSetKeyCallback(glfw_window, key_callback);

    ez_init();
    GeometryManager::get()->setup();
    ShaderManager::get()->setup(fs_join(PROJECT_DIR, "data", "shaders"));
    EzSwapchain swapchain = VK_NULL_HANDLE;
    ez_create_swapchain(glfwGetWin32Window(glfw_window), swapchain);

    Camera* camera = new Camera();
    camera->set_aspect(800.0f / 600.0f);
    camera->set_translation(glm::vec3(0.0f, 10.0f, 30.0f));
    CameraController* camera_controller = new CameraController();
    camera_controller->set_camera(camera);
    //Scene* scene = load_scene(std::string(PROJECT_DIR) + "/data/scenes/test/test.gltf");
    Scene* scene = load_scene(std::string(PROJECT_DIR) + "/data/scenes/test/y_20.gltf");
    Renderer* renderer = new Renderer();
    renderer->set_scene(scene);
    renderer->set_camera(camera);

    while (!glfwWindowShouldClose(glfw_window))
    {
        glfwPollEvents();

        EzSwapchainStatus swapchain_status = ez_update_swapchain(swapchain);

        if (swapchain_status == EzSwapchainStatus::NotReady)
            continue;

        if (swapchain_status == EzSwapchainStatus::Resized)
        {
            camera->set_aspect(swapchain->width / swapchain->height);
        }

        ez_acquire_next_image(swapchain);

        renderer->render(swapchain, Input::get()->get_show_type());

        VkImageMemoryBarrier2 present_barrier = ez_image_barrier(swapchain,
                                                                 0,
                                                                 0,
                                                                 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                                 VK_IMAGE_ASPECT_COLOR_BIT);
        ez_pipeline_barrier(0, 0, nullptr, 1, &present_barrier);

        ez_present(swapchain);

        ez_submit();

        // Reset input
        Input::get()->reset();
    }

    delete renderer;
    delete scene;
    delete camera;
    delete camera_controller;

    ShaderManager::get()->cleanup();
    GeometryManager::get()->cleanup();
    ez_flush();
    ez_destroy_swapchain(swapchain);
    ez_terminate();
    glfwDestroyWindow(glfw_window);
    glfwTerminate();
    return 0;
}