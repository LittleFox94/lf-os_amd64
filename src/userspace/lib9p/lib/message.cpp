#include <9p/message>
#include <9p/wireutils>

#include <stdexcept>

namespace lib9p {
    const uint16_t Message::NoTag;
    const uint32_t Message::NoFid;

    Message* Message::decode(std::istream& data) {
        Type type = (Type)WireUtils::read<uint8_t>(data);

        switch(type) {
            case Tversion: return new class Tversion(data);
            case Rversion: return new class Rversion(data);
            case Tauth:    return new class Tauth(data);
            case Rauth:    return new class Rauth(data);
            case Tattach:  return new class Tattach(data);
            case Rattach:  return new class Rattach(data);
            case Rerror:   return new class Rerror(data);
            case Tflush:   return new class Tflush(data);
            case Rflush:   return new class Rflush(data);
            case Twalk:    return new class Twalk(data);
            case Rwalk:    return new class Rwalk(data);
            case Topen:    return new class Topen(data);
            case Ropen:    return new class Ropen(data);
            case Tcreate:  return new class Tcreate(data);
            case Rcreate:  return new class Rcreate(data);
            case Tread:    return new class Tread(data);
            case Rread:    return new class Rread(data);
            case Twrite:   return new class Twrite(data);
            case Rwrite:   return new class Rwrite(data);
            case Tclunk:   return new class Tclunk(data);
            case Rclunk:   return new class Rclunk(data);
            case Tremove:  return new class Tremove(data);
            case Rremove:  return new class Rremove(data);
            case Tstat:    return new class Tstat(data);
            case Rstat:    return new class Rstat(data);
            case Twstat:   return new class Twstat(data);
            case Rwstat:   return new class Rwstat(data);
            default:
                throw new std::invalid_argument("Unknown, probably invalid, message type");
                break;
        }
    }
}
