#include <GL/glew.h>
#include <turokfs/actor.h>
#include <app.h>

#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <render/shader.h>
#include <render/texture.h>

namespace t4editor {
    vec3 int_to_vec3(int i) {
        int r = (i & 0x000000FF) >>  0;
        int g = (i & 0x0000FF00) >>  8;
        int b = (i & 0x00FF0000) >> 16;
        //printf("%d -> %d, %d, %d -> %f, %f, %f\n", i, r, g, b, float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f);
        return vec3(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f);
    }

    actor_mesh::actor_mesh(SubMesh* mesh, texture* tex, vector<texture*>sm_textures) {
		submesh_textures = sm_textures;
		m_Texture = tex;

        for(size_t i = 0;i < mesh->GetVertexCount();i++) {
            mesh_vert vert;
            memset(&vert, 0, sizeof(mesh_vert));
            mesh->GetTexCoord(i,&vert.texcoord.x);
            mesh->GetNormal(i,&vert.normal.x);
            mesh->GetVertex(i,&vert.position.x);
            vertices.push_back(vert);
        }
        
        vector<unsigned short> firstChunkIndices;
        for(size_t i = 0;i < mesh->GetIndexCount();i++) {
            firstChunkIndices.push_back(mesh->GetIndex(i));
        }
        if(firstChunkIndices.size() != 0) chunkIndices.push_back(firstChunkIndices);
        
        for(size_t c = 0;c < mesh->GetChunkCount();c++) {
            MeshChunk* ch = mesh->GetChunk(c);
            vector<unsigned short> indices;
            for(size_t i = 0;i < ch->GetIndexCount();i++) {
                indices.push_back(ch->GetIndex(i));
            }
            
            if(indices.size() > 0) chunkIndices.push_back(indices);
        }
        
        if(vertices.size() == 0) return;
        
        int err = 0;
        err = glGetError(); if(err != 0) printf("err: %d | %s\n", err, glewGetErrorString(err));
        
        // Setup test openGL triangle
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mesh_vert) * vertices.size(), &vertices[0].position.x, GL_STATIC_DRAW);
        
        ibos = 0;
        if(chunkIndices.size() > 0) {
            ibos = new GLuint[chunkIndices.size()];
            memset(ibos, 0, chunkIndices.size() * sizeof(GLuint));
            glGenBuffers(chunkIndices.size(), ibos);
            for(size_t i = 0;i < chunkIndices.size();i++) {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibos[i]);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * chunkIndices[i].size(), &chunkIndices[i][0], GL_STATIC_DRAW);
            }
        }
        
        err = glGetError(); if(err != 0) printf("err: %d | %s\n", err, glewGetErrorString(err));
    }

    actor_mesh::~actor_mesh() {
        if(vao) glDeleteVertexArrays(1, &vao);
        if(vbo) glDeleteBuffers(1, &vbo);
        if(ibos) {
            glDeleteBuffers(chunkIndices.size(), ibos);
            delete [] ibos;
        }
    }
    
    void actor_mesh::render(shader* s) {
        if(vertices.size() == 0) return;
        int err = 0;
        err = glGetError(); if(err != 0) printf("err: %d | %s\n", err, glewGetErrorString(err));
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(mesh_vert), 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(mesh_vert), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(mesh_vert), (void*)(6 * sizeof(float)));
        
        if(chunkIndices.size() > 0) {
			s->uniform("debug_float", 0.0f);
            for(size_t i = 0;i < chunkIndices.size();i++) {
				texture* tex = 0;
				if(submesh_textures.size() > i) tex = submesh_textures[i];
				else if(m_Texture) tex = m_Texture;
				else tex = app->getDefaultTexture();
				tex->bind();
				glActiveTexture(GL_TEXTURE0);
				s->uniform1i("diffuse_map", 0);
                s->uniform("actor_submesh_chunk_id", int_to_vec3(i));
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibos[i]);
                glDrawElements(GL_TRIANGLE_STRIP, chunkIndices[i].size(), GL_UNSIGNED_SHORT, 0);
            }
        } else {
			if(m_Texture) m_Texture->bind();
			else app->getDefaultTexture()->bind();
			glActiveTexture(GL_TEXTURE0);

			s->uniform1i("diffuse_map", 0);
			s->uniform("debug_float", 1.0f);
            s->uniform("actor_submesh_chunk", int_to_vec3(0));
            glDrawArrays(GL_LINE_STRIP, 0, vertices.size());
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        err = glGetError(); if(err != 0) printf("err: %d | %s\n", err, glewGetErrorString(err));
    }

    actor::actor(application* app, ActorMesh* mesh, ActorDef* def) {
        m_app = app;
        actorTraits = def;
        meshTraits = mesh;
		level* lev = app->getLevel();
        if(mesh) {
            for(size_t i = 0;i < mesh->GetSubMeshCount();i++) {
				texture* t = 0;
				if (i < mesh->m_MeshInfos.size()) {
					MeshInfo minfo = mesh->m_MeshInfos[i];
					if(minfo.TSNR_ID != -1)
					{
						int TexID = mesh->m_TXSTs[mesh->m_TSNRs[minfo.TSNR_ID].TXST_ID].TextureID;
						if(TexID < mesh->m_Textures.size()) t = lev->loadTexture(mesh->m_Textures[TexID]);
					}
				}

                SubMesh* sm = mesh->GetSubMesh(i);
				vector<texture*> sm_textures;
				for(size_t ch = 0;ch < sm->GetChunkCount();ch++) {
					texture* t = 0;
					if(ch < mesh->m_MTRLs.size())
                    {
                        if(mesh->m_MTRLs[ch].Unk4 >= 0 && mesh->m_MTRLs[ch].Unk4 < mesh->m_TSNRs.size())
                        {
                            int TexID = mesh->m_TXSTs[mesh->m_TSNRs[mesh->m_MTRLs[ch].Unk4].TXST_ID].TextureID;
                            if(TexID < mesh->m_Textures.size()) t = lev->loadTexture(mesh->m_Textures[TexID]);
                        }
                    }
					if(t) {
						sm_textures.push_back(t);
					} else {
						//to do: some kind of default texture?
					}
				}

                meshes.push_back(new actor_mesh(sm, t, sm_textures));
                meshes[i]->app = app;
                meshes[i]->parent = this;
                meshes[i]->submesh_id = i;
            }
        }
        
        if(def) {
            actor_id = def->ID;
        }
    }

    actor::~actor() {
        for(size_t i = 0;i < meshes.size();i++) {
            delete meshes[i];
        }
        meshes.clear();
    }

    void actor::render(t4editor::shader *s) {
        vec3 position, rotation;
        vec3 scale = vec3(1.0f, 1.0f, 1.0f);
        
        if(actorTraits) {
            ActorVec3 pos = actorTraits->Position;
            ActorVec3 rot = actorTraits->Rotation;
            ActorVec3 scl = actorTraits->Scale;
            position = vec3(pos.x, pos.y, pos.z);
            rotation = vec3(rot.x, rot.y, rot.z);
            scale    = vec3(scl.x, scl.y, scl.z);
        }
        
        mat4 T = translate(position);
        mat4 R = eulerAngleXYZ(radians(rotation.x),radians(rotation.y),radians(rotation.z));
        mat4 S = glm::scale(scale);
        mat4 model = T * R * S;
        s->uniform("model", model);
        s->uniform("view", m_app->view());
        s->uniform("mvp", m_app->viewproj() * model);
        s->uniform("actor_id", int_to_vec3(editor_id));
        
        if(m_app->getSelectedActor().actorId == editor_id) {
            s->uniform("actor_selected", 1.0f);
        } else s->uniform("actor_selected", 0.0f);
        
        for(size_t i = 0;i < meshes.size();i++) {
            s->uniform("actor_submesh_id", int_to_vec3(i));
            meshes[i]->render(s);
        }
    }
}
