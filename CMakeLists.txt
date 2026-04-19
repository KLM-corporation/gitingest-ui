cmake_minimum_required(VERSION 3.16)
project(GitingestUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.1-docking
)
FetchContent_Populate(imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

set(SRC_FILES
    src/main.cpp
    src/AppUI.cpp
    src/SystemUtils.cpp
    src/NativeIngester.cpp
    src/I18n.cpp
)

if(WIN32)
    list(APPEND SRC_FILES src/GitingestUI.rc)
    set(GUI_TYPE WIN32)
endif()

add_executable(GitingestUI ${GUI_TYPE} ${SRC_FILES} ${IMGUI_SOURCES})

# Supprimer la console sur Windows (CMD) tout en gardant main()
if(MSVC)
    set_target_properties(GitingestUI PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()

# OpenGL est necessaire
find_package(OpenGL REQUIRED)

target_include_directories(GitingestUI PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    src
)

target_link_libraries(GitingestUI PRIVATE glfw OpenGL::GL)

if(WIN32)
    target_link_libraries(GitingestUI PRIVATE Comdlg32 Ole32 Crypt32)
endif()

# ===========================================================
# Python Embeddable + Tiktoken (telecharge une seule fois)
# ===========================================================
set(PYTHON_EMBED_DIR "${CMAKE_BINARY_DIR}/python_embed")
set(PYTHON_ZIP       "${CMAKE_BINARY_DIR}/python311_embed.zip")
set(PYTHON_URL       "https://www.python.org/ftp/python/3.11.9/python-3.11.9-embed-amd64.zip")
set(GETPIP_PATH      "${CMAKE_BINARY_DIR}/get-pip.py")
set(GETPIP_URL       "https://bootstrap.pypa.io/get-pip.py")

if(NOT EXISTS "${PYTHON_EMBED_DIR}/python.exe")
    message(STATUS "[Python] Telechargement de Python 3.11 embeddable...")
    file(DOWNLOAD "${PYTHON_URL}" "${PYTHON_ZIP}" SHOW_PROGRESS STATUS DL_STATUS)

    message(STATUS "[Python] Extraction...")
    file(ARCHIVE_EXTRACT INPUT "${PYTHON_ZIP}" DESTINATION "${PYTHON_EMBED_DIR}")

    # Activer site-packages (necessaire pour pip et tiktoken)
    if(EXISTS "${PYTHON_EMBED_DIR}/python311._pth")
        file(READ "${PYTHON_EMBED_DIR}/python311._pth" PTH_CONTENT)
        string(REPLACE "#import site" "import site" PTH_CONTENT "${PTH_CONTENT}")
        file(WRITE "${PYTHON_EMBED_DIR}/python311._pth" "${PTH_CONTENT}")
    endif()

    message(STATUS "[Python] Installation de pip...")
    file(DOWNLOAD "${GETPIP_URL}" "${GETPIP_PATH}" SHOW_PROGRESS)
    execute_process(
        COMMAND "${PYTHON_EMBED_DIR}/python.exe" "${GETPIP_PATH}" --no-warn-script-location -q
        WORKING_DIRECTORY "${PYTHON_EMBED_DIR}"
    )

    message(STATUS "[Python] Installation de tiktoken...")
    execute_process(
        COMMAND "${PYTHON_EMBED_DIR}/python.exe" -m pip install tiktoken --no-warn-script-location -q
        WORKING_DIRECTORY "${PYTHON_EMBED_DIR}"
    )
    message(STATUS "[Python] python_embed pret avec tiktoken !")
endif()

# Copie de python_embed/ et du logo vers le dossier de sortie apres chaque build
add_custom_command(TARGET GitingestUI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PYTHON_EMBED_DIR}"
        "$<TARGET_FILE_DIR:GitingestUI>/python_embed"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/src/gitingest_logo.png"
        "$<TARGET_FILE_DIR:GitingestUI>/gitingest_logo.png"
    COMMENT "[Asset] Copie des ressources vers le dossier de l'executable..."
)
