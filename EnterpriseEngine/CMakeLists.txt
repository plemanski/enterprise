cmake_minimum_required(VERSION 3.30)

# TODO: Check that this is applied in the assimp cmake. "If I can figure out how to check this where it's set, make a pr"
add_compile_options("/EHsc")

set(TargetName "EnterpriseEngine")

set(CMAKE_CXX_STANDARD 17)

set(IncludeDir "${CMAKE_CURRENT_SOURCE_DIR}/include/")
set(FullOutputDir "${CMAKE_HOME_DIRECTORY}/bin/${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/${TargetName}")
file(GLOB_RECURSE ENTERPRISE_ENGINE_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

set(BUILD_TESTING OFF)
#set ( BUILD_SHARED_LIBS ON )
set(ASSIMP_INSTALL OFF)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/vendor/DirectX-Headers")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/vendor/DirectXTex")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/vendor/assimp")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/shaders")

add_library("${TargetName}" SHARED "${ENTERPRISE_ENGINE_SOURCE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_draw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_tables.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_widgets.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_demo.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_win32.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_dx12.cpp"
        "${SHADER_FILES}"
)

set_property(TARGET ${TargetName} PROPERTY CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Release>:Release>")
set_property(TARGET ${TargetName} PROPERTY CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL$<$<CONFIG:Debug>:Debug>")

add_dependencies("${TargetName}" shaders
        DirectX-Headers
        DirectXTex
        assimp)


target_link_directories("${TargetName}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include")

target_include_directories("${TargetName}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/DirectX-Headers/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends")

target_include_directories("${TargetName}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/Enterprise"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/shaders")

target_link_directories("${TargetName}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends")


target_link_libraries("${TargetName}" PRIVATE "d3d12.lib" "dxgi.lib" "d3dcompiler.lib" "dxguid.lib")
target_link_libraries("${TargetName}" PRIVATE "DirectX-Headers" "DirectXTex")
target_link_libraries("${TargetName}" PRIVATE "assimp")

target_compile_definitions(EnterpriseEngine PUBLIC "EE_PLATFORM_WINDOWS")
target_compile_definitions(EnterpriseEngine PRIVATE "EE_BUILD_DLL")

set_target_properties("${TargetName}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${FullOutputDir}")
set_target_properties("${TargetName}" PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${FullOutputDir}/static/lib")
set_target_properties("${TargetName}" PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${FullOutputDir}")
