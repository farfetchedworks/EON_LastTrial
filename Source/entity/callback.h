#pragma once

struct IMsgBaseCallback {
  virtual void sendMsg(CHandle h_recv, const void* msg) = 0;
};

// An entry that associated a int with a sendMsg callback
struct TCallbackSlot {
  uint32_t          comp_type = 0;
  IMsgBaseCallback* callback = nullptr;
};

// Associates a msg_id with a callback slot object
extern std::unordered_multimap< uint32_t, TCallbackSlot > all_registered_msgs;

