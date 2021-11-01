#pragma once

#include "mcv_platform.h"

class IModule
{
  public:
    IModule(const std::string& name) : _name(name) {}

    const std::string& getName() const { return _name; }
    bool isActive() const { return _isActive; }

    virtual void onFileChanged(const std::string& filename) { }

  protected:
    virtual bool start() { return true; }
    virtual void stop() {}
    virtual void update(float delta) {};
    virtual void render() {};
    virtual void renderUI() {};
    virtual void renderDebug() {};
    virtual void renderUIDebug() {};
    virtual void renderInMenu() {};

  private:
    bool doStart()
    {
        assert(!_isActive);
        if (_isActive) return false;

        PROFILE_FUNCTION(getName().c_str());
        const bool ok = start();
        if (ok)
        {
            _isActive = true;
        }
        return ok;
    }

    void doStop()
    {
      assert(_isActive);
      if (!_isActive) return;

      stop();
        _isActive = false;
    }

    friend class CModuleManager;

  private:
    std::string _name;
    bool _isActive = false;
};

using VModules = std::vector<IModule*>;
using CGamestate = VModules;
