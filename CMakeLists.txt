cmake_minimum_required(VERSION 3.0)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

if(DEFINED ENV{PSVITAIP})
  set(PSVITAIP "$ENV{PSVITAIP}")
else()
  set(PSVITAIP "192.168.1.98")
  message(WARNING "PSVITAIP environment variable is not set, using default IP ${PSVITAIP}")
endif()

if(DEFINED ENV{PARSECOREPATH})
  set(PARSECOREPATH "$ENV{PARSECOREPATH}")
else()
  set(PARSECOREPATH "/Users/v/Repos/vitasdk/vita-parse-core/main.py")
  message(WARNING "PARSECOREPATH environment variable is not set, using default path ${PARSECOREPATH}")
endif()

project(so_loader C CXX)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Dead Space")
set(VITA_TITLEID  "DEADSPACE")
set(VITA_VPKNAME  "DEADSPACE")
set(VITA_VERSION  "01.30")
set(VITA_MKSFOEX_FLAGS "-d ATTRIBUTE2=12")

option(DEBUG "Print debug information to stdout" OFF)
option(DEBUG_GL "Print (very verbose) debug logs of VitaGL/PVR to stdout" OFF)

if (DEBUG)
  add_definitions(-DDEBUG)
endif()
if (DEBUG_GL)
  add_definitions(-DDEBUG_GL)
endif()

SET(DATA_PATH "ux0:data/deadspace/" CACHE STRING "Path to data files")
SET(DATA_PATH_INT "${DATA_PATH}assets/" CACHE STRING "Path to assets folder")
SET(SO_PATH "${DATA_PATH}lib/armeabi/libEAMGameDeadSpace.so" CACHE STRING "Path to .so")
SET(GRAPHICS_API "VGL" CACHE STRING "\"VGL\" for VitaGL, \"PVR\" for PVR_PSP2")

add_definitions(-DDATA_PATH="${DATA_PATH}")
add_definitions(-DDATA_PATH_INT="${DATA_PATH_INT}")
add_definitions(-DSO_PATH="${SO_PATH}")

add_definitions(-D_GNU_SOURCE) # makes sincos, sincosf, etc. visible

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -g -O3 -mfloat-abi=softfp")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=gnu++11")

add_executable(so_loader
        loader/main.c
        loader/android/EAAudioCore.c
        loader/android/java.io.InputStream.c
        loader/default_dynlib.c
        loader/utils/dialog.c
        loader/utils/glutil.c
        loader/jni_fake.c
        loader/patch.c
        loader/utils/utils.c
        loader/utils/settings.c
        loader/reimpl/controls.c
        loader/reimpl/ctype_patch.c
        loader/reimpl/env.c
        loader/reimpl/io.c
        loader/reimpl/log.c
        loader/reimpl/mem.c
        loader/reimpl/pthr.c
        loader/reimpl/sys.c
        lib/sha1/sha1.c
        lib/so_util/so_util.c
        lib/VFPVector/vfp_f32_emu.c
        lib/VFPVector/vfp_f64_emu.c
        lib/VFPVector/vfp_gen.c
        lib/VFPVector/vfp_vector.c
        lib/VFPVector/vfp_vector.h
)

set(EXCEPTION_SAFETY 1)
add_subdirectory(lib/kubridge)
add_dependencies(so_loader kubridge_all)

target_include_directories(so_loader SYSTEM
        PUBLIC ${CMAKE_SOURCE_DIR}/lib/fios
        PUBLIC ${CMAKE_SOURCE_DIR}/lib/libc_bridge
        PUBLIC ${CMAKE_SOURCE_DIR}/lib/sha1
        PUBLIC ${CMAKE_SOURCE_DIR}/lib/so_util
        PUBLIC ${CMAKE_SOURCE_DIR}/lib
)

target_include_directories(so_loader
        PUBLIC ${CMAKE_SOURCE_DIR}/loader
)

if (GRAPHICS_API STREQUAL "PVR")
  add_definitions(-D__psp2__)
  add_definitions(-DGRAPHICS_API=1)
  set(GRAPHICS_LIBS
          libgpu_es4_ext_stub_weak
          libIMGEGL_stub_weak
          libGLESv1_CM_stub_weak
  )
  set(SDL_LIBS)
  set(VPK_INCLUDES
          ${CMAKE_SOURCE_DIR}/module module
  )
else()
  add_definitions(-DGRAPHICS_API=0)
  set(GRAPHICS_LIBS
          vitaGL
          vitashark
          SceShaccCgExt
          taihen_stub
          SceShaccCg_stub
  )
  set(VPK_INCLUDES)
endif()

target_link_libraries(so_loader
        -Wl,--whole-archive pthread -Wl,--no-whole-archive
        z
        m
        stdc++
        ${GRAPHICS_LIBS}
        ScePgf_stub
        mathneon
        ${CMAKE_BINARY_DIR}/lib/kubridge/stubs/libkubridge_stub.a
        SceHid_stub
        SceTouch_stub
        SceMotion_stub
        SceAppMgr_stub
        SceAppUtil_stub
        SceAudio_stub
        SceAudioIn_stub
        SceCommonDialog_stub
        SceDisplay_stub
        SceKernelDmacMgr_stub
        SceFios2_stub
        ${CMAKE_SOURCE_DIR}/lib/libc_bridge/out/libSceLibcBridge_stub.a
        SceCtrl_stub
        SceGxm_stub
        ScePower_stub
        SceTouch_stub
        SceVshBridge_stub
)

add_executable(companion
        companion/main.cpp
)

target_link_libraries(companion
        imgui
        vitaGL
        vitashark
        SceShaccCgExt
        m
        mathneon
        taihen_stub
        SceAppMgr_stub
        SceAudio_stub
        SceCtrl_stub
        SceCommonDialog_stub
        SceDisplay_stub
        SceKernelDmacMgr_stub
        SceGxm_stub
        SceShaccCg_stub
        SceSysmodule_stub
        SceTouch_stub
        SceVshBridge_stub
)

vita_create_self(companion.bin companion UNSAFE)

vita_create_self(eboot.bin so_loader UNSAFE)
vita_create_vpk(${VITA_VPKNAME}.vpk ${VITA_TITLEID} eboot.bin
        VERSION ${VITA_VERSION}
        NAME ${VITA_APP_NAME}
        FILE ${CMAKE_SOURCE_DIR}/livearea/icon0.png sce_sys/icon0.png
          ${CMAKE_SOURCE_DIR}/livearea/pic0.png sce_sys/pic0.png
          ${CMAKE_SOURCE_DIR}/livearea/marker.png sce_sys/livearea/contents/marker.png
          ${CMAKE_SOURCE_DIR}/livearea/conf.png sce_sys/livearea/contents/conf.png
          ${CMAKE_SOURCE_DIR}/livearea/bg0.png sce_sys/livearea/contents/bg0.png
          ${CMAKE_SOURCE_DIR}/livearea/template.xml sce_sys/livearea/contents/template.xml
          ${CMAKE_SOURCE_DIR}/livearea/configurator-bg.png data/configurator-bg.png
          ${CMAKE_BINARY_DIR}/companion.bin companion.bin
          ${VPK_INCLUDES}
)

add_custom_command(TARGET so_loader
        PRE_LINK
        COMMAND cd ${CMAKE_SOURCE_DIR} && make -C lib/libc_bridge
        COMMENT "Building stubs for libc_bridge"
        VERBATIM
)

add_custom_target(send
        COMMAND echo destroy | nc ${PSVITAIP} 1338
        COMMAND curl -T eboot.bin ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/
        COMMAND echo launch ${VITA_TITLEID} | nc ${PSVITAIP} 1338
        DEPENDS ${VITA_VPKNAME}.vpk-vpk
)

add_custom_target(send_kvdb
        COMMAND echo destroy | nc ${PSVITAIP} 1338
        COMMAND curl -T eboot.bin ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/
        COMMAND echo launch ux0:/app/${VITA_TITLEID}/eboot.bin | nc ${PSVITAIP} 1338
        DEPENDS ${VITA_VPKNAME}.vpk-vpk
)

add_custom_target(dump
        COMMAND ${CMAKE_SOURCE_DIR}/get_dump.sh ${PSVITAIP}
        COMMAND python3 ${PARSECOREPATH} coredump so_loader
)

add_custom_target(reboot
        COMMAND echo reboot | nc ${PSVITAIP} 1338
)
