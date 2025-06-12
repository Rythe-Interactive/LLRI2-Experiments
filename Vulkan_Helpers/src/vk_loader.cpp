// Impl
#include "vk_loader.hpp"

// Engine
#include "vk_engine.hpp"

std::optional<std::vector<std::shared_ptr<MeshAsset>>> ImportMesh(VulkanEngine* engine, const std::filesystem::path& fullPath) {
	SDL_assert(is_regular_file(fullPath));
	Assimp::Importer importer;

	constexpr int flags = aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType | aiProcess_ValidateDataStructure | aiProcess_FindInvalidData;

	const aiScene* scene = importer.ReadFile(fullPath.string().c_str(), flags);

	if (nullptr == scene) {
		SDL_Log("[ERROR] Assimp: %s", importer.GetErrorString());
		return std::nullopt;
	}

	SDL_assert(scene->HasMeshes());
	std::vector<std::shared_ptr<MeshAsset>> meshes(scene->mNumMeshes);

	for (unsigned int h = 0; h < scene->mNumMeshes; h++) {
		const aiMesh* mesh = scene->mMeshes[h];
		SDL_assert(mesh->HasPositions() && mesh->HasFaces());

		MeshAsset newMesh{
			.name = mesh->mName.C_Str(),
		};

		GeoSurface newSurface{
			.startIndex = static_cast<uint32_t>(newMesh.surfaces.size()),
			.count = mesh->mNumFaces * 3, // 3 indices per face
		};

		// > Vertices
		SDL_Log("Assimp: Mesh %s has %d vertices", fullPath.c_str(), mesh->mNumVertices);
		std::vector<MyVertex> vertices(mesh->mNumVertices);
		for (int i = 0; i < mesh->mNumVertices; i++) {
			const aiVector3D& pos = mesh->mVertices[i];
			const aiVector3D& tex = mesh->mTextureCoords[0][i];
			SDL_Log("Assimp: Vertex %d: pos{x: %f, y: %f, z: %f} tex{x: %f, y: %f, z: %f}", i, pos.x, pos.y, pos.z, tex.x, tex.y, tex.z);
			vertices[i] = MyVertex{
				.pos = math::float4(pos.x, pos.y, pos.z, 1.0f),
				.uv = math::float2(tex.x, tex.y),
			};
		}
		// > Indices
		std::vector<Uint16> indices(mesh->mNumFaces * 3);
		SDL_Log("Assimp: Mesh %s has %d faces", fullPath.c_str(), mesh->mNumFaces);
		for (int i = 0; i < mesh->mNumFaces; i++) {
			const aiFace& face = mesh->mFaces[i];
			SDL_Log("Assimp: Face %d: ", i);
			for (int j = 0; j < face.mNumIndices; j++) {
				SDL_Log("%d", face.mIndices[j]);
				indices[i * 3 + j] = face.mIndices[j];
			}
			SDL_Log("\n");
		}

		// Material texture path
		SDL_assert(scene->HasMaterials());
		SDL_assert(scene->mNumMaterials == 2); //default and my own
		const aiMaterial* material = scene->mMaterials[1]; // 1 is my material
		SDL_Log("Material %d: %s", 1, material->GetName().C_Str());
		SDL_assert(material->GetTextureCount(aiTextureType_DIFFUSE) == 1);

		aiString* assimpPath = new aiString();
		material->GetTexture(aiTextureType_DIFFUSE, 0, assimpPath);
		SDL_Log("Assimp path: %s", assimpPath->C_Str());
		std::filesystem::path texturePath = fullPath.parent_path() / assimpPath->C_Str();
		delete assimpPath;
		newMesh.texturePath = std::move(texturePath);

		newMesh.surfaces.push_back(newSurface);

		std::optional<GPUMeshBuffers> uploadResult = engine->UploadMesh(indices, vertices);
		if (!uploadResult.has_value()) {
			SDL_Log("Failed to upload mesh %s", newMesh.name.c_str());
			return std::nullopt;
		}
		newMesh.meshBuffers = uploadResult.value();

		meshes[h] = std::make_shared<MeshAsset>(std::move(newMesh));
	}

	return meshes;
}
