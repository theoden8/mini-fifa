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
#include "VertexArray.hpp"
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

  template <typename C>
  static glm::vec2 to_vec2_xy(const C &cont) {
    return glm::vec2(cont.x, cont.y);
  }

  template <typename C>
  static glm::vec3 to_vec3_xyz(const C &cont) {
    return glm::vec3(cont.x, cont.y, cont.z);
  }

	Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
		// data to fill
		std::vector<ModelVertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<ModelTexture> textures;

		// Walk through each of the mesh's vertices
		for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
			ModelVertex vertex;

			vertex.pos = to_vec3_xyz(mesh->mVertices[i]);
      vertex.nrm = to_vec3_xyz(mesh->mNormals[i]);
			// texture coordinates
			if(mesh->mTextureCoords[0]) {
        // does the mesh contain texture coordinates?
				// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
				// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
				vertex.txcoords = to_vec2_xy(mesh->mTextureCoords[0][i]);
			} else {
				vertex.txcoords = glm::vec2(0.0f, 0.0f);
      }
			vertex.tangent = to_vec3_xyz(mesh->mTangents[i]);
      vertex.bitan = to_vec3_xyz(mesh->mBitangents[i]);

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
  sys::File file(filename.c_str());
  img::Image *image = img::load_image(file);
	GLenum format;

	glBindTexture(GL_TEXTURE_2D, textureID); GLERROR
  GLenum pixel_format = gl::Texture::get_gl_pixel_format(image->format);
	glTexImage2D(GL_TEXTURE_2D, 0, pixel_format, image->width, image->height, 0, pixel_format, GL_UNSIGNED_BYTE, image->data); GLERROR
	glGenerateMipmap(GL_TEXTURE_2D); GLERROR

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); GLERROR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); GLERROR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); GLERROR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); GLERROR

  delete image;
	return textureID;
}
