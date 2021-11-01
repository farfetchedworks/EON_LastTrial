#pragma once

// Fake struct
typedef struct _RGNDATA {
  void*   rdh;
  char            Buffer[1];
} RGNDATA;
#include <d3d9.h>

struct TGpuTrace {

  static void push(const char* name) {
    wchar_t wname[128];
    mbstowcs(wname, name, 128);
    D3DPERF_BeginEvent(D3DCOLOR_XRGB(192, 192, 255), wname);
    //rmt_BeginD3D11SampleDynamic(name);
  }

  static void pop() {
    //rmt_EndD3D11Sample();
    D3DPERF_EndEvent();
  }

  static void setMarker(const char* event_name, D3DCOLOR clr = D3DCOLOR_XRGB(192, 255, 255)) {
    wchar_t wname[128];
    mbstowcs(wname, event_name, 128);
    D3DPERF_SetMarker(clr, wname);
  }

};

struct CGpuScope {
  CGpuScope(const char* name) {
    TGpuTrace::push(name);
  }
  ~CGpuScope() {
    TGpuTrace::pop();
  }
};


