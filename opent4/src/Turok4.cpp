#include "Turok4.h"

#include <assert.h>
#include <string>
#include <cctype>

#if !defined(_WIN32)
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <errno.h>
#include <unistd.h>

// r must have strlen(path) + 2 bytes
static int casepath(char const *path, char *r)
{
    size_t l = strlen(path);
    char *p = (char*)alloca(l + 1);
    strcpy(p, path);
    size_t rl = 0;

    DIR *d;
    if (p[0] == '/')
    {
        d = opendir("/");
        p = p + 1;
    }
    else
    {
        d = opendir(".");
        r[0] = '.';
        r[1] = 0;
        rl = 1;
    }

    int last = 0;
    char *c = strsep(&p, "/");
    while (c)
    {
        if (!d)
        {
            return 0;
        }

        if (last)
        {
            closedir(d);
            return 0;
        }

        r[rl] = '/';
        rl += 1;
        r[rl] = 0;

        struct dirent *e = readdir(d);
        while (e)
        {
            if (strcasecmp(c, e->d_name) == 0)
            {
                strcpy(r + rl, e->d_name);
                rl += strlen(e->d_name);

                closedir(d);
                d = opendir(r);

                break;
            }

            e = readdir(d);
        }

        if (!e)
        {
            strcpy(r + rl, c);
            rl += strlen(c);
            last = 1;
        }

        c = strsep(&p, "/");
    }

    if (d) closedir(d);
    return 1;
}

bool stringLowerEquals(std::string a, std::string b)
{
    unsigned int sz = a.length();
    if (b.length() != sz)
        return false;
    for (unsigned int i = 0; i < sz; ++i)
        if (std::tolower(a[i]) != std::tolower(b[i]))
            return false;
    return true;
}

std::string casename(std::string dir, std::string filename)
{
    DIR *d;
    struct dirent *dirp;
    if((d = opendir(dir.c_str())) == NULL) {
        printf("Failed opening dir for case transforming: %s\n", dir.c_str());
        return dir+filename;
    }
    while((dirp = readdir(d)) != NULL) {
        std::string name = std::string(dirp->d_name);
        if(stringLowerEquals(filename, name)) {
            closedir(d);
            return dir + name;
        }
    }
    closedir(d);
    printf("Warning: Did not find filename %s in directory %s\n", filename.c_str(), dir.c_str());
    return dir + filename;
}

#endif

namespace opent4
{
    std::string _tDir;
    void SetTurokDirectory(const std::string& TurokDir)
    {
        //Insure there is no trailing slash in the turok dir
        _tDir = TurokDir;
        while(_tDir.rbegin() != _tDir.rend() && *_tDir.rbegin() == '/')
            _tDir.pop_back();

    }

    std::string GetTurokDirectory()
    {
        return _tDir;
    }

    std::string TransformPseudoPathToRealPath(const std::string& PseudoPath)
    {
        std::string RealPath;

        //Some hacky stuff to resolve various bugs with file paths
        if(PseudoPath[0] == 'Y' && PseudoPath[1] == ':') RealPath = PseudoPath.substr(2, PseudoPath.length() - 1);
        else if(PseudoPath[0] == '/' && PseudoPath[1] == '\\') RealPath = PseudoPath.substr(1, PseudoPath.length() - 1);
        else RealPath = PseudoPath;
        
        RealPath = GetTurokDirectory() + RealPath;

        for(int s = 0; s < RealPath.length(); s++) {
            if(RealPath[s] == '\\')
                RealPath[s] = '/';
            else
                RealPath[s] = std::tolower(RealPath[s]);
        }

        //printf("PseudoPath: %s\n", PseudoPath.c_str());
        //printf("Realpath: %s\n", RealPath.c_str());

        size_t extIdx = RealPath.find_last_of(".");
        if(extIdx != std::string::npos)
        {
            std::string Ext = RealPath.substr(extIdx, RealPath.length());
            if(Ext == ".bmp" || Ext == ".tga")
                RealPath = RealPath.substr(0, extIdx) + ".dds";
        }

#if !defined(_WIN32)
        //On a case sensitive file system os, make sure we have proper case path
        char *r = (char*)alloca(RealPath.length() + 2);
        if (casepath(RealPath.c_str(), r))
        {
            RealPath = std::string(r);
        }

        size_t fileIdx = RealPath.find_last_of("/");
        assert(fileIdx != std::string::npos);
        std::string path = RealPath.substr(0, fileIdx+1);
        std::string filename = RealPath.substr(fileIdx+1, RealPath.length());
        //printf("transforming path %s %s \n", path.c_str(), filename.c_str());
        RealPath = casename(path, filename);
#endif

        return RealPath;
    }

    std::string TransformRealPathToPseudoPath(const std::string& RealPath)
    {
        return "Not needed yet.";
    }
    
    ATRFile::~ATRFile() {
        if(m_Data) delete m_Data;
        if(m_Root) delete m_Root;
        if(m_Mesh) delete m_Mesh;
        if(m_Variables) delete m_Variables;
    }

    /* ATRFile */
    bool ATRFile::Load(const std::string& Filename)
    {
		m_RealFile = Filename;
        printf("ATR: %s\n", Filename.c_str());
        FILE* fp = std::fopen(Filename.c_str(), "rb");
        if(!fp) { printf("Unable to open file.\n"); return false; }

        m_Data = new ByteStream(fp);
        std::fclose(fp);

        if(!CheckHeader()) return false;
        m_Root = new Block();
        m_Root->Load(m_Data);

        unsigned char PathLen = m_Root->GetData()->GetByte();
        m_File = m_Root->GetData()->GetString(PathLen);
        m_Root->GetData()->Offset(1);

        while(!m_Root->GetData()->AtEnd(1))
        {
            Block* b = new Block();
            b->Load(m_Root->GetData());
            m_Root->AddChildBlock(b);
        }

        ProcessBlocks();
        return true;
    }

    void ATRFile::ProcessBlocks()
    {
        for(std::size_t i = 0; i < m_Root->GetChildCount(); i++)
        {
            Block* b = m_Root->GetChild(i);
            ByteStream* Data = b->GetData();
            Data->SetOffset(0);
            switch(b->GetType())
            {
                case BT_ACTOR_MESH:
                {
                    Data->GetByte(); //Path length (not needed)
                    m_ActorMeshFile = Data->GetString();

                    m_Mesh = new ActorMesh();
                    if(!m_Mesh->Load(TransformPseudoPathToRealPath(m_ActorMeshFile))) {
                        delete m_Mesh;
                    }

                    break;
                }
                case BT_ACTOR_INSTANCES:
                {
                    Data->GetByte(); //Path length (not needed)
                    m_InstancesFile = Data->GetString();

                    ATIFile* ATI = new ATIFile();
                    std::string path = TransformPseudoPathToRealPath(m_InstancesFile);
                    if(!ATI->Load(path)) break;
                    m_ActorInstanceFiles.push_back(ATI);

                    break;
                }
                case BT_VERSION:
                {
                    m_Version = Data->GetFloat();
                    break;
                }
                case BT_ACTOR_CODE:
                {
                    m_ActorCode = Data->GetString();
                    break;
                }
                case BT_ACTOR_MESH_AXIS:
                {
                    m_ActorMeshAxis.x = Data->GetFloat();
                    m_ActorMeshAxis.y = Data->GetFloat();
                    m_ActorMeshAxis.z = Data->GetFloat();
                    break;
                }
                case BT_ACTOR_PRECACHE_FILE:
                {
                    Data->GetByte(); //Path length (not needed)
                    m_PrecacheFile = Data->GetString();
                    //printf("Precache files not yet understood.\n");

                    break;
                }
                case BT_ACTOR_VARIABLES  :
                {
                    m_Variables = new ActorVariables();
                    if(!m_Variables->Load(Data)) {
                        printf("Failed to load global actor variables for %s\n", m_File.c_str());
                        delete m_Variables;
                        m_Variables = 0;
                    }
                }
                case BT_ACTOR_MESH_BOUNDS:
                case BT_ACTOR_TEXTURE_SET:
                case BT_ACTOR_PROPERTIES :
                case BT_ACTOR_POSITION   :
                case BT_ACTOR_ROTATION   :
                case BT_ACTOR_SCALE      :
                case BT_DUMMIES          :
                case BT_GRND_Y           :
                case BT_DEFT             :
                case BT_COLS             :
                case BT_MODES            :
                case BT_LINK_LISTS       :
                case BT_LINK             :
                case BT_LINKS            :
                case BT_TRNS             :
                case BT_HOTPS            : { break; }
                default:
                {
                    //printf("Unsupported ATR block type (%s).\n",BlockTypeIDs[b->GetType()].c_str());
                    break;
                }
            }
        }
    }

    bool ATRFile::Save(const std::string& Filename)
    {
        ByteStream* Data = new ByteStream();
        if(!Data->WriteData(4, m_Hdr)) {
			delete Data;
			return false;
		}

		m_Root->GetData()->SetOffset(0);
        if(!m_Root->Save(Data, true)) {
			delete Data;
			return false;
		}

		Data->SetOffset(0);
		FILE* fp = fopen(Filename.c_str(), "wb");
		fwrite(Data->Ptr(), Data->GetSize(), 1, fp);
		fclose(fp);
		delete Data;

		for(int i = 0;i < m_ActorInstanceFiles.size();i++) {
			m_ActorInstanceFiles[i]->Save(m_ActorInstanceFiles[i]->GetFile().c_str());
		}
        return true;
    }

    bool ATRFile::CheckHeader()
    {
        if(!m_Data->GetData(4, m_Hdr))
        {
            printf("Unable to read header. (Empty file?)\n");
                        return false;
        }

        if(m_Hdr[1] != 'a'
        || m_Hdr[2] != 't'
        || m_Hdr[3] != 'r')
        {
            printf("Invalid ATR header.\n");
            return false;
        }

        return true;
    }

    /* ATIFile */
    ATIFile::~ATIFile()
    {
        for(size_t i = 0; i < m_Blocks.size(); i++) delete m_Blocks[i];
        for(size_t i = 0; i < m_Actors.size(); i++)
        {
            if(m_Actors[i]->Actor->GetActorVariables()) delete m_Actors[i]->Actor->GetActorVariables();
            delete m_Actors[i];
        }
        
        for(size_t i = 0;i < m_LoadedAtrs.size();i++) {
            delete m_LoadedAtrs[i];
        }
    }

    bool ATIFile::Load(const std::string& File)
    {
        printf("ATI: %s\n", File.c_str());
        FILE* fp = std::fopen(File.c_str(), "rb");
        if(!fp) { printf("Unable to open file at path: %s\n", File.c_str()); return false; }
        ByteStream* Data = new ByteStream(fp);
        std::fclose(fp);

        if(!Data->GetData(4,m_Hdr)) { printf("Unable to read header.\n"); delete Data; return false; }

        if(m_Hdr[1] != 'a'
        || m_Hdr[2] != 't'
        || m_Hdr[3] != 'i')
        {
            printf("Incorrect header, not actor instances file.\n");
            delete Data;
            return false;
        }

        while(!Data->AtEnd(1))
        {
            Block* b = new Block();
            b->Load(Data);
            m_Blocks.push_back(b);
        }

        ProcessBlocks();

        delete Data;
        m_File = File;
        return true;
    }

    bool ATIFile::Save(const std::string& File)
    {
        ByteStream* out = new ByteStream();
		out->WriteData(4, m_Hdr);
		for(size_t i = 0;i < m_Blocks.size();i++) {
			switch(m_Blocks[i]->GetType())
            {
				case BT_ACTOR           : { if(!SaveActorBlock(i)) { return false; } break; }
                case BT_PATH            : { break; }
                case BT_NAVDATA         : { break; }
                case BT_ACTOR_VARIABLES : { break; }
                case BT_VERSION         : { break; }
                default:
                {
                    //printf("Unsupported ATI block type (%s).\n",BlockTypeIDs[m_Blocks[i]->GetType()].c_str());
                    break;
                }
            }
			bool isRootNode = false;
			if(m_Blocks[i]->GetTypeString() == "ACTOR") isRootNode = true;
			m_Blocks[i]->Save(out, isRootNode);
		}
		
		out->SetOffset(0);
		FILE* fp = fopen(File.c_str(), "wb");
		fwrite(out->Ptr(), out->GetSize(), 1, fp);
		fclose(fp);
		delete out;
        return false;
    }

    void ATIFile::ProcessBlocks()
    {
        for(size_t i = 0; i < m_Blocks.size(); i++)
        {
            switch(m_Blocks[i]->GetType())
            {
                case BT_ACTOR           : { ProcessActorBlock(i); break; }
                case BT_PATH            : { break; }
                case BT_NAVDATA         : { break; }
                case BT_ACTOR_VARIABLES : { break; }
                case BT_VERSION         : { break; }
                default:
                {
                    //printf("Unsupported ATI block type (%s).\n",BlockTypeIDs[m_Blocks[i]->GetType()].c_str());
                    break;
                }
            }
        }
    }

    void ATIFile::ProcessActorBlock(size_t Idx)
    {
        Block* b = m_Blocks[Idx];
        b->GetData()->SetOffset(0);
        unsigned char PathLen = b->GetData()->GetByte();
        std::string Path = b->GetData()->GetString(PathLen);
        b->GetData()->Offset(1);

        while(!b->GetData()->AtEnd(1))
        {
            Block* aBlock = new Block();
            aBlock->Load(b->GetData());
            b->AddChildBlock(aBlock);
        }
        
        ATRFile* atr = LoadATR(TransformPseudoPathToRealPath(Path));
        if(!atr) return;

        ActorDef* d = new ActorDef();
        d->ActorFile = Path;
        d->BlockIdx  = Idx ;
        d->Parent    = this;
        d->PathID = d->ID = -1;
        d->Actor = new Actor(atr);

        for(int i = 0;i < b->GetChildCount();i++)
        {
            Block* cBlock = b->GetChild(i);
            ByteStream* Data = cBlock->GetData();

            switch(cBlock->GetType())
            {
                case BT_ACTOR_POSITION:
                {
                    d->Position.x = Data->GetFloat();
                    d->Position.y = Data->GetFloat();
                    d->Position.z = Data->GetFloat();
                    break;
                }
                case BT_ACTOR_ROTATION:
                {
                    d->Rotation.x = Data->GetFloat();
                    d->Rotation.y = Data->GetFloat();
                    d->Rotation.z = Data->GetFloat();
                    break;
                }
                case BT_ACTOR_SCALE   :
                {
                    d->Scale.x = Data->GetFloat();
                    d->Scale.y = Data->GetFloat();
                    d->Scale.z = Data->GetFloat();
                    break;
                }
                case BT_ACTOR_NAME    :
                {
                    d->Name = Data->GetString();
                    break;
                }
                case BT_ACTOR_ID      :
                {
                    d->ID = (unsigned char)Data->GetByte();
                    break;
                }
                case BT_ACTOR_PATH_ID :
                {
                    d->PathID = (unsigned char)Data->GetByte();
                    break;
                }
                case BT_ACTOR_VARIABLES:
                {
                    ActorVariables* v = new ActorVariables();
                    if(!v->Load(Data)) { printf("Unable to load actor variables.\n"); }
                    d->Actor->SetActorVariables(v);
                    break;
                }
                case BT_ACTOR_LINK:
                {
                    break;
                }
                default:
                {
                    //printf("Unsupported actor block type (%s).\n",BlockTypeIDs[cBlock->GetType()].c_str());
                }
            }
        }

        m_Actors.push_back(d);
        d->Actor->m_Def = d;
    }

    bool ATIFile::SaveActorBlock(size_t Idx)
    {
        Block* b = m_Blocks[Idx];
		ActorDef* d = m_Actors[Idx]; // I hope this is right... I think it is;
        b->GetData()->SetOffset(0);
		//We don't need to re-save this value, but we need to navigate to the end of this data for the next part
        unsigned char PathLen = b->GetData()->GetByte();
        std::string Path = b->GetData()->GetString(PathLen);
        b->GetData()->Offset(1);

		//update block data
        for(int i = 0;i < b->GetChildCount();i++)
        {
            Block* cBlock = b->GetChild(i);
            ByteStream* Data = cBlock->GetData();
			Data->SetOffset(0);

            switch(cBlock->GetType())
            {
                case BT_ACTOR_POSITION:
                {
					if(!Data->WriteFloat(d->Position.x)) return false;
					if(!Data->WriteFloat(d->Position.y)) return false;
					if(!Data->WriteFloat(d->Position.z)) return false;
                    break;
                }
                case BT_ACTOR_ROTATION:
                {
					if(!Data->WriteFloat(d->Rotation.x)) return false;
					if(!Data->WriteFloat(d->Rotation.y)) return false;
					if(!Data->WriteFloat(d->Rotation.z)) return false;
                    break;
                }
                case BT_ACTOR_SCALE   :
                {
					if(!Data->WriteFloat(d->Scale.x)) return false;
					if(!Data->WriteFloat(d->Scale.y)) return false;
					if(!Data->WriteFloat(d->Scale.z)) return false;
					if(d->Scale.x > 5) {
						printf("Huhtf?\n");
					}
                    break;
                }
                case BT_ACTOR_NAME    :
                {
					if(!Data->WriteString(d->Name)) return false;
                    break;
                }
                case BT_ACTOR_ID      :
                {
					if(!Data->WriteByte(d->ID)) return false;
                    break;
                }
                case BT_ACTOR_PATH_ID :
                {
					if(!Data->WriteByte(d->PathID)) return false;
                    break;
                }
                case BT_ACTOR_VARIABLES:
                {
                    if(!d->Actor->GetActorVariables()->Save(Data)) return false;
                    break;
                }
                case BT_ACTOR_LINK:
                {
                    break;
                }
                default:
                {
                    //printf("Unsupported actor block type (%s).\n",BlockTypeIDs[cBlock->GetType()].c_str());
                }
            }

			Data->SetOffset(0);
        }
		return true;
    }
    
    ATRFile* ATIFile::LoadATR(const std::string &path) {
        //see if the file was loaded already
        for(size_t i = 0;i < m_LoadedAtrs.size();i++) {
            if(m_LoadedAtrPaths[i] == path) {
                m_LoadedAttrRefs[i]++;
                return m_LoadedAtrs[i];
            }
        }
        
        //nope
        ATRFile* file = new ATRFile();
        if(!file->Load(path)) {
            printf("Failed to load an ATR file referenced by the level\n");
            printf("The file was %s\n", path.c_str());
            delete file;
            return 0;
        }
        
        m_LoadedAtrs.push_back(file);
        m_LoadedAtrPaths.push_back(path);
        m_LoadedAttrRefs.push_back(1);
        return file;
    }
}
