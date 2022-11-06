#include <exception>
#include <unordered_map>

#include <sys/fileops.h>

#include <9p/helper/client>
#include <9p/transports/lf_os>

namespace lib9p {
    class _cstdlibglobals {
        public:
            _cstdlibglobals()
                : _transport(LFOSTransport(0)), _client(Client(_transport)) {

                if(_instance) {
                    throw new std::runtime_error("Only a single instance of lib9p::_cstdlibglobals is allowed!");
                }

                _instance  = this;
            }

            void registerFileops() {
                static struct file_operations fileops = {
                    _init,
                    _open,
                    _close,
                    _read,
                    _write
                };

                __fileops_set_default(&fileops);
            }

        private:
            static _cstdlibglobals* _instance;

            LFOSTransport _transport;
            Client        _client;

            int open(const char* path, int flags) {
                Client::Entry* e = _instance->_client.open(std::string(path));

                if(!e) {
                    return -ENOENT;
                }

                return -ENOSYS;
            }

            int close(int fd) {
                return -ENOSYS;
            }

            ssize_t read(int fd, void* buffer, size_t flags) {
                return -ENOSYS;
            }

            ssize_t write(int fd, const void* buffer, size_t len) {
                return -ENOSYS;
            }

            // singleton helpers

            static void _init() {
                _instance->_client.connect();
                _instance->_transport.discoverService();
            }

            static int _open(const char* path, int flags) {
                return _instance->open(path, flags);
            }

            static int _close(int fd) {
                return _instance->close(fd);
            }

            static ssize_t _read(int fd, void* buffer, size_t flags) {
                return _instance->read(fd, buffer, flags);
            }

            static ssize_t _write(int fd, const void* buffer, size_t len) {
                return _instance->write(fd, buffer, len);
            }
    };

    _cstdlibglobals __cstdlibglobals;
    _cstdlibglobals* _cstdlibglobals::_instance;
}

extern "C" void __init_lib9p() {
    lib9p::__cstdlibglobals.registerFileops();
}
