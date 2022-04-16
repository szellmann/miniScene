

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
#include <fstream>

namespace mini {
  namespace scene {

    void brixReplicate(int ac, char **av)
    {
      std::string outFileName = "a.obj";
      std::string inFileName = "";
      vec3f vx = { 100, 0, 0 };
      vec3f vy = { 0, 0, 100 };
      float scale = 1.f;
      int numReplications = 20;
      bool flat = true;
      for (int i=1;i<ac;i++) {
        std::string arg = av[i];
        if (arg == "-o") {
          outFileName = av[++i]; 
        } else if (arg == "--flat") {
          flat = true;
        } else if (arg == "--not-flat") {
          flat = false;
        } else if (arg == "-n") {
          numReplications = std::atof(av[++i]);
        } else if (arg == "-s") {
          scale = std::atof(av[++i]);
        } else if (arg == "-vx") {
          vx.x = std::atof(av[++i]);
          vx.y = std::atof(av[++i]);
          vx.z = std::atof(av[++i]);
        } else if (arg == "-vy") {
          vy.x = std::atof(av[++i]);
          vy.y = std::atof(av[++i]);
          vy.z = std::atof(av[++i]);
        } else if (arg[0] != '-')
          inFileName = arg;
        else
          throw std::runtime_error("unknown cmdline argument '"+arg+"'");
      }
      if (inFileName.empty())
        throw std::runtime_error("no input file specified");

      std::cout << OWL_TERMINAL_LIGHT_BLUE
                << "loading brx file from " << inFileName 
                << OWL_TERMINAL_DEFAULT << std::endl;
      Scene::SP in = Scene::load(inFileName);
      std::cout << OWL_TERMINAL_LIGHT_GREEN
                << "#brx2obj: scene loaded."
                << OWL_TERMINAL_DEFAULT << std::endl;

      Scene::SP out = std::make_shared<Scene>();
      srand48(128);
      vec3f center = in->getBounds().center();
      for (int i=0;i<numReplications;i++) {
        float u = drand48();
        float v = drand48();
        float r = drand48();
        vec3f N = normalize(cross(vx,vy));
        affine3f xfm;
        xfm = xfm * affine3f::translate(center + u*vx + v*vy);
        xfm = xfm * affine3f::rotate(N,r*2*M_PI);
        xfm = xfm * affine3f::scale(scale);
        xfm = xfm * affine3f::translate(-center);

        if (flat) {
          for (auto org : in->instances) {
#if 1
            for (auto mesh : org->object->meshes) {
              Object::SP newObj = std::make_shared<Object>();
              Mesh::SP newMesh = std::make_shared<Mesh>();
              *newMesh = *mesh;
              newObj->meshes.push_back(newMesh);
              Instance::SP newInst = std::make_shared<Instance>(newObj,
                                                                xfm*org->xfm);
              out->instances.push_back(newInst);
            }
#else
            Object::SP newObj = std::make_shared<Object>();
            for (auto mesh : org->object->meshes) {
              Mesh::SP newMesh = std::make_shared<Mesh>();
              *newMesh = *mesh;
              newObj->meshes.push_back(newMesh);
            }
            out->instances.push_back(std::make_shared<Instance>(newObj,
                                                                xfm*org->xfm));
#endif
          }
        } else {
          for (auto org : in->instances) {
            out->instances.push_back(std::make_shared<Instance>(org->object,
                                                                xfm*org->xfm));
          }
        }
      }
      std::cout << "created instantiated scene with " << out->instances.size() << " instances total" << std::endl;
        
      std::cout << OWL_TERMINAL_LIGHT_BLUE
                << "saving to " << outFileName 
                << OWL_TERMINAL_DEFAULT << std::endl;
      // writeToOBJ(out,outFileName);
      out->save(outFileName);
      std::cout << OWL_TERMINAL_LIGHT_GREEN
                << "#brixReplicate: replicated model written...."
                << OWL_TERMINAL_DEFAULT << std::endl;
    }
    
  }
}

int main(int ac, char **av)
{ mini::scene::brixReplicate(ac,av); return 0; }
