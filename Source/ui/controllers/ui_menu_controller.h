#pragma once

#include "ui/ui_controller.h"
#include "ui/ui.fwd.h"

namespace ui
{
    class CMenuController : public IController
    {
      public:
        void reset();
        void update(float elapsed) override;
        void bind(const std::string& buttonName, Callback callback);

        void selectOption(int idx);

      private:
        void nextOption();
        void prevOption();
        void highlightOption();
        void confirmOption();
        void processMouseHover();
        int getButton(VEC2 mouse_pos);

        static constexpr int kUndefinedOption = -1;
        struct TOption
        {
            CButton* button = nullptr;
            Callback callback;
        };

        std::vector<TOption> _options;
        int _currentOption = kUndefinedOption;
        VEC2 last_mouse_pos = VEC2::Zero;
        bool isHoverButton = false;
    };
}
