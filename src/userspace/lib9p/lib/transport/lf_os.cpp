#include <9p/transports/lf_os>

#include <sstream>
#include <iostream>
#include <system_error>

#include <sys/syscalls.h>
#include <sys/known_services.h>

namespace lib9p {
    void LFOSTransport::discoverService() {
        ::Message* msg                     = static_cast<::Message*>(malloc(sizeof(::Message)));
        msg->size                          = sizeof(::Message);
        msg->user_size                     = 0;
        msg->type                          = MT_ServiceDiscovery;
        msg->user_data.ServiceDiscovery.mq = _queue;
        memcpy(&msg->user_data.ServiceDiscovery.serviceIdentifier, &FileSystemDriverUUID, sizeof(uuid_t));

        uint64_t error;
        sc_do_ipc_service_discover(&FileSystemDriverUUID, _queue, msg, &error);

        if(error) {
            if(error != ENOENT) {
                throw new std::system_error(error, std::generic_category(), "Error sending service discovery message for FileSystemDriverUUID");
            }
            else {
                return;
            }
        }

        while(true) {
            memset(msg, 0, msg->size);
            msg->size      = sizeof(::Message);
            msg->user_size = 0;
            msg->type      = MT_Invalid;

            do {
                sc_do_ipc_mq_poll(_queue, true, msg, &error);

                if (error == EMSGSIZE) {
                    msg = static_cast<::Message*>(realloc(msg, msg->size));
                    error = EAGAIN;
                }
            } while(error == EAGAIN);

            if(error) {
                throw new std::system_error(error, std::generic_category(), "Error receiving service discovery response message");
            }

            if(msg->type == MT_ServiceDiscovery && msg->user_data.ServiceDiscovery.response) {
                _clientTransportArg.recipient_pid = msg->sender;
                _clientTransportArg.queue_id      = msg->user_data.ServiceDiscovery.mq;
                break;
            } else if (msg->type == MT_9P && _responseHandler) {
                Message* p9msg = this->decode(msg);
                _responseHandler->handle9pResponse(p9msg);
            }
        }
    }

    void LFOSTransport::loop() {
        uint64_t error;

        while(true) {
            size_t size = sizeof(::Message);
            ::Message* msg = (::Message*)malloc(size);
            msg->size = size;

            do {
                sc_do_ipc_mq_poll(_queue, true, msg, &error);

                if(error == EMSGSIZE) {
                    msg = (::Message*)realloc(msg, msg->size);
                    error = EAGAIN;
                }
            } while(error == EAGAIN);

            if(msg->type == MT_9P || msg->type == MT_ServiceDiscovery) {
                Message* decoded = decode(msg);

                if(decoded) {
                    if(_connectionHandler) {
                        if(!_connections.contains(msg->sender)) {
                            TransportArgument* arg = new TransportArgument{
                                .recipient_pid = msg->sender,
                                .queue_id      = msg->user_data.NineP.response_queue
                            };
                            _connections[msg->sender] = new Connection(this, (void*)arg);
                            _connectionHandler->newConnection(_connections[msg->sender]);
                        }

                        _connections[msg->sender]->push(decoded);
                    }
                    else if(_responseHandler) {
                        _responseHandler->handle9pResponse(decoded);
                    }

                    delete decoded;
                }
            }

            free(msg);
        }
    }

    void LFOSTransport::writeMessage(const Message* msg, void* transportArg) {
        TransportArgument* arg = &_clientTransportArg;

        if(transportArg) {
            arg = reinterpret_cast<TransportArgument*>(transportArg);
        }

        send(arg->recipient_pid, arg->queue_id, _queue, *msg);
    }

    void LFOSTransport::send(pid_t pid, uint64_t send_mq, uint64_t recv_mq, const Message& msg) {
        std::ostringstream encoded;
        msg.encode(encoded);

        std::string s = encoded.str();

        size_t msize = sizeof(::Message) + sizeof(::Message::UserData::NinePData) + s.length();
        ::Message* m = (::Message*)malloc(msize);

        m->size = msize;
        m->user_size = sizeof(::Message::UserData::NinePData) + s.length();
        m->type = MT_9P;
        m->user_data.NineP.response_queue = recv_mq;
        memcpy(&m->user_data.NineP.message, s.data(), s.length());

        uint64_t error;
        sc_do_ipc_mq_send(send_mq, pid, m, &error);

        if(error) {
            std::cerr << "Error sending message: " << std::strerror(error) << std::endl;
        }
    }

    Message* LFOSTransport::decode(::Message* msg) {
        size_t overhead = sizeof(::Message::UserData::NinePData);

        if(msg->type == MT_9P && msg->user_size > overhead) {
            std::istringstream message(std::string(msg->user_data.NineP.message, msg->user_size - overhead));
            uint32_t msg_len = WireUtils::read<uint32_t>(message);

            if(msg->user_size == msg_len + overhead) {
                Message* decoded = nullptr;

                try {
                    decoded = Message::decode(message);
                }
                catch(std::exception* e) {
                    std::cerr << "Error decoding message: " << e->what() << std::endl;
                }

                return decoded;
            }
            else {
                std::cerr << "expected " << msg_len + overhead << ", got " << msg->user_size << std::endl;
            }
        }
        else if(msg->type == MT_ServiceDiscovery && !msg->user_data.ServiceDiscovery.response && _answerServiceDiscovery) {
            std::cout << "Received a service discovery request" << std::endl;

            if(memcmp(&msg->user_data.ServiceDiscovery.serviceIdentifier, &FileSystemDriverUUID, sizeof(FileSystemDriverUUID)) == 0) {
                uint64_t clientQueue = msg->user_data.ServiceDiscovery.mq;

                msg->user_size                           = sizeof(::Message::UserData::ServiceDiscoveryData);
                msg->user_data.ServiceDiscovery.response = true;
                msg->user_data.ServiceDiscovery.mq       = _queue;

                uint64_t error;
                sc_do_ipc_mq_send(clientQueue, msg->sender, msg, &error);

                if(error) {
                    throw new std::system_error(error, std::generic_category(), "Error sending service discovery message for FileSystemDriverUUID");
                }
            }
        }

        return nullptr;
    }
}
