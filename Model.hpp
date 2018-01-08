#pragma once

#include <vector>
#include <string>
#include <map>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Debug.hpp"
#include "Logger.hpp"
#include "ShaderProgram.hpp"
#include "ShaderAttrib.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include "ImageLoader.hpp"

unsigned int TextureFromFile(const char *path, const std::string &directory, bool gamma = false);

struct Model {
  std::string path;
  std::vector<ModelTexture> textures_loaded;
  std::vector<Mesh> meshes;
  std::string directory;
  bool gammaCorrection;

  Model(std::string path, bool gamma=false):
    path(path), gammaCorrection(gamma)
  {}

  void init() {
		Logger::Info("Started loading model %s\n", path.c_str());
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
      /* Logger::Error("msg=%s\n", importer.GetErrorString()); */
      std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
      return;
    }
    directory = path.substr(0, path.find_last_of('/'));
    processNode(scene->mRootNode, scene);
    Logger::Info("Finished loading model %s\n", path.c_str());
  }

  void processNode(aiNode *node, const aiScene *scene) {
    for(unsigned i = 0; i < node->mNumMeshes; ++i) {
      aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
      auto m = processMesh(mesh, scene);
      m.init();
      meshes.push_back(m);
    }
    for(unsigned i = 0; i < node->mNumChildren; ++i) {
      processNode(node->mChildren[i], scene);
    }
  }

	Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
		// data to fill
		std::vector<ModelVertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<ModelTexture> textures;

		// Walk through each of the mesh's vertices
		for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
			ModelVertex vertex;
			glm::vec3 vec; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
			// positions
			vec.x = mesh->mVertices[i].x;
			vec.y = mesh->mVertices[i].y;
			vec.z = mesh->mVertices[i].z;
			vertex.pos = vec;
			// normals
			vec.x = mesh->mNormals[i].x;
			vec.y = mesh->mNormals[i].y;
			vec.z = mesh->mNormals[i].z;
			vertex.nrm = vec;
			// texture coordinates
			if(mesh->mTextureCoords[0]) {
        // does the mesh contain texture coordinates?
				glm::vec2 vec;
				// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
				// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.txcoords = vec;
			} else {
				vertex.txcoords = glm::vec2(0.0f, 0.0f);
      }
			// tangent
			vec.x = mesh->mTangents[i].x;
			vec.y = mesh->mTangents[i].y;
			vec.z = mesh->mTangents[i].z;
			vertex.tangent = vec;
			// bitangent
			vec.x = mesh->mBitangents[i].x;
			vec.y = mesh->mBitangents[i].y;
			vec.z = mesh->mBitangents[i].z;
			vertex.bitan = vec;
			vertices.push_back(vertex);
		}
		// now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];
			// retrieve all indices of the face and store them in the indices vector
			for(unsigned int j = 0; j < face.mNumIndices; j++) {
        indices.push_back(face.mIndices[j]);
      }
		}
		// process materials
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		// we assume a convention for sampler names in the shaders. Each diffuse texture should be named
		// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
		// Same applies to other texture as the following list summarizes:
		// diffuse: texture_diffuseN
		// specular: texture_specularN
		// normal: texture_normalN

		// 1. diffuse maps
		std::vector<ModelTexture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		// 2. specular maps
		std::vector<ModelTexture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		// 3. normal maps
		std::vector<ModelTexture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		// 4. height maps
		std::vector<ModelTexture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
		textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

		// return a mesh object created from the extracted mesh data
		return Mesh(vertices, indices, textures);
	}

	std::vector<ModelTexture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName) {
		std::vector<ModelTexture> textures;
		for(unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
			aiString str;
			mat->GetTexture(type, i, &str);
			// check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
			bool skip = false;
			for(unsigned int j = 0; j < textures_loaded.size(); j++) {
				if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
					textures.push_back(textures_loaded[j]);
					skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
					break;
				}
			}
			if(!skip) {
        // if texture hasn't been loaded already, load it
				ModelTexture texture;
				texture.id = TextureFromFile(str.C_Str(), this->directory);
				texture.type = typeName;
				texture.path = str.C_Str();
				textures.push_back(texture);
				textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
			}
		}
		return textures;
	}

  template <typename... ShaderTs>
  void display(gl::ShaderProgram<ShaderTs...> &program) {
    for(unsigned i = 0; i < meshes.size(); ++i) {
      meshes[i].display(program);
    }
  }

  void clear() {
    for(auto &m : meshes) {
      m.clear();
    }
  }
};

unsigned int TextureFromFile(const char *path, const std::string &directory, bool gamma) {
	std::string filename = std::string(path);
	filename = directory + '/' + filename;

	unsigned int textureID;
	glGenTextures(1, &textureID); GLERROR

	int width, height, nrComponents;
  File fp(filename.c_str());
  img::Image *image = img::load_image(fp);
	GLenum format;

	glBindTexture(GL_TEXTURE_2D, textureID); GLERROR
	glTexImage2D(GL_TEXTURE_2D, 0, image->format, image->width, image->height, 0, image->format, GL_UNSIGNED_BYTE, image->data); GLERROR
	glGenerateMipmap(GL_TEXTURE_2D); GLERROR

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); GLERROR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); GLERROR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); GLERROR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); GLERROR

  delete image;
	return textureID;
}
