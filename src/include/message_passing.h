#ifndef _MESSAGE_PASSING_H_INCLUDED
#define _MESSAGE_PASSING_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include <uuid.h>

struct Message {
    //! Size of the message, including metadata
    size_t size;

    //! Size of the user data
    size_t user_size;

    //! Sender of the message
    pid_t sender;

    //! Type of the message
    enum {
        //! Invalid message, only size is valid
        MT_Invalid,

        MT_IO,
        MT_Signal,
        MT_HardwareInterrupt,
        MT_ServiceDiscovery,

        MT_UserDefined = 1024,
    } type;

    union UserData {
        struct IOUserData {
            //! File descriptor this data is for
            int  fd;

            //! $user_size - sizeof(IO) bytes of data
            char data[0];
        } IO;

        struct SignalUserData {
            //! Signal identifier
            uint16_t signal;
        } Signal;

        struct HardwareInterruptUserData {
            uint16_t interrupt;
        } HardwareInterrupt;

        struct ServiceDiscoveryData {
            uuid_t   serviceIdentifier;
            bool     response;
            uint64_t mq;
            char     discoveryData[0];
        } ServiceDiscovery;

        char raw[0];
    } user_data;
}__attribute__((packed));

#endif
