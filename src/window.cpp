#include <window.h>
#include <stdio.h>
#include <imgui.h>
#include <gui/imgui_setup.h>

namespace t4editor {
    window::window(int w, int h) {
        if(!glfwInit()) {
            printf("Failed to initialize GLFW\n");
        }
        
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        
        m_window = glfwCreateWindow(w, h, "Turok4 Editor", 0, 0);
        if(!m_window) {
            glfwTerminate();
            printf("Failed to create GLFW window\n");
            m_window = 0;
        } else {
            glfwMakeContextCurrent(m_window);
            
            glewExperimental = GL_TRUE;
            if (glewInit() != GLEW_OK) {
                printf("Couldn't initialize GLEW library.\n");
                glfwTerminate();
                m_window = 0;
                return;
            }
            
            printf("GL Version: %s\n", glGetString(GL_VERSION));
            printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
            printf("Vendor: %s\n", glGetString(GL_VENDOR));
            printf("Renderer: %s\n", glGetString(GL_RENDERER));
            
            ImGui_ImplGlfwGL3_Init(m_window, true);
        }
    }
    window::~window() {
        if(m_window) {
            glfwTerminate();
            ImGui_ImplGlfwGL3_Shutdown();
        }
    }

    bool window::isOpen() const {
        if(!m_window) return false;
        return !glfwWindowShouldClose(m_window);
    }
    void window::beginFrame() {
        ImGui_ImplGlfwGL3_NewFrame();
    }
    void window::endFrame() {
        glfwSwapBuffers(m_window);
    }
    void window::poll() {
        glfwPollEvents();
    }
}