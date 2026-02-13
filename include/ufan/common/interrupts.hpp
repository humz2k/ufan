#pragma once

#include <signal.h>
#include <thread>

namespace ufan::common {

namespace interrupts_impl {
inline volatile sig_atomic_t should_run = 1;
inline void sigint_handler(int signal) { should_run = 0; }
inline void setup() { signal(SIGINT, sigint_handler); }
} // namespace interrupts_impl

template <typename F> inline void run_forever(F&& lambda) {
    interrupts_impl::setup();
    while (interrupts_impl::should_run) {
        lambda();
    }
}

template <typename F> inline void run_forever() {
    interrupts_impl::setup();
    while (interrupts_impl::should_run) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        volatile int x = 0; // to not optimize out this loop
    }
}

} // namespace ufan::common
