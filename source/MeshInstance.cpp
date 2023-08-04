#include "Pch.h"
#include "MeshInstance.h"

#include "File.h"

//---------------------------
constexpr float DEFAULT_BLENDING = 0.33f;
constexpr int BLEND_TO_BIND_POSE = -1;
constexpr Mesh::KeyframeBone blendb_zero(Vec3::Zero, Quat::Identity, Vec3::One);

typedef vector<byte>::const_iterator BoneIter;

//=================================================================================================
MeshInstance::MeshInstance(Mesh* mesh) : preload(false), mesh(mesh), needUpdate(true), ptr(nullptr), baseSpeed(1.f), matScale(nullptr)
{
	assert(mesh && mesh->IsLoaded() && mesh->IsAnimated());

	matBones.resize(mesh->head.nBones);
	blendb.resize(mesh->head.nBones);
	groups.resize(mesh->head.nGroups);
	memcpy(&blendb[0], &blendbZero, sizeof(blendbZero));
}

//=================================================================================================
void MeshInstance::Play(Mesh::Animation* anim, int flags, uint group)
{
	assert(anim);

	Group& gr = GetGroup(group);

	// ignoruj animacjê
	if(IsSet(flags, PLAY_IGNORE) && gr.anim == anim)
		return;

	// resetuj szybkoœæ i blending
	gr.speed = baseSpeed;
	gr.blendMax = DEFAULT_BLENDING;

	int newState = 0;

	// blending
	if(!IsSet(flags, PLAY_NO_BLEND))
	{
		SetupBlending(group);
		SetBit(newState, FLAG_BLENDING);
		if(IsSet(flags, PLAY_BLEND_WAIT))
			SetBit(newState, FLAG_BLEND_WAIT);
		gr.blendTime = 0.f;
	}

	// ustaw animacjê
	gr.anim = anim;
	gr.prio = ((flags & 0x60) >> 5);
	gr.state = newState | FLAG_PLAYING | FLAG_GROUP_ACTIVE;
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

		// odtwarzaj animacjê
		if(IsSet(gr.state, FLAG_PLAYING))
		{
			needUpdate = true;

			if(IsSet(gr.state, FLAG_BLEND_WAIT))
			{
				if(IsSet(gr.state, FLAG_BLENDING))
					continue;
			}

			// odtwarzaj od ty³u
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

	// oblicz przekszta³cenia dla ka¿dej grupy
	const word nGroups = mesh->head.nGroups;
	for(word boneGroup = 0; boneGroup < nGroups; ++boneGroup)
	{
		const Group& grBones = groups[boneGroup];
		const vector<byte>& bones = mesh->groups[boneGroup].bones;
		int animGroup;

		// ustal z któr¹ animacj¹ ustalaæ blending
		animGroup = GetUsableGroup(boneGroup);

		if(animGroup == BLEND_TO_BIND_POSE)
		{
			// nie ma ¿adnej animacji
			if(grBones.IsBlending())
			{
				// jest blending pomiêdzy B--->0
				float bt = grBones.blendTime / grBones.blendMax;

				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					Mesh::KeyframeBone::Interpolate(tmpKeyf, blendb[b], blendbZero, bt);
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
			const Group& grAnim = groups[animGroup];
			bool hit;
			const int index = grAnim.GetFrameIndex(hit);
			const vector<Mesh::Keyframe>& frames = grAnim.anim->frames;

			if(grAnim.IsBlending() || grBones.IsBlending())
			{
				// jest blending
				const float bt = (grBones.IsBlending() ? (grBones.blendTime / grBones.blendMax) :
					(grAnim.blendTime / grAnim.blendMax));

				if(hit)
				{
					// równe trafienie w klatkê
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
					// trzeba interpolowaæ
					const float t = (grAnim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
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
					// równe trafienie w klatkê
					const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
					for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
					{
						const word b = *it;
						keyf[b - 1].Mix(boneToParentPoseMat[b], mesh->bones[b].mat);
					}
				}
				else
				{
					// trzeba interpolowaæ
					const float t = (grAnim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
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

	if(predraw)
		predraw(ptr, BoneToParentPoseMat, 0);

	// Macierze przekszta³caj¹ce ze wsp. danej koœci do wsp. modelu w ustalonej pozycji
	// (To obliczenie nale¿a³oby po³¹czyæ z poprzednim)
	Matrix BoneToModelPoseMat[Mesh::MAX_BONES];
	BoneToModelPoseMat[0] = Matrix::IdentityMatrix;
	for(word i = 1; i < mesh->head.nBones; ++i)
	{
		const Mesh::Bone& bone = mesh->bones[i];

		// Jeœli to koœæ g³ówna, przekszta³cenie z danej koœci do nadrzêdnej = z danej koœci do modelu
		// Jeœli to nie koœæ g³ówna, przekszta³cenie z danej koœci do modelu = z danej koœci do nadrzêdnej * z nadrzêdnej do modelu
		if(bone.parent == 0)
			BoneToModelPoseMat[i] = boneToParentPoseMat[i];
		else
			BoneToModelPoseMat[i] = boneToParentPoseMat[i] * BoneToModelPoseMat[bone.parent];
	}

	// przeskaluj koœci
	if(matScale)
	{
		for(int i = 0; i < mesh->head.nBones; ++i)
			BoneToModelPoseMat[i] = BoneToModelPoseMat[i] * matScale[i];
	}

	// Macierze zebrane koœci - przekszta³caj¹ce z modelu do koœci w pozycji spoczynkowej * z koœci do modelu w pozycji bie¿¹cej
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

	// nowe ustalanie z której grupy braæ animacjê!
	// teraz wybiera wed³ug priorytetu
	animGroup = GetUsableGroup(boneGroup);

	if(animGroup == BLEND_TO_BIND_POSE)
	{
		// nie ma ¿adnej animacji
		if(grBones.IsBlending())
		{
			// jest blending pomiêdzy B--->0
			const float bt = grBones.blendTime / grBones.blendMax;

			for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
			{
				const word b = *it;
				Mesh::KeyframeBone::Interpolate(blendb[b], blendb[b], blendbZero, bt);
			}
		}
		else
		{
			// brak blendingu, wszystko na zero
			for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				memcpy(&blendb[*it], &blendbZero, sizeof(blendbZero));
		}
	}
	else
	{
		// jest jakaœ animacja
		const Group& grAnim = groups[animGroup];
		bool hit;
		const int index = grAnim.GetFrameIndex(hit);
		const vector<Mesh::Keyframe>& frames = grAnim.anim->frames;

		if(grAnim.IsBlending() || grBones.IsBlending())
		{
			// je¿eli grAnim == grBones to mo¿na to zoptymalizowaæ

			// jest blending
			const float bt = (grBones.IsBlending() ? (grBones.blendTime / grBones.blendMax) :
				(grAnim.blendTime / grAnim.blendMax));

			if(hit)
			{
				// równe trafienie w klatkê
				const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					Mesh::KeyframeBone::Interpolate(blendb[b], blendb[b], keyf[b - 1], bt);
				}
			}
			else
			{
				// trzeba interpolowaæ
				const float t = (grAnim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
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
				// równe trafienie w klatkê
				const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					blendb[b] = keyf[b - 1];
				}
			}
			else
			{
				// trzeba interpolowaæ
				const float t = (grAnim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
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

	// znajdz podrzêdne grupy które nie s¹ aktywne i ustaw im blending
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
int MeshInstance::GetHighestPriority(uint& _group) const
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
int MeshInstance::GetUsableGroup(uint group) const
{
	uint topGroup;
	int highestPrio = GetHighestPriority(topGroup);
	if(highestPrio == -1)
	{
		// brak jakiejkolwiek animacji
		return BLEND_TO_BIND_POSE;
	}
	else if(groups[group].IsActive() && groups[group].prio == highestPrio)
	{
		// u¿yj animacji z aktualnej grupy, to mo¿e byæ równoczeœnie 'topGroup'
		return group;
	}
	else
	{
		// u¿yj animacji z grupy z najwy¿szym priorytetem
		return topGroup;
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
	bool frameEndInfo, frameEndInfo2;
	if(version == 0)
	{
		f >> frameEndInfo;
		f >> frameEndInfo2;
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
		const string& animName = f.ReadString1();
		if(animName.empty())
			group.anim = nullptr;
		else
			group.anim = mesh->GetAnimation(animName.c_str());
		if(version >= 1)
			f >> group.frameEnd;
		else
		{
			if(index == 0)
				group.frameEnd = frameEndInfo;
			else if(index == 1)
				group.frameEnd = frameEndInfo2;
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
	byte groupsCount;

	f >> groupsCount;
	if(!f)
		return false;

	if(preload)
		groups.resize(groupsCount);

	for(Group& group : groups)
	{
		f >> group.time;
		f >> group.speed;
		f >> group.state;
		f.ReadCasted<byte>(group.prio);
		f.ReadCasted<byte>(group.usedGroup);
		const string& animId = f.ReadString1();
		f >> group.frameEnd;
		if(!f)
			return false;

		if(!animId.empty())
		{
			if(preload)
			{
				string* str = StringPool.Get();
				*str = animId;
				group.anim = (Mesh::Animation*)str;
			}
			else
			{
				group.anim = mesh->GetAnimation(animId.c_str());
				if(!group.anim)
				{
					Error("Missing animation '%s'.", animId.c_str());
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

//=================================================================================================
void MeshInstance::SetMesh(Mesh* mesh)
{
	assert(preload && mesh);

	if(this->mesh == mesh)
		return;

	this->mesh = mesh;
	matBones.resize(mesh->head.nBones);
	blendb.resize(mesh->head.nBones);
	groups.resize(mesh->head.nGroups);
}

//=================================================================================================
void MeshInstance::SetAnimation(Mesh::Animation* anim, float p)
{
	assert(anim && InRange(p, 0.f, 1.f));

	groups[0].anim = anim;
	groups[0].blendTime = 0.f;
	groups[0].state = FLAG_GROUP_ACTIVE;
	groups[0].time = anim->length * p;
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
