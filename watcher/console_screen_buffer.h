#pragma once

#define NOMINMAX
#include <Windows.h>

#include <algorithm>
#include <string>
#include <vector>
#include <cmath>

namespace console {
    enum color_t {
        BLACK       = 0,
        DARKBLUE    = FOREGROUND_BLUE,
        DARKGREEN   = FOREGROUND_GREEN,
        DARKCYAN    = FOREGROUND_GREEN | FOREGROUND_BLUE,
        DARKRED     = FOREGROUND_RED,
        DARKMAGENTA = FOREGROUND_RED | FOREGROUND_BLUE,
        DARKYELLOW  = FOREGROUND_RED | FOREGROUND_GREEN,
        DARKGRAY    = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
        GRAY        = FOREGROUND_INTENSITY,
        BLUE        = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
        GREEN       = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
        CYAN        = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
        RED         = FOREGROUND_INTENSITY | FOREGROUND_RED,
        MAGENTA     = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
        YELLOW      = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
        WHITE       = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    };
    struct position_t {
        const std::int16_t x;
        const std::int16_t y;
        position_t(std::int16_t _y, std::int16_t _x) : x(_x), y(_y) {}
    };
    struct at_start_t : position_t { at_start_t() : position_t(0, 0) {} };
    struct foreground_color_t {
        const WORD color;
        foreground_color_t(WORD c) : color(c) {}
    };
    struct background_color_t {
        const WORD color;
        background_color_t(WORD c) : color(c) {}
    };

    struct line_up_t   {};
    struct line_down_t {};

    template <short t>
    struct next_t {};

    struct flush_t {};

    class screen_buffer_t {
        using handle_t = HANDLE;
    public:

        screen_buffer_t() {
            //_grid = std::vector<CHAR_INFO*>(_lines);
            for (auto& line : _grid) {
                //line = new CHAR_INFO[_columns];
                for (auto jj = 0; jj < _columns; jj++) {
                    line[jj].Attributes = _attr;
                    line[jj].Char.AsciiChar = ' ';
                }
            }
        }
        ~screen_buffer_t() {
            (void)make_inactive();
            (void)close();
        }

        inline void
        reload() {
            CHAR_INFO basic;
            basic.Char.AsciiChar = ' ';
            basic.Attributes = _attr;
            for (auto& line : _grid) {
                //delete line;
            }
            //_grid = std::vector<CHAR_INFO*>(_lines);
            for (auto& line : _grid) {
                //line = new CHAR_INFO[_columns];
                for (auto jj = 0; jj < _columns; jj++) {
                    line[jj].Attributes = _attr;
                    line[jj].Char.AsciiChar = ' ';
                }
            }
            clean_screen();
        }

        inline bool
        create() {
            if (_created) {
                ::CloseHandle(_handle);
                _created = false;
            }
            _handle = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                                FILE_SHARE_READ,
                                                NULL,
                                                CONSOLE_TEXTMODE_BUFFER,
                                                NULL);
            (void)GetConsoleScreenBufferInfo(_handle, &_info);
            return _created = _handle != INVALID_HANDLE_VALUE;
        }

        inline bool
        make_active() {
            _old_handle = GetStdHandle(STD_OUTPUT_HANDLE);
            return _activated = ::SetConsoleActiveScreenBuffer(_handle);
        }

        inline bool
        make_inactive() {
            if (_activated) {
                _activated = ::SetConsoleActiveScreenBuffer(_old_handle);
                return !_activated;
            }
            return false;
        }

        inline bool
        close() {
            return ::CloseHandle(_handle);
        }

        inline void
        clean_screen() {
            for (auto& line : _grid) {
                for (auto jj = 0; jj < _columns; jj++) {
                    line[jj].Char.AsciiChar = ' ';
                }
            }
            _x = _y = 0;
        }

        inline void
        set_number_of_lines(std::int16_t new_number_of_lines) {
            _lines = new_number_of_lines;
            reload();
        }

        inline void
        set_number_of_columns(std::int16_t new_number_of_cols) {
            _columns = new_number_of_cols;
            reload();
        }

        template <class T>
        inline screen_buffer_t&
        operator << (T obj) {
            return this->operator<< (std::to_string(obj));
        }

        inline screen_buffer_t&
        operator << (std::string_view str) {
            return this->operator<< (str.data());
        }

        inline screen_buffer_t&
        operator << (const char* data) {
            return this->operator<< (std::string(data));
        }

        inline bool
        flush() {
            COORD buffer_size{ _columns, _lines };
            COORD position{ 0, 0 };
            SMALL_RECT rect{ 0, 0, _columns, _lines };
            return ::WriteConsoleOutput(_handle,
                                        _grid[0],
                                        buffer_size,
                                        position,
                                        &rect);
        }

        inline screen_buffer_t&
        operator << (const std::string& str) {
            for (auto& c : str) {
                switch (c) {
                case '\n':
                    for (auto jj = _x; jj < _columns; jj++) {
                        _grid[_y][jj].Char.AsciiChar = ' ';
                    }
                    increment_x(_columns - _x);
                    break;
                case '\r': increment_x(-_x);                                 break;
                case '\b': increment_x(-1);                                  break;
                case '\t': increment_x(4);                                   break;
                default:
                    _grid[_y][_x].Char.AsciiChar = c;
                    _grid[_y][_x].Attributes = _attr;
                    increment_x(1);
                    break;
                }
            }
            return *this;
        }

        inline screen_buffer_t&
        operator << (const position_t &pos) {
            _x = pos.x;
            _y = pos.y;
            return *this;
        }

        inline screen_buffer_t&
        operator << (const foreground_color_t &obj) {
            _attr = 0x000F & obj.color;
            return *this;
        }

        inline screen_buffer_t&
        operator << (const background_color_t &obj) {
            _attr = 0x00F0 & obj.color;
            return *this;
        }

        inline screen_buffer_t&
        operator << (flush_t) {
            flush();
            return *this;
        }

        inline screen_buffer_t&
        operator << (line_up_t) {
            _y = std::max(_y - 1, 0);
            return *this;
        }

        inline screen_buffer_t&
        operator << (line_down_t) {
            _y = std::min(_y + 1, _lines - 1);
            return *this;
        }

        template <short t>
        inline screen_buffer_t&
        operator << (next_t<t>) {
            increment_x(t);
            return *this;
        }

    private:

        inline void
        increment_x(short v) {
            _x += v;
            if (_x >= _columns) {
                _x = _x- _columns;
                _y = (std::int16_t)std::min(_y + 1, _lines - 1);
            }
        }

        bool                               _created{ false };
        bool                               _activated{ false };
        std::int16_t                       _lines{ 50 };
        std::int16_t                       _columns{ 50 };
        std::int16_t                       _x{ 0 };
        std::int16_t                       _y{ 0 };
        WORD                               _attr{ color_t::WHITE };
        //std::vector<CHAR_INFO*>            _grid;
        CHAR_INFO                          _grid[50][50] = {{}};
        handle_t                           _handle{ INVALID_HANDLE_VALUE };
        handle_t                           _old_handle{ INVALID_HANDLE_VALUE };
        CONSOLE_SCREEN_BUFFER_INFO         _info{};
    };
}

