// ======================================================================== //
// Copyright 2018++ Ingo Wald                                               //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "miniScene/Scene.h"
#include "miniScene/Serialized.h"
#include "miniScene/IO.h"
#include <sstream>

namespace mini {

    enum { FORMAT_VERSION = 12 };
  /* VERSION HISTORY
     12: embree-style materials, with virtual material read/write
   */
  

#define PARALLELILIZE_GETBOUNDS 1
  
  const size_t expected_magic = 4321000000ULL+FORMAT_VERSION;

  /*! computes the bounding box of a input box undergoing an affine
      transform; e.g., if we have the (object-space) bounds of an
      object we can use that to compute "a" world-space box (not
      necessarily tight, but definitively a boundning box) */
  inline box3f transformedBoxBounds(const affine3f &xfm,
                                    const box3f &box)
  {
    box3f bounds;
    for (int i=0;i<8;i++) {
      vec3f corner((i&1?box.lower:box.upper).x,
                   (i&2?box.lower:box.upper).y,
                   (i&4?box.lower:box.upper).z);
      bounds.extend(xfmPoint(xfm,corner));
    }
    return bounds;
  }


  typedef enum { INVALID=0,
    DISNEY,
    MATTE,
    PLASTIC,
    METAL,
    VELVET,
    METALLICPAINT,
    THINGLASS,
    DIELECTRIC
  } MaterialTag;
  
  MaterialTag materialTagOf(Material::SP mat)
  {
    PRINT(mat->toString());
    if (mat->as<DisneyMaterial>()) return DISNEY;
    if (mat->as<Matte>()) return MATTE;
    if (mat->as<Metal>()) return METAL;
    if (mat->as<Velvet>()) return VELVET;
    if (mat->as<Plastic>()) return PLASTIC;
    if (mat->as<MetallicPaint>()) return METALLICPAINT;
    if (mat->as<Dielectric>()) return DIELECTRIC;
    if (mat->as<ThinGlass>()) return THINGLASS;
    
    throw std::runtime_error("un-supported material type "+mat->toString()+" in Scene::save");
  }
  
  Material::SP createMaterialFromTag(MaterialTag tag)
  {
    switch (tag){
    case DISNEY: return DisneyMaterial::create();
    case METAL: return Metal::create();
    case VELVET: return Velvet::create();
    case PLASTIC: return Plastic::create();
    case MATTE: return Matte::create();
    case DIELECTRIC: return Dielectric::create();
    case THINGLASS: return ThinGlass::create();
    case METALLICPAINT: return MetallicPaint::create();
    }
    throw std::runtime_error("un-supported material tag "+std::to_string((int)tag)+" in Scene::load");
  }
  
  int getID(Texture::SP texture,
            const std::map<Texture::SP,int> &serialized)
  {
    auto it = serialized.find(texture);
    if (it == serialized.end()) return -1;
    return it->second;
  }
  
  // ------------------------------------------------------------------
  void Plastic::write(std::ofstream &out,
                             const std::map<Texture::SP,int> &textures)
  {
    io::writeElement(out,this->Ks);
    io::writeElement(out,this->eta);
    io::writeElement(out,this->pigmentColor);
    io::writeElement(out,this->roughness);
  }

  void Plastic::read(std::ifstream &in,
                            const std::vector<Texture::SP> &textures)
  {
    io::readElement(in,this->Ks);
    io::readElement(in,this->eta);
    io::readElement(in,this->pigmentColor);
    io::readElement(in,this->roughness);
  }
  
  // ------------------------------------------------------------------
  void Matte::write(std::ofstream &out,
                             const std::map<Texture::SP,int> &textures)
  {
    io::writeElement(out,this->reflectance);
  }

  void Matte::read(std::ifstream &in,
                            const std::vector<Texture::SP> &textures)
  {
    io::readElement(in,this->reflectance);
  }
  
  // ------------------------------------------------------------------
  void MetallicPaint::write(std::ofstream &out,
                             const std::map<Texture::SP,int> &textures)
  {
    io::writeElement(out,this->glitterColor);
    io::writeElement(out,this->glitterSpread);
    io::writeElement(out,this->shadeColor);
    io::writeElement(out,this->eta);
  }

  void MetallicPaint::read(std::ifstream &in,
                            const std::vector<Texture::SP> &textures)
  {
    io::readElement(in,this->glitterColor);
    io::readElement(in,this->glitterSpread);
    io::readElement(in,this->shadeColor);
    io::readElement(in,this->eta);
  }
  
  // ------------------------------------------------------------------
  void ThinGlass::write(std::ofstream &out,
                             const std::map<Texture::SP,int> &textures)
  {
    io::writeElement(out,this->eta);
    io::writeElement(out,this->thickness);
    io::writeElement(out,this->transmission);
  }

  void ThinGlass::read(std::ifstream &in,
                            const std::vector<Texture::SP> &textures)
  {
    io::readElement(in,this->eta);
    io::readElement(in,this->thickness);
    io::readElement(in,this->transmission);
  }
  
  // ------------------------------------------------------------------
  void Dielectric::write(std::ofstream &out,
                             const std::map<Texture::SP,int> &textures)
  {
    io::writeElement(out,this->etaInside);
    io::writeElement(out,this->etaOutside);
    io::writeElement(out,this->transmission);
  }

  void Dielectric::read(std::ifstream &in,
                            const std::vector<Texture::SP> &textures)
  {
    io::readElement(in,this->etaInside);
    io::readElement(in,this->etaOutside);
    io::readElement(in,this->transmission);
  }
  
  // ------------------------------------------------------------------
  void Metal::write(std::ofstream &out,
                             const std::map<Texture::SP,int> &textures)
  {
    io::writeElement(out,this->eta);
    io::writeElement(out,this->k);
    io::writeElement(out,this->roughness);
  }

  void Metal::read(std::ifstream &in,
                            const std::vector<Texture::SP> &textures)
  {
    io::readElement(in,this->eta);
    io::readElement(in,this->k);
    io::readElement(in,this->roughness);
  }
  
  // ------------------------------------------------------------------
  void Velvet::write(std::ofstream &out,
                             const std::map<Texture::SP,int> &textures)
  {
    io::writeElement(out,this->reflectance);
    io::writeElement(out,this->horizonScatteringColor);
    io::writeElement(out,this->horizonScatteringFallOff);
    io::writeElement(out,this->backScattering);
  }

  void Velvet::read(std::ifstream &in,
                            const std::vector<Texture::SP> &textures)
  {
    io::readElement(in,this->reflectance);
    io::readElement(in,this->horizonScatteringColor);
    io::readElement(in,this->horizonScatteringFallOff);
    io::readElement(in,this->backScattering);
  }
  
  // ------------------------------------------------------------------
  void DisneyMaterial::write(std::ofstream &out,
                             const std::map<Texture::SP,int> &textures)
  {
    io::writeElement(out,this->emission);
    io::writeElement(out,this->baseColor);
    io::writeElement(out,this->metallic);
    io::writeElement(out,this->roughness);
    io::writeElement(out,this->transmission);
    io::writeElement(out,this->ior);

    io::writeElement(out,getID(this->colorTexture,textures));
    io::writeElement(out,getID(this->alphaTexture,textures));
  }

  void DisneyMaterial::read(std::ifstream &in,
                            const std::vector<Texture::SP> &textures)
  {
    io::readElement(in,this->emission);
    io::readElement(in,this->baseColor);
    io::readElement(in,this->metallic);
    io::readElement(in,this->roughness);
    io::readElement(in,this->transmission);
    io::readElement(in,this->ior);
    {
      int texID = io::readElement<int>(in);
      assert(texID >= 0);
      assert(texID < textures.size());
      this->colorTexture = textures[texID];
    }
    {
      int texID = io::readElement<int>(in);
      assert(texID >= 0);
      assert(texID < textures.size());
      this->alphaTexture = textures[texID];
    }
  }

  
  std::string DirLight::toString()
  {
    std::stringstream ss;
    ss << "DirLight{dir=" << direction << ", rad=" << radiance << "}";
    return ss.str();
  }
  
  box3f Mesh::getBounds() const
  {
    box3f bounds;
#if PARALLELILIZE_GETBOUNDS
    if (vertices.size() > 16*1024) {
      std::mutex boundsMutex;
      parallel_for_blocked
        ((size_t)0,vertices.size(),16*1024,
         [&](size_t begin, size_t end) {
           box3f blockBox;
           for (size_t i=begin;i<end;i++)
             blockBox.extend(vertices[i]);
           std::lock_guard<std::mutex> lock(boundsMutex);
           bounds.extend(blockBox);
         });
    } else
#endif
      for (auto vtx : vertices)
        bounds.extend(vtx);
    return bounds;
  }
    
  box3f Object::getBounds() const
  {
    box3f bounds;
#if PARALLELILIZE_GETBOUNDS
    if (meshes.size() > 16*1024) {
      std::mutex boundsMutex;
      parallel_for_blocked
        ((size_t)0,meshes.size(),16*1024,
         [&](size_t begin, size_t end) {
           box3f blockBox;
           for (size_t i=begin;i<end;i++)
             blockBox.extend(meshes[i]->getBounds());
           std::lock_guard<std::mutex> lock(boundsMutex);
           bounds.extend(blockBox);
         });
    } else
#endif
    for (auto mesh : meshes)
      bounds.extend(mesh->getBounds());
    return bounds;
  }

    
  box3f Instance::getBounds() const
  {
    const box3f box = object->getBounds();
    return transformedBoxBounds(xfm,box);
  }
  
  box3f Scene::getBounds() const
  {
    box3f bounds;
#if PARALLELILIZE_GETBOUNDS
    std::map<Object::SP,box3f> objectBounds;
    
    // ------------------------------------------------------------------
    // first, make a list of all the objects being used in the scene
    // ------------------------------------------------------------------
    std::mutex mutex;
    parallel_for_blocked
      ((size_t)0,instances.size(),1024,
       [&](size_t begin, size_t end) {
         std::set<Object::SP> blockObjects;
         box3f blockBox;
         for (size_t i=begin;i<end;i++)
           blockObjects.insert(instances[i]->object);
         std::lock_guard<std::mutex> lock(mutex);
         for (auto obj : blockObjects)
           objectBounds[obj] = box3f();
       });
    
    // ------------------------------------------------------------------
    // second, make a linear list of all objects, so we can do them in
    // parallel
    // ------------------------------------------------------------------
    std::vector<Object::SP> uniqueObjects;
    uniqueObjects.reserve(objectBounds.size());
    for (auto it : objectBounds)
      uniqueObjects.push_back(it.first);
    
    // ------------------------------------------------------------------
    // third, compute all the object bounds
    // ------------------------------------------------------------------
    parallel_for
      (uniqueObjects.size(),
       [&](size_t objID) {
         auto obj = uniqueObjects[objID];
         // this parallel access is fine because no new node
         // will ever get created in the map; they all
         // already exist, we just find() them here.
         objectBounds[obj] = obj->getBounds();
       });
    
    // ------------------------------------------------------------------
    // last, do all the instances in parallel
    // ------------------------------------------------------------------
    parallel_for_blocked
      ((size_t)0,instances.size(),1024,
       [&](size_t begin, size_t end) {
         box3f blockBox;
         for (size_t i=begin;i<end;i++) {
           auto inst = instances[i];
           box3f objBounds = objectBounds[inst->object];
           blockBox.extend(transformedBoxBounds(inst->xfm,objBounds));
         }
         std::lock_guard<std::mutex> lock(mutex);
         bounds.extend(blockBox);
       });
#else
    for (auto inst : instances)
      bounds.extend(inst->getBounds());
#endif
    return bounds;
  }
    
    
  void Scene::save(const std::string &baseName)
  {
    std::ofstream out(baseName,std::ios::binary);
    if (!out.good())
      throw std::runtime_error("could not open file '"+baseName+"'");
    SerializedScene serialized(this);
      
    io::writeElement(out,expected_magic);

    // ------------------------------------------------------------------
    // textures
    // ------------------------------------------------------------------
    io::writeElement(out,serialized.textures.list.size());
    for (auto tex : serialized.textures.list) {
      if (/* only first one may/will be null */!tex) {
        io::writeElement(out,int(0));
      } else {
        io::writeElement(out,int(1));
        io::writeElement(out,tex->size);
        io::writeElement(out,tex->format);
        io::writeElement(out,tex->filterMode);
        io::writeVector(out,tex->data);
      }
    }

    // ------------------------------------------------------------------
    // lights
    // ------------------------------------------------------------------
    io::writeVector(out,quadLights);
    io::writeVector(out,dirLights);
    if (envMapLight) {
      io::writeElement(out,int(1));
      io::writeElement(out,envMapLight->transform);
      Texture::SP tex = envMapLight->texture;
      assert(tex);
      io::writeElement(out,tex->size);
      io::writeElement(out,tex->format);
      io::writeElement(out,tex->filterMode);
      io::writeVector(out,tex->data);
    } else
      io::writeElement(out,int(0));
        
    // ------------------------------------------------------------------
    // materials
    // ------------------------------------------------------------------
    io::writeElement(out,serialized.materials.list.size());
    for (auto mat : serialized.materials.list) {
      // io::writeElement(out,(MaterialData&)*mat);
#if 1
      // version 12
      int tag = (int)materialTagOf(mat);
      PRINT(tag);
      io::writeElement(out,tag);
      mat->write(out,serialized.textures.registry);
#else
      // old version 11
      io::writeElement(out,mat->emission);
      io::writeElement(out,mat->baseColor);
      io::writeElement(out,mat->metallic);
      io::writeElement(out,mat->roughness);
      io::writeElement(out,mat->transmission);
      io::writeElement(out,mat->ior);

      io::writeElement(out,serialized.getID(mat->colorTexture));
      io::writeElement(out,serialized.getID(mat->alphaTexture));
#endif
    }
      
    // ------------------------------------------------------------------
    // objects and meshes
    // ------------------------------------------------------------------
    io::writeElement(out,serialized.objects.size());
    for (auto &obj : serialized.objects.list) {

      io::writeElement(out,obj->meshes.size());
      for (auto mesh : obj->meshes) {
        if (!mesh) { io::writeElement(out,int(0)); continue; }

        io::writeElement(out,int(1));
        io::writeVector(out,mesh->indices);
        io::writeVector(out,mesh->vertices);
        io::writeVector(out,mesh->normals);
        io::writeVector(out,mesh->texcoords);
        int matID = serialized.getID(mesh->material);
        assert(matID >= 0);
        io::writeElement(out,matID);
      }
    }

    // ------------------------------------------------------------------
    // instances
    // ------------------------------------------------------------------
    io::writeElement(out,instances.size());
    for (auto &inst : instances) {
      if (!inst) { io::writeElement(out,int(0)); continue; }

      io::writeElement(out,int(1));
      io::writeElement(out,inst->xfm);
      io::writeElement(out,int(serialized.getID(inst->object)));
    }
      
    // ------------------------------------------------------------------
    // proxies and owner masks
    // ------------------------------------------------------------------
    // io::writeVector(out,proxies);
    // io::writeVector(out,ownedOn);

    // ------------------------------------------------------------------
    // wrap-up: write end-of file marker
    // ------------------------------------------------------------------
    io::writeElement(out,expected_magic);
    if (!out.good())
      throw std::runtime_error("some error happened while writing '"+baseName+"'");
  }
    
  Scene::SP Scene::load(const std::string &baseName)
  {
    std::ifstream in(baseName,std::ios::binary);
    if (!in.good())
      throw std::runtime_error("could not open Scene{"+baseName+"}");
    Scene::SP scene = std::make_shared<Scene>();

    size_t magic = io::readElement<size_t>(in);
    int format_version = 12;
    if (magic == expected_magic) {
      // all good, this is our format we'd also write
    } else if (magic == expected_magic-1) {
      // version 11 - old mini::Material handling - we should still be able to read this.
      format_version = 11;
    } else
      throw std::runtime_error("invalid or incompatible 'mini' scene file (wrong file magic) - cannot load");
      
    // ------------------------------------------------------------------
    // textures
    // ------------------------------------------------------------------
    std::vector<Texture::SP> textures;
    size_t numTextures = io::readElement<size_t>(in);
    for (int i=0;i<numTextures;i++) {
      // if (i==0)
      //   textures.push_back(0); // first one is always 0
      // else {
      int valid;
      io::readElement(in,valid);
      if (!valid) {
        textures.push_back({});
      } else {
        Texture::SP tex = std::make_shared<Texture>();
        io::readElement(in,tex->size);
        io::readElement(in,tex->format);
        io::readElement(in,tex->filterMode);
        io::readVector(in,tex->data);
        textures.push_back(tex);
      }
    }

    // ------------------------------------------------------------------
    // lights
    // ------------------------------------------------------------------
    io::readVector(in,scene->quadLights);
    io::readVector(in,scene->dirLights);
    const int hasEnvMap = io::readElement<int>(in);
    if (hasEnvMap) {
      scene->envMapLight = std::make_shared<EnvMapLight>();
      io::readElement(in,scene->envMapLight->transform);
      Texture::SP tex = scene->envMapLight->texture = std::make_shared<Texture>();
      io::readElement(in,tex->size);
      io::readElement(in,tex->format);
      io::readElement(in,tex->filterMode);
      io::readVector(in,tex->data);
    }
    
    // ------------------------------------------------------------------
    // materials
    // ------------------------------------------------------------------
    std::vector<Material::SP> materials;
    size_t numMaterials = io::readElement<size_t>(in);
    for (int i=0;i<numMaterials;i++) {
      // io::readElement(in,(MaterialData&)*mat);
#if 1
      int tag;
      if (format_version == 11)
        // "DISNEY" is the direct equivalent to whatever we had before version 11
        tag = DISNEY;
      else
        io::readElement(in,tag);
      Material::SP mat = createMaterialFromTag((MaterialTag)tag);
      mat->read(in,textures);
#else
      Material::SP mat = std::make_shared<Material>();
      io::readElement(in,mat->emission);
      io::readElement(in,mat->baseColor);
      io::readElement(in,mat->metallic);
      io::readElement(in,mat->roughness);
      io::readElement(in,mat->transmission);
      io::readElement(in,mat->ior);
      {
        int texID = io::readElement<int>(in);
        assert(texID >= 0);
        assert(texID < textures.size());
        mat->colorTexture = textures[texID];
      }
      {
        int texID = io::readElement<int>(in);
        assert(texID >= 0);
        assert(texID < textures.size());
        mat->alphaTexture = textures[texID];
      }
#endif
      materials.push_back(mat);
    }

    // ------------------------------------------------------------------
    // objects and meshes
    // ------------------------------------------------------------------
    size_t numObjects = io::readElement<size_t>(in);
    std::vector<Object::SP> objects;
    for (int objID=0;objID<numObjects;objID++) {
      size_t numMeshes = io::readElement<size_t>(in);
      Object::SP object = std::make_shared<Object>();

      for (int meshID=0;meshID<(int)numMeshes;meshID++) {
        int isValid = io::readElement<int>(in);
        if (!isValid) {
          continue;
        }
        Mesh::SP mesh = std::make_shared<Mesh>();
        io::readVector(in,mesh->indices);
        io::readVector(in,mesh->vertices);
        io::readVector(in,mesh->normals);
        io::readVector(in,mesh->texcoords);
        int matID = io::readElement<int>(in);
        assert(matID >= 0);
        assert(matID < materials.size());
        mesh->material = materials[matID];
        object->meshes.push_back(mesh);
      }
      objects.push_back(object);
    }

    // ------------------------------------------------------------------
    // instances
    // ------------------------------------------------------------------
    size_t numInstances = io::readElement<size_t>(in);
    for (int instID=0;instID<numInstances;instID++) {
      int isValid = io::readElement<int>(in);
      if (!isValid) {
        scene->instances.push_back(0);
        continue;
      }
      Instance::SP inst = std::make_shared<Instance>();
      io::readElement(in,inst->xfm);
      inst->object = objects[io::readElement<int>(in)];
      scene->instances.push_back(inst);
    }

    // ------------------------------------------------------------------
    // wrap-up
    // ------------------------------------------------------------------

    size_t magicAtEnd = io::readElement<size_t>(in);
    if (magicAtEnd != expected_magic
        &&
        magicAtEnd != (expected_magic-1)
        )
      throw std::runtime_error("incomplete or incompatible miniScene/.mini file - cannot load");
      
    return scene;
  }

} // ::brix

