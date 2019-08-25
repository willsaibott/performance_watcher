#pragma once

#include <deque>
namespace performance {

    template<class T = double>
    class moving_average_t {
    public:
    moving_average_t() = default;
    moving_average_t(size_t new_size) {
        set_window_size(new_size);
    }

    inline void
    set_window_size(size_t new_size) noexcept {
        _window_size = new_size;
        _window.resize(new_size);
    }

    inline T
    add_sample(T new_sample) {
        if (_n_of_samples >= _window_size) {
            _window.pop_front();
        }
        else {
            _n_of_samples++;
        }
        _window.push_back(new_sample);
        _average = 0;
        for (auto& elem : _window) {
            _average += elem;
        }
        _average /= _n_of_samples;
        return _average;
    }

    inline T
    operator +=(T new_sample) {
        return add_sample(new_sample);
    }

    inline T
    operator *() const noexcept {
        return _average;
    }

    private:
        T                 _average{ 0 };
        std::deque<T>     _window;
        size_t            _window_size{32};
        size_t            _n_of_samples{ 0 };
    };
}