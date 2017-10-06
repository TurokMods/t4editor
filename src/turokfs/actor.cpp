#include <GL/glew.h>
#include <turokfs/actor.h>
#include <app.h>

#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <render/shader.h>

namespace t4editor {
    actor_mesh::actor_mesh(SubMesh* mesh) {
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
    
    void actor_mesh::render() {
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
            for(size_t i = 0;i < chunkIndices.size();i++) {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibos[i]);
                glDrawElements(GL_TRIANGLE_STRIP, chunkIndices[i].size(), GL_UNSIGNED_SHORT, 0);
            }
        } else {
            glDrawArrays(GL_LINE_STRIP, 0, vertices.size());
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        err = glGetError(); if(err != 0) printf("err: %d | %s\n", err, glewGetErrorString(err));
    }
    
    actor::actor(application* app, const Actor* def) {
        m_app = app;
        ActorMesh* mesh = def->GetMesh();
        if(mesh) {
            for(size_t i = 0;i < mesh->GetSubMeshCount();i++) {
                SubMesh* sm = mesh->GetSubMesh(i);
                meshes.push_back(new actor_mesh(sm));
            }
        }
        
        if(def->GetDef()) {
            ActorVec3 rot = def->GetDef()->Rotation;
            ActorVec3 pos = def->GetDef()->Position;
            ActorVec3 scl = def->GetDef()->Scale;
            rotation = vec3(rot.x, rot.y, rot.z);
            position = vec3(pos.x, pos.y, pos.z);
            scale = vec3(scl.x, scl.y, scl.z);
        } else {
            scale = vec3(1.0f, 1.0f, 1.0f);
        }
    }
    actor::~actor() {
        for(size_t i = 0;i < meshes.size();i++) {
            delete meshes[i];
        }
        meshes.clear();
    }
    void actor::render(t4editor::shader *s) {
        mat4 model = glm::scale(scale) * eulerAngleXYZ(rotation.x,rotation.y,rotation.z) * translate(position);
        s->uniform("model", model);
        s->uniform("mvp", m_app->viewproj() * model);
        
        for(size_t i = 0;i < meshes.size();i++) {
            meshes[i]->render();
        }
    }
}