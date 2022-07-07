// ======================================================================== //
// Copyright 2022++ Ingo Wald                                               //
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

#include <string>
#include <map>
#include "miniScene/Scene.h"
#include "miniScene/Serialized.h"

namespace mini {

    void usage(const std::string& error = "")
    {
        if (!error.empty())
            std::cerr << MINI_COLOR_RED << "Error: " << error
            << MINI_COLOR_DEFAULT << std::endl << std::endl;
        std::cout << "miniSubdivide a.mini -o subdivided.mini" << std::endl;
        exit(error.empty() ? 0 : 1);
    }

    std::string midpointIndexToString(int32_t i1, int32_t i2)
    {
        if (i1 < i2)
            return std::to_string(i1) + "," + std::to_string(i2);
        else
            return std::to_string(i2) + "," + std::to_string(i1);
    }

    void miniSubdivide(int ac, char** av)
    {
        std::string outFileName = "";

        if (ac == 1)
            usage();
        std::string inFileName = "";
        for (int i = 1; i < ac; i++)
        {
            std::string arg = av[i];
            if (arg[0] != '-')
                inFileName = arg;
            else if (arg == "-o")
                outFileName = av[++i];
            else
                usage("unknown cmdline argument '" + arg + "'");
        }

        if (inFileName.empty())
            usage("no input file names specified");
        if (outFileName.empty())
            usage("no output file name specified");

        try
        {
            std::cout << "Creating scene \n";

            std::cout << MINI_COLOR_LIGHT_BLUE
                << "Loading mini file from " << inFileName
                << MINI_COLOR_DEFAULT << std::endl;

            Scene::SP scene = Scene::load(inFileName);

            // Save uniques instances, objects and meshes
            std::map<Instance::SP, Instance::SP> instances;
            std::map<Object::SP, Object::SP> objects;
            std::map<Mesh::SP, Mesh::SP> meshes;

            // List of new instances
            std::vector<Instance::SP> newInstances;

            // Iterate each instance
            for (auto& inst : scene->instances)
            {
                // Reuse instance if found
                if (instances.find(inst) != instances.end())
                {
                    newInstances.push_back(instances[inst]);
                    continue;
                }
                // Reuse object if found
                if (objects.find(inst->object) != objects.end())
                {
                    newInstances.push_back(Instance::create(objects[inst->object]));
                    continue;
                }
                // List of new meshes
                std::vector<Mesh::SP> newMeshes;

                // Iterate each mesh
                for (auto& mesh : inst->object->meshes)
                {
                    // Reuse mesh if found
                    if (meshes.find(mesh) != meshes.end())
                    {
                        newMeshes.push_back(meshes[mesh]);
                        continue;
                    }
                    // Mapping between original vertices and midpoints vs new vertices
                    std::map<std::string, int32_t> vertexMap;
                    // Create a mesh with a dummy material
                    Mesh::SP newMesh = Mesh::create(Material::create());
                    // List of new vertices and indices
                    std::vector<vec3f> vertices;
                    std::vector<vec3i> indices;
                    // Iterate each triangle
                    for (auto& index : mesh->indices)
                    {
                        int32_t i[3]; // indices for original vertices
                        int32_t j[3]; // indices for midpoints
                        vec3f v[3];   // original vertices
                        vec3f u[3];   // midpoints
                        // get original vertices
                        for (int k = 0; k < 3; k++)
                        {
                            i[k] = index[k];
                            v[k] = mesh->vertices[i[k]];
                        }
                        // calculate midpoints and add to 'vertices'
                        for (int k = 0; k < 3; k++)
                        {
                            int i1 = k;
                            int i2 = (k + 1) % 3;
                            u[k] = (v[i1] + v[i2]) / 2.0f;
                            std::string indexString = midpointIndexToString(i[i1], i[i2]);
                            if (vertexMap.find(indexString) != vertexMap.end())
                            {
                                j[k] = vertexMap[indexString];
                            }
                            else
                            {
                                j[k] = vertices.size();
                                vertexMap[indexString] = j[k];
                                vertices.push_back(u[k]);
                            }
                        }
                        // add vertices to 'vertices'
                        for (int k = 0; k < 3; k++)
                        {
                            std::string indexString = std::to_string(i[k]);
                            if (vertexMap.find(indexString) != vertexMap.end())
                            {
                                i[k] = vertexMap[indexString];
                            }
                            else
                            {
                                i[k] = vertices.size();
                                vertexMap[indexString] = i[k];
                                vertices.push_back(v[k]);
                            }
                        }
                        // add 4 subtriangles to 'indices'
                        for (int k = 0; k < 3; k++)
                        {
                            vec3i newIndex;
                            newIndex[0] = i[k];
                            newIndex[1] = j[k];
                            newIndex[2] = j[(k + 2) % 3];
                            indices.push_back(newIndex);
                        }
                        vec3i newIndex;
                        newIndex[0] = j[0];
                        newIndex[1] = j[1];
                        newIndex[2] = j[2];
                        indices.push_back(newIndex);
                    }
                    // Update new mesh
                    newMesh->vertices = vertices;
                    newMesh->indices = indices;
                    std::cout << "Original: vertices=" << mesh->vertices.size() << ", "
                        << "triangles=" << mesh->indices.size() << std::endl;
                    std::cout << "New: vertices=" << newMesh->vertices.size() << ", "
                        << "triangles=" << newMesh->indices.size() << std::endl;
                    // Append to the list of new meshes
                    newMeshes.push_back(newMesh);
                    // Update unique set of meshes for later use
                    meshes[mesh] = newMesh;
                }
                // Create a new object with the new list of meshes
                Object::SP newObject = Object::create(newMeshes);
                // Create a new instance with the new object
                Instance::SP newInstance = Instance::create(newObject);
                // Append to the list of new instances
                newInstances.push_back(newInstance);
                // Update unique set of objects for later use
                objects[inst->object] = newObject;
                // Update unique set of instances for later use
                instances[inst] = newInstance;
            }
            // Create a new scene from the list of new instances
            Scene::SP newScene = Scene::create(newInstances);

            // Save scene
            std::cout << "saving scene \n";
            newScene->save(outFileName);

            std::cout << MINI_COLOR_LIGHT_GREEN
                << "#miniInfo: subdivided scene saved."
                << MINI_COLOR_DEFAULT << std::endl;
        }
        catch (const std::exception& exc)
        {
            std::cerr << exc.what();
        }
    }
} // ::mini

int main(int ac, char** av)
{
    mini::miniSubdivide(ac, av); return 0;
}


