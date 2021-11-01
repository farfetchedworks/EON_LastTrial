#pragma once

// ----------------------------------------------------------
// To compute time elapsed between ctor and elapsed()
class CTimer {
  uint64_t time_stamp;
public:
  CTimer() {
    reset();
  }

  // Ticks!
  uint64_t getTimeStamp() const {
    uint64_t now;
    ::QueryPerformanceCounter((LARGE_INTEGER*)&now);
    return now;
  }

  // Reset counter to current timestamp
  void reset() {
    time_stamp = getTimeStamp();
  }

  double elapsed() {
    uint64_t now = getTimeStamp();
    uint64_t delta_ticks = now - time_stamp;

    LARGE_INTEGER freq;
    if (::QueryPerformanceFrequency(&freq)) {
      double delta_secs = (double)(delta_ticks) / (double)freq.QuadPart;
      return delta_secs;
    }
    fatal("QueryPerformanceFrequency returned false!!!\n");
    return 0.f;
  }

  double elapsedAndReset() {
    double delta = elapsed();
    reset();
    return delta;
  }

};



// ----------------------------------------------------------
// Holds a global with the elapsed/unscaled and current time
struct TElapsedTime {
  float  delta = 0.f;
  double current = 0.f;
  float  scale_factor = 1.0f;
  float  delta_unscaled = 0.f;
  void set(double new_time) {
    delta_unscaled = (float)(new_time - current);
    current = new_time;
    delta = delta_unscaled * scale_factor;
  }
};

extern TElapsedTime Time;