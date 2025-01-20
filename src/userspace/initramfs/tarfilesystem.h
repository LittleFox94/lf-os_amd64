#include <variant>

#include <9p/exception>
#include <9p/helper/filesystem>

namespace nineptar {
    class TarFileSystem: public lib9p::FileSystem {
        public:
            TarFileSystem(const std::string& archive);

            virtual lib9p::Qid attach(lib9p::Connection* conn, std::string user, std::string fs);

            virtual lib9p::Stat stat(lib9p::Connection* conn, const lib9p::Qid& qid);

            virtual std::vector<lib9p::Qid> walk(lib9p::Connection* conn, std::vector<std::string>& names, const lib9p::Qid& start);

            virtual uint32_t open(lib9p::Connection* conn, const lib9p::Qid& qid, uint8_t mode);

            virtual std::vector<uint8_t> read(lib9p::Connection* conn, const lib9p::Qid& qid, uint64_t offset, uint32_t length);

        private:
            struct Node {
                std::string name;

                enum class Type {
                    File,
                    Directory,
                } type;

                uint32_t    mode;
                std::string user;
                std::string group;
                std::string muser;
                uint64_t    mtime;

                std::variant<std::vector<uint8_t>, std::vector<Node*>> data;

                Node* parent;

                Node(Type t) :
                    type(t),
                    data(std::vector<Node*>()) {
                }

                Node(Type t, std::string_view d) :
                    type(t),
                    data(std::vector<uint8_t>(d.data(), d.data() + d.length())) {
                }
            };

            std::vector<Node*> _nodes;

            lib9p::Qid nodeQid(const Node* node);

            lib9p::Stat nodeStat(const Node* node);

            Node* nodeByPath(Node* root, const std::vector<std::string>& path);

            Node* nodeByPath(Node* root, const std::string& path);
    };
};
