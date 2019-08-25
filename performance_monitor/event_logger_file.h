#pragma once
#include "session_trace_handler.h"
#include <optional>
#include <functional>
#include <future>

#include <evntcons.h>
#include <tdh.h>

namespace std {
    template <class T>
    using optional_ref = optional<reference_wrapper<T>>;
}

namespace performance {

    using event_trace_logfile_t = EVENT_TRACE_LOGFILE;
    using trace_event_info_t    = TRACE_EVENT_INFO;
    using event_t               = PEVENT_TRACE;
    inline void WINAPI
    event_callback(__in  event_t etrace);

    class event_logger_file_t {
        friend inline void WINAPI event_callback(__in  event_t etrace);
        const std::size_t buffer_size    = 1024;
        const std::size_t tr_buffer_size = sizeof(trace_event_info_t) + buffer_size;
    public:
        using callback_t = std::function < void (event_t)>;

        ~event_logger_file_t() noexcept {
            close();
            if (_thread.joinable()) {
                _thread.join();
            }
        }

        inline void
        set_session_handler(const session_trace_handler_t& session) {
            _session = session;
        }

        inline void
        add_callback(callback_t callback) {
            _callbacks.push_back(callback);
        }

        inline bool
        open() {
            try {
                _open();
            }
            catch (std::exception& err) {
                std::cerr << "Error when opening trace log: " << err.what();
                close();
                return false;
            }
            return true;
        }

        inline bool
        process() noexcept(false){
            _thread = std::thread([this](){
                try {
                    if (_trace_is_open) {
                        auto status = ::ProcessTrace(&_trace_handle, 1, NULL, NULL);
                        if (ERROR_SUCCESS != status) {
                            throw std::runtime_error("Failed to ProcessTrace: " + session_trace_handler_t::get_last_error_as_string());
                        }
                    }
                    else {
                        throw std::runtime_error("Trace handle isn't open");
                    }
                }
                catch (std::exception &err) {
                    _failed_to_process = true;
                    std::cerr << err.what();
                }
            });
            std::this_thread::sleep_for(std::chrono::milliseconds{ 200 });
            return _thread.joinable() && !_failed_to_process;
        }

        inline bool
        close() noexcept {
            if (_trace_is_open) {
                auto status = ::CloseTrace(_trace_handle);
                return ERROR_SUCCESS != status;
            }
            _trace_is_open = false;
            return true;
        }

    private:
        inline void
        _open() noexcept(false) {
            if (!_session.has_value()) {
                throw std::runtime_error("No valid session was given");
            }
            const auto& session = _session->get();
            _session_name = session.session_name();
            _log_path     = session.log_trace_path().string();
            memset(&_e_log_file, 0, sizeof(_e_log_file));

            //_e_log_file.LogFileName         = _log_path.data();
            _e_log_file.LoggerName          = _session_name.data();
            // real time mode and use EventRecordCallback
            _e_log_file.ProcessTraceMode    = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
            // register buffer callback
            //_e_log_file.BufferCallback      = &this->buffer_callback;
            // register record callback function
            _e_log_file.EventCallback       = &event_callback;
            _e_log_file.Context             = NULL;

            _trace_handle = ::OpenTrace(&_e_log_file);
            if (_trace_handle == INVALID_PROCESSTRACE_HANDLE) {
                throw std::runtime_error("Failed to OpenTrace: " + session_trace_handler_t::get_last_error_as_string());
            }
            _trace_is_open = true;
        }

        /*PEVENT_TRACE_LOGFILEA
        buffer_callback(PEVENT_RECORD pEvent) {

        }*/

        std::optional_ref<const session_trace_handler_t> _session{ std::nullopt };
        event_trace_logfile_t                            _e_log_file{};
        trace_handle_t                                   _trace_handle{ INVALID_PROCESSTRACE_HANDLE };
        std::string                                      _session_name;
        std::string                                      _log_path;
        static std::vector<callback_t>                   _callbacks;
        bool                                             _trace_is_open{ false };
        bool                                             _failed_to_process{ false };
        std::thread                                      _thread;
    };

    inline void WINAPI
    event_callback(__in  event_t etrace) {
        for (auto callback : event_logger_file_t::_callbacks) {
            callback(etrace);
        }
    }
}

