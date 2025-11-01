cmake_minimum_required(VERSION 3.22)

project(OpenGLExamples)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 23)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

target_compile_definitions(${PROJECT_NAME} PUBLIC "DATA_DIRECTORY=\"./Data/\"")

file(GLOB_RECURSE PROJECT_SRC CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/Source/*.cpp")
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SRC})
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/Source")

# opengl
find_package(OpenGL REQUIRED)
if(NOT OpenGL_FOUND)
    message(FATAL_ERROR "Failed to find OpenGL!")
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(External/glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# glew
add_library(glew STATIC External/glew/src/glew.c)
target_include_directories(glew PUBLIC External/glew/include)
target_compile_definitions(glew PUBLIC GLEW_STATIC GLEW_NO_GLU)
target_link_libraries(glew PUBLIC OpenGL::GL ${CMAKE_DL_LIBS})
target_link_libraries(${PROJECT_NAME} PRIVATE glew)

# glm
target_include_directories(${PROJECT_NAME} PRIVATE
    External/glm)

# stbimage
target_include_directories(${PROJECT_NAME} PRIVATE
    External/stbimage)

# assimp
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
if(WIN32)
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
else()
    set(ASSIMP_BUILD_ZLIB OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_MINIZIP ON CACHE BOOL "" FORCE)
endif()
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "")
add_subdirectory(External/assimp)
target_link_libraries(${PROJECT_NAME} PRIVATE assimp)

# imgui
set(IMGUI_PATH "${CMAKE_SOURCE_DIR}/External/imgui")
add_library(ImGui STATIC
    "${IMGUI_PATH}/imgui.cpp"
    "${IMGUI_PATH}/imgui_demo.cpp"
    "${IMGUI_PATH}/imgui_draw.cpp"
    "${IMGUI_PATH}/imgui_tables.cpp"
    "${IMGUI_PATH}/imgui_widgets.cpp"
    "${IMGUI_PATH}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp")
target_include_directories(ImGui PUBLIC ${IMGUI_PATH} "${IMGUI_PATH}/backends")
target_link_libraries(ImGui PRIVATE glfw OpenGL::GL ${CMAKE_DL_LIBS})
target_link_libraries(${PROJECT_NAME} PRIVATE ImGui)

add_custom_target(run
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    USES_TERMINAL
    VERBATIM)
