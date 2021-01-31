#include <9p/helper/filesystem>
#include <9p/exception>

namespace lib9p {
    FileSystem::~FileSystem() {
    }

    void FileSystem::setAsHandler(Connection* conn) {
        conn->onStat(this);
        conn->onClunk(this);
        conn->onRead(this);
        conn->onWalk(this);
        conn->onOpen(this);
    }

    void FileSystem::clunk(Connection* conn, const Qid& qid) {
    }
}
