cmake_minimum_required(VERSION 3.2.2)

#isNotLib, is Execute project

project(xdrc)

include_directories(../)

if(${CMAKE_HOST_WIN32})
    include_directories(../compat)
    include_directories(../msvc_xdrpp/include)
    add_definitions(
            -D_MBCS
            -D_CRT_SECURE_NO_WARNINGS
    )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    include_directories(../msvc_xdrpp/include)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GS /GL /W3 /Gy /Zc:wchar_t /Zi /Gm- /O2 /sdl /Zc:inline /fp:precise /errorReport:prompt /WX- /Zc:forScope /Gd /Oi /MD /EHsc /nologo")
    add_definitions(
            -D_MBCS
            -D_CRT_SECURE_NO_WARNINGS
    )
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST /LTCG:incremental /NXCOMPAT /DYNAMICBASE /DEBUG /MACHINE:X64 /OPT:REF /OPT:ICF /ERRORREPORT:PROMPT /NOLOGO /TLBID:1")
endif()

if((CMAKE_CXX_COMPILER_ID MATCHES "Clang") OR (${CMAKE_COMPILER_IS_GNUCXX}))
    include_directories(../../)
    add_definitions(-DHAVE_CONFIG_H)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -std=c++11 -Wall -pthread")
endif()

set(XDRC_SOURCE_FILES
        ../compat/getopt_long.c
        gen_hh.cc
        gen_server.cc
        xdrc.cc
        )

set(OUTPUT_XDRC_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/parse.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/scan.cc
        )

set(TRACK_XDRC_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/scan.ll
        ${CMAKE_CURRENT_SOURCE_DIR}/parse.yy
        )

add_custom_command(
        OUTPUT ${OUTPUT_XDRC_FILES}
        COMMAND flex --nounistd -o scan.cc scan.ll
        COMMAND bison -o parse.cc --defines=parse.hh parse.yy
        MAIN_DEPENDENCY ${TRACK_XDRC_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set(ADD_GEN_FILES
        parse.cc
        scan.cc
        )

add_executable(xdrc ${XDRC_SOURCE_FILES} ${OUTPUT_XDRC_FILES})

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_link_libraries(xdrc kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32 ole32 oleaut32 uuid odbc32 odbccp32)
endif()