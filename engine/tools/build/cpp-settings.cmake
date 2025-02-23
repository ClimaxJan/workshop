# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COMPILE_OPTIONS "")
set(LINK_OPTIONS "")

if (WIN32)

    # Disable secure CRT warnings on windows, our code is designed to be platform agnostic
    # and these get in the way of that.
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -D_CRT_SECURE_NO_WARNINGS)

    # Platform type define.
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -DWS_WINDOWS)

endif()

# Architecture define.
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -DWS_X86)
else()
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -DWS_X64)
endif()

# We always want to generate debug information in all configurations.
set(COMPILE_OPTIONS ${COMPILE_OPTIONS} /Zi)
set(LINK_OPTIONS ${LINK_OPTIONS} /DEBUG)

# Ensure we build with multiple cores where possible.
set(COMPILE_OPTIONS ${COMPILE_OPTIONS} /MP)

set(DEBUG_COMPILE_OPTIONS   ${COMPILE_OPTIONS} -DWS_DEBUG)
set(PROFILE_COMPILE_OPTIONS ${COMPILE_OPTIONS} -DWS_PROFILE)
set(RELEASE_COMPILE_OPTIONS ${COMPILE_OPTIONS} -DWS_RELEASE)

set(DEBUG_LINK_OPTIONS   ${LINK_OPTIONS})
set(PROFILE_LINK_OPTIONS ${LINK_OPTIONS})
set(RELEASE_LINK_OPTIONS ${LINK_OPTIONS})

if (WIN32)

    # Enable pix on all non-release builds.
    set(DEBUG_COMPILE_OPTIONS   ${DEBUG_COMPILE_OPTIONS} -DUSE_PIX)
    set(PROFILE_COMPILE_OPTIONS ${PROFILE_COMPILE_OPTIONS} -DUSE_PIX)    

endif()

add_compile_options(
    "$<$<CONFIG:Debug>:${DEBUG_COMPILE_OPTIONS}>"
    "$<$<CONFIG:Profile>:${PROFILE_COMPILE_OPTIONS}>"
    "$<$<CONFIG:Release>:${RELEASE_COMPILE_OPTIONS}>"
)

add_link_options(
    "$<$<CONFIG:Debug>:${DEBUG_LINK_OPTIONS}>"
    "$<$<CONFIG:Profile>:${PROFILE_LINK_OPTIONS}>"
    "$<$<CONFIG:Release>:${RELEASE_LINK_OPTIONS}>"
)