#pragma once

#include "mcv_platform.h"
#include "utils/utils.h"
#include "murmur3/murmur3.h"
#include <fstream>

// From time.h
TElapsedTime Time;

// From file_context.h
std::vector< const std::string* > TFileContext::files;

bool isWndFocused() {
    return (GetForegroundWindow() == CApplication::get().getHandle());
}

bool isPressed(int key) {
    return ::GetAsyncKeyState(key) & 0x8000;
}

void dbg(const char* format, ...) {
  PROFILE_FUNCTION("dbg");
  va_list argptr;
  va_start(argptr, format);
  char dest[1024 * 8];
  int n = _vsnprintf(dest, sizeof(dest), format, argptr);
  dest[n] = 0x00;
  va_end(argptr);
  // This takes forever...
  ::OutputDebugString(dest);
}

bool fatal(const char* format, ...) {
    va_list argptr;
    va_start(argptr, format);
    char dest[1024 * 8];
    int n = _vsnprintf(dest, sizeof(dest), format, argptr);
    dest[n] = 0x00;
    va_end(argptr);
    ::OutputDebugString(dest);

    /*std::string file_context = TFileContext::getFileContextStack();
    if (!file_context.empty()) {
        strcat(dest, "Files context:\n");
        strcat(dest, file_context.c_str());
    }*/

    std::string file_context = TFileContext::getFileContextStack();
    if (!file_context.empty()) {
      strcat(dest, "\nAt file context:\n");
      strcat(dest, file_context.c_str());
    }

    if (MessageBox(nullptr, dest, "Error!", MB_RETRYCANCEL) == IDCANCEL)
        exit(-1);
    return false;
}

void centerMousePos()
{
    RECT wRect;
    HWND hWnd = CApplication::get().getHandle();

    GetClientRect(hWnd, &wRect);
    POINT p;
    p.x = wRect.left;
    p.y = wRect.top;
    ClientToScreen(hWnd, &p);
    VEC2 centerPos(
        p.x + (wRect.right / 2.f),
        p.y + (wRect.bottom / 2.f));
    SetCursorPos((int)centerPos.x, (int)centerPos.y);
}

void printVEC2(VEC2 v)
{
    dbg("(%f %f)\n", v.x, v.y);
}

void printVEC3(VEC3 v)
{
    dbg("(%f %f %f)\n", v.x, v.y, v.z);
}

void printFloat(const std::string& name, float v)
{
    dbg("%s: %f\n", name.c_str(), v);
}

float clampRotation(float angle)
{
    float pi2 = 2.0f * (float)M_PI;
    int turns = (int)floor(angle / (pi2));
    return angle - pi2 * turns;
}

// --------------------------------------------------------
json loadJson(const std::string& filename) {
    PROFILE_FUNCTION(filename);

    json j;

    while (true) {

        std::ifstream ifs(filename.c_str());
        if (!ifs.is_open()) {
            fatal("Failed to open json file %s\n", filename.c_str());
            continue;
        }

#ifdef NDEBUG

        j = json::parse(ifs, nullptr, false);
        if (j.is_discarded()) {
            ifs.close();
            fatal("Failed to parse json file %s\n", filename.c_str());
            continue;
        }

#else

        try
        {
            // parsing input with a syntax error
            j = json::parse(ifs);
        }
        catch (json::parse_error & e)
        {
            ifs.close();
            // output exception information
            fatal("Failed to parse json file %s\n%s\nAt offset: %d\n"
                , filename.c_str(), e.what(), e.byte);
            continue;
        }

#endif

        // The json is correct, we can leave the while loop
        break;
    }

    return j;
}

// generate a hash from the input buffer
uint32_t getID(const void* buff, size_t nbytes) {
  uint32_t seed = 0;
  uint32_t out_value = 0;
  MurmurHash3_x86_32(buff, (uint32_t)nbytes, seed, &out_value);
  assert(out_value != 0);
  return out_value;
}

uint32_t getID(const char* txt) {
  return getID(txt, strlen(txt));
}

void toLowerCase(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(),
        [](unsigned char c) { return std::tolower(c); });
}

bool includes(const std::string& str, const std::string& token) {
    return str.find(token) != std::string::npos;
}

bool fileExists(const char* name) {
  FILE *f = fopen( name, "rb" );
  if( !f )
    return false;
  fclose( f );
  return true;
}

bool isPointInRectangle(VEC2 point, VEC2 size, VEC2 pos)
{
    VEC2 pointLocal = point - pos;
    return pointLocal.x >= 0.f && pointLocal.y >= 0.f
        && pointLocal.x <= 0.f + size.x && pointLocal.y <= 0.f + size.y;
}