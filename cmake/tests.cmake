include(CTest)

if(${BUILD_TESTING})
    add_subdirectory(src/kernel/tests)
    add_subdirectory(src/lib/tiny-stl)
    add_subdirectory(src/userspace/liblfos)

    add_custom_target(runtime-tests)
    add_custom_target(tests DEPENDS test-kernel runtime-tests test-tinystl test-liblfos)

    file(GLOB_RECURSE runtime_tests RELATIVE ${CMAKE_SOURCE_DIR}/src/runtime-tests/ CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/runtime-tests/*.cxx)

    foreach(test IN LISTS runtime_tests)
        if(${test} STREQUAL "gtest_main.cxx")
            continue()
        endif()

        string(REGEX REPLACE "^(.*).cxx$" "\\1" test ${test})

        make_bootable_image(runtime-test_${test}.img shared/runtime-tests/${test})
        add_dependencies(runtime-test_${test}.img stage_userspace)

        set(qemu_command
            ${qemu}
            ${QEMU_FLAGS}
            --chardev file,id=test_output,path=runtime-test_${test}.xml
            --device isa-debugcon,iobase=0x1f05,chardev=test_output
            --drive format=raw,file=runtime-test_${test}.img,if=none,id=boot_drive
            --device nvme,drive=boot_drive,serial=1234
        )

        set(test_command
            sh ${CMAKE_SOURCE_DIR}/src/runtime-tests/run.sh ${qemu_command} --no-reboot
        )

        add_test(
            NAME runtime-test_${test}
            COMMAND ${test_command}
        )

        set(GDB_COMMANDS "add-symbol-file shared/runtime-tests/${test}")
        configure_file(gdbinit.in gdbinit-runtime_test-${test})

        add_custom_target(
            runtime-test_${test}
            COMMAND ${test_command}
            DEPENDS firmware.qcow2 runtime-test_${test}.img
            USES_TERMINAL # not really, but also makes them run sequentially -
                          # preventing multiple access to the shared firmware.qcow2
        )
        add_dependencies(runtime-tests runtime-test_${test})

        add_custom_target(
            debug-runtime-test_${test}
            COMMAND ${qemu_command} --daemonize -S
            COMMAND ${gdb} -ix gdbinit-runtime_test-${test}
            DEPENDS gdbinit-runtime_test-${test} firmware.qcow2 runtime-test_${test}.img
            USES_TERMINAL
        )
    endforeach()
endif()
