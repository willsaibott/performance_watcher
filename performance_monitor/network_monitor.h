#pragma once

#include "session_trace_handler.h"
#include "event_logger_file.h"
#include "tcpip.h"
#include "timestamp.h"
#include <evntrace.h>


namespace performance {
    #pragma pack (push, 1)
    struct tcp_data_t {
        std::size_t     packages{ 0 };
        std::size_t     connections{ 0 };
        std::size_t     connections_lost{ 0 };
        std::size_t     max_seg_size{ 0 };
        std::size_t     pkg_sent{ 0 };
        std::size_t     pkg_recv{ 0 };
        std::int64_t    bytes_sent{ 0 };
        std::int64_t    bytes_recv{ 0 };
        std::int64_t    retransmissions{ 0 };
        std::double_t   interval_ms{ 0 };
        std::int64_t    last_timestamp{ 0 };

        tcp_data_t() = default;
        tcp_data_t(const volatile tcp_data_t& obj) {
            packages = obj.packages;
            connections = obj.connections;
            connections_lost = obj.connections_lost;
            max_seg_size = obj.max_seg_size;
            pkg_sent = obj.pkg_sent;
            pkg_recv = obj.pkg_recv;
            bytes_sent = obj.bytes_sent;
            bytes_recv = obj.bytes_recv;
            retransmissions = obj.retransmissions;
            interval_ms = obj.interval_ms;
            last_timestamp = obj.last_timestamp;
        }
    };

    struct udp_data_t {
        std::size_t     packages{ 0 };
        std::size_t     connections_lost{ 0 };
        std::size_t     max_seg_size{ 0 };
        std::size_t     pkg_sent{ 0 };
        std::size_t     pkg_recv{ 0 };
        std::int64_t    bytes_sent{ 0 };
        std::int64_t    bytes_recv{ 0 };
        std::double_t   interval_ms{ 0 };
        std::int64_t    last_timestamp{ 0 };

        udp_data_t() = default;
        udp_data_t(const volatile udp_data_t& obj) {
            packages = obj.packages;
            connections_lost = obj.connections_lost;
            max_seg_size = obj.max_seg_size;
            pkg_sent = obj.pkg_sent;
            pkg_recv = obj.pkg_recv;
            bytes_sent = obj.bytes_sent;
            bytes_recv = obj.bytes_recv;
            interval_ms = obj.interval_ms;
            last_timestamp = obj.last_timestamp;
        }
    };
    #pragma pack (pop)

    class network_monitor_t {
    public:
        /** Please, see https://docs.microsoft.com/en-us/windows/win32/etw/nt-kernel-logger-constants */
        const uuid_t _tcpip_guid{ 0x9a280ac0,
                                  0xc8e0,
                                  0x11d1,
                                  0x84, 0xe2, 0x00, 0xc0, 0x4f, 0xb9, 0x98, 0xa2 };
        const uuid_t _udpip_guid{ 0xbf3a50c5,
                                  0xa9c9,
                                  0x4988,
                                  0xa0, 0x05, 0x2d, 0xf0, 0xb7, 0xc8, 0x0f, 0x80 };

        network_monitor_t() {};
        ~network_monitor_t() {};

        inline bool
        start(const uuid_t& provider_guid, const short pid) {
            _pid = pid;
            if (_session.start(provider_guid)) {
                _elogger.set_session_handler(_session);
                _elogger.add_callback([this](auto e) { event_callback(e); });
                if (_elogger.open()) {
                    return _elogger.process();
                }
            }
            return false;
        }

        inline void WINAPI
        event_callback(__in event_t e) {
            using namespace mof;
            // filter by pid
            short& e_pid = *(short*) (e->MofData);
            if (e_pid != _pid)
                return;
            if (is_tcpip(e)) {
                InterlockedIncrementSizeT(&(_tcp_data.packages));
                switch (e->Header.Class.Type) {
                case EVENT_TRACE_TYPE_CONNECT:
                {
                    tcp::connect_t& conn = *((tcp::connect_t*) e->MofData);
                    InterlockedIncrementSizeT(&(_tcp_data.connections));
                    _tcp_data.max_seg_size = conn.mss;
                    break;
                }
                case EVENT_TRACE_TYPE_DISCONNECT:
                {
                    tcp::disconnect_t& conn = *((tcp::disconnect_t*) e->MofData);
                    InterlockedDecrementSizeT(&(_tcp_data.connections));
                    InterlockedIncrementSizeT(&(_tcp_data.connections_lost));
                    break;
                }
                case EVENT_TRACE_TYPE_ACCEPT:
                case EVENT_TRACE_TYPE_RECONNECT:
                    break;
                case EVENT_TRACE_TYPE_RETRANSMIT:
                {
                    tcp::retransmit_t& conn = *((tcp::retransmit_t*) e->MofData);
                    InterlockedIncrement64(&(_tcp_data.retransmissions));
                    break;
                }
                case EVENT_TRACE_TYPE_RECEIVE:
                {
                    tcp::receive_t& recv = *((tcp::receive_t*) e->MofData);
                    InterlockedIncrementSizeT(&(_tcp_data.pkg_recv));
                    InterlockedAdd64(&(_tcp_data.bytes_recv), recv.size);
                    break;
                }
                case EVENT_TRACE_TYPE_SEND:
                {
                    tcp::receive_t& send = *((tcp::receive_t*) e->MofData);
                    InterlockedIncrementSizeT(&(_tcp_data.pkg_sent));
                    InterlockedAdd64(&(_tcp_data.bytes_recv), send.size);
                    break;
                }
                case EVENT_TRACE_TYPE_CONNFAIL:
                    InterlockedIncrementSizeT(&(_tcp_data.connections_lost));
                }
                _tcp_data.last_timestamp =
                    std::max(e->Header.TimeStamp.QuadPart, (int64_t)_tcp_data.last_timestamp);
            }
            else if (is_udpip(e)) {
                switch (e->Header.Class.Type) {
                case EVENT_TRACE_TYPE_RECEIVE:
                {
                    udp::receive_t& recv = *((udp::receive_t*) e->MofData);
                    InterlockedIncrementSizeT(&(_udp_data.pkg_recv));
                    InterlockedAdd64(&(_udp_data.bytes_recv), recv.size);
                    break;
                }
                case EVENT_TRACE_TYPE_SEND:
                {
                    udp::receive_t& send = *((udp::receive_t*) e->MofData);
                    InterlockedIncrementSizeT(&(_udp_data.pkg_sent));
                    InterlockedAdd64(&(_udp_data.bytes_recv), send.size);
                    break;
                }
                case EVENT_TRACE_TYPE_CONNFAIL:
                    InterlockedIncrementSizeT(&(_udp_data.connections_lost));
                    break;
                }
                _udp_data.last_timestamp =
                    std::max(e->Header.TimeStamp.QuadPart, (int64_t)_udp_data.last_timestamp);
            }
        }

        tcp_data_t
        tcp_data() const noexcept {
            tcp_data_t data = _tcp_data;
            auto last_ts = _last_tcp_data.last_timestamp;
            data.interval_ms = (data.last_timestamp - last_ts) / (double)_timestamp.frequency() * 1E6;
            _last_tcp_data = data;
            return data;
        }

        udp_data_t
        udp_data() const noexcept {
            auto data = _udp_data;
            auto last_ts = _last_udp_data.last_timestamp;
            data.interval_ms = (data.last_timestamp - last_ts) / (double)_timestamp.frequency() * 1E6;
            _last_udp_data = data;
            return data;
        }

    private:

        inline bool
        is_tcpip(const event_t e) const noexcept {
            return ::IsEqualGUID(_tcpip_guid, e->Header.Guid);
        }

        inline bool
        is_udpip(const event_t e) const noexcept {
            return ::IsEqualGUID(_udpip_guid, e->Header.Guid);
        }

        short                                     _pid{ std::numeric_limits<short>::max() };
        timestamp_t                               _timestamp;
        volatile tcp_data_t                       _tcp_data;
        volatile udp_data_t                       _udp_data;
        mutable tcp_data_t                        _last_tcp_data;
        mutable udp_data_t                        _last_udp_data;
        session_trace_handler_t                   _session;
        event_logger_file_t                       _elogger;
    };
}


