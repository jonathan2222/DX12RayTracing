#include "PreCompiled.h"
#include "FBXLoader.h"

#include "Core/CorePlatform.h"

#include "Loaders/openfbx/ofbx.h"

using namespace RS;

Mesh* RS::FBXLoader::Load(const std::string& path)
{
	std::string modelPath = RS_MODEL_PATH + path;

	Mesh* pMesh = new Mesh();

	CorePlatform::BinaryFile pFile = CorePlatform::LoadBinaryFile(modelPath);

	// Ignoring certain nodes will only stop them from being processed not tokenised(i.e.they will still be in the tree)
	ofbx::LoadFlags flags =
//		ofbx::LoadFlags::IGNORE_MODELS |
		ofbx::LoadFlags::IGNORE_BLEND_SHAPES |
		ofbx::LoadFlags::IGNORE_CAMERAS |
		ofbx::LoadFlags::IGNORE_LIGHTS |
//		ofbx::LoadFlags::IGNORE_TEXTURES |
		ofbx::LoadFlags::IGNORE_SKIN |
		ofbx::LoadFlags::IGNORE_BONES |
		ofbx::LoadFlags::IGNORE_PIVOTS |
//		ofbx::LoadFlags::IGNORE_MATERIALS |
		ofbx::LoadFlags::IGNORE_POSES |
		ofbx::LoadFlags::IGNORE_VIDEOS |
		ofbx::LoadFlags::IGNORE_LIMBS |
//		ofbx::LoadFlags::IGNORE_MESHES |
		ofbx::LoadFlags::IGNORE_ANIMATIONS;

	ofbx::IScene* pScene = nullptr;

	pScene = ofbx::load((const ofbx::u8*)pFile.pData.get(), (int)pFile.size, (ofbx::u16)flags);

	if (!pScene)
	{
		LOG_ERROR("[FBXLoader::Loade] Error: {}", ofbx::getError());
		delete pMesh;
		return nullptr;
	}

	// Fill mesh from scene.

	int meshCount = pScene->getMeshCount();
	if (meshCount <= 0)
	{
		LOG_ERROR("[FBXLoader::Loade] Error: No mesh was found in the file! File: {}", path.c_str());
		delete pMesh;
		return nullptr;
	}

	if (meshCount > 1)
		LOG_WARNING("[FBXLoader::Loade] Warning: Multiple meshes was found in the file, using the first one! File: {}", path.c_str());

	const ofbx::Mesh* pFbxMesh = pScene->getMesh(0);
	const ofbx::Geometry* pGeometry = pFbxMesh->getGeometry();
	const ofbx::GeometryData& geometryData = pGeometry->getGeometryData();
	ofbx::Vec3Attributes positions = geometryData.getPositions();

	int indices_offset = 0;
	int mesh_count = pScene->getMeshCount();

	// output unindexed geometry
	for (int mesh_idx = 0; mesh_idx < mesh_count; ++mesh_idx) {
		const ofbx::Mesh& mesh = *pScene->getMesh(mesh_idx);
		const ofbx::GeometryData& geom = mesh.getGeometryData();
		const ofbx::Vec3Attributes positions = geom.getPositions();
		const ofbx::Vec3Attributes normals = geom.getNormals();
		const ofbx::Vec2Attributes uvs = geom.getUVs();

		// each ofbx::Mesh can have several materials == partitions
		int acctualNumPartitions = geom.getPartitionCount();
		int numPartitions = std::min(1, acctualNumPartitions); // Only load 1 for now.
		for (int partition_idx = 0; partition_idx < numPartitions; ++partition_idx) {
			//fprintf(fp, "o obj%d_%d\ng grp%d\n", mesh_idx, partition_idx, mesh_idx);
			const ofbx::GeometryPartition& partition = geom.getPartition(partition_idx);

			// partitions most likely have several polygons, they are not triangles necessarily, use ofbx::triangulate if you want triangles
			for (int polygon_idx = 0; polygon_idx < partition.polygon_count; ++polygon_idx) {
				const ofbx::GeometryPartition::Polygon& polygon = partition.polygons[polygon_idx];

				int* triIndices = new int[3 * (polygon.vertex_count - 2)];
				int numVertices = ofbx::triangulate(geom, polygon, triIndices);

				bool has_normals = normals.values != nullptr;
				bool has_uvs = uvs.values != nullptr;
				for (int i = 0; i < numVertices; ++i) {
					int index = triIndices[i];
					ofbx::Vec3 v = positions.get(index);

					Vertex vertex;
					vertex.position.SetData(&v.x);
					if (has_normals) {
						ofbx::Vec3 n = normals.get(index);
						vertex.normal.SetData(&n.x);
					}

					if (has_uvs) {
						ofbx::Vec2 uv = uvs.get(index);
						vertex.uv.SetData(&uv.x);
					}

					pMesh->vertices.push_back(vertex);
				}
				delete[] triIndices;
			}

			//for (int polygon_idx = 0; polygon_idx < partition.polygon_count; ++polygon_idx) {
			//	const ofbx::GeometryPartition::Polygon& polygon = partition.polygons[polygon_idx];
			//	//fputs("f ", fp);
			//	for (int i = polygon.from_vertex; i < polygon.from_vertex + polygon.vertex_count; ++i) {
			//		//fprintf(fp, "%d ", 1 + i + indices_offset);
			//	}
			//	//fputs("\n", fp);
			//}

			indices_offset += positions.count;
		}
	}

	return pMesh;
}
