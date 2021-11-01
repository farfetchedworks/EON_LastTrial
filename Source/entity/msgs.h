#pragma once

// Template to implement classes that forward the sendMsg callback
// to a concrete method of the class TComp
template< typename TComp, typename TMsg >
struct TMsgCallback : public IMsgBaseCallback {

  // Pointer to a function of the class TComp that receives a const TMsg& as a single argument
  // Let's call the type 'TMethod'
  // Return type of the method must be -void-
  typedef void (TComp::*TMethod)(const TMsg&);

  TMethod method = nullptr;

  TMsgCallback(TMethod new_method)
    : method(new_method)
  {}

  void sendMsg(CHandle h_recv, const void* generic_msg) override {
    // Auto conversion from handle to concrete Component type
    TComp* obj_recv = h_recv;
    assert(obj_recv);
    // Cast the generic msg to my concrete msg type
    const TMsg* msg = static_cast<const TMsg*>(generic_msg);
    assert(method);
    (obj_recv->*method)(*msg);
  }

};

// Global msg register entry function
template< typename TComp, typename TMsg, typename TMethod >
void registerMsg(TMethod method) {
  // The key/value to be inserted in the all_registered_msgs map
  std::pair<uint32_t, TCallbackSlot > v;
  v.first = TMsg::getMsgID();
  v.second.comp_type = getObjectManager<TComp>()->getType();
  v.second.callback = new TMsgCallback< TComp, TMsg >(method);
  all_registered_msgs.insert(v);
}

// Function that will return unique number each time is called.
// Each msg type will call this funcion once.
uint32_t getNextUniqueMsgID();

// Each TMsgStruct defining this will get assigned a new uint32_t
// associated and stored to this static method of each struct type
#define DECL_MSG_ID( )                                \
  static uint32_t getMsgID() {                        \
    static uint32_t msg_id = getNextUniqueMsgID();    \
    return msg_id;                                    \
  }

#define DECL_MSG( acomp, amsg, amethod ) \
  registerMsg< acomp, amsg >(&acomp::amethod)

// When someone calls sendMsg on a handle, convert the handle to an entity object
// using the getObjectManager and call the sendMsg of the entity instance
// If the entity is not valid, the msg is lost
template< class TMsg >
void CHandle::sendMsg(const TMsg& msg) {
  CEntity* e = getObjectManager<CEntity>()->getAddrFromHandle(*this);
  if (e)
    e->sendMsg(msg);
}