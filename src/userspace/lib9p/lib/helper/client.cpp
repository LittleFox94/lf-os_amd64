#include <random>
#include <regex>
#include <iostream>

#include <9p/helper/client>
#include <9p/message>


namespace lib9p {
    Client::Client(Transport& transport)
        : _transport(transport) {
        _transport.setResponseHandler(this);
    }

    void Client::connect() {
        sendMessage<Tversion>([this](const Message* msg) {
            if (msg->getType() != Message::Rversion) {
                throw std::runtime_error("received unexpected message");
            }

            auto rversion = dynamic_cast<const Rversion*>(msg);

            _msize = rversion->getMsize();
            std::cout << "received Rversion for version " << rversion->getVersion() << " and MSize " << _msize;

            if (rversion->getVersion() != "9P2000") {
                throw std::runtime_error("received unexpected version");
            }

            sendMessage<Tattach>([this](const Message* msg) {
                if (msg->getType() != Message::Rattach) {
                    throw std::runtime_error("received unexpected message");
                }

                _connected = true;
            }, 0, 0, "", "");
        }, 8192, "9P2000");
    }

    uint16_t Client::nextTag() {
        uint16_t tag;
        auto rng_dist   = std::uniform_int_distribution<uint16_t>(0, 65535);
        auto rng_engine = std::subtract_with_carry_engine<uint16_t, 12, 5, 12>();

        do {
            tag = rng_dist(rng_engine);
        } while(_pendingOperations.find(tag) != _pendingOperations.end());

        return tag;
    }

    void Client::handle9pResponse(const Message* msg) {
        uint16_t tag = msg->getTag();

        if(_pendingOperations.find(tag) != _pendingOperations.end()) {
            _pendingOperations[tag](msg);
            _pendingOperations.erase(tag);
        }
        else {
            throw new std::runtime_error("Received response with unknown tag");
        }
    }

    Client::Entry* Client::open(const std::string& path) {
        std::regex regexz("/");
        std::vector<std::string> pathSegments = {
            std::sregex_token_iterator(
                path.begin(),
                path.end(),
                regexz,
                -1
            ),
            std::sregex_token_iterator()
        };

        sendMessage<Twalk>([](const Message* msg) {
            if (msg->getType() != Message::Rwalk) {
                throw std::runtime_error("received unexpected message");
            }
        }, 0, 1, pathSegments);

        return nullptr;
    }
}
