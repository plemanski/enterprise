cmake_minimum_required(VERSION 3.30)

set(TargetName "EnterpriseEngine")

set(CMAKE_CXX_STANDARD 17)

set (FullOutputDir "${CMAKE_HOME_DIRECTORY}/bin/${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/${TargetName}")


file(GLOB_RECURSE ENTERPRISE_ENGINE_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")


add_library("${TargetName}" SHARED "${ENTERPRISE_ENGINE_SOURCE}")


target_link_directories("${TargetName}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include")
target_include_directories("${TargetName}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include")

target_compile_definitions("${TargetName}" PUBLIC "EE_PLATFORM_WINDOWS")
target_compile_definitions("${TargetName}" PUBLIC "EE_BUILD_DLL")

set_target_properties("${TargetName}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${FullOutputDir}")
set_target_properties("${TargetName}" PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${FullOutputDir}/static/lib")
set_target_properties("${TargetName}" PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${FullOutputDir}")
