cmake_minimum_required(VERSION 3.30)

set(TargetName "EnterpriseEngine")

set(CMAKE_CXX_STANDARD 17)

set (FullOutputDir "${CMAKE_HOME_DIRECTORY}/bin/${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/${TargetName}")



file(GLOB_RECURSE ENTERPRISE_ENGINE_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/vendor/DirectX-Headers")

add_library("${TargetName}" SHARED "${ENTERPRISE_ENGINE_SOURCE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_draw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_tables.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_widgets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_demo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_win32.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_dx12.cpp"
)


target_link_directories("${TargetName}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include")

target_include_directories("${TargetName}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include")
target_include_directories("${TargetName}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/DirectX-Headers/include")

target_include_directories("${TargetName}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui")
target_include_directories("${TargetName}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends")
target_link_directories("${TargetName}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends")

target_include_directories("${TargetName}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/Enterprise")

target_link_libraries("${TargetName}" PUBLIC "d3d12.lib" "dxgi.lib" "d3dcompiler.lib")
target_link_libraries("${TargetName}" PUBLIC "DirectX-Headers")

target_compile_definitions(EnterpriseEngine PUBLIC "EE_PLATFORM_WINDOWS")
target_compile_definitions(EnterpriseEngine PRIVATE "EE_BUILD_DLL")

set_target_properties("${TargetName}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${FullOutputDir}")
set_target_properties("${TargetName}" PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${FullOutputDir}/static/lib")
set_target_properties("${TargetName}" PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${FullOutputDir}")
