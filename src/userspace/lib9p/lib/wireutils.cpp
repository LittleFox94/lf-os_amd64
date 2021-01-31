#include <9p/wireutils>

namespace lib9p { namespace WireUtils {
    template<>
    uint8_t read(std::istream& data) {
        return uint8_t(data.get());
    }

    template<>
    uint16_t read(std::istream& data) {
        uint8_t c[2];
        data.read((char*)c, 2);
        return c[0] | (c[1] << 8);
    }

    template<>
    uint32_t read(std::istream& data) {
        uint8_t c[4];
        data.read((char*)c, 4);
        return c[0] | (c[1] << 8) | (c[2] << 16) | (c[3] << 24);
    }

    template<>
    uint64_t read(std::istream& data) {
        uint8_t c[8];
        data.read((char*)c, 8);

        return  (uint64_t)c[0]        | ((uint64_t)c[1] << 8)  |
               ((uint64_t)c[2] << 16) | ((uint64_t)c[3] << 24) |
               ((uint64_t)c[4] << 32) | ((uint64_t)c[5] << 40) |
               ((uint64_t)c[6] << 48) | ((uint64_t)c[7] << 56);
    }

    template<>
    std::string read(std::istream& data) {
        uint16_t len = read<uint16_t>(data);

        std::string ret(len, 0);
        data.read(ret.data(), len);

        return ret;
    }

    template<>
    std::string const read(std::istream& data) {
        return read<std::string>(data);
    }

    template<>
    std::vector<std::string> read(std::istream& data) {
        uint16_t num = read<uint16_t>(data);
        std::vector<std::string> strings(num);

        for(size_t i = 0; i < num; ++i) {
            strings[i] = read<std::string>(data);
        }

        return strings;
    }

    template<>
    Stat read(std::istream& data) {
        return Stat(data);
    }

    template<>
    Qid read(std::istream& data) {
        return Qid(data);
    }

    template<>
    std::vector<Qid> read(std::istream& data) {
        uint16_t num = read<uint16_t>(data);

        std::vector<Qid> qids(num);
        for(size_t i = 0; i < num; ++i) {
            qids[i] = read<Qid>(data);
        }

        return qids;
    }

    template<>
    std::basic_string<uint8_t> read(std::istream& data) {
        uint32_t size = read<uint32_t>(data);
        std::basic_string<uint8_t> ret(size, ' ');
        data.read((char*)ret.data(), size);
        return ret;
    }

    void write(const std::basic_string_view<uint8_t> val, std::ostream& data) {
        write((uint32_t)val.length(), data);
        data.write((char*)val.data(), val.length());
    }

    void write(const std::string val, std::ostream& data) {
        write((uint16_t)val.length(), data);
        data.write(val.data(), val.length());
    }

    void write(const std::vector<std::string> val, std::ostream& data) {
        write((uint16_t)val.size(), data);

        for(std::string s : val) {
            write(s, data);
        }
    }

    void write(uint8_t val, std::ostream& data) {
        data << (char)val;
    }

    void write(uint16_t val, std::ostream& data) {
        data << (char)(val & 0xFF) << ((char)(val >> 8));
    }

    void write(uint32_t val, std::ostream& data) {
        data << (char)(val & 0xFF)           << ((char)((val >> 8) & 0xFF))
             << ((char)((val >> 16) & 0xFF)) << ((char)((val >> 24) & 0xFF));
    }

    void write(uint64_t val, std::ostream& data) {
        data << (char)(val & 0xFF)           << ((char)((val >> 8) & 0xFF))
             << ((char)((val >> 16) & 0xFF)) << ((char)((val >> 24) & 0xFF))
             << ((char)((val >> 32) & 0xFF)) << ((char)((val >> 40) & 0xFF))
             << ((char)((val >> 48) & 0xFF)) << ((char)((val >> 56) & 0xFF));
    }

    void write(const Qid qid, std::ostream& data) {
        qid.encode(data);
    }

    void write(const std::vector<Qid> qids, std::ostream& data) {
        write((uint16_t)qids.size(), data);

        for(const Qid& qid : qids) {
            qid.encode(data);
        }
    }

    void write(const Stat stat, std::ostream& data) {
        std::ostringstream body;
        stat.encode(body);

        const std::string& statData = body.str();
        write(statData, data);
    }
} }
