if((CMAKE_CXX_COMPILER_ID MATCHES "Clang") OR (${CMAKE_COMPILER_IS_GNUCXX}))
    include (TestBigEndian)
    TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
    if(IS_BIG_ENDIAN)
        set(WORDS_BIGENDIAN ON)
    endif()

    set(XDRPP_WORDS_BIGENDIAN ON)

    if(${CMAKE_COMPILER_IS_GNUCXX})
        set(CPP_COMMAND ON)
        set(CPP_COMMAND_STRING "gcc -E -xc")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CPP_COMMAND ON)
        set(CPP_COMMAND_STRING "clang -E -xc")
    endif()

    set(HAVE_GETOPT_LONG_ONLY ON)
    set(GETOPT_LONG_ONLY 1)

    set(HAVE_INTTYPES_H ON)
    set(HAVE_INTTYPES_H_VALUE 1)

    set(HAVE_MEMORY_H ON)
    set(HAVE_MEMORY_H_VALUE 1)

    set(HAVE_STDINT_H ON)
    set(HAVE_STDINT_H_VALUE 1)

    set(HAVE_STDLIB_H ON)
    set(HAVE_STDLIB_H_VALUE 1)

    set(HAVE_STRINGS_H ON)
    set(HAVE_STRINGS_H_VALUE 1)

    set(HAVE_STRING_H ON)
    set(HAVE_STRING_H_VALUE 1)

    set(HAVE_SYS_STAT_H ON)
    set(HAVE_SYS_STAT_H_VALUE 1)

    set(HAVE_SYS_TYPES_H ON)
    set(HAVE_SYS_TYPES_H_VALUE 1)

    set(HAVE_UNISTD_H ON)
    set(HAVE_UNISTD_H_VALUE 1)

    set(PACKAGE ON)
    set(PACKAGE_VALUE "xdrpp")

    set(PACKAGE_BUGREPORT ON)

    set(PACKAGE_NAME ON)
    set(PACKAGE_NAME_VALUE "xdrpp")

    set(PACKAGE_STRING ON)
    set(PACKAGE_STRING_VALUE "xdrpp 0")

    set(PACKAGE_TARNAME ON)
    set(PACKAGE_TARNAME_VALUE "xdrpp")

    set(PACKAGE_URL ON)

    set(PACKAGE_VERSION ON)
    set(PACKAGE_VERSION_VALUE 0)

    set(STDC_HEADERS ON)
    set(STDC_HEADERS_VALUE 1)

    set(VERSION ON)
    set(VERSION_VALUE "0")

    set(YYTEXT_POINTER ON)
    set(YYTEXT_POINTER_VALUE 1)

    if(NOT ${CMAKE_HOST_APPLE})
        option(AC_APPLE_UNIVERSAL_BUILD "Enable" ON)
        set(AC_APPLE_UNIVERSAL_BUILD)
    endif()

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h @ONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/xdrpp/build_endian.h.in ${CMAKE_CURRENT_SOURCE_DIR}/xdrpp/build_endian.h @ONLY)
endif()