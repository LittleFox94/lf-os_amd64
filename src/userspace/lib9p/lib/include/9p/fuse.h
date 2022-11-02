/**
 * lib9p additions to fuse
 */

#pragma once

#if __cplusplus
extern "C" {
#endif

#include <fuse.h>

//! Export the filesystem implemented with ops under the given name
int fuse_9p_exportfs(const char* name, const struct fuse_operations* ops, void* private_data);

//! Start the main loop, making the exported filesystems usable by other processes
int fuse_9p_mainloop(int argc, char* argv[]);

#if __cplusplus
}

#ifdef _LIB9P_INTERNALS
#include <9p/helper/filesystem>
#include <9p/helper/server_starter>

namespace lib9p {
    class FuseFileSystem : public lib9p::FileSystem {
        public:
            FuseFileSystem(const struct fuse_operations* ops, const std::string& name);

            virtual lib9p::Qid attach(lib9p::Connection* conn, std::string user, std::string fs);

            virtual lib9p::Stat stat(lib9p::Connection* conn, const lib9p::Qid& qid);

            virtual void clunk(lib9p::Connection* conn, const lib9p::Qid& qid);

            virtual uint32_t open(lib9p::Connection* conn, const lib9p::Qid& qid, uint8_t mode);

            virtual std::basic_string<uint8_t> read(lib9p::Connection* conn, const lib9p::Qid& qid, uint64_t off, uint32_t count);

            virtual std::vector<lib9p::Qid> walk(lib9p::Connection* conn, std::vector<std::string>& names, const lib9p::Qid& start);

        private:
            const struct fuse_operations* _ops;
            const std::string&            _name;

            std::unordered_map<std::string, const lib9p::Qid::Type> _paths;
            std::unordered_map<lib9p::Qid, uint64_t>                _qidFileHandles;

            const std::string qid2Path(const lib9p::Qid& qid);

            lib9p::Qid path2Qid(const std::string path, const lib9p::Qid::Type type);

            lib9p::Qid::Type translate(struct stat const* stat);

            lib9p::Stat translate(const lib9p::Qid& qid, struct stat const* stat);
    };

    class FUSE {
        public:
            static FUSE* getInstance();

            FUSE();

            int exportFS(const std::string& name, const struct fuse_operations* ops, void* private_data);

            void fillContext(struct fuse_context* ctx);

            int mainloop();

        private:
            ServerStarter                _server;
            static FUSE*                 _instance;
            std::map<std::string, void*> _fsPrivateData;

            struct CurrentFileSystem {
                std::string fs_name;
            };

            tss_t _currentFileSystem;

            struct CurrentFileSystem* ensureCfsTls();

            void setCurrentFileSystem(const std::string& name);

            const std::string& getCurrentFileSystem();

        friend class FuseFileSystem;
    };
}
#endif // _LIB9P_INTERNALS
#endif // __cplusplus
