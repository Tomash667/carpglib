#include "Pch.h"
#include "MeshInstance.h"

#include "File.h"

//---------------------------
const float DEFAULT_BLENDING = 0.33f;
const int BLEND_TO_BIND_POSE = -1;
void(*MeshInstance::Predraw)(void*, Matrix*, int) = nullptr;
constexpr Mesh::KeyframeBone blendb_zero(Vec3::Zero, Quat::Identity, Vec3::One);

//=================================================================================================
MeshInstance::MeshInstance(Mesh* mesh) : preload(false), mesh(mesh), needUpdate(true), ptr(nullptr), baseSpeed(1.f), matScale(nullptr)
{
	assert(mesh && mesh->IsLoaded() && mesh->IsAnimated());

	matBones.resize(mesh->head.nBones);
	blendb.resize(mesh->head.nBones);
	groups.resize(mesh->head.nGroups);
	memcpy(&blendb[0], &blendb_zero, sizeof(blendb_zero));
}

//=================================================================================================
void MeshInstance::Play(Mesh::Animation* anim, int flags, uint group)
{
	assert(anim);

	Group& gr = GetGroup(group);

	// ignoruj animacj�
	if(IsSet(flags, PLAY_IGNORE) && gr.anim == anim)
		return;

	// resetuj szybko�� i blending
	gr.speed = baseSpeed;
	gr.blendMax = DEFAULT_BLENDING;

	int new_state = 0;

	// blending
	if(!IsSet(flags, PLAY_NO_BLEND))
	{
		SetupBlending(group);
		SetBit(new_state, FLAG_BLENDING);
		if(IsSet(flags, PLAY_BLEND_WAIT))
			SetBit(new_state, FLAG_BLEND_WAIT);
		gr.blendTime = 0.f;
	}

	// ustaw animacj�
	gr.anim = anim;
	gr.prio = ((flags & 0x60) >> 5);
	gr.state = new_state | FLAG_PLAYING | FLAG_GROUP_ACTIVE;
	if(IsSet(flags, PLAY_ONCE))
		SetBit(gr.state, FLAG_ONCE);
	if(IsSet(flags, PLAY_BACK))
	{
		SetBit(gr.state, FLAG_BACK);
		gr.time = anim->length;
	}
	else
		gr.time = 0.f;
	if(IsSet(flags, PLAY_STOP_AT_END))
		SetBit(gr.state, FLAG_STOP_AT_END);
	gr.frameEnd = false;

	// anuluj blending w innych grupach
	if(IsSet(flags, PLAY_NO_BLEND))
	{
		for(int g = 0; g < mesh->head.nGroups; ++g)
		{
			if(g != group && (!groups[g].IsActive() || groups[g].prio < gr.prio))
				ClearBit(groups[g].state, FLAG_BLENDING);
		}
	}
}

//=================================================================================================
void MeshInstance::Deactivate(uint group, bool inUpdate)
{
	Group& gr = GetGroup(group);
	if(IsSet(gr.state, FLAG_GROUP_ACTIVE))
	{
		SetupBlending(group, true, inUpdate);
		gr.state = FLAG_BLENDING;
		gr.blendTime = 0.f;
		gr.blendMax = DEFAULT_BLENDING;
	}
}

//=================================================================================================
void MeshInstance::Update(float dt)
{
	for(word i = 0; i < mesh->head.nGroups; ++i)
	{
		Group& gr = groups[i];

		if(IsSet(gr.state, FLAG_UPDATED))
		{
			ClearBit(gr.state, FLAG_UPDATED);
			continue;
		}

		// blending
		if(IsSet(gr.state, FLAG_BLENDING))
		{
			needUpdate = true;
			gr.blendTime += dt;
			if(gr.blendTime >= gr.blendMax)
				ClearBit(gr.state, FLAG_BLENDING);
		}

		// odtwarzaj animacj�
		if(IsSet(gr.state, FLAG_PLAYING))
		{
			needUpdate = true;

			if(IsSet(gr.state, FLAG_BLEND_WAIT))
			{
				if(IsSet(gr.state, FLAG_BLENDING))
					continue;
			}

			// odtwarzaj od ty�u
			if(IsSet(gr.state, FLAG_BACK))
			{
				gr.time -= dt * gr.speed;
				if(gr.time < 0) // przekroczono czas animacji
				{
					gr.frameEnd = true;
					if(IsSet(gr.state, FLAG_ONCE))
					{
						gr.time = 0;
						if(IsSet(gr.state, FLAG_STOP_AT_END))
							Stop(i);
						else
							Deactivate(i, true);
					}
					else
					{
						gr.time = fmod(gr.time, gr.anim->length) + gr.anim->length;
						if(gr.anim->nFrames == 1)
						{
							gr.time = 0;
							Stop(i);
						}
					}
				}
			}
			else // odtwarzaj normalnie
			{
				gr.time += dt * gr.speed;
				if(gr.time >= gr.anim->length) // przekroczono czas animacji
				{
					gr.frameEnd = true;
					if(IsSet(gr.state, FLAG_ONCE))
					{
						gr.time = gr.anim->length;
						if(IsSet(gr.state, FLAG_STOP_AT_END))
							Stop(i);
						else
							Deactivate(i, true);
					}
					else
					{
						gr.time = fmod(gr.time, gr.anim->length);
						if(gr.anim->nFrames == 1)
						{
							gr.time = 0;
							Stop(i);
						}
					}
				}
			}
		}
	}
}

//====================================================================================================
void MeshInstance::SetupBones()
{
	if(!needUpdate)
		return;
	needUpdate = false;

	Matrix boneToParentPoseMat[Mesh::MAX_BONES];
	boneToParentPoseMat[0] = Matrix::IdentityMatrix;
	Mesh::KeyframeBone tmpKeyf;

	// oblicz przekszta�cenia dla ka�dej grupy
	const word nGroups = mesh->head.nGroups;
	for(word boneGroup = 0; boneGroup < nGroups; ++boneGroup)
	{
		const Group& grBones = groups[boneGroup];
		const vector<byte>& bones = mesh->groups[boneGroup].bones;
		int animGroup;

		// ustal z kt�r� animacj� ustala� blending
		animGroup = GetUsableGroup(boneGroup);

		if(animGroup == BLEND_TO_BIND_POSE)
		{
			// nie ma �adnej animacji
			if(grBones.IsBlending())
			{
				// jest blending pomi�dzy B--->0
				float bt = grBones.blendTime / grBones.blendMax;

				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					Mesh::KeyframeBone::Interpolate(tmpKeyf, blendb[b], blendb_zero, bt);
					tmpKeyf.Mix(boneToParentPoseMat[b], mesh->bones[b].mat);
				}
			}
			else
			{
				// brak blendingu, wszystko na zero
				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					boneToParentPoseMat[b] = mesh->bones[b].mat;
				}
			}
		}
		else
		{
			const Group& gr_anim = groups[animGroup];
			bool hit;
			const int index = gr_anim.GetFrameIndex(hit);
			const vector<Mesh::Keyframe>& frames = gr_anim.anim->frames;

			if(gr_anim.IsBlending() || grBones.IsBlending())
			{
				// jest blending
				const float bt = (grBones.IsBlending() ? (grBones.blendTime / grBones.blendMax) :
					(gr_anim.blendTime / gr_anim.blendMax));

				if(hit)
				{
					// r�wne trafienie w klatk�
					const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
					for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
					{
						const word b = *it;
						Mesh::KeyframeBone::Interpolate(tmpKeyf, blendb[b], keyf[b - 1], bt);
						tmpKeyf.Mix(boneToParentPoseMat[b], mesh->bones[b].mat);
					}
				}
				else
				{
					// trzeba interpolowa�
					const float t = (gr_anim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
					const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
					const vector<Mesh::KeyframeBone>& keyf2 = frames[index + 1].bones;

					for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
					{
						const word b = *it;
						Mesh::KeyframeBone::Interpolate(tmpKeyf, keyf[b - 1], keyf2[b - 1], t);
						Mesh::KeyframeBone::Interpolate(tmpKeyf, blendb[b], tmpKeyf, bt);
						tmpKeyf.Mix(boneToParentPoseMat[b], mesh->bones[b].mat);
					}
				}
			}
			else
			{
				// nie ma blendingu
				if(hit)
				{
					// r�wne trafienie w klatk�
					const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
					for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
					{
						const word b = *it;
						keyf[b - 1].Mix(boneToParentPoseMat[b], mesh->bones[b].mat);
					}
				}
				else
				{
					// trzeba interpolowa�
					const float t = (gr_anim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
					const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
					const vector<Mesh::KeyframeBone>& keyf2 = frames[index + 1].bones;

					for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
					{
						const word b = *it;
						Mesh::KeyframeBone::Interpolate(tmpKeyf, keyf[b - 1], keyf2[b - 1], t);
						tmpKeyf.Mix(boneToParentPoseMat[b], mesh->bones[b].mat);
					}
				}
			}
		}
	}

	if(ptr)
		Predraw(ptr, boneToParentPoseMat, 0);

	// Macierze przekszta�caj�ce ze wsp. danej ko�ci do wsp. modelu w ustalonej pozycji
	// (To obliczenie nale�a�oby po��czy� z poprzednim)
	Matrix BoneToModelPoseMat[Mesh::MAX_BONES];
	BoneToModelPoseMat[0] = Matrix::IdentityMatrix;
	for(word i = 1; i < mesh->head.nBones; ++i)
	{
		const Mesh::Bone& bone = mesh->bones[i];

		// Je�li to ko�� g��wna, przekszta�cenie z danej ko�ci do nadrz�dnej = z danej ko�ci do modelu
		// Je�li to nie ko�� g��wna, przekszta�cenie z danej ko�ci do modelu = z danej ko�ci do nadrz�dnej * z nadrz�dnej do modelu
		if(bone.parent == 0)
			BoneToModelPoseMat[i] = boneToParentPoseMat[i];
		else
			BoneToModelPoseMat[i] = boneToParentPoseMat[i] * BoneToModelPoseMat[bone.parent];
	}

	// przeskaluj ko�ci
	if(matScale)
	{
		for(int i = 0; i < mesh->head.nBones; ++i)
			BoneToModelPoseMat[i] = BoneToModelPoseMat[i] * matScale[i];
	}

	// Macierze zebrane ko�ci - przekszta�caj�ce z modelu do ko�ci w pozycji spoczynkowej * z ko�ci do modelu w pozycji bie��cej
	matBones[0] = Matrix::IdentityMatrix;
	for(word i = 1; i < mesh->head.nBones; ++i)
		matBones[i] = mesh->modelToBone[i] * BoneToModelPoseMat[i];
}

//=================================================================================================
void MeshInstance::SetupBlending(uint boneGroup, bool first, bool inUpdate)
{
	int animGroup;
	const Group& grBones = groups[boneGroup];
	const vector<byte>& bones = mesh->groups[boneGroup].bones;

	// nowe ustalanie z kt�rej grupy bra� animacj�!
	// teraz wybiera wed�ug priorytetu
	animGroup = GetUsableGroup(boneGroup);

	if(animGroup == BLEND_TO_BIND_POSE)
	{
		// nie ma �adnej animacji
		if(grBones.IsBlending())
		{
			// jest blending pomi�dzy B--->0
			const float bt = grBones.blendTime / grBones.blendMax;

			for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
			{
				const word b = *it;
				Mesh::KeyframeBone::Interpolate(blendb[b], blendb[b], blendb_zero, bt);
			}
		}
		else
		{
			// brak blendingu, wszystko na zero
			for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				memcpy(&blendb[*it], &blendb_zero, sizeof(blendb_zero));
		}
	}
	else
	{
		// jest jaka� animacja
		const Group& gr_anim = groups[animGroup];
		bool hit;
		const int index = gr_anim.GetFrameIndex(hit);
		const vector<Mesh::Keyframe>& frames = gr_anim.anim->frames;

		if(gr_anim.IsBlending() || grBones.IsBlending())
		{
			// je�eli gr_anim == grBones to mo�na to zoptymalizowa�

			// jest blending
			const float bt = (grBones.IsBlending() ? (grBones.blendTime / grBones.blendMax) :
				(gr_anim.blendTime / gr_anim.blendMax));

			if(hit)
			{
				// r�wne trafienie w klatk�
				const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					Mesh::KeyframeBone::Interpolate(blendb[b], blendb[b], keyf[b - 1], bt);
				}
			}
			else
			{
				// trzeba interpolowa�
				const float t = (gr_anim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
				const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
				const vector<Mesh::KeyframeBone>& keyf2 = frames[index + 1].bones;
				Mesh::KeyframeBone tmpKeyf;

				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					Mesh::KeyframeBone::Interpolate(tmpKeyf, keyf[b - 1], keyf2[b - 1], t);
					Mesh::KeyframeBone::Interpolate(blendb[b], blendb[b], tmpKeyf, bt);
				}
			}
		}
		else
		{
			// nie ma blendingu
			if(hit)
			{
				// r�wne trafienie w klatk�
				const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					blendb[b] = keyf[b - 1];
				}
			}
			else
			{
				// trzeba interpolowa�
				const float t = (gr_anim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
				const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
				const vector<Mesh::KeyframeBone>& keyf2 = frames[index + 1].bones;

				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					Mesh::KeyframeBone::Interpolate(blendb[b], keyf[b - 1], keyf2[b - 1], t);
				}
			}
		}
	}

	// znajdz podrz�dne grupy kt�re nie s� aktywne i ustaw im blending
	if(first)
	{
		for(uint group = 0; group < mesh->head.nGroups; ++group)
		{
			Group& gr = groups[group];
			if(group != boneGroup && (!gr.IsActive() || gr.prio < grBones.prio))
			{
				SetupBlending(group, false);
				SetBit(gr.state, FLAG_BLENDING);
				if(inUpdate && group > boneGroup)
					SetBit(gr.state, FLAG_UPDATED);
				gr.blendTime = 0;
			}
		}
	}
}

//=================================================================================================
void MeshInstance::ClearEndResult()
{
	for(Group& group : groups)
		group.frameEnd = false;
}

//=================================================================================================
bool MeshInstance::IsBlending() const
{
	for(int i = 0; i < mesh->head.nGroups; ++i)
	{
		if(IsSet(groups[i].state, FLAG_BLENDING))
			return true;
	}
	return false;
}

//=================================================================================================
int MeshInstance::GetHighestPriority(uint& _group)
{
	int best = -1;

	for(uint i = 0; i < uint(mesh->head.nGroups); ++i)
	{
		if(groups[i].IsActive() && groups[i].prio > best)
		{
			best = groups[i].prio;
			_group = i;
		}
	}

	return best;
}

//=================================================================================================
int MeshInstance::GetUsableGroup(uint group)
{
	uint top_group;
	int highest_prio = GetHighestPriority(top_group);
	if(highest_prio == -1)
	{
		// brak jakiejkolwiek animacji
		return BLEND_TO_BIND_POSE;
	}
	else if(groups[group].IsActive() && groups[group].prio == highest_prio)
	{
		// u�yj animacji z aktualnej grupy, to mo�e by� r�wnocze�nie 'top_group'
		return group;
	}
	else
	{
		// u�yj animacji z grupy z najwy�szym priorytetem
		return top_group;
	}
}

//=================================================================================================
void MeshInstance::DisableAnimations()
{
	for(Group& group : groups)
	{
		group.anim = nullptr;
		group.state = 0;
	}
	for(int i = 0; i < mesh->head.nBones; ++i)
		matBones[i] = Matrix::IdentityMatrix;
	needUpdate = false;
}

//=================================================================================================
void MeshInstance::SetToEnd(Mesh::Animation* anim)
{
	assert(anim);

	groups[0].anim = anim;
	groups[0].blendTime = 0.f;
	groups[0].state = FLAG_GROUP_ACTIVE;
	groups[0].time = anim->length;
	groups[0].usedGroup = 0;
	groups[0].prio = 3;

	if(mesh->head.nGroups > 1)
	{
		for(int i = 1; i < mesh->head.nGroups; ++i)
		{
			groups[i].anim = nullptr;
			groups[i].state = 0;
			groups[i].usedGroup = 0;
			groups[i].time = groups[0].time;
			groups[i].blendTime = groups[0].blendTime;
		}
	}

	needUpdate = true;

	SetupBones();
}

//=================================================================================================
void MeshInstance::SetToEnd()
{
	groups[0].blendTime = 0.f;
	groups[0].state = FLAG_GROUP_ACTIVE;
	groups[0].time = groups[0].anim->length;
	groups[0].usedGroup = 0;
	groups[0].prio = 1;

	for(uint i = 1; i < groups.size(); ++i)
	{
		groups[i].state = 0;
		groups[i].usedGroup = 0;
		groups[i].blendTime = 0;
	}

	needUpdate = true;

	SetupBones();
}

//=================================================================================================
void MeshInstance::ResetAnimation()
{
	SetupBlending(0);

	groups[0].time = 0.f;
	groups[0].blendTime = 0.f;
	SetBit(groups[0].state, FLAG_BLENDING | FLAG_PLAYING);
}

//=================================================================================================
float MeshInstance::Group::GetBlendT() const
{
	if(IsBlending())
		return blendTime / blendMax;
	else
		return 1.f;
}

//=================================================================================================
void MeshInstance::Save(FileWriter& f) const
{
	for(const Group& group : groups)
	{
		f << group.time;
		f << group.speed;
		f << (group.state & ~FLAG_BLENDING); // don't save blending
		f << group.prio;
		f << group.usedGroup;
		if(group.anim)
			f << group.anim->name;
		else
			f.Write0();
		f << group.frameEnd;
	}
}

//=================================================================================================
void MeshInstance::SaveV2(StreamWriter& f) const
{
	const byte groupCount = (byte)groups.size();
	f << groupCount;
	if(groupCount == 1u)
	{
		const Group& group = groups[0];
		f << group.time;
		f << group.speed;
		f << (group.state & ~FLAG_BLENDING); // don't save blending
		if(group.anim)
			f << group.anim->name;
		else
			f.Write0();
		f << group.frameEnd;
	}
	else
	{
		for(const Group& group : groups)
		{
			f << group.time;
			f << group.speed;
			f << (group.state & ~FLAG_BLENDING); // don't save blending
			f << group.prio;
			f << group.usedGroup;
			if(group.anim)
				f << group.anim->name;
			else
				f.Write0();
			f << group.frameEnd;
		}
	}
}

//=================================================================================================
void MeshInstance::SaveOptional(StreamWriter& f, MeshInstance* meshInst)
{
	if(!meshInst || !meshInst->IsActive())
		f.Write0();
	else
		meshInst->SaveV2(f);
}

//=================================================================================================
void MeshInstance::Load(FileReader& f, int version)
{
	bool frame_end_info, frame_end_info2;
	if(version == 0)
	{
		f >> frame_end_info;
		f >> frame_end_info2;
	}

	int index = 0;
	for(Group& group : groups)
	{
		f >> group.time;
		f >> group.speed;
		group.blendTime = 0.f;
		f >> group.state;
		f >> group.prio;
		f >> group.usedGroup;
		const string& anim_name = f.ReadString1();
		if(anim_name.empty())
			group.anim = nullptr;
		else
			group.anim = mesh->GetAnimation(anim_name.c_str());
		if(version >= 1)
			f >> group.frameEnd;
		else
		{
			if(index == 0)
				group.frameEnd = frame_end_info;
			else if(index == 1)
				group.frameEnd = frame_end_info2;
			else
				group.frameEnd = false;
		}
		++index;
	}

	needUpdate = true;
}

//=================================================================================================
void MeshInstance::LoadV2(StreamReader& f)
{
	byte groupCount;
	f >> groupCount;
	groups.resize(groupCount);

	if(groupCount == 1u)
	{
		MeshInstance::Group& group = groups[0];
		f >> group.time;
		f >> group.speed;
		f >> group.state;
		const string& animName = f.ReadString1();
		if(!animName.empty())
			group.animName = StringPool.Get(animName);
		f >> group.frameEnd;
		group.usedGroup = 0;
		group.prio = 0;
	}
	else
	{
		for(MeshInstance::Group& group : groups)
		{
			f >> group.time;
			f >> group.speed;
			f >> group.state;
			f >> group.prio;
			f >> group.usedGroup;
			const string& animName = f.ReadString1();
			if(!animName.empty())
				group.animName = StringPool.Get(animName);
			f >> group.frameEnd;
		}
	}
}

//=================================================================================================
void MeshInstance::LoadOptional(StreamReader& f, MeshInstance*& meshInst)
{
	uint pos = f.GetPos();
	byte groups = f.Read<byte>();
	if(groups == 0u)
		meshInst = nullptr;
	else
	{
		f.SetPos(pos);
		meshInst = new MeshInstance(nullptr);
		meshInst->LoadV2(f);
	}
}

//=================================================================================================
void MeshInstance::LoadSimple(FileReader& f)
{
	groups.resize(1u);
	Group& group = groups[0];
	group.state = f.Read<int>();
	group.usedGroup = 0;
	group.prio = 0;
	if(group.state != 0)
	{
		f >> group.time;
		f >> group.blendTime;
		group.animName = StringPool.Get("first");
	}
	else
		group.animName = nullptr;
}

//=================================================================================================
void MeshInstance::Write(StreamWriter& f) const
{
	f.WriteCasted<byte>(groups.size());
	for(const Group& group : groups)
	{
		f << group.time;
		f << group.speed;
		f << group.state;
		f.WriteCasted<byte>(group.prio);
		f.WriteCasted<byte>(group.usedGroup);
		if(group.anim)
			f << group.anim->name;
		else
			f.Write0();
		f << group.frameEnd;
	}
}

//=================================================================================================
bool MeshInstance::Read(StreamReader& f)
{
	byte groups_count;

	f >> groups_count;
	if(!f)
		return false;

	if(preload)
		groups.resize(groups_count);

	for(Group& group : groups)
	{
		f >> group.time;
		f >> group.speed;
		f >> group.state;
		f.ReadCasted<byte>(group.prio);
		f.ReadCasted<byte>(group.usedGroup);
		const string& anim_id = f.ReadString1();
		f >> group.frameEnd;
		if(!f)
			return false;

		if(!anim_id.empty())
		{
			if(preload)
			{
				string* str = StringPool.Get();
				*str = anim_id;
				group.anim = (Mesh::Animation*)str;
			}
			else
			{
				group.anim = mesh->GetAnimation(anim_id.c_str());
				if(!group.anim)
				{
					Error("Missing animation '%s'.", anim_id.c_str());
					return false;
				}
			}
		}
		else
			group.anim = nullptr;
	}

	needUpdate = true;
	return true;
}

//=================================================================================================
bool MeshInstance::ApplyPreload(Mesh* mesh)
{
	assert(mesh
		&& mesh->IsLoaded()
		&& mesh->IsAnimated()
		&& preload
		&& !this->mesh
		&& groups.size() == mesh->head.nGroups);

	preload = false;
	this->mesh = mesh;
	matBones.resize(mesh->head.nBones);
	blendb.resize(mesh->head.nBones);

	for(Group& group : groups)
	{
		string* name = group.animName;
		if(name)
		{
			group.anim = mesh->GetAnimation(name->c_str());
			if(!group.anim)
			{
				if(*name == "first")
					group.anim = &mesh->anims[0];
				else
				{
					Error("Invalid animation '%s' for mesh '%s'.", name->c_str(), mesh->path.c_str());
					StringPool.Free(name);
					return false;
				}
			}
			StringPool.Free(name);
		}
	}

	return true;
}
