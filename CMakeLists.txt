cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# ---- Project ----

# Note: update this to your new project's name and version
project(Smark 
  VERSION 1.0
  LANGUAGES CXX C
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
endif()


# ---- Add dependencies via CPM ----
# see https://github.com/TheLartians/CPM.cmake for more info

include(cmake/CPM.cmake)

# PackageProject.cmake will be used to make our target installable
CPMAddPackage(
  NAME PackageProject.cmake
  GITHUB_REPOSITORY TheLartians/PackageProject.cmake
  VERSION 1.2
)

CPMAddPackage(
  NAME HttpParser
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/third_party/http_parser/
)

CPMAddPackage(
  NAME uv_a
  GITHUB_REPOSITORY libuv/libuv
  VERSION 1.37.0
)

packageProject(
  NAME uv_a
  VERSION ${PROJECT_VERSION}
  BINARY_DIR ${uv_a_BINARY_DIR}
  INCLUDE_DIR ${uv_a_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  DEPENDENCIES 
)

CPMAddPackage(
  NAME copp
  GITHUB_REPOSITORY owt5008137/libcopp
  GIT_TAG 1.2.1
)

packageProject(
  NAME copp
  VERSION ${PROJECT_VERSION}
  BINARY_DIR ${copp_BINARY_DIR}
  INCLUDE_DIR ${copp_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  DEPENDENCIES 
)

target_include_directories(copp PUBLIC ${copp_SOURCE_DIR}/include)

if (NOT TARGET lua)
  CPMFindPackage(
    NAME lua
    GITHUB_REPOSITORY lua/lua
    VERSION 5.3.5
    # lua has no CMakeLists.txt
    DOWNLOAD_ONLY YES
  )

  if (lua_ADDED)
    # we have just downloaded the lua source as lua isn't installed locally
    # create a new independent library LuaForGlue that is aliased to lua 
    # this allows installing and using LuaGlue without interfering with other installations of lua  
    FILE(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
    FILE(GLOB lua_headers ${lua_SOURCE_DIR}/*.h)
    add_library(LuaForGlue ${lua_sources} ${lua_headers})
    set_target_properties(LuaForGlue PROPERTIES LINKER_LANGUAGE C)

    target_include_directories(LuaForGlue
      PUBLIC
        $<BUILD_INTERFACE:${lua_SOURCE_DIR}>
    )

    if(ANDROID)
    target_compile_definitions(LuaForGlue PRIVATE LUA_USE_POSIX LUA_USE_DLOPEN)
    elseif(EMSCRIPTEN OR WIN32)
    elseif(UNIX OR APPLE)
      target_compile_definitions(LuaForGlue PRIVATE LUA_USE_POSIX_SPAWN LUA_USE_POSIX)
    endif()

    packageProject(
      NAME LuaForGlue
      VERSION 5.3.5
      BINARY_DIR ${lua_BINARY_DIR}
      INCLUDE_DIR ${lua_SOURCE_DIR}
      INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
    )

    add_library(lua ALIAS LuaForGlue)
    set(ADDITIONAL_GLUE_DEPENDENCIES "LuaForGlue")
  endif()
endif()

# ---- Add source files ----

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files automatically.
# Keep that in mind when changing files, or explicitly mention them here.
FILE(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
FILE(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# ---- Create library ----

# Note: for header-only libraries change all PUBLIC flags to INTERFACE and create an interface target:
# add_library(Smark INTERFACE)  
# set_target_properties(Smark PROPERTIES INTERFACE_COMPILE_FEATURES cxx_std_17)

add_library(Smark ${headers} ${sources})

set_target_properties(Smark PROPERTIES CXX_STANDARD 17)

# beeing a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(Smark PUBLIC "$<$<BOOL:${MSVC}>:/permissive->")

# Link dependencies (if required)
# target_link_libraries(Smark PUBLIC cxxopts)
target_link_libraries(Smark PUBLIC uv_a HttpParser LuaForGlue copp)

target_include_directories(Smark
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# ---- Create an installable target ----
# this allows users to install and find the library via `find_package()`.

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  DEPENDENCIES 
)
