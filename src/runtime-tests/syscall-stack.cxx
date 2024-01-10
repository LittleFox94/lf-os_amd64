#include <wchar.h>
#include <stdint.h>

#include <sys/syscalls.h>

#include <gtest/gtest.h>

TEST(StackCorruption, Syscall) {
    static const wchar_t pattern = 0x1F05;
    volatile wchar_t     data_on_stack[512];
    wmemset((wchar_t*)data_on_stack, pattern, sizeof(data_on_stack) / sizeof(wchar_t));

    // make sure the array is filled as expected
    for(size_t i = 0; i < sizeof(data_on_stack) / sizeof(wchar_t); ++i) {
        EXPECT_EQ(data_on_stack[i], pattern);
    }

    // static to make them life in .data instead of the stack
    static uint64_t rsp_before, rsp_after;
    static uint64_t rbp_before, rbp_after;

    asm volatile("        \n\
            mov %%rsp, %0 \n\
            mov %%rbp, %1 \n\
        "
        :
            "=m"(rsp_before),
            "=m"(rbp_before)
    );

    sc_do_scheduler_sleep(0);

    asm volatile("        \n\
            mov %%rsp, %0 \n\
            mov %%rbp, %1 \n\
        "
        :
            "=m"(rsp_after),
            "=m"(rbp_after)
    );

    EXPECT_EQ(rsp_before, rsp_after);
    EXPECT_EQ(rbp_before, rbp_after);

    for(size_t i = 0; i < sizeof(data_on_stack) / sizeof(wchar_t); ++i) {
        EXPECT_EQ(data_on_stack[i], pattern);
    }
}

TEST(StackCorruption, Interrupts) {
    uint64_t error = 0;
    sc_do_hardware_interrupt_notify(0, true, 0, &error);
    EXPECT_EQ(error, 0);

    static const wchar_t pattern = 0x1F05;
    volatile wchar_t     data_on_stack[512];
    wmemset((wchar_t*)data_on_stack, pattern, sizeof(data_on_stack) / sizeof(wchar_t));

    // make sure the array is filled as expected
    for(size_t i = 0; i < sizeof(data_on_stack) / sizeof(wchar_t); ++i) {
        EXPECT_EQ(data_on_stack[i], pattern);
    }

    // static to make them life in .data instead of the stack
    static uint64_t rsp_before, rsp_after;
    static uint64_t rbp_before, rbp_after;

    asm volatile("        \n\
            mov %%rsp, %0 \n\
            mov %%rbp, %1 \n\
        "
        :
            "=m"(rsp_before),
            "=m"(rbp_before)
    );

    bool received = false;

    Message* msg  = (Message*)malloc(sizeof(Message));
    memset(msg, 0, sizeof(Message));
    msg->size = sizeof(Message);

    while(!received) {
        sc_do_ipc_mq_poll(0, true, msg, &error);

        if(error == EMSGSIZE) {
            size_t size = msg->size;
            msg         = (Message*)realloc(msg, size);
            memset(msg, 0, size);
            msg->size = size;
        }
        else if(error == 0) {
            EXPECT_EQ(msg->type, MT_HardwareInterrupt);
            EXPECT_EQ(msg->user_data.HardwareInterrupt.interrupt, 0);

            received = true;
        }
        else if(error != EAGAIN) {
            FAIL();
        }
    }

    asm volatile("        \n\
            mov %%rsp, %0 \n\
            mov %%rbp, %1 \n\
        "
        :
            "=m"(rsp_after),
            "=m"(rbp_after)
    );

    EXPECT_EQ(rsp_before, rsp_after);
    EXPECT_EQ(rbp_before, rbp_after);

    for(size_t i = 0; i < sizeof(data_on_stack) / sizeof(wchar_t); ++i) {
        EXPECT_EQ(data_on_stack[i], pattern);
    }
}
