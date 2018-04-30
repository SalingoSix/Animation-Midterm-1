project(SOIL2_src)
cmake_minimum_required(VERSION 2.6)

option(DISABLE_EGL "Force-disable compiling against EGL." OFF)

set(SOURCE_FILES
        etc1_utils.c
        etc1_utils.h
        image_DXT.c
        image_DXT.h
        image_helper.c
        image_helper.h
        jo_jpeg.h
        pkm_helper.h
        pvr_helper.h
        SOIL2.c
        SOIL2.h
        stb_image.h
        stb_image_write.h
        stbi_DDS.h
        stbi_DDS_c.h
        stbi_ext.h
        stbi_ext_c.h
        stbi_pkm_c.h
        stbi_pkm.h
        stbi_pvr.h
        stbi_pvr_c.h
        )

add_library(SOIL2 STATIC ${SOURCE_FILES})

if (DISABLE_EGL)
    message(STATUS "Compiling without EGL. \nNote that SOIL_GL_GetProcAddress won't work properly on GLES devices.")
    target_compile_definitions(SOIL2 PRIVATE SOIL_NO_EGL=1)
endif ()