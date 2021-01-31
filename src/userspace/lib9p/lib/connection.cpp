#include <iostream>

#include <9p/connection>
#include <9p/transport>
#include <9p/message>

#include <9p/exception>

namespace lib9p {
    void Connection::push(Message* msg) {
        try {
            switch(msg->getType()) {
                case Message::Tversion:
                    if(_version == "") {
                        Tversion* version = dynamic_cast<Tversion*>(msg);

                        if(version->getVersion().substr(0, 6) == "9P2000") {
                            _version = "9P2000";
                            _maxMessageSize = version->getMsize();

                            respond(new Rversion(std::max(_maxMessageSize, (uint32_t)4096), _version), msg);
                        }
                    }
                    break;

                case Message::Tattach:
                    if(_attachHandler) {
                        Tattach* attach = dynamic_cast<Tattach*>(msg);

                        if(_fids.find(attach->getFid()) != _fids.end()) {
                            throw new FIDInUse();
                        }

                        _fids[attach->getFid()] = _attachHandler->attach(this, attach->getUName(), attach->getAName());
                        respond(new Rattach(_fids[attach->getFid()]), msg);
                    }
                    else {
                        throw new NotImplemented();
                    }
                    break;

                case Message::Tstat:
                    if(_statHandler) {
                        Tstat* stat = dynamic_cast<Tstat*>(msg);

                        if(_fids.find(stat->getFid()) == _fids.end()) {
                            throw new UnknownFID();
                        }
                        else {
                            Stat s = _statHandler->stat(this, _fids[stat->getFid()]);
                            respond(new Rstat(s), msg);
                        }
                    }
                    else {
                        throw new NotImplemented();
                    }
                    break;

                case Message::Twalk:
                    if(_walkHandler) {
                        Twalk* walk = dynamic_cast<Twalk*>(msg);

                        if(_fids.find(walk->getFid()) == _fids.end()) {
                            throw new UnknownFID();
                        }
                        else if(_fids.find(walk->getNewFid()) != _fids.end()) {
                            throw new FIDInUse();
                        }
                        else {
                            std::vector<std::string> names = walk->getWNames();
                            std::vector<Qid> qids;

                            if(names.size()) {
                                 qids = _walkHandler->walk(this, names, _fids[walk->getFid()]);

                                 if(qids.size() == names.size()) {
                                    _fids[walk->getNewFid()] = *(qids.end() - 1);
                                 }
                            }
                            else {
                                _fids[walk->getNewFid()] = _fids[walk->getFid()];
                            }

                            if(names.size() && !qids.size()) {
                                respond(new Rerror("illegal path element"), msg);
                            }
                            else {
                                respond(new Rwalk(qids), msg);
                            }
                        }
                    }
                    else {
                        throw new NotImplemented();
                    }
                    break;

                case Message::Tclunk:
                    {
                        Tclunk* clunk = dynamic_cast<Tclunk*>(msg);

                        if(_fids.find(clunk->getFid()) == _fids.end()) {
                            throw new UnknownFID();
                        }
                        else {
                            if(_clunkHandler) {
                                _clunkHandler->clunk(this, _fids[clunk->getFid()]);
                            }

                            _fids.erase(clunk->getFid());

                            respond(new Rclunk(), msg);
                        }
                    }
                    break;

                case Message::Topen:
                    {
                        Topen* o = dynamic_cast<Topen*>(msg);

                        if(_fids.find(o->getFid()) == _fids.end()) {
                            throw new UnknownFID();
                        }
                        else {
                            Qid& qid = _fids[o->getFid()];

                            if(_openHandler) {
                                uint32_t iounit = _openHandler->open(this, qid, o->getMode());
                                respond(new Ropen(qid, iounit), msg);
                            }
                            else {
                                throw new NotImplemented();
                            }
                        }
                    }
                    break;

                case Message::Tread:
                    {
                        Tread* r = dynamic_cast<Tread*>(msg);

                        if(_fids.find(r->getFid()) == _fids.end()) {
                            throw new UnknownFID();
                        }
                        else {
                            Qid& qid = _fids[r->getFid()];

                            if(_readHandler) {
                                std::basic_string<uint8_t> data = _readHandler->read(this, qid, r->getOffset(), r->getCount());
                                respond(new Rread(data), msg);
                            }
                        }
                    }
                    break;

                default:
                    throw new NotImplemented();
            }
        }
        catch(std::runtime_error* e) {
            respond(new Rerror(e->what()), msg);
        }
    }

    void Connection::respond(Message* msg, Message* original) {
        msg->setTag(original->getTag());
        _transport->writeMessage(msg, _transportArg);

        delete msg;
    }
}
