cmake_minimum_required(VERSION 3.30)
set(TargetName "EnterpriseGame")

set(CMAKE_CXX_STANDARD 17)

set (FullOutputDir "${CMAKE_HOME_DIRECTORY}/bin/${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/${TargetName}")


file(GLOB_RECURSE ENTERPRISE_GAME_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(${TargetName} WIN32 "${ENTERPRISE_GAME_SOURCE}")

# Credit: Mike Marcin (https://mikemarcin.com/posts/win32_without_winmain/)
set_target_properties( ${TargetName} PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\""
                                                CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug$<$<CONFIG:Debug>:Debug>" )

target_include_directories(EnterpriseGame PUBLIC "${CMAKE_SOURCE_DIR}/EnterpriseEngine/src")
#target_include_directories(EnterpriseGame PUBLIC "${CMAKE_SOURCE_DIR}/EnterpriseEngine/vendor/spdlog/include")
#target_include_directories("${TargetName}" PUBLIC "${CMAKE_SOURCE_DIR}/vendor/imgui")
#target_link_directories("${TargetName}" PUBLIC "${CMAKE_SOURCE_DIR}/vendor/imgui/backends")

target_compile_definitions(EnterpriseGame PUBLIC EE_PLATFORM_WINDOWS)

target_link_libraries(EnterpriseGame PUBLIC EnterpriseEngine)

set_target_properties("${TargetName}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${FullOutputDir}")
set_target_properties("${TargetName}" PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${FullOutputDir}/static/lib")
set_target_properties("${TargetName}" PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${FullOutputDir}")

