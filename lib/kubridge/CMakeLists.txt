cmake_minimum_required(VERSION 3.0)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(kubridge C ASM)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/exceptions_user.h
                   COMMAND arm-vita-eabi-as ${CMAKE_CURRENT_SOURCE_DIR}/src/exceptions_user.S -o exceptions_user.o
                   COMMAND arm-vita-eabi-objcopy -O binary exceptions_user.o exceptions_user.bin
                   COMMAND xxd -i exceptions_user.bin ${CMAKE_CURRENT_BINARY_DIR}/exceptions_user.h
                   COMMAND rm -f exceptions_user.o exceptions_user.bin
                   DEPENDS src/exceptions_user.S)

add_executable(kubridge
  src/main.c
  src/exceptions.c
  src/exceptions.S
  ${CMAKE_CURRENT_BINARY_DIR}/exceptions_user.h
)

target_include_directories(kubridge PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(kubridge
  taihenForKernel_stub
  taihenModuleUtils_stub
  SceCpuForDriver_stub
  SceSysclibForDriver_stub
  SceSysmemForDriver_stub
  SceProcEventForDriver_stub
  SceThreadmgrForDriver_stub
  SceDebugForDriver_stub
  SceSysrootForKernel_stub
  SceSysrootForDriver_stub
)

vita_create_self(kubridge.skprx kubridge CONFIG exports.yml UNSAFE)

vita_create_stubs(stubs kubridge ${CMAKE_CURRENT_SOURCE_DIR}/exports.yml KERNEL)

add_custom_target(kubridge_all
  ALL
  DEPENDS libkubridge_stub.a
  DEPENDS libkubridge_stub_weak.a
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/stubs/
  DESTINATION lib
  FILES_MATCHING PATTERN "*.a"
)

install(FILES kubridge.h
  DESTINATION include
)
