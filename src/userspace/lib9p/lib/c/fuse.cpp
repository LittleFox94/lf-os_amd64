#include <cerrno>
#include <cstring>
#include <cstdio>
#include <threads.h>

#include <system_error>
#include <map>
#include <vector>
#include <unordered_map>

#include <9p/exception>
#include <9p/fuse.h>

namespace lib9p {
    using namespace std::string_literals;

    FuseFileSystem::FuseFileSystem(const struct fuse_operations* ops, const std::string name)
        : _ops(ops), _name(name) {
    }

    lib9p::Qid FuseFileSystem::attach(lib9p::Connection* conn, std::string user, std::string fs) {
        FUSE::getInstance()->setCurrentFileSystem(_name);

        setAsHandler(conn);
        return path2Qid("", lib9p::Qid::QTDIR);
    }

    lib9p::Stat FuseFileSystem::stat(lib9p::Connection* conn, const lib9p::Qid& qid) {
        FUSE::getInstance()->setCurrentFileSystem(_name);

        if(_ops->getattr) {
            std::string fusePath = qid2Path(qid);

            if(fusePath == "") {
                fusePath = "/"s;
            }

            struct stat stat;
            struct fuse_file_info fileInfo;
            int e = _ops->getattr(fusePath.c_str(), &stat, &fileInfo);

            if(e < 0) {
                throw new std::system_error(-e, std::generic_category());
            }

            return translate(qid, &stat);
        }

        throw new lib9p::NotImplemented();
    }

    void FuseFileSystem::clunk(lib9p::Connection* conn, const lib9p::Qid& qid) {
        FUSE::getInstance()->setCurrentFileSystem(_name);

        // XXX: we _could_ remove the path for this qid from _paths
        //      but we might reuse the memory then, having a nearly
        //      identical qid later pointing to another path
        //      ----
        //      Nope, does not work as clunk only destroys FIDs. If the
        //      QID is used in another FID we would make lots of fun
        //      errors 🙃
    }

    uint32_t FuseFileSystem::open(lib9p::Connection* conn, const lib9p::Qid& qid, uint8_t mode) {
        FUSE::getInstance()->setCurrentFileSystem(_name);

        const std::string& path = qid2Path(qid);

        bool dirSupport = _ops->opendir || _ops->readdir;

        if(qid.type() == lib9p::Qid::QTDIR && mode == 0 && dirSupport) {
            if(_ops->opendir) {
                struct fuse_file_info fileInfo;
                int err = _ops->opendir(path.c_str(), &fileInfo);

                if(err < 0) {
                    throw new std::system_error(-err, std::generic_category());
                }

                _qidFileHandles[qid] = fileInfo.fh;
            }

            return 0;
        }
        else if(qid.type() != lib9p::Qid::QTDIR && _ops->open) {
            struct fuse_file_info fileInfo;
            int err = _ops->open(path.c_str(), &fileInfo);

            if(err < 0) {
                throw new std::system_error(-err, std::generic_category());
            }

            _qidFileHandles[qid] = fileInfo.fh;
            return 0;
        }

        throw new lib9p::NotImplemented();
    }

    std::vector<uint8_t> FuseFileSystem::read(lib9p::Connection* conn, const lib9p::Qid& qid, uint64_t off, uint32_t count) {
        FUSE::getInstance()->setCurrentFileSystem(_name);

        std::string path = qid2Path(qid);

        if(qid.type() == lib9p::Qid::QTDIR && _ops->readdir) {
            if(path == "") {
                path = "/"s;
            }

            std::vector<uint8_t> ret;
            ret.reserve(conn->maxMessageSize());

            struct fillDirectoryArgs {
                std::vector<uint8_t>& stream;
                const std::string     path;
                FuseFileSystem*       fs;
                lib9p::Connection*    conn;
                const uint64_t        offset;
            } args = { ret, path, this, conn, off };

            auto fillDirectory = [](void* buffer, const char* name, const struct stat* s, off_t off, enum fuse_fill_dir_flags flags)->int {
                struct fillDirectoryArgs* args = static_cast<struct fillDirectoryArgs*>(buffer);

                // when this is the second or more 9p read
                // but the filesystem driver ignores the offset, we abort
                if(args->offset && !off) {
                    return 1;
                }

                struct stat sbuf;

                if(!s) {
                    if(args->fs->_ops->getattr) {
                        // we try to get some stat info if we didn't when from the start
                        // but don't error out if we don't get any
                        struct fuse_file_info fileInfo;
                        args->fs->_ops->getattr((args->path + "/"s + name).c_str(), &sbuf, &fileInfo);
                    }
                    else {
                        return 1;
                    }
                }
                else {
                    sbuf = *s;
                }

                const lib9p::Qid::Type type    = args->fs->translate(&sbuf);
                const lib9p::Qid&      qid     = args->fs->path2Qid(args->path + "/"s + name, type);
                const lib9p::Stat&     stat    = args->fs->translate(qid, &sbuf);

                std::stringstream statBuffer;
                stat.encode(statBuffer);

                size_t statLen   = statBuffer.tellp();
                size_t bufferLen = static_cast<size_t>(args->stream.size()) + statLen;

                if(bufferLen <= args->conn->maxMessageSize()) {
                    std::string str = statBuffer.str();
                    args->stream.insert(args->stream.end(), str.c_str(), str.c_str() + str.length());
                    return 0;
                }
                else {
                    if(off == 0) {
                        // filesystem driver is ignoring the offset, going for a
                        // "one readdir reads the whole dir in one run"
                        // We have to take the entries that fit in the first message
                        // and buffer all the others for later calls to
                        // FuseFileSystem::read(), so we can serve them without calling
                        // _ops->readdir() again
                        throw new lib9p::NotImplemented();
                    }
                    else {
                        return 1;
                    }
                }
            };

            struct fuse_file_info fileInfo;

            if(_qidFileHandles.contains(qid)) {
                fileInfo.fh = _qidFileHandles[qid];
            }

            int err = _ops->readdir(path.c_str(), reinterpret_cast<void*>(&args), fillDirectory, off, &fileInfo, static_cast<enum fuse_readdir_flags>(0));

            if(err < 0) {
                throw new std::system_error(-err, std::generic_category());
            }

            return ret;
        }
        else if(qid.type() != lib9p::Qid::QTDIR && _ops->read) {
            struct fuse_file_info fileInfo;
            fileInfo.fh = _qidFileHandles[qid];

            uint8_t *data = new uint8_t[count];
            int read = _ops->read(path.c_str(), (char*)data, count, off, &fileInfo);

            if(read < 0) {
                throw new std::system_error(-read, std::generic_category());
            }

            auto ret = std::vector<uint8_t>(data, data + read);
            delete[] data;
            return ret;
        }

        throw new lib9p::NotImplemented();
    }

    std::vector<lib9p::Qid> FuseFileSystem::walk(lib9p::Connection* conn, std::vector<std::string>& names, const lib9p::Qid& start) {
        FUSE::getInstance()->setCurrentFileSystem(_name);

        std::string startPath = qid2Path(start);

        std::vector<std::string> fullPaths;
        fullPaths.reserve(names.size());
        for(auto name = names.cbegin(); name != names.cend(); ++name) {
            std::string bp;
            if(fullPaths.size() == 0) {
                bp = startPath;
            }
            else {
                bp = fullPaths.back();
            }

            fullPaths.push_back(bp + "/" + *name);
        }

        std::vector<lib9p::Qid> ret;
        ret.reserve(names.size());

        for(auto fp = fullPaths.cbegin(); fp != fullPaths.cend(); ++fp) {
            lib9p::Qid::Type type;

            if(_paths.contains(*fp)) {
                type = _paths[*fp];
            }
            else if(_ops->getattr) {
                struct stat stat;
                struct fuse_file_info fileInfo;

                int err = _ops->getattr(fp->c_str(), &stat, &fileInfo);

                if(err < 0) {
                    throw new std::system_error(-err, std::generic_category());
                }

                type = translate(&stat);
            }
            else {
                throw new lib9p::NotImplemented();
            }

            if(type == lib9p::Qid::QTDIR || fp == fullPaths.cend() - 1) {
                ret.push_back(path2Qid(*fp, type));
            }
        }

        return ret;
    }

    const std::string FuseFileSystem::qid2Path(const lib9p::Qid& qid) {
        auto pathPair = reinterpret_cast<const std::pair<const std::string, const lib9p::Qid::Type>*>(qid.path());
        return pathPair->first;
    }

    lib9p::Qid FuseFileSystem::path2Qid(const std::string path, const lib9p::Qid::Type type) {
        if(_paths.find(path) == _paths.end()) {
            _paths.insert({path, type});
        }

        const auto p_it = _paths.find(path);

        if(p_it->second != type) {
            throw new lib9p::QidTypeError(type, p_it->second);
        }

        // "References and pointers to data stored in the container are only invalidated by
        // erasing that element, even when the corresponding iterator is invalidated."
        //  -- https://en.cppreference.com/w/cpp/container/unordered_map - Notes below "Iterator invalidation"
        // So dereferencing the iterator gives us a reference to the pair in the map. Making this
        // reference into a pointer gives us a pointer that is valid as long as the pair is not erased
        // from the map
        // XXX: of course this exposes pointers to the clients, ones we dereference after they give them
        //      back to us - we should add some validation maybe
        const std::pair<const std::string, const lib9p::Qid::Type>& pairRef = *p_it;
        const uint64_t qid_path = reinterpret_cast<uint64_t>(&pairRef);
        return lib9p::Qid(type, 0, qid_path);
    }

    lib9p::Qid::Type FuseFileSystem::translate(struct stat const* stat) {
        uint16_t type = lib9p::Qid::QTFILE;

        if((stat->st_mode & S_IFSOCK) == S_IFSOCK ||
           (stat->st_mode & S_IFCHR)  == S_IFCHR  ||
           (stat->st_mode & S_IFIFO)  == S_IFIFO) {
            type |= lib9p::Qid::QTAPPEND;
        }

        if((stat->st_mode & S_IFLNK) == S_IFLNK) {
            type |= lib9p::Qid::QTSYMLINK;
        }

        if((stat->st_mode & S_IFDIR) == S_IFDIR) {
            type |= lib9p::Qid::QTDIR;
        }

        return static_cast<lib9p::Qid::Type>(type);
    }

    lib9p::Stat FuseFileSystem::translate(const lib9p::Qid& qid, struct stat const* stat) {
        lib9p::Qid::Type type = translate(stat);

        if(qid.type() != type) {
            throw new lib9p::QidTypeError(qid.type(), type);
        }

        uint32_t mode = stat->st_mode & 0777;

        if(type & lib9p::Qid::QTDIR) {
            mode |= (1 << 31);
        }

        if(type & lib9p::Qid::QTAPPEND) {
            mode |= (1 << 30);
        }

        uint32_t dev = stat->st_dev;

        auto pathPair           = reinterpret_cast<const std::pair<const std::string, const lib9p::Qid::Type>*>(qid.path());
        auto filenamePos        = pathPair->first.find_last_of("/") + 1;
        const std::string& name = pathPair->first.substr(filenamePos);

        // TODO: we are supposed to translate the IDs to names
        std::string uid = std::to_string(stat->st_uid);
        std::string gid = std::to_string(stat->st_gid);

        return lib9p::Stat(
            type, dev,
            qid,
            mode,
            stat->st_atim.tv_sec,
            stat->st_mtim.tv_sec,
            stat->st_size,
            name.c_str(),
            uid, gid, ""s
        );
    }

    FUSE* FUSE::getInstance() {
        if(!_instance) {
            _instance = new FUSE();
        }

        return _instance;
    }

    FUSE::FUSE() :
#if __LF_OS__
        _server(0)
#else
        _server(1337)
#endif
    {
        if(thrd_success != tss_create(&_currentFileSystem, ::operator delete)) {
            throw new std::system_error(EINVAL, std::generic_category());
        }
    }

    int FUSE::exportFS(const std::string& name, const struct fuse_operations* ops, void* private_data) {
        try {
            _server.addFileSystem(name, new FuseFileSystem(ops, name));
            _fsPrivateData[name] = private_data;
        }
        catch(std::runtime_error* ex) {
            return EINVAL;
        }

        return 0;
    }

    void FUSE::fillContext(struct fuse_context* ctx) {
        const std::string& fs = getCurrentFileSystem();

        ctx->private_data = _fsPrivateData[fs];
    }

    int FUSE::mainloop() {
        try {
            _server.loop();
        }
        catch(std::runtime_error* ex) {
            return EINVAL;
        }

        return 0;
    }

    struct FUSE::CurrentFileSystem* FUSE::ensureCfsTls() {
        struct CurrentFileSystem* tl = static_cast<struct CurrentFileSystem*>(tss_get(_currentFileSystem));

        if(!tl) {
            tss_set(_currentFileSystem, static_cast<void*>(new CurrentFileSystem()));
        }

        return static_cast<struct CurrentFileSystem*>(tss_get(_currentFileSystem));
    }

    void FUSE::setCurrentFileSystem(const std::string& name) {
        struct CurrentFileSystem* tl = ensureCfsTls();
        tl->fs_name = name;
    }

    const std::string& FUSE::getCurrentFileSystem() {
        struct CurrentFileSystem* tl = ensureCfsTls();
        return tl->fs_name;
    }


    FUSE* FUSE::_instance = 0;
}

extern "C" {
    int fuse_9p_exportfs(const char* name, const struct fuse_operations* ops, void* private_data) {
        return lib9p::FUSE::getInstance()->exportFS(std::string(name), ops, private_data);
    }

    int fuse_9p_mainloop(int argc, char* argv[]) {
        return lib9p::FUSE::getInstance()->mainloop();
    }

    int fuse_main(int argc, char* argv[], const struct fuse_operations* ops, void* private_data) {
        int err = fuse_9p_exportfs("", ops, private_data);
        if(err != 0) {
            fprintf(stderr, "Error in fuse_9p_exportfs: %s (%d)", strerror(err), err);
        }

        return fuse_9p_mainloop(argc, argv);
    }

    struct fuse_context* fuse_get_context() {
        struct fuse_context* context = static_cast<struct fuse_context*>(malloc(sizeof(struct fuse_context)));
        lib9p::FUSE::getInstance()->fillContext(context);
        return context;
    }
}
