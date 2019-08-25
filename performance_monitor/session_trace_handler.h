#pragma once

#pragma comment(lib, "rpcrt4.lib")
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <strsafe.h>
#include <wmistr.h>
#include <evntrace.h>

#include <filesystem>
#include <exception>
#include <iostream>
#include <string>

namespace std {
    inline std::string
    to_string(const UUID& uuid) {
        char* str;
        (void)UuidToStringA(&uuid, (RPC_CSTR*)& str);
        std::string ret{ str };
        RpcStringFreeA((RPC_CSTR*)& str);
        return ret;
    }
}

namespace performance {
    using uuid_t = UUID;
    using trace_handle_t = TRACEHANDLE;
    using event_trace_properties_t = EVENT_TRACE_PROPERTIES;

    class session_trace_handler_t {
    public:

        session_trace_handler_t() {
            set_session_name("ETW-section");
            set_log_trace_path(
                std::filesystem::temp_directory_path() / "ETW" / _session_name.append(".etl"));
        }

        ~session_trace_handler_t() {
            stop();
        }

        inline bool
        start(const uuid_t& provider_id) {
            try {
                _provider_guid = provider_id;
                start();
                std::cout << "Session "  << _session_handle
                          << " at " << _log_trace_file_path
                          << " was created and enabled successfully.";
            }
            catch (std::exception& err) {
                std::cerr << err.what();
                stop();
                return false;
            }
            return true;
        }

        inline bool
        stop() {
            bool no_errors{ true };
            if (_session_handle && _session_handle != INVALID_PROCESSTRACE_HANDLE) {
                if (_session_enabled) {
                    auto status = EnableTraceEx2(_session_handle,
                                                 (LPCGUID)& _provider_guid,
                                                 EVENT_CONTROL_CODE_DISABLE_PROVIDER,
                                                 TRACE_LEVEL_INFORMATION,
                                                 0,
                                                 0,
                                                 0,
                                                 NULL);
                    if (ERROR_SUCCESS != status) {
                        std::cerr << "EnableTraceEx2(disable) failed: " << get_last_error_as_string();
                        no_errors = false;
                    }
                }
                auto status = ControlTrace(_session_handle,
                                           _session_name.data(),
                                           event_trace_properties(),
                                           EVENT_TRACE_CONTROL_STOP);
                if (ERROR_SUCCESS != status) {
                    std::cerr << "ControlTrace(stop) failed: " << get_last_error_as_string();
                    no_errors = false;
                }
                status = ::StopTrace(_session_handle, _session_name.data(), event_trace_properties());
                if (ERROR_SUCCESS != status) {
                    std::cerr << "Unable to stop trace: " << get_last_error_as_string();
                    no_errors = false;
                }
            }
            _session_handle         = INVALID_PROCESSTRACE_HANDLE;
            _event_trace_properties = nullptr;
            return no_errors;
        }

        inline trace_handle_t
        session_handle() const noexcept {
            return _session_handle;
        }

        inline event_trace_properties_t*
        event_trace_properties() const noexcept {
            return (event_trace_properties_t*)(_event_trace_properties.get());
        }

        inline void
        set_session_name(const std::string_view session_name) {
            if (_session_enabled) return;
            _session_name = session_name.data();
        }

        inline void
        set_log_trace_path(const std::filesystem::path& path) {
            if (_session_enabled) return;
            if (!std::filesystem::exists(path.parent_path())) {
                std::filesystem::create_directories(path.parent_path());
            }
            _log_trace_file_path = std::filesystem::absolute(path).make_preferred();
        }

        inline std::filesystem::path
        log_trace_path() const noexcept {
            return _log_trace_file_path;
        }

        inline std::string_view
        session_name() const noexcept {
            return _session_name;
        }

        inline static uuid_t
        create_guid() {
            uuid_t guid;
            auto result = ~RPC_S_OK;
            while (result != RPC_S_OK) {
                result = UuidCreate(&guid);
            }
            return guid;
        }

        inline static std::string
        get_last_error_as_string() {
            //Get the error message, if any.
            DWORD errorMessageID = ::GetLastError();
            if (errorMessageID == 0)
                return std::string(); //No error message has been recorded

            LPSTR messageBuffer = nullptr;
            size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)& messageBuffer, 0, NULL);

            std::string message(messageBuffer, size);

            //Free the buffer.
            LocalFree(messageBuffer);

            return std::to_string(errorMessageID).append(" original error: ")
                                                 .append(message);
        }

        uuid_t
        provider_guid() const noexcept {
            return _provider_guid;
        }

        uuid_t
        session_guid() const noexcept {
            return _session_guid;
        }

    private:


        inline void
        start() {
            const std::string log_path_str = _log_trace_file_path.string();
            const auto log_name_size       = sizeof(char) * log_path_str.size() + 1;
            const auto session_name_size = sizeof(char) * _session_name.size() + 1;
            const auto buffer_size =
                sizeof(event_trace_properties_t) + log_name_size + session_name_size;

            if (_session_name.empty()) {
                throw std::runtime_error("session name must not be empty.");
            }
            if (log_path_str.empty()) {
                throw std::runtime_error("log path must not be empty.");
            }

            _event_trace_properties = std::make_unique<byte[]>(buffer_size);
            if (_event_trace_properties == nullptr) {
                throw std::runtime_error("Unable to allocate bytes for session properties");
            }

            memset(_event_trace_properties.get(), 0, buffer_size);
            auto& session_props               = *event_trace_properties();
            session_props.Wnode.BufferSize    = (uint32_t)buffer_size;
            session_props.Wnode.Flags         = WNODE_FLAG_TRACED_GUID;
            session_props.Wnode.ClientContext = 1; //QPC clock resolution
            session_props.Wnode.Guid          = _session_guid;
            session_props.LogFileMode         = EVENT_TRACE_REAL_TIME_MODE;
            session_props.MaximumFileSize     = 1;  // 1 MB
            session_props.LoggerNameOffset    = (uint32_t)sizeof(EVENT_TRACE_PROPERTIES);
            session_props.LogFileNameOffset   = (uint32_t)sizeof(EVENT_TRACE_PROPERTIES) + (uint32_t)session_name_size;
            session_props.EnableFlags         = EVENT_TRACE_FLAG_NETWORK_TCPIP | EVENT_TRACE_FLAG_DISK_FILE_IO | EVENT_TRACE_FLAG_DISK_IO;
            StringCbCopyA(
                (STRSAFE_LPSTR)((char*)_event_trace_properties.get() + session_props.LogFileNameOffset),
                log_name_size,
                log_path_str.data());

            // Create the trace session.
            auto status = StartTrace((PTRACEHANDLE)& _session_handle,
                                     _session_name.data(),
                                     event_trace_properties());
            if (ERROR_SUCCESS != status) {
                throw std::runtime_error("Unable to start trace: " + get_last_error_as_string());
            }
            status = EnableTraceEx2(_session_handle,
                                    (LPGUID)&_provider_guid,
                                    EVENT_CONTROL_CODE_ENABLE_PROVIDER,
                                    TRACE_LEVEL_INFORMATION,
                                    0, 0, 0, NULL);
            if (ERROR_SUCCESS != status) {
                throw std::runtime_error("Unable to enable trace: " + get_last_error_as_string());
            }
            _session_enabled = true;
        }

        bool                                         _session_enabled{ false };
        trace_handle_t                               _session_handle{ INVALID_PROCESSTRACE_HANDLE };
        std::unique_ptr<byte[]>                      _event_trace_properties{nullptr};
        uuid_t                                       _session_guid{ 0x123a54c5, 0xc9a9, 0x9488, 0xa3, 0x35, 0x2e, 0xfa, 0xb4, 0xb1, 0x77, 0x88 };
        uuid_t                                       _provider_guid{ create_guid() };
        std::filesystem::path                        _log_trace_file_path;
        std::string                                  _session_name;
    };
}

