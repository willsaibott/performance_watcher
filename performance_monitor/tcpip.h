#pragma once

#include <cinttypes>
#include <evntrace.h>
#include <evntcons.h>
#pragma pack(push, 1)

/** please, see https://docs.microsoft.com/en-us/windows/win32/etw/tcpip */
namespace mof {

    class EventTrace {
        std::uint16_t EventSize;
        std::uint16_t ReservedHeaderField;
        std::uint8_t  EventType;
        std::uint8_t  TraceLevel;
        std::uint16_t TraceVersion;
        std::uint64_t ThreadId;
        std::uint64_t TimeStamp;
        std::uint8_t  EventGuid[16];
        std::uint32_t KernelTime;
        std::uint32_t UserTime;
        std::uint32_t InstanceId;
        std::uint8_t  ParentGuid[16];
        std::uint32_t ParentInstanceId;
        std::uint32_t MofData;
        std::uint32_t MofLength;
    };

    struct MSNT_SystemTrace : EventTrace {
        std::uint32_t Flags;
    };

    namespace tcp {
        struct TcpIp_TypeGroup1 {
            std::uint32_t PID;
            std::uint32_t size;
            std::uint8_t  daddr[4];
            std::uint8_t  saddr[4];
            std::uint16_t dport;
            std::uint16_t sport;
            std::uint32_t seqnum;
            std::uint32_t connid;
        };

        struct TcpIp_TypeGroup2 {
            std::uint32_t PID;
            std::uint32_t size;
            std::uint8_t  daddr[4];
            std::uint8_t  saddr[4];
            std::uint16_t dport;
            std::uint16_t sport;
            std::uint16_t mss;
            std::uint16_t sackopt;
            std::uint16_t tsopt;
            std::uint16_t wsopt;
            std::uint16_t rcvwin;
            std::int16_t  rcvwinscale;
            std::int16_t  sndwinscale;
            std::uint32_t seqnum;
            std::uint16_t connid;
        };

        struct TcpIp_SendIPV4 {
            std::uint32_t PID;
            std::uint32_t size;
            std::uint8_t  daddr[4];
            std::uint8_t  saddr[4];
            std::uint16_t dport;
            std::uint16_t sport;
            std::uint32_t startime;
            std::uint32_t endtime;
            std::uint32_t seqnum;
            std::uint32_t connid;
        };

        struct TcpIp_Fail {
            std::uint16_t Proto;
            std::uint16_t FailureCode;
        };

        struct accept_t : TcpIp_TypeGroup2 {};
        struct connect_t : TcpIp_TypeGroup2 {};
        struct disconnect_t : TcpIp_TypeGroup1 {};
        struct receive_t : TcpIp_TypeGroup1 {};
        struct reconnect_t : TcpIp_TypeGroup1 {};
        struct retransmit_t : TcpIp_TypeGroup1 {};
        struct send_t : TcpIp_SendIPV4 {};
        struct fail_t : TcpIp_Fail {};
    }

    /** please, see https://docs.microsoft.com/en-us/windows/win32/etw/udpip */
    namespace udp {
        struct UdpIp_TypeGroup1 {
            std::uint32_t PID;
            std::uint32_t size;
            std::uint8_t  daddr[4];
            std::uint8_t  saddr[4];
            std::uint16_t dport;
            std::uint16_t sport;
            std::uint32_t seqnum;
            std::uint32_t connid;
        };

        struct UdpIp_Fail {
            std::uint16_t Proto;
            std::uint16_t FailureCode;
        };

        struct receive_t : UdpIp_TypeGroup1 {};
        struct send_t : UdpIp_TypeGroup1 {};
        struct fail_t : UdpIp_Fail {};
    }
}
#pragma pack(pop)