#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

#include <EngineTypes.h>

#include <window.h>
#include <gui/panel.h>
#include <render/camera.h>
#include <gui/ImGuizmo.h>

#include <turokfs/fs.h>
#include <turokfs/entry.h>
#include <turokfs/level.h>
#include <turokfs/actor.h>
#include <render/shader.h>
#include <render/framebuffer.h>
#include <render/texture.h>

#include <glm/glm.hpp>

namespace t4editor {
    class ui_panel;
            
    typedef struct {
        int actorId;
        int actorSubmeshId;
        int actorSubmeshChunkId;
    } actorUnderCursor;
    
    class application : public event_receiver {
        public:
            application(int argc,const char* argv[]);
            ~application();
        
            bool initialize();

            bool load_config();
        
            void add_panel(ui_panel* panel);
            void remove_panel(ui_panel* panel);
        
            virtual void onEvent(event* e);
        
            window* getWindow() const { return m_window; }
            string gameDataPath() const { return m_dataPath; }
            string editorDataPath() const { return m_editorDataPath; }
            turokfs* getTurokData() const { return m_fs; }
            framebuffer* getFrame() const { return m_framebuffer; }
        
            level* getLevel() const { return m_level; }
            texture* getDefaultTexture() const { return m_defaultTex; }
			void trigger_repaint() { m_viewChanged = true; }
            
            Camera* GetCamera() const { return m_Camera; }
        
            actorUnderCursor getActorUnderCursor() const { return m_actorUnderCursor; }
            actorUnderCursor getSelectedActor() const { return m_selectedActor; }
			bool is_importing_actor() const { return m_isImportingActor; }

            void set_picked_actor_info(actorUnderCursor hovered, actorUnderCursor selected) {
                m_actorUnderCursor = hovered;
                m_selectedActor = selected;
            }

            void load_level(const string& path);
			void test_level();
			void level_test_done();


			//threaded processes
			void update_actor_cache();
			void restore_backups();
			void monitor_testing(const string& path);
        
            //actor variables
            void define_actor_var_type(const string& vname, const string& vtype) { m_actor_var_types[vname] = vtype; }
            string get_actor_var_type(const string& vname) const;
        
            //actor properties
            void define_actor_block_type(const string& bname, const string& btype) { m_actor_block_types[bname] = btype; }
            string get_actor_block_type(const string& bname) const;

            int run();
        
            texture* getTexture(std::string file_path);

            float get_cache_update_progress() const { return m_cacheProgress; }
            bool is_updating_cache() const { return m_updatingCache; }
            string get_last_file_cached() const { return m_cacheLastFile; }
            
            void ToggleCulling() { m_UseCulling = !m_UseCulling; }

			float get_restore_progress() const { return m_restoreProgress; }
			bool is_restoring_backups() const { return m_restoringBackup; }
			string get_last_file_restored() const { return m_restoreLastFile; }

			bool is_transforming_actor() const { return m_transformingActor; }
			ImGuizmo::OPERATION get_transform_operation() const { return m_transformType; }
			void set_transform_operation(ImGuizmo::OPERATION op) { m_transformType = op; }

			vec2 levelViewCursorPos;
			vec2 levelViewSize;
			bool m_cursorOverLevelView;

        protected:
            texture* loadTexture(std::string file_path);
            std::unordered_map<std::string, texture*> m_textures;

            window* m_window;
            turokfs* m_fs;
        
            vector <string> m_args;
            string m_dataPath;
            string m_editorDataPath;
            int m_windowWidth;
            int m_windowHeight;
            int m_windowPosX;
            int m_windowPosY;
            bool m_UseCulling;

            actorUnderCursor m_actorUnderCursor;
            actorUnderCursor m_selectedActor;
            
            Camera* m_Camera;

			bool m_viewChanged;
			bool m_transformingActor;
			ImGuizmo::OPERATION m_transformType;
            
            vector<ui_panel*> m_panels;
        
			string m_levelToLoad;
            level* m_level;
            shader* m_shader;
            texture* m_defaultTex;
            framebuffer* m_framebuffer;

			bool m_updatingCache;
			string m_cacheLastFile;
			float m_cacheProgress;

			bool m_restoringBackup;
			string m_restoreLastFile;
			float m_restoreProgress;
        
            unordered_map<string, string> m_actor_var_types;
            unordered_map<string, string> m_actor_block_types;

			actor* m_actorToImport;
			vec3 m_actorImportPos;
			bool m_isImportingActor;
    };
}
