#include "PCH.hpp"
#include "Converter.h"
#include "Mesh.h"

const Vec3 DefaultSpecularColor(1, 1, 1);
const float DefaultSpecularIntensity = 0.2f;
const int DefaultSpecularHardness = 10;
extern bool anyWarning;

// Przechowuje dane na temat wybranej ko�ci we wsp. globalnych modelu w uk�adzie DirectX
struct BONE_INTER_DATA
{
	Vec3 HeadPos;
	Vec3 TailPos;
	float HeadRadius;
	float TailRadius;
	float Length;
	uint TmpBoneIndex; // Indeks tej ko�ci w Armature w TMP, liczony od 0 - stanowi mapowanie nowych na stare ko�ci
};

struct INTERMEDIATE_VERTEX_SKIN_DATA
{
	byte Index1;
	byte Index2;
	float Weight1;
};

cstring point_types[Mesh::Point::MAX] = {
	"PLAIN_AXES",
	"SPHERE",
	"CUBE",
	"ARROWS",
	"SINGLE_ARROW",
	"CIRCLE",
	"CONE"
};

void Converter::ConvertQmshTmpToQmsh(QMSH *Out, tmp::QMSH &QmshTmp, ConversionData& cs)
{
	allowDoubles = cs.allowDoubles;

	// Przekszta�� uk�ad wsp�rz�dnych
	TransformQmshTmpCoords(&QmshTmp);

	// BoneInterData odpowiada ko�ciom wyj�ciowym QMSH.Bones, nie wej�ciowym z TMP.Armature!!!
	std::vector<BONE_INTER_DATA> BoneInterData;

	Out->Flags = 0;
	if(cs.exportPhy)
		Out->Flags |= FLAG_PHYSICS;
	else
	{
		if(QmshTmp.static_anim)
			Out->Flags |= FLAG_STATIC;
		if(QmshTmp.Armature != nullptr)
		{
			Out->Flags |= FLAG_SKINNING;

			// Przetw�rz ko�ci
			TmpToQmsh_Bones(Out, &BoneInterData, QmshTmp);

			// Przetw�rz animacje
			if(!QmshTmp.static_anim)
				TmpToQmsh_Animations(Out, QmshTmp);
		}

		CreateBoneGroups(*Out, QmshTmp, cs);
	}

	if(QmshTmp.split)
		Out->Flags |= FLAG_SPLIT;

	BlenderToDirectxTransform(&Out->camera_pos, QmshTmp.camera_pos);
	BlenderToDirectxTransform(&Out->camera_target, QmshTmp.camera_target);
	BlenderToDirectxTransform(&Out->camera_up, QmshTmp.camera_up);

	Info("Processing mesh (NVMeshMender and more)...");

	struct NV_FACE
	{
		uint MaterialIndex;
		NV_FACE(uint MaterialIndex) : MaterialIndex(MaterialIndex) { }
	};

	// Dla kolejnych obiekt�w TMP
	for(uint oi = 0; oi < QmshTmp.Meshes.size(); oi++)
	{
		tmp::MESH & o = *QmshTmp.Meshes[oi].get();
		vertexNormals = o.vertex_normals;

		// Policz wp�yw ko�ci na wierzcho�ki tej siatki
		std::vector<INTERMEDIATE_VERTEX_SKIN_DATA> VertexSkinData;
		if((Out->Flags & FLAG_SKINNING) != 0)
			CalcVertexSkinData(&VertexSkinData, o, *Out, QmshTmp, BoneInterData);

		// Skonstruuj struktury po�rednie
		// UWAGA! Z�o�ono�� kwadratowa.
		// My�la�em nad tym ca�y dzie� i nic m�drego nie wymy�li�em jak to �adniej zrobi� :)

		bool MaterialIndicesUse[16] = {};

		std::vector<MeshMender::Vertex> NvVertices;
		std::vector<unsigned int> MappingNvToTmpVert;
		std::vector<unsigned int> NvIndices;
		std::vector<NV_FACE> NvFaces;
		std::vector<unsigned int> NvMappingOldToNewVert;

		NvVertices.reserve(o.Vertices.size());
		NvIndices.reserve(o.Faces.size() * 4);
		NvFaces.reserve(o.Faces.size());
		NvMappingOldToNewVert.reserve(o.Vertices.size());

		// Dla kolejnych �cianek TMP

		for(uint fi = 0; fi < o.Faces.size(); fi++)
		{
			tmp::FACE & f = o.Faces[fi];

			if(f.MaterialIndex >= 16)
				throw "Material index out of range.";
			MaterialIndicesUse[f.MaterialIndex] = true;

			// Pierwszy tr�jk�t
			NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[0], o.Vertices[f.VertexIndices[0]], f.TexCoords[0], f.Normal));
			NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[1], o.Vertices[f.VertexIndices[1]], f.TexCoords[1], f.Normal));
			NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[2], o.Vertices[f.VertexIndices[2]], f.TexCoords[2], f.Normal));
			// (wierzcho�ki dodane/zidenksowane, indeksy dodane - zostaje �cianka)
			NvFaces.push_back(NV_FACE(f.MaterialIndex));

			// Drugi tr�jk�t
			if(f.NumVertices == 4)
			{
				NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[0], o.Vertices[f.VertexIndices[0]], f.TexCoords[0], f.Normal));
				NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[2], o.Vertices[f.VertexIndices[2]], f.TexCoords[2], f.Normal));
				NvIndices.push_back(TmpConvert_AddVertex(NvVertices, MappingNvToTmpVert, f.VertexIndices[3], o.Vertices[f.VertexIndices[3]], f.TexCoords[3], f.Normal));
				NvFaces.push_back(NV_FACE(f.MaterialIndex));
			}
		}

		// Struktury po�rednie chyba ju� poprawnie wype�nione, czas odpali� NVMeshMender!

		float MinCreaseCosAngle = cosf(ToRadians(30)); // AutoSmoothAngle w przeciwie�stwie do k�t�w Eulera jest w stopniach!
		float WeightNormalsByArea = 1.0f;
		bool CalcNormals = !o.vertex_normals && !cs.exportPhy;
		bool RespectExistingSplits = CalcNormals;
		bool FixCylindricalWrapping = false;

		MeshMender mender;
		if(!mender.Mend(
			NvVertices,
			NvIndices,
			NvMappingOldToNewVert,
			MinCreaseCosAngle, MinCreaseCosAngle, MinCreaseCosAngle,
			WeightNormalsByArea,
			CalcNormals ? MeshMender::CALCULATE_NORMALS : MeshMender::DONT_CALCULATE_NORMALS,
			RespectExistingSplits ? MeshMender::RESPECT_SPLITS : MeshMender::DONT_RESPECT_SPLITS,
			FixCylindricalWrapping ? MeshMender::FIX_CYLINDRICAL : MeshMender::DONT_FIX_CYLINDRICAL))
		{
			throw "NVMeshMender failed.";
		}

		assert(NvIndices.size() % 3 == 0);

		if(NvVertices.size() > (uint)std::numeric_limits<word>::max())
			throw Format("Too many vertices in object \"%s\": %u", o.Name.c_str(), NvVertices.size());

		// Skonstruuj obiekty wyj�ciowe QMSH

		// Liczba u�ytych materia�ow
		uint NumMatUse = 0;
		for(uint mi = 0; mi < 16; mi++)
			if(MaterialIndicesUse[mi])
				NumMatUse++;

		// Wierzcho�ki (to b�dzie wsp�lne dla wszystkich podsiatek QMSH powsta�ych z bie��cego obiektu QMSH TMP)
		uint MinVertexIndexUsed = Out->Vertices.size();
		uint NumVertexIndicesUsed = NvVertices.size();
		Out->Vertices.reserve(Out->Vertices.size() + NumVertexIndicesUsed);
		for(uint vi = 0; vi < NvVertices.size(); vi++)
		{
			QMSH_VERTEX v;
			v.Pos = NvVertices[vi].pos;
			v.Tex.x = NvVertices[vi].s;
			v.Tex.y = NvVertices[vi].t;
			v.Normal = NvVertices[vi].normal;
			//BlenderToDirectxTransform(&v.Tangent, NvVertices[vi].tangent);
			//BlenderToDirectxTransform(&v.Binormal, NvVertices[vi].binormal);
			v.Tangent = NvVertices[vi].tangent;
			v.Binormal = NvVertices[vi].binormal;
			if(QmshTmp.Armature != nullptr)
			{
				const INTERMEDIATE_VERTEX_SKIN_DATA &ivsd = VertexSkinData[MappingNvToTmpVert[NvMappingOldToNewVert[vi]]];
				v.BoneIndices = (ivsd.Index1) | (ivsd.Index2 << 8);
				v.Weight1 = ivsd.Weight1;
			}
			Out->Vertices.push_back(v);
		}

		// Dla wszystkich u�ytych materia��w
		for(uint mi = 0; mi < 16; mi++)
		{
			if(MaterialIndicesUse[mi])
			{
				shared_ptr<QMSH_SUBMESH> submesh(new QMSH_SUBMESH);

				submesh->normal_factor = 0;
				submesh->specular_factor = 0;
				submesh->specular_color_factor = 0;

				// Nazwa materia�u
				if(mi < o.Materials.size())
				{
					tmp::Material& m = *o.Materials[mi];

					// Nazwa podsiatki
					// (Je�li wi�cej u�ytych materia��w, to obiekt jest rozbijany na wiele podsiatek i do nazwy dodawany jest materia�)
					submesh->Name = (NumMatUse == 1 ? o.Name : o.Name + "." + m.name);
					submesh->MaterialName = m.name;
					submesh->specular_color = m.specular_color;
					submesh->specular_intensity = m.specular_intensity;
					submesh->specular_hardness = m.specular_hardness;

					if(m.image.empty())
					{
						// nowe mo�liwo�ci, kilka tekstur
						for(std::vector<shared_ptr<tmp::Texture> >::iterator tit = m.textures.begin(), tend = m.textures.end(); tit != tend; ++tit)
						{
							tmp::Texture& t = **tit;
							if(t.use_diffuse && !t.use_normal && !t.use_specular && !t.use_specular_color)
							{
								// diffuse
								if(!submesh->texture.empty())
								{
									throw Format("Mesh '%s', material '%s', texture '%s': material already have diffuse texture '%s'!",
										o.Name.c_str(), m.name.c_str(), t.image.c_str(), submesh->texture.c_str());
								}
								submesh->texture = t.image;
								if(!Equal(t.diffuse_factor, 1.f))
								{
									Warn("Mesh '%s', material '%s', texture '%s' has diffuse factor %g (only 1.0 is supported).", o.Name.c_str(), m.name.c_str(), t.image.c_str(),
										t.diffuse_factor);
									anyWarning = true;
								}
							}
							else if(!t.use_diffuse && t.use_normal && !t.use_specular && !t.use_specular_color)
							{
								// normal
								if(!submesh->normalmap_texture.empty())
								{
									throw Format("Mesh '%s', material '%s', texture '%s': material already have normal texture '%s'!",
										o.Name.c_str(), m.name.c_str(), t.image.c_str(), submesh->normalmap_texture.c_str());
								}
								submesh->normalmap_texture = t.image;
								submesh->normal_factor = t.normal_factor;

								// eksportuj tangents
								Out->Flags |= FLAG_TANGENTS;
							}
							else if(!t.use_diffuse && !t.use_normal && t.use_specular && t.use_specular_color)
							{
								// specular
								if(!submesh->specularmap_texture.empty())
								{
									throw Format("Mesh '%s', material '%s', texture '%s': material already have specular texture '%s'!",
										o.Name.c_str(), m.name.c_str(), t.image, submesh->specularmap_texture.c_str());
								}
								submesh->specularmap_texture = t.image;
								submesh->specular_factor = t.specular_factor;
								submesh->specular_color_factor = t.specular_color_factor;
							}
						}
					}
					else
					{
						// zwyk�a tekstura diffuse
						submesh->texture = m.image;
					}
				}
				else
				{
					submesh->Name = o.Name;
					submesh->MaterialName = o.Name;
					submesh->specular_color = DefaultSpecularColor;
					submesh->specular_intensity = DefaultSpecularIntensity;
					submesh->specular_hardness = DefaultSpecularHardness;
				}

				// Indeksy
				submesh->MinVertexIndexUsed = MinVertexIndexUsed;
				submesh->NumVertexIndicesUsed = NumVertexIndicesUsed;
				submesh->FirstTriangle = Out->Indices.size() / 3;

				// Dodaj indeksy
				submesh->NumTriangles = 0;
				Out->Indices.reserve(Out->Indices.size() + NvIndices.size());
				for(uint fi = 0, ii = 0; fi < NvFaces.size(); fi++, ii += 3)
				{
					if(NvFaces[fi].MaterialIndex == mi)
					{
						Out->Indices.push_back((word)(NvIndices[ii] + MinVertexIndexUsed));
						Out->Indices.push_back((word)(NvIndices[ii + 1] + MinVertexIndexUsed));
						Out->Indices.push_back((word)(NvIndices[ii + 2] + MinVertexIndexUsed));
						submesh->NumTriangles++;
					}
				}

				// Podsiatka gotowa
				Out->Submeshes.push_back(submesh);
			}
		}
	}

	if(QmshTmp.force_tangents)
		Out->Flags |= FLAG_TANGENTS;

	ConvertPoints(QmshTmp, *Out);
	CalcBoundingVolumes(*Out);
}

void Converter::TmpToQmsh_Bones(QMSH *Out, std::vector<BONE_INTER_DATA> *OutBoneInterData, const tmp::QMSH &QmshTmp)
{
	Info("Processing bones...");
	const tmp::ARMATURE & TmpArmature = *QmshTmp.Armature.get();

	// Ta macierz przekszta�ca wsp. z lokalnych obiektu Armature do globalnych �wiata.
	// B�dzie ju� w uk�adzie DirectX.
	Matrix ArmatureToWorldMat;
	AssemblyBlenderObjectMatrix(&ArmatureToWorldMat, TmpArmature.Position, TmpArmature.Orientation, TmpArmature.Size);
	BlenderToDirectxTransform(&ArmatureToWorldMat);

	// Dla ka�dej ko�ci g��wnego poziomu
	for(uint bi = 0; bi < TmpArmature.Bones.size(); bi++)
	{
		const tmp::BONE & TmpBone = *TmpArmature.Bones[bi].get();
		if(TmpBone.Parent.empty())
		{
			// Utw�rz ko�� QMSH
			shared_ptr<QMSH_BONE> Bone(new QMSH_BONE);
			Out->Bones.push_back(Bone);

			Bone->Name = TmpBone.Name;
			Bone->ParentIndex = 0; // Brak parenta

			TmpToQmsh_Bone(
				Bone.get(),
				Out->Bones.size(), // Bez -1, bo indeksujemy od 0
				Out,
				TmpBone,
				TmpArmature,
				0, // Nie ma parenta
				ArmatureToWorldMat);
		}
	}

	// Sprawdzenie liczby ko�ci
	if(Out->Bones.size() != QmshTmp.Armature->Bones.size())
	{
		WarnOnce(1, "Skipped %u invalid bones.", QmshTmp.Armature->Bones.size() - Out->Bones.size());
		anyWarning = true;
	}

	// Policzenie Bone Inter Data
	OutBoneInterData->resize(Out->Bones.size());
	for(uint bi = 0; bi < Out->Bones.size(); bi++)
	{
		QMSH_BONE & OutBone = *Out->Bones[bi].get();
		for(uint bj = 0; bj <= TmpArmature.Bones.size(); bj++) // To <= to cze�� tego zabezpieczenia
		{
			assert(bj < TmpArmature.Bones.size()); // zabezpieczenie �e po�r�d ko�ci �r�d�owych zawsze znajdzie si� te� ta docelowa

			const tmp::BONE & InBone = *TmpArmature.Bones[bj].get();
			if(OutBone.Name == InBone.Name)
			{
				BONE_INTER_DATA & bid = (*OutBoneInterData)[bi];

				bid.TmpBoneIndex = bj;

				Vec3 v;
				BlenderToDirectxTransform(&v, InBone.Head);
				bid.HeadPos = Vec3::Transform(v, ArmatureToWorldMat);
				BlenderToDirectxTransform(&v, InBone.Tail);
				bid.TailPos = Vec3::Transform(v, ArmatureToWorldMat);

				if(!Equal(TmpArmature.Size.x, TmpArmature.Size.y) || !Equal(TmpArmature.Size.y, TmpArmature.Size.z))
				{
					WarnOnce(2342235, "Non uniform scaling of Armature object may give invalid bone envelopes.");
					anyWarning = true;
				}

				float ScaleFactor = (TmpArmature.Size.x + TmpArmature.Size.y + TmpArmature.Size.z) / 3.f;

				bid.HeadRadius = InBone.HeadRadius * ScaleFactor;
				bid.TailRadius = InBone.TailRadius * ScaleFactor;

				bid.Length = Vec3::Distance(bid.HeadPos, bid.TailPos);

				break;
			}
		}
	}
}

// Funkcja rekurencyjna
void Converter::TmpToQmsh_Bone(QMSH_BONE *Out, uint OutIndex, QMSH *OutQmsh, const tmp::BONE &TmpBone, const tmp::ARMATURE &TmpArmature,
	uint ParentIndex, const Matrix &WorldToParent)
{
	Out->Name = TmpBone.Name;
	Out->ParentIndex = ParentIndex;

	// TmpBone.Matrix_Armaturespace zawiera przekszta�cenie ze wsp. lokalnych tej ko�ci do wsp. armature w uk�. Blender
	// BoneToArmature zawiera przekszta�cenie ze wsp. lokalnych tej ko�ci do wsp. armature w uk�. DirectX
	Matrix BoneToArmature;
	BlenderToDirectxTransform(&BoneToArmature, TmpBone.matrix);
	// Out->Matrix b�dzie zwiera�o przekszta�cenie ze wsp. tej ko�ci do jej parenta w uk�. DirectX
	Out->matrix = BoneToArmature * WorldToParent;
	Matrix ArmatureToBone = BoneToArmature.Inverse();

	Out->RawMatrix = TmpBone.matrix;
	Out->head = Vec4(TmpBone.Head, TmpBone.HeadRadius);
	Out->tail = Vec4(TmpBone.Tail, TmpBone.TailRadius);
	Out->connected = TmpBone.connected;

	// Ko�ci potomne
	for(uint bi = 0; bi < TmpArmature.Bones.size(); bi++)
	{
		const tmp::BONE & TmpSubBone = *TmpArmature.Bones[bi].get();
		if(TmpSubBone.Parent == TmpBone.Name)
		{
			shared_ptr<QMSH_BONE> SubBone(new QMSH_BONE);
			OutQmsh->Bones.push_back(SubBone);
			Out->Children.push_back(OutQmsh->Bones.size());

			TmpToQmsh_Bone(
				SubBone.get(),
				OutQmsh->Bones.size(), // Bez -1, bo indeksujemy od 0
				OutQmsh,
				TmpSubBone,
				TmpArmature,
				OutIndex,
				ArmatureToBone);
		}
	}
}

void Converter::TmpToQmsh_Animations(QMSH *OutQmsh, const tmp::QMSH &QmshTmp)
{
	Info("Processing animations...");

	for(uint i = 0; i < QmshTmp.Actions.size(); i++)
	{
		shared_ptr<QMSH_ANIMATION> Animation(new QMSH_ANIMATION);
		TmpToQmsh_Animation(Animation.get(), *QmshTmp.Actions[i].get(), *OutQmsh, QmshTmp);
		OutQmsh->Animations.push_back(Animation);
	}

	if(OutQmsh->Animations.size() > std::numeric_limits<word>::max())
		throw "Too many animations";
}

void Converter::TmpToQmsh_Animation(QMSH_ANIMATION *OutAnimation, const tmp::ACTION &TmpAction, const QMSH &Qmsh, const tmp::QMSH &QmshTmp)
{
	OutAnimation->Name = TmpAction.Name;

	uint BoneCount = Qmsh.Bones.size();
	// Wsp�czynnik do przeliczania klatek na sekundy
	if(QmshTmp.FPS == 0)
		throw "FPS cannot be zero.";
	float TimeFactor = 1.f / (float)QmshTmp.FPS;

	// Lista wska�nik�w do krzywych dotycz�cych poszczeg�lnych parametr�w danej ko�ci QMSH
	// (Mog� by� nullptr)

	struct CHANNEL_POINTERS {
		const tmp::CURVE *LocX, *LocY, *LocZ, *QuatX, *QuatY, *QuatZ, *QuatW, *SizeX, *SizeY, *SizeZ;
		uint LocX_index, LocY_index, LocZ_index, QuatX_index, QuatY_index, QuatZ_index, QuatW_index, SizeX_index, SizeY_index, SizeZ_index;
	};
	std::vector<CHANNEL_POINTERS> BoneChannelPointers;
	BoneChannelPointers.resize(BoneCount);
	bool WarningInterpolation = false, WarningExtend = false;
	float EarliestNextFrame = std::numeric_limits<float>::max();
	// Dla kolejnych ko�ci
	for(uint bi = 0; bi < BoneCount; bi++)
	{
		BoneChannelPointers[bi].LocX = nullptr;  BoneChannelPointers[bi].LocX_index = 0;
		BoneChannelPointers[bi].LocY = nullptr;  BoneChannelPointers[bi].LocY_index = 0;
		BoneChannelPointers[bi].LocZ = nullptr;  BoneChannelPointers[bi].LocZ_index = 0;
		BoneChannelPointers[bi].QuatX = nullptr; BoneChannelPointers[bi].QuatX_index = 0;
		BoneChannelPointers[bi].QuatY = nullptr; BoneChannelPointers[bi].QuatY_index = 0;
		BoneChannelPointers[bi].QuatZ = nullptr; BoneChannelPointers[bi].QuatZ_index = 0;
		BoneChannelPointers[bi].QuatW = nullptr; BoneChannelPointers[bi].QuatW_index = 0;
		BoneChannelPointers[bi].SizeX = nullptr; BoneChannelPointers[bi].SizeX_index = 0;
		BoneChannelPointers[bi].SizeY = nullptr; BoneChannelPointers[bi].SizeY_index = 0;
		BoneChannelPointers[bi].SizeZ = nullptr; BoneChannelPointers[bi].SizeZ_index = 0;

		//Znajd� kana� odpowiadaj�cy tej ko�ci
		const string & BoneName = Qmsh.Bones[bi]->Name;
		for(uint ci = 0; ci < TmpAction.Channels.size(); ci++)
		{
			if(TmpAction.Channels[ci]->Name == BoneName)
			{
				// Kana� znaleziony - przejrzyj jego krzywe
				const tmp::CHANNEL &TmpChannel = *TmpAction.Channels[ci].get();
				for(uint curvi = 0; curvi < TmpChannel.Curves.size(); curvi++)
				{
					const tmp::CURVE *TmpCurve = TmpChannel.Curves[curvi].get();
					// Zidentyfikuj po nazwie
					if(TmpCurve->Name == "LocX")
					{
						BoneChannelPointers[bi].LocX = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "LocY")
					{
						BoneChannelPointers[bi].LocY = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "LocZ")
					{
						BoneChannelPointers[bi].LocZ = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "QuatX")
					{
						BoneChannelPointers[bi].QuatX = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "QuatY")
					{
						BoneChannelPointers[bi].QuatY = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "QuatZ")
					{
						BoneChannelPointers[bi].QuatZ = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "QuatW")
					{
						BoneChannelPointers[bi].QuatW = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "ScaleX")
					{
						BoneChannelPointers[bi].SizeX = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "ScaleY")
					{
						BoneChannelPointers[bi].SizeY = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
					else if(TmpCurve->Name == "ScaleZ")
					{
						BoneChannelPointers[bi].SizeZ = TmpCurve;
						if(!TmpCurve->Points.empty())
							EarliestNextFrame = std::min(EarliestNextFrame, TmpCurve->Points[0].x);
					}
				}

				break;
			}
		}

		if(BoneChannelPointers[bi].LocX != nullptr && BoneChannelPointers[bi].LocX->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].LocY != nullptr && BoneChannelPointers[bi].LocY->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].LocZ != nullptr && BoneChannelPointers[bi].LocZ->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].QuatX != nullptr && BoneChannelPointers[bi].QuatX->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].QuatY != nullptr && BoneChannelPointers[bi].QuatY->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].QuatZ != nullptr && BoneChannelPointers[bi].QuatZ->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].QuatW != nullptr && BoneChannelPointers[bi].QuatW->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].SizeX != nullptr && BoneChannelPointers[bi].SizeX->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].SizeY != nullptr && BoneChannelPointers[bi].SizeY->Interpolation == tmp::INTERPOLATION_CONST ||
			BoneChannelPointers[bi].SizeZ != nullptr && BoneChannelPointers[bi].SizeZ->Interpolation == tmp::INTERPOLATION_CONST)
		{
			WarningInterpolation = true;
		}
	}

	// Ostrze�enia
	if(WarningInterpolation)
	{
		WarnOnce(2352634, "Constant IPO interpolation mode not supported.");
		anyWarning = true;
	}

	// Wygenerowanie klatek kluczowych
	// (Nie ma ani jednej krzywej albo �adna nie ma punkt�w - nie b�dzie klatek kluczowych)
	while(EarliestNextFrame < std::numeric_limits<float>::max())
	{
		float NextNext = std::numeric_limits<float>::max();
		shared_ptr<QMSH_KEYFRAME> Keyframe(new QMSH_KEYFRAME);
		Keyframe->Time = (EarliestNextFrame - 1) * TimeFactor; // - 1, bo Blender liczy klatki od 1
		Keyframe->Bones.resize(BoneCount);
		for(uint bi = 0; bi < BoneCount; bi++)
		{
			QMSH_KEYFRAME_BONE& keyframeBone = Keyframe->Bones[bi];
			keyframeBone.Translation.x = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].LocX, &BoneChannelPointers[bi].LocX_index, EarliestNextFrame, &NextNext);
			keyframeBone.Translation.z = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].LocY, &BoneChannelPointers[bi].LocY_index, EarliestNextFrame, &NextNext);
			keyframeBone.Translation.y = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].LocZ, &BoneChannelPointers[bi].LocZ_index, EarliestNextFrame, &NextNext);
			keyframeBone.Rotation.x = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].QuatX, &BoneChannelPointers[bi].QuatX_index, EarliestNextFrame, &NextNext);
			keyframeBone.Rotation.z = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].QuatY, &BoneChannelPointers[bi].QuatY_index, EarliestNextFrame, &NextNext);
			keyframeBone.Rotation.y = CalcTmpCurveValue(0.f, BoneChannelPointers[bi].QuatZ, &BoneChannelPointers[bi].QuatZ_index, EarliestNextFrame, &NextNext);
			keyframeBone.Rotation.w = -CalcTmpCurveValue(1.f, BoneChannelPointers[bi].QuatW, &BoneChannelPointers[bi].QuatW_index, EarliestNextFrame, &NextNext);
			keyframeBone.Scaling.x = CalcTmpCurveValue(1.f, BoneChannelPointers[bi].SizeX, &BoneChannelPointers[bi].SizeX_index, EarliestNextFrame, &NextNext);
			keyframeBone.Scaling.z = CalcTmpCurveValue(1.f, BoneChannelPointers[bi].SizeY, &BoneChannelPointers[bi].SizeY_index, EarliestNextFrame, &NextNext);
			keyframeBone.Scaling.y = CalcTmpCurveValue(1.f, BoneChannelPointers[bi].SizeZ, &BoneChannelPointers[bi].SizeZ_index, EarliestNextFrame, &NextNext);
		}
		OutAnimation->Keyframes.push_back(Keyframe);

		EarliestNextFrame = NextNext;
	}

	// Czas trwania animacji, skalowanie czasu
	if(OutAnimation->Keyframes.empty())
	{
		OutAnimation->Length = 0.f;
		Warn("Animation '%s' have no frames.", OutAnimation->Name.c_str());
		anyWarning = true;
	}
	else if(OutAnimation->Keyframes.size() == 1)
	{
		OutAnimation->Length = 0.f;
		OutAnimation->Keyframes[0]->Time = 0.f;
	}
	else
	{
		float TimeOffset = -OutAnimation->Keyframes[0]->Time;
		for(uint ki = 0; ki < OutAnimation->Keyframes.size(); ki++)
			OutAnimation->Keyframes[ki]->Time += TimeOffset;
		OutAnimation->Length = OutAnimation->Keyframes[OutAnimation->Keyframes.size() - 1]->Time;
	}

	if(OutAnimation->Keyframes.size() > std::numeric_limits<word>::max())
		throw Format("Too many keyframes in animation \"%s\"", OutAnimation->Name.c_str());
}

// Zwraca warto�� krzywej w podanej klatce
// TmpCurve mo�e by� nullptr.
// PtIndex - indeks nast�pnego punktu
// NextFrame - je�li nast�pna klatka tej krzywej istnieje i jest mniejsza ni� NextFrame, ustawia NextFrame na ni�
float Converter::CalcTmpCurveValue(float DefaultValue, const tmp::CURVE *TmpCurve, uint *InOutPtIndex, float Frame, float *InOutNextFrame)
{
	// Nie ma krzywej
	if(TmpCurve == nullptr)
		return DefaultValue;
	// W og�le nie ma punkt�w
	if(TmpCurve->Points.empty())
		return DefaultValue;
	// To jest ju� koniec krzywej - przed�u�enie ostatniej warto�ci
	if(*InOutPtIndex >= TmpCurve->Points.size())
		return TmpCurve->Points[TmpCurve->Points.size() - 1].y;

	const Vec2 & NextPt = TmpCurve->Points[*InOutPtIndex];

	// - Przestawi� PtIndex (opcjonalnie)
	// - Przestawi� NextFrame
	// - Zwr�ci� warto��

	// Jeste�my dok�adnie w tym punkcie (lub za, co nie powinno mie� miejsca)
	if(Equal(NextPt.x, Frame) || (Frame > NextPt.x))
	{
		(*InOutPtIndex)++;
		if(*InOutPtIndex < TmpCurve->Points.size())
			*InOutNextFrame = std::min(*InOutNextFrame, TmpCurve->Points[*InOutPtIndex].x);
		return NextPt.y;
	}
	// Jeste�my przed pierwszym punktem - przed�u�enie warto�ci sta�ej w lewo
	else if(*InOutPtIndex == 0)
	{
		*InOutNextFrame = std::min(*InOutNextFrame, NextPt.x);
		return NextPt.y;
	}
	// Jeste�my mi�dzy punktem poprzednim a tym
	else
	{
		*InOutNextFrame = std::min(*InOutNextFrame, NextPt.x);
		const Vec2 & PrevPt = TmpCurve->Points[*InOutPtIndex - 1];
		// Interpolacja liniowa warto�ci wynikowej
		float t = (Frame - PrevPt.x) / (NextPt.x - PrevPt.x);
		return Lerp(PrevPt.y, NextPt.y, t);
	}
}

// Przekszta�ca pozycje, wektory i wsp�rz�dne tekstur z uk�adu Blendera do uk�adu DirectX
// Uwzgl�dnia te� przekszta�cenia ca�ych obiekt�w wprowadzaj�c je do przekszta�ce� poszczeg�lnych wierzcho�k�w.
// Zamienia te� kolejno�� wierzcho�k�w w �ciankach co by by�y odwr�cone w�a�ciw� stron�.
// NIE zmienia danych ko�ci ani animacji
void Converter::TransformQmshTmpCoords(tmp::QMSH *InOut)
{
	for(uint oi = 0; oi < InOut->Meshes.size(); oi++)
	{
		tmp::MESH & o = *InOut->Meshes[oi].get();

		// Zbuduj macierz przekszta�cania tego obiektu
		Matrix Mat, MatRot;
		AssemblyBlenderObjectMatrix(&Mat, o.Position, o.Orientation, o.Size);
		MatRot = Matrix::Rotation(o.Orientation);
		// Je�li obiekt jest sparentowany do Armature
		// To jednak nic, bo te wsp�rz�dne s� ju� wyeksportowane w uk�adzie globalnym modelu a nie wzgl�dem parenta.
		if(!o.ParentArmature.empty() && !InOut->static_anim)
		{
			assert(InOut->Armature != nullptr);
			Matrix ArmatureMat, ArmatureMatRot;
			AssemblyBlenderObjectMatrix(&ArmatureMat, InOut->Armature->Position, InOut->Armature->Orientation, InOut->Armature->Size);
			ArmatureMatRot = Matrix::Rotation(InOut->Armature->Orientation);
			Mat = Mat * ArmatureMat;
			MatRot = MatRot * ArmatureMatRot;
		}

		// Przekszta�� wierzcho�ki
		for(uint vi = 0; vi < o.Vertices.size(); vi++)
		{
			tmp::VERTEX & v = o.Vertices[vi];
			tmp::VERTEX T;
			T.Pos = Vec3::Transform(v.Pos, Mat);
			T.Normal = Vec3::Transform(v.Normal, MatRot);
			BlenderToDirectxTransform(&v.Pos, T.Pos);
			BlenderToDirectxTransform(&v.Normal, T.Normal);
		}

		// Przekszta�� �cianki
		for(uint fi = 0; fi < o.Faces.size(); fi++)
		{
			tmp::FACE & f = o.Faces[fi];

			// Wsp. tekstur
			for(uint vi = 0; vi < f.NumVertices; vi++)
				f.TexCoords[vi].y = 1.0f - f.TexCoords[vi].y;

			// Zamie� kolejno�� wierzcho�k�w w �ciance
			if(f.NumVertices == 3)
			{
				std::swap(f.VertexIndices[0], f.VertexIndices[2]);
				std::swap(f.TexCoords[0], f.TexCoords[2]);
			}
			else
			{
				std::swap(f.VertexIndices[0], f.VertexIndices[3]);
				std::swap(f.VertexIndices[1], f.VertexIndices[2]);
				std::swap(f.TexCoords[0], f.TexCoords[3]);
				std::swap(f.TexCoords[1], f.TexCoords[2]);
			}

			BlenderToDirectxTransform(&f.Normal);
		}
	}
}

// Szuka takiego wierzcho�ka w NvVertices.
// Je�li nie znajdzie, dodaje nowy.
// Czy znalaz� czy doda�, zwraca jego indeks w NvVertices.
uint Converter::TmpConvert_AddVertex(std::vector<MeshMender::Vertex>& NvVertices, std::vector<unsigned int>& MappingNvToTmpVert, uint TmpVertIndex,
	const tmp::VERTEX& TmpVertex, const Vec2& TexCoord, const Vec3& face_normal)
{
	if(!allowDoubles)
	{
		for(uint i = 0; i < NvVertices.size(); i++)
		{
			MeshMender::Vertex& v = NvVertices[i];
			// Pozycja identyczna...
			if(v.pos == TmpVertex.Pos &&
				// TexCoord mniej wi�cej...
				Equal(TexCoord.x, v.s) && Equal(TexCoord.y, v.t))
			{
				if(!vertexNormals || v.normal == TmpVertex.Normal)
					return i;
			}
		}
	}

	// Nie znaleziono
	MeshMender::Vertex v;
	v.pos = TmpVertex.Pos;
	v.s = TexCoord.x;
	v.t = TexCoord.y;
	v.normal = (vertexNormals ? TmpVertex.Normal : face_normal);
	NvVertices.push_back(v);
	MappingNvToTmpVert.push_back(TmpVertIndex);

	return NvVertices.size() - 1;
}

void Converter::CalcVertexSkinData(std::vector<INTERMEDIATE_VERTEX_SKIN_DATA> *Out, const tmp::MESH &TmpMesh, const QMSH &Qmsh,
	const tmp::QMSH &QmshTmp, const std::vector<BONE_INTER_DATA> &BoneInterData)
{
	Out->resize(TmpMesh.Vertices.size());

	// Kolejny algorytm pisany z wielk� nadziej� �e zadzia�a, wolny ale co z tego, lepszy taki
	// ni� �aden, i tak si� nad nim namy�la�em ca�y dzie�, a drugi poprawia�em bo si� wszystko pomiesza�o!

	// Nie ma skinningu
	if(TmpMesh.ParentArmature.empty())
	{
		for(uint vi = 0; vi < TmpMesh.Vertices.size(); vi++)
		{
			(*Out)[vi].Index1 = 0;
			(*Out)[vi].Index2 = 0;
			(*Out)[vi].Weight1 = 1.f;
		}
	}
	// Jest skinning ca�ej siatki do jednej ko�ci
	else if(!TmpMesh.ParentBone.empty())
	{
		// Znajd� indeks tej ko�ci
		byte BoneIndex = 0;
		for(uint bi = 0; bi < Qmsh.Bones.size(); bi++)
		{
			if(Qmsh.Bones[bi]->Name == TmpMesh.ParentBone)
			{
				BoneIndex = bi + 1;
				break;
			}
		}
		// Nie znaleziono
		if(BoneIndex == 0)
		{
			WarnOnce(13497325, "Object parented to non existing bone.");
			anyWarning = true;
		}
		// Przypisz t� ko�� do wszystkich wierzcho�k�w
		for(uint vi = 0; vi < TmpMesh.Vertices.size(); vi++)
		{
			(*Out)[vi].Index1 = BoneIndex;
			(*Out)[vi].Index2 = BoneIndex;
			(*Out)[vi].Weight1 = 1.f;
		}
	}
	// Vertex Group lub Envelope
	else
	{
		// U�� szybk� list� wierzcho�k�w z wagami dla ka�dej ko�ci

		// Typ reprezentuje dla danej ko�ci grup� wierzcho�k�w: Indeks wierzcho�ka => Waga
		typedef std::map<uint, float> INFLUENCE_MAP;
		std::vector<INFLUENCE_MAP> BoneInfluences;
		BoneInfluences.resize(Qmsh.Bones.size());
		// Dla ka�dej ko�ci
		for(uint bi = 0; bi < Qmsh.Bones.size(); bi++)
		{
			// Znajd� odpowiadaj�c� jej grup� wierzcho�k�w w przetwarzanej w tym wywo�aniu funkcji siatce
			uint VertexGroupIndex = TmpMesh.VertexGroups.size();
			for(uint gi = 0; gi < TmpMesh.VertexGroups.size(); gi++)
			{
				if(TmpMesh.VertexGroups[gi]->Name == Qmsh.Bones[bi]->Name)
				{
					VertexGroupIndex = gi;
					break;
				}
			}
			// Je�li znaleziono, przepisz wszystkie wierzcho�ki
			if(VertexGroupIndex < TmpMesh.VertexGroups.size())
			{
				const tmp::VERTEX_GROUP & VertexGroup = *TmpMesh.VertexGroups[VertexGroupIndex].get();
				for(uint vi = 0; vi < VertexGroup.VerticesInGroup.size(); vi++)
				{
					BoneInfluences[bi].insert(INFLUENCE_MAP::value_type(
						VertexGroup.VerticesInGroup[vi].Index,
						VertexGroup.VerticesInGroup[vi].Weight));
				}
			}
		}

		// Dla ka�dego wierzcho�ka
		for(uint vi = 0; vi < TmpMesh.Vertices.size(); vi++)
		{
			// Zbierz wszystkie wp�ywaj�ce na niego ko�ci
			struct INFLUENCE
			{
				uint BoneIndex; // Uwaga! Wy�ej by�o VertexIndex -> Weight, a tutaj jest BoneIndex -> Weight - ale masakra !!!
				float Weight;

				bool operator > (const INFLUENCE &Influence) const { return Weight > Influence.Weight; }
			};
			std::vector<INFLUENCE> VertexInfluences;

			// Wp�yw przez Vertex Groups
			for(uint bi = 0; bi < Qmsh.Bones.size(); bi++)
			{
				INFLUENCE_MAP::iterator biit = BoneInfluences[bi].find(vi);
				if(biit != BoneInfluences[bi].end())
				{
					INFLUENCE Influence = { bi + 1, biit->second };
					VertexInfluences.push_back(Influence);
				}
			}

			// �adna ko�� na niego nie wp�ywa - wyp�yw z Envelopes
			if(VertexInfluences.empty())
			{
				// U�� list� wp�ywaj�cych ko�ci
				for(uint bi = 0; bi < Qmsh.Bones.size(); bi++)
				{
					// Nie jestem pewny czy to jest dok�adnie algorytm u�ywany przez Blender,
					// nie jest nigdzie dok�adnie opisany, ale z eksperyment�w podejrzewam, �e tak.
					// Ko�� w promieniu - wp�yw maksymalny.
					// Ko�� w zakresie czego� co nazwa�em Extra Envelope te� jest wp�yw (podejrzewam �e s�abnie z odleg�o�ci�)
					// Promie� Extra Envelope, jak uda�o mi si� wreszcie ustali�, jest niezale�ny od Radius w sensie
					// �e rozci�ga si� ponad Radius zawsze na tyle ile wynosi (BoneLength / 4)

					// Dodatkowy promie� zewn�trzny
					float ExtraEnvelopeRadius = BoneInterData[bi].Length * 0.25f;

					// Pozycja wierzcho�ka ju� jest w uk�adzie globalnym modelu i w konwencji DirectX
					float W = PointToBone(
						TmpMesh.Vertices[vi].Pos,
						BoneInterData[bi].HeadPos, BoneInterData[bi].HeadRadius, BoneInterData[bi].HeadRadius + ExtraEnvelopeRadius,
						BoneInterData[bi].TailPos, BoneInterData[bi].TailRadius, BoneInterData[bi].TailRadius + ExtraEnvelopeRadius);

					if(W > 0.f)
					{
						INFLUENCE Influence = { bi + 1, W };
						VertexInfluences.push_back(Influence);
					}
				}
			}
			// Jakie� ko�ci na niego wp�ywaj� - we� z tych ko�ci

			// Zero ko�ci
			if(VertexInfluences.empty())
			{
				(*Out)[vi].Index1 = 0;
				(*Out)[vi].Index2 = 0;
				(*Out)[vi].Weight1 = 1.f;
			}
			// Tylko jedna ko��
			else if(VertexInfluences.size() == 1)
			{
				(*Out)[vi].Index1 = VertexInfluences[0].BoneIndex;
				(*Out)[vi].Index2 = VertexInfluences[0].BoneIndex;
				(*Out)[vi].Weight1 = 1.f;
			}
			// Dwie lub wi�cej ko�ci na li�cie wp�ywaj�cych na ten wierzcho�ek
			else
			{
				// Posortuj wp�ywy na wierzcho�ek malej�co, czyli od najwi�kszej wagi
				std::sort(VertexInfluences.begin(), VertexInfluences.end(), std::greater<INFLUENCE>());
				// We� dwie najwa�niejsze ko�ci
				(*Out)[vi].Index1 = VertexInfluences[0].BoneIndex;
				(*Out)[vi].Index2 = VertexInfluences[1].BoneIndex;
				// Oblicz wag� pierwszej
				// WA�NY WZ�R NA ZNORMALIZOWAN� WAG� PIERWSZ� Z DW�CH DOWOLNYCH WAG !!!
				(*Out)[vi].Weight1 = VertexInfluences[0].Weight / (VertexInfluences[0].Weight + VertexInfluences[1].Weight);
			}
		}
	}
}

// Liczy kolizj� punktu z bry�� wyznaczon� przez dwie po��czone kule, ka�da ma sw�j �rodek i promie�
// Wygl�ta to co� jak na ko�cach dwie p�kule a w �rodku walec albo �ci�ty sto�ek.
// Ka�dy koniec ma dwa promienie. Zwraca wp�yw ko�ci na ten punkt, tzn.:
// - Je�li punkt le�y w promieniu promienia wewn�trznego, zwraca 1.
// - Je�li punkt le�y w promieniu promienia zewn�trznego, zwraca 1..0, im dalej tym mniej.
// - Je�li punkt le�y poza promieniem zewn�trznym, zwraca 0.
float Converter::PointToBone(const Vec3 &Pt,
	const Vec3 &Center1, float InnerRadius1, float OuterRadius1,
	const Vec3 &Center2, float InnerRadius2, float OuterRadius2)
{
	float T = ClosestPointOnLine(Pt, Center1, Center2 - Center1) / Vec3::DistanceSquared(Center1, Center2);
	if(T <= 0.f)
	{
		float D = Vec3::Distance(Pt, Center1);
		if(D <= InnerRadius1)
			return 1.f;
		else if(D < OuterRadius1)
			return 1.f - (D - InnerRadius1) / (OuterRadius1 - InnerRadius1);
		else
			return 0.f;
	}
	else if(T >= 1.f)
	{
		float D = Vec3::Distance(Pt, Center2);
		if(D <= InnerRadius2)
			return 1.f;
		else if(D < OuterRadius2)
			return 1.f - (D - InnerRadius2) / (OuterRadius2 - InnerRadius2);
		else
			return 0.f;
	}
	else
	{
		float InterInnerRadius = Lerp(InnerRadius1, InnerRadius2, T);
		float InterOuterRadius = Lerp(OuterRadius1, OuterRadius2, T);
		Vec3 InterCenter = Vec3::Lerp(Center1, Center2, T);

		float D = Vec3::Distance(Pt, InterCenter);
		if(D <= InterInnerRadius)
			return 1.f;
		else if(D < InterOuterRadius)
			return 1.f - (D - InterInnerRadius) / (InterOuterRadius - InterInnerRadius);
		else
			return 0.f;
	}
}

void Converter::CreateBoneGroups(QMSH& qmsh, const tmp::QMSH &QmshTmp, ConversionData& cs)
{
	if(QmshTmp.Armature == nullptr)
		return;

	if(!QmshTmp.Armature->Groups.empty())
	{
		// use exported bone groups
		qmsh.Groups.resize(QmshTmp.Armature->Groups.size());
		for(uint i = 0; i < qmsh.Groups.size(); ++i)
		{
			QMSH_GROUP& group = qmsh.Groups[i];
			tmp::BONE_GROUP& tmp_group = *QmshTmp.Armature->Groups[i].get();
			group.name = tmp_group.Name;
			if(tmp_group.Parent.empty())
				group.parent = i;
			else
			{
				group.parent = QmshTmp.Armature->FindBoneGroupIndex(tmp_group.Parent);
				if(group.parent == (uint)-1)
					throw Format("Invalid parent bone group '%s'.", tmp_group.Parent.c_str());
			}
		}

		// verify all bones have valid bone group
		for(uint i = 0; i < qmsh.Bones.size(); ++i)
		{
			tmp::BONE& tmp_bone = *QmshTmp.Armature->Bones[i].get();
			if(tmp_bone.Group.empty())
				throw Format("Missing bone group for bone '%s'.", tmp_bone.Name.c_str());
			uint index = QmshTmp.Armature->FindBoneGroupIndex(tmp_bone.Group);
			if(index == (uint)-1)
				throw Format("Invalid parent bone group '%s' for bone '%s'.", tmp_bone.Group.c_str(), tmp_bone.Name.c_str());
			qmsh.Groups[index].bones.push_back(qmsh.Bones[i].get());
		}
	}
	else
	{
		qmsh.Groups.resize(1);
		QMSH_GROUP& gr = qmsh.Groups[0];
		gr.name = "default";
		gr.parent = 0;

		for(uint i = 0; i < qmsh.Bones.size(); ++i)
			gr.bones.push_back(&*qmsh.Bones[i]);
	}
}

// Sk�ada translacj�, rotacje i skalowanie w macierz w uk�adzie Blendera,
// kt�ra wykonuje te operacje transformuj�c ze wsp. lokalnych obiektu o podanych
// parametrach do wsp. globalnych Blendera
void Converter::AssemblyBlenderObjectMatrix(Matrix *Out, const Vec3 &Loc, const Vec3 &Rot, const Vec3 &Size)
{
	Matrix TmpMat;
	*Out = Matrix::Scale(Size);
	TmpMat = Matrix::RotationX(Rot.x);
	*Out *= TmpMat;
	TmpMat = Matrix::RotationY(Rot.y);
	*Out *= TmpMat;
	TmpMat = Matrix::RotationZ(Rot.z);
	*Out *= TmpMat;
	TmpMat = Matrix::Translation(Loc);
	*Out *= TmpMat;
}

// Przekszta�ca punkt lub wektor ze wsp�rz�dnych Blendera do DirectX
void Converter::BlenderToDirectxTransform(Vec3 *Out, const Vec3 &In)
{
	Out->x = In.x;
	Out->y = In.z;
	Out->z = In.y;
}

void Converter::BlenderToDirectxTransform(Vec3 *InOut)
{
	std::swap(InOut->y, InOut->z);
}

// Przekszta�ca macierz przekszta�caj�c� punkty/wektory z takiej dzia�aj�cej na wsp�rz�dnych Blendera
// na tak� dzia�aj�c� na wsp�rz�dnych DirectX-a (i odwrotnie te�).
void Converter::BlenderToDirectxTransform(Matrix *Out, const Matrix &In)
{
	Out->_11 = In._11;
	Out->_14 = In._14;
	Out->_41 = In._41;
	Out->_44 = In._44;

	Out->_12 = In._13;
	Out->_13 = In._12;

	Out->_43 = In._42;
	Out->_42 = In._43;

	Out->_31 = In._21;
	Out->_21 = In._31;

	Out->_24 = In._34;
	Out->_34 = In._24;

	Out->_22 = In._33;
	Out->_33 = In._22;
	Out->_23 = In._32;
	Out->_32 = In._23;
}

void Converter::BlenderToDirectxTransform(Matrix *InOut)
{
	std::swap(InOut->_12, InOut->_13);
	std::swap(InOut->_42, InOut->_43);

	std::swap(InOut->_21, InOut->_31);
	std::swap(InOut->_24, InOut->_34);

	std::swap(InOut->_22, InOut->_33);
	std::swap(InOut->_23, InOut->_32);
}

void Converter::ConvertPoints(tmp::QMSH& tmp, QMSH& mesh)
{
	Info("Converting points...");

	for(std::vector< shared_ptr<tmp::POINT> >::const_iterator it = tmp.points.begin(), end = tmp.points.end(); it != end; ++it)
	{
		shared_ptr<QMSH_POINT> point(new QMSH_POINT);
		tmp::POINT& pt = *((*it).get());

		point->name = pt.name;
		point->size = Vec3(abs(pt.size.x), abs(pt.size.y), abs(pt.size.z));
		BlenderToDirectxTransform(&point->size);
		point->type = (word)-1;
		for(int i = 0; i < Mesh::Point::MAX; ++i)
		{
			if(pt.type == point_types[i])
			{
				point->type = i;
				break;
			}
		}
		if(point->type == (word)-1)
			throw Format("Invalid point type '%s'.", pt.type.c_str());
		BlenderToDirectxTransform(&point->matrix, pt.matrix);
		BlenderToDirectxTransform(&point->rot, pt.rot);
		point->rot.y = Clip(-point->rot.y);

		uint i = 0;
		for(std::vector< shared_ptr<QMSH_BONE> >::const_iterator it = mesh.Bones.begin(), end = mesh.Bones.end(); it != end; ++i, ++it)
		{
			if((*it)->Name == pt.bone)
			{
				point->bone = i;
				break;
			}
		}

		mesh.Points.push_back(point);
	}
}

// Wylicza parametry bry� otaczaj�cych siatk�
void Converter::CalcBoundingVolumes(QMSH &Qmsh)
{
	Info("Calculating bounding volumes...");

	CalcBoundingVolumes(Qmsh, &Qmsh.BoundingSphereRadius, &Qmsh.BoundingBox);

	if((Qmsh.Flags & FLAG_SPLIT) != 0)
	{
		for(std::vector<shared_ptr<QMSH_SUBMESH> >::iterator it = Qmsh.Submeshes.begin(), end = Qmsh.Submeshes.end(); it != end; ++it)
			CalcBoundingVolumes(Qmsh, **it);
	}
}

void Converter::CalcBoundingVolumes(const QMSH &Qmsh, float *OutSphereRadius, Box *OutBox)
{
	if(Qmsh.Vertices.empty())
	{
		*OutSphereRadius = 0.f;
		*OutBox = Box(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
	}
	else
	{
		float MaxDistSq = Qmsh.Vertices[0].Pos.LengthSquared();
		OutBox->v1 = OutBox->v2 = Qmsh.Vertices[0].Pos;

		for(uint i = 1; i < Qmsh.Vertices.size(); i++)
		{
			MaxDistSq = std::max(MaxDistSq, Qmsh.Vertices[i].Pos.LengthSquared());
			OutBox->v1 = Vec3::Min(OutBox->v1, Qmsh.Vertices[i].Pos);
			OutBox->v2 = Vec3::Max(OutBox->v2, Qmsh.Vertices[i].Pos);
		}

		*OutSphereRadius = sqrtf(MaxDistSq);
	}
}

void Converter::CalcBoundingVolumes(const QMSH& mesh, QMSH_SUBMESH& sub)
{
	float radius = 0.f;
	Vec3 center;
	float inf = std::numeric_limits<float>::infinity();
	Box box(inf, inf, inf, -inf, -inf, -inf);

	for(uint i = 0; i < sub.NumTriangles; ++i)
	{
		uint face_index = i + sub.FirstTriangle;
		for(uint j = 0; j < 3; ++j)
		{
			uint index = mesh.Indices[face_index * 3 + j];
			box.AddPoint(mesh.Vertices[index].Pos);
		}
	}

	center = box.Midpoint();
	sub.box = box;

	for(uint i = 0; i < sub.NumTriangles; ++i)
	{
		uint face_index = i + sub.FirstTriangle;
		for(uint j = 0; j < 3; ++j)
		{
			uint index = mesh.Indices[face_index * 3 + j];
			const Vec3 &Tmp = mesh.Vertices[index].Pos;
			float dist = Vec3::Distance(center, Tmp);
			radius = std::max(radius, dist);
		}
	}

	sub.center = center;
	sub.range = radius;
}
