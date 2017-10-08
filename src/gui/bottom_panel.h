#pragma once
#include <gui/panel.h>
#include <gui/imgui_tabs.h>
#include <event.h>

#include <imgui.h>
using namespace ImGui;

namespace t4editor {
    class bottom_panel : public ui_panel {
        public:
            bottom_panel() {
                setName("Lower panel");
                setHasTitleBar(false);
                setCanResize(false);
                setCanMove(false);
                setCanClose(false);
            }
        
            ~bottom_panel() {
            }
        
            virtual void onAttach(ui_panel* toPanel) {
                vec2 sz = m_app->getWindow()->getSize();
                sz.y *= 0.7f;
                sz.x -= (sz.x * 0.75f);
                
                setPosition(vec2(m_app->getWindow()->getSize().x * 0.75f, 20.0f));
                setSize(sz);
            }
        
            virtual void onEvent(event* e) {
                if(e->name == "window_resize") {
                    vec2 sz = m_app->getWindow()->getSize();
                    sz.y -= (sz.y * 0.7f) + 24.0f;
                    
                    setPosition(vec2(0, m_app->getWindow()->getSize().y - sz.y));
                    setSize(sz);
                }
            }
        
            virtual void renderContent() {
                BeginTabBar("", ImVec2(500, 26));
                    DrawTabsBackground();
                    if(AddTab("Logs")) {
                        Text("logs here");
                    }
                    if(AddTab("other stuff")) {
                        Text("other stuff here");
                    }
                EndTabBar();
            }
    };
}
