cmake_minimum_required(VERSION 3.30)
project(Enterprise)

set(CMAKE_CXX_STANDARD 17)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")


# Vendor libraries
# Project libraries
add_subdirectory(EnterpriseEngine)
add_subdirectory(EnterpriseGame)