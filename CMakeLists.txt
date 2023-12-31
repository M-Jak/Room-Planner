cmake_minimum_required (VERSION 3.0)
cmake_policy(VERSION 3.0)

project(Room-Planner)

set(CMAKE_CXX_STANDARD 17) # this does nothing for MSVC, use target_compile_options below
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

if(WIN32)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
endif(WIN32)

link_directories(${CMAKE_SOURCE_DIR}/lib)

# find the required packages
find_package(GLM REQUIRED)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")
find_package(GLFW3 REQUIRED)
message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")
find_package(ASSIMP REQUIRED)
message(STATUS "Found ASSIMP in ${ASSIMP_INCLUDE_DIR}")
# find_package(SOIL REQUIRED)
# message(STATUS "Found SOIL in ${SOIL_INCLUDE_DIR}")
# find_package(GLEW REQUIRED)
# message(STATUS "Found GLEW in ${GLEW_INCLUDE_DIR}")
if(APPLE)
  find_package(Freetype REQUIRED)
  message(STATUS "Found Freetype in ${FREETYPE_INCLUDE_DIRS}")
endif(APPLE)

if(WIN32)
  set(LIBS glfw3 opengl32 assimp freetype irrKlang)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
elseif(UNIX AND NOT APPLE)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
  find_package(OpenGL REQUIRED)
  add_definitions(${OPENGL_DEFINITIONS})
  find_package(X11 REQUIRED)
  # note that the order is important for setting the libs
  # use pkg-config --libs $(pkg-config --print-requires --print-requires-private glfw3) in a terminal to confirm
  set(LIBS ${GLFW3_LIBRARY} X11 Xrandr Xinerama Xi Xxf86vm Xcursor GL dl pthread freetype ${ASSIMP_LIBRARY})
  set (CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")
elseif(APPLE)
  INCLUDE_DIRECTORIES(/System/Library/Frameworks)
  FIND_LIBRARY(COCOA_LIBRARY Cocoa)
  FIND_LIBRARY(OpenGL_LIBRARY OpenGL)
  FIND_LIBRARY(IOKit_LIBRARY IOKit)
  FIND_LIBRARY(CoreVideo_LIBRARY CoreVideo)
  MARK_AS_ADVANCED(COCOA_LIBRARY OpenGL_LIBRARY)
  SET(APPLE_LIBS ${COCOA_LIBRARY} ${IOKit_LIBRARY} ${OpenGL_LIBRARY} ${CoreVideo_LIBRARY})
  SET(APPLE_LIBS ${APPLE_LIBS} ${GLFW3_LIBRARY} ${ASSIMP_LIBRARY} ${FREETYPE_LIBRARIES})
  set(LIBS ${LIBS} ${APPLE_LIBS})
else()
  set(LIBS )
endif(WIN32)




configure_file(configuration/root_directory.h.in configuration/root_directory.h)
include_directories(${CMAKE_BINARY_DIR}/configuration)

# first create relevant static libraries required for other projects
add_library(STB_IMAGE "src/stb_image.cpp")
set(LIBS ${LIBS} STB_IMAGE)

add_library(GLAD "src/glad.c")
set(LIBS ${LIBS} GLAD)

macro(makeLink src dest target)
  add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${src} ${dest}  DEPENDS  ${dest} COMMENT "mklink ${src} -> ${dest}")
endmacro()

function(create_project_from_sources folder)
    file(GLOB SOURCE
            "src/${folder}/*.h"
            "src/${folder}/*.cpp"
            "src/${folder}/*.vs"
            "src/${folder}/*.fs"
            "src/${folder}/*.tcs"
            "src/${folder}/*.tes"
            "src/${folder}/*.gs"
            "src/${folder}/*.cs"
	    "src/imgui/*.cpp"
    )

    # Set the target name based on the folder (chapter) name
    set(NAME ${folder})

    if (NOT SOURCE)
        message(WARNING "No source files found for chapter: ${folder}")
    else()
        add_executable(${NAME} ${SOURCE})

        target_link_libraries(${NAME} ${LIBS})

        if(MSVC)
            target_compile_options(${NAME} PRIVATE /std:c++17 /MP)
            target_link_options(${NAME} PUBLIC /ignore:4099)
        endif(MSVC)

        # Update the output directory to use "${folder}"
        set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${folder}")

        # Copy shader files to the build directory
        file(GLOB SHADERS
                "src/${folder}/*.vs"
                "src/${folder}/*.fs"
                "src/${folder}/*.tcs"
                "src/${folder}/*.tes"
                "src/${folder}/*.gs"
                "src/${folder}/*.cs"
		"src/${folder}/*.glsl"
		"src/${folder}/*.vert"
		"src/${folder}/*.frag"
		"src/${folder}/*.geom"
		"src/${folder}/*.comp"
		"src/${folder}/*.tesse"
		"src/${folder}/*.tessc"
        )

        if(WIN32)
            foreach(SHADER ${SHADERS})
                add_custom_command(TARGET ${NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SHADER} $<TARGET_FILE_DIR:${NAME}>)
            endforeach(SHADER)
        elseif(UNIX AND NOT APPLE)
            file(COPY ${SHADERS} DESTINATION ${CMAKE_SOURCE_DIR}/bin/${folder})
        elseif(APPLE)
            foreach(SHADER ${SHADERS})
                get_filename_component(SHADERNAME ${SHADER} NAME)
                makeLink(${SHADER} ${CMAKE_SOURCE_DIR}/bin/${folder}/${SHADERNAME} ${NAME})
            endforeach(SHADER)
        endif(WIN32)

        if(MSVC)
            configure_file(${CMAKE_SOURCE_DIR}/configuration/visualstudio.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.vcxproj.user @ONLY)
        endif(MSVC)
    endif()
endfunction()


# then create a project file per tutorial
foreach(CHAPTER "Room-Planner")
    create_project_from_sources(${CHAPTER})
endforeach(CHAPTER)

include_directories(${CMAKE_SOURCE_DIR}/includes ${CMAKE_SOURCE_DIR}/includes/imgui ${CMAKE_SOURCE_DIR}/includes/imgui/misc/debugger ${CMAKE_SOURCE_DIR}/includes/imgui/backends)
