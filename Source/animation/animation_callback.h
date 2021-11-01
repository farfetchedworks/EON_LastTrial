#pragma once
#include "Cal3d/cal3d.h"
#include "Cal3d/animcallback.h"

class CHandle;

struct CAnimationCallback : public CalAnimationCallback
{
    CHandle getOwnerEntity(void* userData);
    void endCallback(CalCoreAnimation* anim);
};

// --------------------------------------------------------

template<class T>
CalAnimationCallback* callbackFactory() {
    return new T();
}

typedef CalAnimationCallback* (*CCallbackFactory)(void);

class CallbackRegistry {
public:
    std::map<std::string, CCallbackFactory> callbacks;

    static CallbackRegistry& _instance_() {
        static CallbackRegistry instance;
        return instance;
    };

    void add(std::string name, CCallbackFactory factory) {
        callbacks[name] = factory;
    }

    CCallbackFactory get(std::string name) {
        auto it = callbacks.find(name);
        if (it != callbacks.end())
            return it->second;
        fatal("No callback named %s\n", name.c_str());
        return CCallbackFactory();
    }
};

#define CallbackRegistry CallbackRegistry::_instance_()

class CCallbackRegistration
{
public:
    CCallbackRegistration(std::string module_name, CCallbackFactory creator) {
        CallbackRegistry.add(module_name, creator);
    }
};

#define REGISTER_CALLBACK(name, callback) \
    CCallbackRegistration _callback_registration_ ## callback(name, &callbackFactory<callback>);