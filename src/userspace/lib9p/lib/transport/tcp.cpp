#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <iostream>
#include <stdexcept>

#include <9p/transports/tcp>
#include <9p/wireutils>

namespace lib9p {
    TCPServerTransport::TCPServerTransport(uint16_t port, ConnectionHandler* handler)
        : TCPTransport(handler) {
        _fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        setsockopt(_fd, SOL_SOCKET, SO_REUSEPORT, &_fd, sizeof(int));
        sockaddr_in addr;
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        bind(_fd, (sockaddr*)&addr, sizeof(addr));
        listen(_fd, 1000);
    }

    void TCPServerTransport::loop() {
        int nfds = _fd + 1;
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(_fd, &rfds);

        while(select(nfds, &rfds, NULL, NULL, NULL) != -1) {
            if(FD_ISSET(_fd, &rfds)) {
                int connfd = accept(_fd, NULL, NULL);

                _buffers[connfd]     = std::string();
                _connections[connfd] = new Connection(this, (void*)(uint64_t)connfd);
                _connectionHandler->newConnection(_connections[connfd]);
            }

            std::vector<int> cleanup;

            for(auto i = _connections.begin(); i != _connections.end(); ++i) {
                std::string& buffer = _buffers[i->first];

                if(FD_ISSET(i->first, &rfds)) {
                    char b[512];
                    ssize_t size = recv(i->first, b, 512, MSG_DONTWAIT);

                    if(size == 0) {
                        close(i->first);
                        cleanup.push_back(i->first);
                        continue;
                    }

                    buffer.insert(buffer.end(), b, b + size);

                    while(buffer.size() >= 4) {
                        std::istringstream message(buffer);
                        uint32_t msg_len = WireUtils::read<uint32_t>(message);

                        if(msg_len <= buffer.size()) {
                            Message* decoded = nullptr;

                            try {
                                decoded = Message::decode(message);
                            }
                            catch(std::exception* e) {
                                std::cerr << "Error decoding message: " << e->what() << std::endl;
                            }

                            if(decoded) {
                                buffer.erase(buffer.begin(), buffer.begin() + msg_len);
                                i->second->push(decoded);

                                delete decoded;
                            }
                        }
                    }
                }

                FD_SET(i->first, &rfds);
                nfds = std::max(i->first + 1, nfds);
            }

            for(auto i = cleanup.begin(); i != cleanup.end(); ++i) {
                delete _connections[*i];
                _connections.erase(*i);
                _buffers.erase(*i);
                FD_CLR(*i, &rfds);
            }

            FD_SET(_fd, &rfds);
            nfds = std::max(_fd + 1, nfds);
        }
    }

    void TCPTransport::writeMessage(const Message* msg, void* pollArg) {
        std::ostringstream data;
        msg->encode(data);

        const std::string& str = data.str();
        send((int)(uint64_t)pollArg, str.data(), str.length(), 0);
    }
}
