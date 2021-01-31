#include <9p/stat>
#include <9p/wireutils>

namespace lib9p {
    Stat::Stat(std::istream& data) {
        WireUtils::read<uint16_t>(data); // read the length
                                         // TODO: we currently ignore it,
                                         // better would maybe be to read $length
                                         // bytes from the stream and try to decode
                                         // the object from those bytes only
        WireUtils::read<uint16_t>(data); // for some reason we have length + 2 and length
                                         // in the stream, so we grab the second one, too

        _type   = WireUtils::read<uint16_t>(data);
        _dev    = WireUtils::read<uint32_t>(data);
        _qid    = Qid(data);
        _mode   = WireUtils::read<uint32_t>(data);
        _atime  = WireUtils::read<uint32_t>(data);
        _mtime  = WireUtils::read<uint32_t>(data);
        _length = WireUtils::read<uint64_t>(data);
        _name   = WireUtils::read<std::string>(data);
        _uid    = WireUtils::read<std::string>(data);
        _gid    = WireUtils::read<std::string>(data);
        _muid   = WireUtils::read<std::string>(data);
    }

    void Stat::encode(std::ostream& data) const {
        std::ostringstream body;
        WireUtils::write(_type, body);
        WireUtils::write(_dev, body);
        WireUtils::write(_qid, body);

        WireUtils::write(_mode, body);
        WireUtils::write(_atime, body);
        WireUtils::write(_mtime, body);
        WireUtils::write(_length, body);

        WireUtils::write(_name, body);
        WireUtils::write(_uid, body);
        WireUtils::write(_gid, body);
        WireUtils::write(_muid, body);

        const auto& str = body.str();
        WireUtils::write((uint16_t)(str.length()), data);

        data << str;
    }

    const bool Stat::operator==(const Stat& b) const {
        return b._type   == _type   && b._dev   == _dev     && b._qid   == _qid
            && b._mode   == _mode   && b._atime == _atime   && b._mtime == _mtime
            && b._length == _length && b._name  == _name    && b._uid   == _uid
            && b._gid    == _gid    && b._muid  == _muid;
    }

    const bool Stat::operator!=(const Stat& b) const {
        return b._type   != _type   || b._dev   != _dev     || b._qid   != _qid
            || b._mode   != _mode   || b._atime != _atime   || b._mtime != _mtime
            || b._length != _length || b._name  != _name    || b._uid   != _uid
            || b._gid    != _gid    || b._muid  != _muid;
    }
}
