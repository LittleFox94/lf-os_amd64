#include <9p/qid>
#include <9p/wireutils>

#include <stdexcept>

namespace lib9p {
    Qid::Qid(std::istream& data) {
        _type    = (Type)WireUtils::read<uint8_t>(data);
        _version = WireUtils::read<uint32_t>(data);
        _path    = WireUtils::read<uint64_t>(data);
    }

    const bool Qid::operator==(const Qid& b) const {
        return b._type == _type && b._version == _version && b._path == _path;
    }

    const bool Qid::operator!=(const Qid& b) const {
        return b._type != _type || b._version != _version || b._path != _path;
    }

    void Qid::encode(std::ostream& data) const {
        WireUtils::write((uint8_t)_type, data);
        WireUtils::write(_version, data);
        WireUtils::write(_path, data);
    }
}
