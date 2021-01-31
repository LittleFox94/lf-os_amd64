#include <stdint.h>
#include <string>
#include <functional>
#include <system_error>

#include <9p/helper/server_starter>
#include <9p/exception>

#if __LF_OS__
#include <9p/transports/lf_os>
#include <sys/known_services.h>
#include <sys/syscalls.h>

lib9p::ServerStarter::ServerStarter(uint64_t message_queue, bool registerService)
    : _transport(new lib9p::LFOSTransport(message_queue, this)) {
    if(registerService) {
        uint64_t error;
        sc_do_ipc_service_register(&FileSystemDriverUUID, message_queue, &error);

        if(error != 0) {
            throw new std::system_error(error, std::generic_category(), "Error while registering filesystem service");
        }

        dynamic_cast<lib9p::LFOSTransport*>(_transport)->setAnswerServiceDiscovery(true);
    }
}
#else
#include <9p/transports/tcp>

lib9p::ServerStarter::ServerStarter(uint16_t port) :
    _transport(new lib9p::TCPServerTransport(port, this)) {
}
#endif

namespace lib9p {
    ServerStarter::~ServerStarter() {
        for(auto fs = _filesystems.begin(); fs != _filesystems.end(); ++fs) {
            delete fs->second;
        }

        delete _transport;
    }

    void ServerStarter::addFileSystem(std::string name, FileSystem* fs) {
        if(_filesystems.count(name) != 0) {
            throw new std::runtime_error("Filesystem with that name already registered");
        }

        _filesystems[name] = fs;
    }

    void ServerStarter::newConnection(Connection* conn) {
        conn->onAttach(this);
    }

    void ServerStarter::loop() {
        _transport->loop();
    }

    lib9p::Qid ServerStarter::attach(Connection* conn , std::string user, std::string fs) {
        if(_filesystems.count(fs) == 1) {
            return _filesystems[fs]->attach(conn, user, "");
        }

        throw new lib9p::NotImplemented();
    }
}
