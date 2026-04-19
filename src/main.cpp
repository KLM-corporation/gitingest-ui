#include <iostream>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include "AppUI.h"
#include "stb_image.h"

// Gestion d'erreur pour GLFW
static void glfw_error_callback(int error, const char* description)
{
    // On ignore l'erreur 65545 (Clipboard conversion error) qui est un faux positif courant sur Windows
    if (error == 65545) return;
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

int main(int, char**)
{
    // Initialisation
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Définir la version OpenGL + GLSL
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Création de la fenêtre avec contexte OpenGL
    GLFWwindow* window = glfwCreateWindow(1280, 800, "Gitingest UI", nullptr, nullptr);
    if (window == nullptr)
        return 1;

    // Définir l'icône de la fenêtre
    {
        GLFWimage images[1];
        int channels;
        images[0].pixels = stbi_load("gitingest_logo.png", &images[0].width, &images[0].height, &channels, 4);
        if (images[0].pixels) {
            glfwSetWindowIcon(window, 1, images);
            stbi_image_free(images[0].pixels);
        }
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Activation du VSync

    // Initialisation du contexte Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Navigation Clavier
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Mode fenêtres détachables (Docking)
    
    // Léger scaling global par rapport au début du projet (qui était à 1.5f)
    io.FontGlobalScale = 1.7f; 

    // Configuration des plateformes / rendus backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Démarrage de la logique applicative
    AppUI ui;
    ui.Init();

    // Boucle principale
    while (!glfwWindowShouldClose(window))
    {
        // Traiter les événements (clavier, souris, fenêtre)
        glfwPollEvents();

        // Démarrer une nouvelle frame pour Dear ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Rendu de l'interface
        ui.Render();

        ImGui::Render();
        
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        
        // Couleur d'arrière-plan de la fenêtre OpenGL (hors ImGui)
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Finaliser l'affichage ImGui
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Échanger les buffers pour afficher la frame
        glfwSwapBuffers(window);
    }

    // Nettoyage avant de quitter
    ui.Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
