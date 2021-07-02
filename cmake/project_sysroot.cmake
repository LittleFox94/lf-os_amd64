include(config)

include(staging)
multistage(sysroot libc stdlibs)

if(${subproject} STREQUAL "sysroot" AND ${current_stage} STREQUAL "root")
    set(CPACK_PACKAGE_NAME     "lf_os-sysroot")
    set(CPACK_PACKAGE_CONTACT  "Mara Sophie Grosch <littlefox@lf-net.org>")
    set(CPACK_PACKAGE_HOMEPAGE "https://praios.lf-net.org/littlefox/lf-os_amd64")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Everything you need to build software for LF OS")

    set(CPACK_GENERATOR        "DEB;TXZ")
    set(CPACK_SOURCE_GENERATOR "")

    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
    set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

    set(CPACK_DEBIAN_COMPRESSION_TYPE   xz)
    set(CPACK_DEBIAN_PACKAGE_SECTION    devel)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS    "lf_os-toolchain")
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "cmake (>= 3.14), ninja")

    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/")
    install(DIRECTORY ${lf_os_sysroot} DESTINATION "lf_os/")

    include(CPack)
endif()

