#pragma once

template< typename T >
class NamedValues {

public:

  struct TEntry {
    T           value = 0;
    // const char* name = "unknown";
    std::string name = "unknown";
  };

private:
  const TEntry* data = nullptr;
  int           count = 0;

public:
  NamedValues(const TEntry* new_data, int new_count)
    : data(new_data)
    , count(new_count)
  {}

  int size() const { return count; }

  void set(const TEntry* new_data, int new_count) {
    data = new_data;
    count = new_count;
  }

  const TEntry* nth(int idx) const {
    assert(idx >= 0 && idx < size());
    return data + idx;
  }

  const char* nameOf(T value) const {
    for (int i = 0; i < count; ++i) {
      if (data[i].value == value)
        return data[i].name.c_str();
    }
    return "unknown";
  }

  T valueOf(const char* name) const {
    assert(name);
    for (int i = 0; i < count; ++i) {
      if (strcmp(data[i].name.c_str(), name) == 0)
        return data[i].value;
    }
    fatal("Can't identify value for name '%s' in the %d entries\n", name, count);
    return T();
  }

  bool debugInMenu(const char* title, T& state) const {

    // Simplified one-liner Combo() using an accessor function
    struct FuncHolder {
      static bool ItemGetter(void* data, int idx, const char** out_str) {
        NamedValues<T>* nv = (NamedValues<T>*)data;
        *out_str = nv->nth(idx)->name.c_str();
        return true;
      }
    };

    // Convert T value to index for ImGui
    int curr_idx = 0;
    for (int i = 0; i < count; ++i) {
      if (data[i].value == state) {
        curr_idx = i;
        break;
      }
    }

    if (ImGui::Combo(title, &curr_idx, &FuncHolder::ItemGetter, (void*)this, this->size())) {
      state = data[curr_idx].value;
      return true;
    }
    return false;
  }

};

