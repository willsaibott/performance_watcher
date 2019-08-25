// watcher.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <sstream>
#include <string>
#include <iostream>
#include <performance_monitor/network_monitor.h>
#include <performance_monitor/moving_average.h>

#include "console_screen_buffer.h"

inline static console::screen_buffer_t screen;
inline static bool s_running = true;
const auto Kib = 1024;
const auto Mib = 1024 * Kib;
const auto Gib = 1024 * Mib;
namespace perf = performance;
inline std::string
get_readable_size(double value) {
    value *= 8;
    if (value >= Gib) {
        return std::to_string(value / Gib).append(" Gbps");
    }
    else if (value >= Mib) {
        return std::to_string(value / Mib).append(" Mbps");
    }
    else if (value >= Kib) {
        return std::to_string(value / Kib).append(" Kbps");
    }
    else {
        return std::to_string(value).append(" bps");
    }
}

BOOL WINAPI
consoleHandler(DWORD signal) {
    using namespace std::chrono_literals;
    if (signal == CTRL_C_EVENT) {
        screen.clean_screen();
        s_running = false;
        screen << "Stopped by user" << console::flush_t{};
        screen.make_inactive();
        std::cout << "\n\n\nStopped by user\n\n";
    }
    return TRUE;
}

int main(int argc, char* argv[]) {
    using namespace std::chrono_literals;
    SetConsoleTitle("Peformance Monitor Watcher 2019");
    if (argc < 2) {
        std::cerr << "Invalid call. Usage: watcher.exe <PID: short> <Interval(ms): ull>";
        return EXIT_FAILURE;
    }
    if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
        std::cout << "\nERROR: Could not set control handler";
        return EXIT_FAILURE;
    }
    system("logman stop ETW-section.etl -ets > out.txt");
    try {
        auto interval = (argc >= 3 ? std::stoull(argv[2]) : 100ull);
        short pid = std::stoi(argv[1]);
        uuid_t my_id = perf::session_trace_handler_t::create_guid();

        perf::network_monitor_t monitor;
        if (monitor.start(my_id, pid)) {

            if (!screen.create()) {
                std::cerr << "Unable to create console buffer: "
                    << perf::session_trace_handler_t::get_last_error_as_string();
                return EXIT_FAILURE;
            }
            if (!screen.make_active()) {
                std::cerr << "Unable to make console buffer active: "
                    << perf::session_trace_handler_t::get_last_error_as_string();
                return EXIT_FAILURE;
            }

            screen << console::foreground_color_t{console::color_t::DARKCYAN}
                <<  "TCP data:"
                << console::foreground_color_t{ console::color_t::WHITE }
                << "\nconnections:      "
                << "\nconnections_lost: "
                << "\nmax_seg_size:     "
                << "\npackages:         "
                << "\nretransmissions:  "
                << "\npkgs sent:        "
                << "\npkgs recv:        "
                << "\nbytes sent:       "
                << "\nbytes recv:       "
                << "\nbytes sent speed: "
                << "\nbytes sent speed: "
                << "\nbytes recv speed: "
                << "\nbytes recv speed: "
                << "\ninterval:         "
                << "\nlast timestamp:   "
                << console::foreground_color_t{ console::color_t::DARKCYAN }
                << "\n\nUDP data:       "
                << console::foreground_color_t{ console::color_t::WHITE }
                << "\nconnections_lost: "
                << "\nmax_seg_size:     "
                << "\npackages:         "
                << "\npkgs sent:        "
                << "\npkgs recv:        "
                << "\nbytes sent:       "
                << "\nbytes recv:       "
                << "\nbytes sent speed: "
                << "\nbytes sent speed: "
                << "\nbytes recv speed: "
                << "\nbytes recv speed: "
                << "\ninterval:         "
                << "\nlast timestamp:   "
                << console::flush_t{};

            std::stringstream ss;
            perf::moving_average_t bytes_sent1{ 1024 }, bytes_recv1{ 1024 };
            perf::moving_average_t bytes_sent2{ 1024 }, bytes_recv2{ 1024 };
            perf::moving_average_t udp_bytes_sent1{ 1024 }, udp_bytes_recv1{ 1024 };
            perf::moving_average_t udp_bytes_sent2{ 1024 }, udp_bytes_recv2{ 1024 };
            auto last_tcp_data = monitor.tcp_data();
            auto last_udp_data = monitor.udp_data();
            perf::timestamp_t ts;
            while (s_running) {
                auto tcp_data = monitor.tcp_data();
                auto udp_data = monitor.udp_data();
                auto interv1 = tcp_data.interval_ms > 0 ? tcp_data.interval_ms : 1E9;
                auto interv2 = (tcp_data.last_timestamp - last_tcp_data.last_timestamp) / 100.0;
                auto udp_interv1 = udp_data.interval_ms > 0 ? udp_data.interval_ms : 1E9;
                auto udp_interv2 = (udp_data.last_timestamp - last_udp_data.last_timestamp) / 100.0;
                interv2 = interv2 > 0 ? interv2 : 1E9;
                udp_interv2 = udp_interv2 > 0 ? udp_interv2 : 1E9;

                bytes_sent1 += (tcp_data.bytes_sent - last_tcp_data.bytes_sent) / interv1;
                bytes_sent2 += (tcp_data.bytes_sent - last_tcp_data.bytes_sent) / interv2;
                bytes_recv1 += (tcp_data.bytes_recv - last_tcp_data.bytes_recv) / interv1;
                bytes_recv2 += (tcp_data.bytes_recv - last_tcp_data.bytes_recv) / interv2;
                udp_bytes_sent1 += (udp_data.bytes_sent - last_udp_data.bytes_sent) / interv1;
                udp_bytes_sent2 += (udp_data.bytes_sent - last_udp_data.bytes_sent) / interv2;
                udp_bytes_recv1 += (udp_data.bytes_recv - last_udp_data.bytes_recv) / interv1;
                udp_bytes_recv2 += (udp_data.bytes_recv - last_udp_data.bytes_recv) / interv2;

                std::this_thread::sleep_for(5ms);

                if (ts.ms() > interval) {
                    screen
                        << console::position_t{ 1, 18 } << tcp_data.connections
                        << console::position_t{ 2, 18 } << tcp_data.connections_lost
                        << console::position_t{ 3, 18 } << tcp_data.max_seg_size
                        << console::position_t{ 4, 18 } << tcp_data.packages
                        << console::position_t{ 5, 18 } << tcp_data.retransmissions
                        << console::position_t{ 6, 18 } << tcp_data.pkg_sent
                        << console::position_t{ 7, 18 } << tcp_data.pkg_recv
                        << console::position_t{ 8, 18 } << tcp_data.bytes_sent
                        << console::position_t{ 9, 18 } << tcp_data.bytes_recv
                        << console::position_t{ 10, 18 } << get_readable_size(*bytes_sent1)
                        << console::position_t{ 11, 18 } << get_readable_size(*bytes_sent2)
                        << console::position_t{ 12, 18 } << get_readable_size(*bytes_recv1)
                        << console::position_t{ 13, 18 } << get_readable_size(*bytes_recv2)
                        << console::position_t{ 14, 18 } << tcp_data.interval_ms
                        << console::position_t{ 15, 18 } << tcp_data.last_timestamp
                        << console::position_t{ 18, 18 } << udp_data.connections_lost
                        << console::position_t{ 19, 18 } << udp_data.max_seg_size
                        << console::position_t{ 20, 18 } << udp_data.packages
                        << console::position_t{ 21, 18 } << udp_data.pkg_sent
                        << console::position_t{ 22, 18 } << udp_data.pkg_recv
                        << console::position_t{ 23, 18 } << udp_data.bytes_sent
                        << console::position_t{ 24, 18 } << udp_data.bytes_recv
                        << console::position_t{ 25, 18 } << get_readable_size(*udp_bytes_sent1)
                        << console::position_t{ 26, 18 } << get_readable_size(*udp_bytes_sent2)
                        << console::position_t{ 27, 18 } << get_readable_size(*udp_bytes_recv1)
                        << console::position_t{ 28, 18 } << get_readable_size(*udp_bytes_recv2)
                        << console::position_t{ 29, 18 } << udp_data.interval_ms
                        << console::position_t{ 30, 18 } << udp_data.last_timestamp
                        << console::flush_t{};
                    ts.now();
                }
                std::this_thread::sleep_for(300ms);
            }
        }
        else {
            std::cerr << "Failed to start monitor. Aborting\n";
            return EXIT_FAILURE;
        }
    }
    catch (std::invalid_argument& err) {
        std::cerr << "Fail to parse input arguments.\n"
                  << "Usage: watcher.exe <PID: short> <Interval(ms): ull>\n"
                  << "Original exception: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (std::out_of_range& err) {
        std::cerr << "Fail to parse input arguments.\n"
                  << "Usage: watcher.exe <PID: short> <Interval(ms): ull>\n"
                  << "Original exception: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (std::exception& err) {
        std::cerr << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started:
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
