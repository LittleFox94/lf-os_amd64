#include "tarfilesystem.h"

#include <iostream>
#include <iomanip>
#include <map>
#include <iterator>
#include <regex>
#include <vector>
#include <variant>
#include <string_view>

namespace nineptar {
    TarFileSystem::TarFileSystem(const std::string& archiveData) {
        auto archive = std::stringstream(archiveData);

        while(archive) {
            std::string filename;
            uint64_t    mode;
            uint64_t    length;
            uint64_t    mtime;
            uint64_t    checksum;
            uint8_t     type;
            std::string linkedFilename;
            std::string ustar;
            std::string owner;
            std::string group;
            std::string filenamePrefix;

            archive >> std::setw(100)                   >> filename;
            archive >> std::setw(7)  >> std::setbase(8) >> mode;
            archive.ignore(17);
            archive >> std::setw(11) >> std::setbase(8) >> length;
            archive.ignore(1);
            archive >> std::setw(11) >> std::setbase(8) >> mtime;
            archive.ignore(1);
            archive >> std::setw(7)  >> std::setbase(8) >> checksum;
            archive.ignore(1);
            archive >> type;
            archive >> std::setw(100) >> linkedFilename;
            archive >> std::setw(5)   >> ustar;
            archive.ignore(3);
            archive >> std::setw(32)  >> owner;
            archive >> std::setw(32)  >> group;
            archive.ignore(16);
            archive >> std::setw(155) >> filenamePrefix;
            archive.ignore(12);

            std::erase(filename, '\0');
            std::erase(linkedFilename, '\0');
            std::erase(owner, '\0');
            std::erase(group, '\0');
            std::erase(filenamePrefix, '\0');

            if(ustar == "ustar") {
                filename = filenamePrefix + filename;
            }

            if(filename.starts_with(".")) {
                filename.erase(0, 1);
            }

            if(filename.ends_with("/")) {
                filename.erase(filename.length() - 1, 1);
            }

            if((type == 0 || type == '0' || type == '5') && (filename != "" || !_nodes.size())) {
                Node* node;

                switch(type) {
                    case 0:
                    case '0':
                        node = new Node(
                            Node::Type::File,
                            archiveData.substr(archive.tellg(), length)
                        );
                        break;
                    case '5':
                        node = new Node(Node::Type::Directory);
                        break;
                }

                node->mode = mode;
                node->mtime = mtime;
                node->user  = owner;
                node->group = group;
                node->muser = owner;
                node->name  = filename.substr(filename.find_last_of("/") + 1);

                if(node->name == "") {
                    node->name = "/";
                }

                std::string path = filename.substr(0, filename.find_last_of("/"));

                if(_nodes.size()) {
                    Node* parent = nodeByPath(_nodes[0], path);

                    if(parent) {
                        node->parent = parent;
                        std::vector<Node*>& children = std::get<std::vector<Node*>>(parent->data);
                        children.push_back(node);
                    }
                }

                _nodes.push_back(node);
            }

            if(length % 512) {
                length += 512 - (length % 512);
            }

            archive.ignore(length);
        }
    }

    lib9p::Qid TarFileSystem::attach(lib9p::Connection* conn, std::string user, std::string fs) {
        setAsHandler(conn);
        return nodeQid(_nodes[0]);
    }

    lib9p::Stat TarFileSystem::stat(lib9p::Connection* conn, const lib9p::Qid& qid) {
        if(qid.path() >= _nodes.size()) {
            throw new lib9p::NotImplemented();
        }

        return nodeStat(_nodes[qid.path()]);
    }

    std::vector<lib9p::Qid> TarFileSystem::walk(lib9p::Connection* conn, std::vector<std::string>& names, const lib9p::Qid& start) {
        std::vector<lib9p::Qid> res;

        Node* dest    = nodeByPath(_nodes[start.path()], names);
        Node* current = dest;

        size_t i = names.size();
        while(current && i--) {
            res.push_back(nodeQid(current));
            current = current->parent;
        }

        std::reverse(res.begin(), res.end());
        return res;
    }

    uint32_t TarFileSystem::open(lib9p::Connection* conn, const lib9p::Qid& qid, uint8_t mode) {
        const Node* node = _nodes[qid.path()];

        if(!node) {
            throw new lib9p::NotImplemented();
        }

        return uint32_t(0);
    }

    std::basic_string<uint8_t> TarFileSystem::read(lib9p::Connection* conn, const lib9p::Qid& qid, uint64_t offset, uint32_t length) {
        const Node* node = _nodes[qid.path()];

        if(!node) {
            throw new lib9p::NotImplemented();
        }

        std::basic_ostringstream<uint8_t> res;

        if(node->type == Node::Type::File) {
            const std::string_view& content = std::get<std::string_view>(node->data);
            res.write((uint8_t*)(content.data() + offset), std::min(size_t(length), content.length() - offset));
        }
        else if(node->type == Node::Type::Directory) {
            static uint64_t last_offset;
            static std::vector<Node*>::const_iterator last_iterator;

            if(!offset || offset == last_offset) {
                if(!offset) {
                    last_offset = 0;
                }

                const std::vector<Node*>& children = std::get<std::vector<Node*>>(node->data);
                for(auto it = offset ? last_iterator : children.cbegin(); it != children.cend(); ++it) {
                    std::stringstream single;
                    lib9p::Stat stat = nodeStat(*it);
                    stat.encode(single);

                    if(res.str().length() + single.str().length() <= length) {
                        const std::string& s = single.str();
                        res.write((uint8_t*)s.data(), s.length());
                    }
                    else {
                        last_offset   += res.str().length();
                        last_iterator = it;
                        break;
                    }
                }
            }
        }

        return res.str();
    }

    lib9p::Qid TarFileSystem::nodeQid(const Node* node) {
        const auto& inode = std::find(_nodes.begin(), _nodes.end(), node);

        return lib9p::Qid(
            node->type == Node::Type::Directory ? lib9p::Qid::QTDIR : lib9p::Qid::QTFILE,
            0,
            uint64_t(std::distance(_nodes.begin(), inode))
        );
    }

    lib9p::Stat TarFileSystem::nodeStat(const Node* node) {
        uint64_t length = 0;
        uint32_t mode   = node->mode;

        if(node->type == Node::Type::File) {
            length = std::get<std::string_view>(node->data).length();
        }
        else if(node->type == Node::Type::Directory) {
            mode |= 0x80000000;
        }

        return lib9p::Stat(0, 0, nodeQid(node), mode, node->mtime, node->mtime, length, node->name, node->user, node->group, node->muser);
    }

    TarFileSystem::Node* TarFileSystem::nodeByPath(Node* root, const std::vector<std::string>& path) {
        Node* current = root;

        for(size_t i = 0; i < path.size(); ++i && current && current->type == Node::Type::Directory) {
            const std::string& name = path[i];
            Node* node = nullptr;

            if(name == "..") {
                node = current->parent;

                if(!node) {
                    node = current;
                }
            }
            if(name == "") {
                node = current;
            }
            else {
                const std::vector<Node*>& children = std::get<std::vector<Node*>>(current->data);
                std::vector<Node*>::const_iterator it = std::find_if(children.cbegin(), children.cend(), [&](const Node* node) { return node->name == name; });

                if(it != children.cend()) {
                    node = *it;
                }
            }

            if(!node) {
                return nullptr;
            }

            current = node;
        }

        return current;
    }

    TarFileSystem::Node* TarFileSystem::nodeByPath(Node* root, const std::string& path) {
        std::regex regexz("/");
        std::vector<std::string> parts(std::sregex_token_iterator(path.begin(), path.end(), regexz, -1),
                                      std::sregex_token_iterator());
        return nodeByPath(root, parts);
    }
};
