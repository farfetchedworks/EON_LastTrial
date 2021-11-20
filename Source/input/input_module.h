#pragma once

#include "modules/module.h"
#include "input/input.fwd.h"
#include "input/input_mapping.h"

namespace input
{
    class CModule : public IModule
    {
    public:
        CModule(const std::string& name, int id);

        bool start() override;
        void update(float dt) override;
        void renderInMenu() override;

        void registerDevice(IDevice* device);
        void loadMapping(const std::string& filename);
        void clearInput();
        
        int getId() const { return _id; }
        bool isBlocked() const { return _blocked; }
        std::vector<IDevice*> getDevices() { return _devices; }
        const TKeyboardData& getKeyboard() const { return _keyboard; }
        const TMouseData& getMouse() const { return _mouse; }
        const TPadData& getPad() const { return _pad; }

        const TButton& getButton(const std::string& name) const;
        const TButton& getKey(EKeyboardKey key) const;
        const TButton& getMouseButton(EMouseButton bt) const;
        const TButton& getPadButton(EPadButton bt) const;
        const TButton& getButton(const TButtonDef& def) const;

        const TButton& operator[](const std::string& name) const;
        const TButton& operator[](EKeyboardKey key) const;
        const TButton& operator[](EPadButton bt) const;

        void feedback(const TVibrationData& data);
        
        const VEC2& getMousePosition() { return _mouse.position; };
    
        static void loadDefinitions(const std::string& filename);
        static const TButtonDef* getButtonDefinition(const std::string& name);
        static const std::string& getButtonName(const TButtonDef& def);

        void blockInput() { _blocked = true; };
        void unBlockInput() { _blocked = false; };
        void toggleBlockInput() { _blocked = !_blocked; };

    private:
        int _id = 0;
        bool _blocked;
        bool _playerInput;

        VDevices _devices;
        TKeyboardData _keyboard;
        TMouseData _mouse;
        TPadData _pad;
        CMapping _mapping;

        static std::map<std::string, TButtonDef> _definitions;
    };
}
