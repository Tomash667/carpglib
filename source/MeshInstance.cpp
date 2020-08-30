#include "Pch.h"
#include "MeshInstance.h"

#include "File.h"

//---------------------------
const float DEFAULT_BLENDING = 0.33f;
const int BLEND_TO_BIND_POSE = -1;
void(*MeshInstance::Predraw)(void*, Matrix*, int) = nullptr;
Mesh::KeyframeBone blendb_zero(Vec3::Zero, Quat::Identity, Vec3::One);

//=================================================================================================
MeshInstance::MeshInstance(Mesh* mesh) : preload(false), mesh(mesh), need_update(true), ptr(nullptr), base_speed(1.f), mat_scale(nullptr)
{
	assert(mesh && mesh->IsLoaded());

	mat_bones.resize(mesh->head.n_bones);
	blendb.resize(mesh->head.n_bones);
	groups.resize(mesh->head.n_groups);
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
	gr.speed = base_speed;
	gr.blend_max = DEFAULT_BLENDING;

	int new_state = 0;

	// blending
	if(!IsSet(flags, PLAY_NO_BLEND))
	{
		SetupBlending(group);
		SetBit(new_state, FLAG_BLENDING);
		if(IsSet(flags, PLAY_BLEND_WAIT))
			SetBit(new_state, FLAG_BLEND_WAIT);
		gr.blend_time = 0.f;
	}

	// ustaw animacjê
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
	gr.frame_end = false;

	// anuluj blending w innych grupach
	if(IsSet(flags, PLAY_NO_BLEND))
	{
		for(int g = 0; g < mesh->head.n_groups; ++g)
		{
			if(g != group && (!groups[g].IsActive() || groups[g].prio < gr.prio))
				ClearBit(groups[g].state, FLAG_BLENDING);
		}
	}
}

//=================================================================================================
void MeshInstance::Deactivate(uint group, bool in_update)
{
	Group& gr = GetGroup(group);
	if(IsSet(gr.state, FLAG_GROUP_ACTIVE))
	{
		SetupBlending(group, true, in_update);
		gr.state = FLAG_BLENDING;
		gr.blend_time = 0.f;
		gr.blend_max = DEFAULT_BLENDING;
	}
}

//=================================================================================================
void MeshInstance::Update(float dt)
{
	for(word i = 0; i < mesh->head.n_groups; ++i)
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
			need_update = true;
			gr.blend_time += dt;
			if(gr.blend_time >= gr.blend_max)
				ClearBit(gr.state, FLAG_BLENDING);
		}

		// odtwarzaj animacjê
		if(IsSet(gr.state, FLAG_PLAYING))
		{
			need_update = true;

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
					gr.frame_end = true;
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
						if(gr.anim->n_frames == 1)
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
					gr.frame_end = true;
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
						if(gr.anim->n_frames == 1)
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
	if(!need_update)
		return;
	need_update = false;

	Matrix BoneToParentPoseMat[Mesh::MAX_BONES];
	BoneToParentPoseMat[0] = Matrix::IdentityMatrix;
	Mesh::KeyframeBone tmp_keyf;

	// oblicz przekszta³cenia dla ka¿dej grupy
	const word n_groups = mesh->head.n_groups;
	for(word bones_group = 0; bones_group < n_groups; ++bones_group)
	{
		const Group& gr_bones = groups[bones_group];
		const vector<byte>& bones = mesh->groups[bones_group].bones;
		int anim_group;

		// ustal z któr¹ animacj¹ ustalaæ blending
		anim_group = GetUsableGroup(bones_group);

		if(anim_group == BLEND_TO_BIND_POSE)
		{
			// nie ma ¿adnej animacji
			if(gr_bones.IsBlending())
			{
				// jest blending pomiêdzy B--->0
				float bt = gr_bones.blend_time / gr_bones.blend_max;

				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					Mesh::KeyframeBone::Interpolate(tmp_keyf, blendb[b], blendb_zero, bt);
					tmp_keyf.Mix(BoneToParentPoseMat[b], mesh->bones[b].mat);
				}
			}
			else
			{
				// brak blendingu, wszystko na zero
				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					BoneToParentPoseMat[b] = mesh->bones[b].mat;
				}
			}
		}
		else
		{
			const Group& gr_anim = groups[anim_group];
			bool hit;
			const int index = gr_anim.GetFrameIndex(hit);
			const vector<Mesh::Keyframe>& frames = gr_anim.anim->frames;

			if(gr_anim.IsBlending() || gr_bones.IsBlending())
			{
				// jest blending
				const float bt = (gr_bones.IsBlending() ? (gr_bones.blend_time / gr_bones.blend_max) :
					(gr_anim.blend_time / gr_anim.blend_max));

				if(hit)
				{
					// równe trafienie w klatkê
					const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
					for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
					{
						const word b = *it;
						Mesh::KeyframeBone::Interpolate(tmp_keyf, blendb[b], keyf[b - 1], bt);
						tmp_keyf.Mix(BoneToParentPoseMat[b], mesh->bones[b].mat);
					}
				}
				else
				{
					// trzeba interpolowaæ
					const float t = (gr_anim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
					const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
					const vector<Mesh::KeyframeBone>& keyf2 = frames[index + 1].bones;

					for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
					{
						const word b = *it;
						Mesh::KeyframeBone::Interpolate(tmp_keyf, keyf[b - 1], keyf2[b - 1], t);
						Mesh::KeyframeBone::Interpolate(tmp_keyf, blendb[b], tmp_keyf, bt);
						tmp_keyf.Mix(BoneToParentPoseMat[b], mesh->bones[b].mat);
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
						keyf[b - 1].Mix(BoneToParentPoseMat[b], mesh->bones[b].mat);
					}
				}
				else
				{
					// trzeba interpolowaæ
					const float t = (gr_anim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
					const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
					const vector<Mesh::KeyframeBone>& keyf2 = frames[index + 1].bones;

					for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
					{
						const word b = *it;
						Mesh::KeyframeBone::Interpolate(tmp_keyf, keyf[b - 1], keyf2[b - 1], t);
						tmp_keyf.Mix(BoneToParentPoseMat[b], mesh->bones[b].mat);
					}
				}
			}
		}
	}

	if(ptr)
		Predraw(ptr, BoneToParentPoseMat, 0);

	// Macierze przekszta³caj¹ce ze wsp. danej koœci do wsp. modelu w ustalonej pozycji
	// (To obliczenie nale¿a³oby po³¹czyæ z poprzednim)
	Matrix BoneToModelPoseMat[Mesh::MAX_BONES];
	BoneToModelPoseMat[0] = Matrix::IdentityMatrix;
	for(word i = 1; i < mesh->head.n_bones; ++i)
	{
		const Mesh::Bone& bone = mesh->bones[i];

		// Jeœli to koœæ g³ówna, przekszta³cenie z danej koœci do nadrzêdnej = z danej koœci do modelu
		// Jeœli to nie koœæ g³ówna, przekszta³cenie z danej koœci do modelu = z danej koœci do nadrzêdnej * z nadrzêdnej do modelu
		if(bone.parent == 0)
			BoneToModelPoseMat[i] = BoneToParentPoseMat[i];
		else
			BoneToModelPoseMat[i] = BoneToParentPoseMat[i] * BoneToModelPoseMat[bone.parent];
	}

	// przeskaluj koœci
	if(mat_scale)
	{
		for(int i = 0; i < mesh->head.n_bones; ++i)
			BoneToModelPoseMat[i] = BoneToModelPoseMat[i] * mat_scale[i];
	}

	// Macierze zebrane koœci - przekszta³caj¹ce z modelu do koœci w pozycji spoczynkowej * z koœci do modelu w pozycji bie¿¹cej
	mat_bones[0] = Matrix::IdentityMatrix;
	for(word i = 1; i < mesh->head.n_bones; ++i)
		mat_bones[i] = mesh->model_to_bone[i] * BoneToModelPoseMat[i];
}

//=================================================================================================
void MeshInstance::SetupBlending(uint bones_group, bool first, bool in_update)
{
	int anim_group;
	const Group& gr_bones = groups[bones_group];
	const vector<byte>& bones = mesh->groups[bones_group].bones;

	// nowe ustalanie z której grupy braæ animacjê!
	// teraz wybiera wed³ug priorytetu
	anim_group = GetUsableGroup(bones_group);

	if(anim_group == BLEND_TO_BIND_POSE)
	{
		// nie ma ¿adnej animacji
		if(gr_bones.IsBlending())
		{
			// jest blending pomiêdzy B--->0
			const float bt = gr_bones.blend_time / gr_bones.blend_max;

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
		// jest jakaœ animacja
		const Group& gr_anim = groups[anim_group];
		bool hit;
		const int index = gr_anim.GetFrameIndex(hit);
		const vector<Mesh::Keyframe>& frames = gr_anim.anim->frames;

		if(gr_anim.IsBlending() || gr_bones.IsBlending())
		{
			// je¿eli gr_anim == gr_bones to mo¿na to zoptymalizowaæ

			// jest blending
			const float bt = (gr_bones.IsBlending() ? (gr_bones.blend_time / gr_bones.blend_max) :
				(gr_anim.blend_time / gr_anim.blend_max));

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
				const float t = (gr_anim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
				const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
				const vector<Mesh::KeyframeBone>& keyf2 = frames[index + 1].bones;
				Mesh::KeyframeBone tmp_keyf;

				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					Mesh::KeyframeBone::Interpolate(tmp_keyf, keyf[b - 1], keyf2[b - 1], t);
					Mesh::KeyframeBone::Interpolate(blendb[b], blendb[b], tmp_keyf, bt);
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

	// znajdz podrzêdne grupy które nie s¹ aktywne i ustaw im blending
	if(first)
	{
		for(uint group = 0; group < mesh->head.n_groups; ++group)
		{
			Group& gr = groups[group];
			if(group != bones_group && (!gr.IsActive() || gr.prio < gr_bones.prio))
			{
				SetupBlending(group, false);
				SetBit(gr.state, FLAG_BLENDING);
				if(in_update && group > bones_group)
					SetBit(gr.state, FLAG_UPDATED);
				gr.blend_time = 0;
			}
		}
	}
}

//=================================================================================================
void MeshInstance::ClearEndResult()
{
	for(Group& group : groups)
		group.frame_end = false;
}

//=================================================================================================
bool MeshInstance::IsBlending() const
{
	for(int i = 0; i < mesh->head.n_groups; ++i)
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

	for(uint i = 0; i < uint(mesh->head.n_groups); ++i)
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
		// u¿yj animacji z aktualnej grupy, to mo¿e byæ równoczeœnie 'top_group'
		return group;
	}
	else
	{
		// u¿yj animacji z grupy z najwy¿szym priorytetem
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
	for(int i = 0; i < mesh->head.n_bones; ++i)
		mat_bones[i] = Matrix::IdentityMatrix;
	need_update = false;
}

//=================================================================================================
void MeshInstance::SetToEnd(Mesh::Animation* anim)
{
	assert(anim);

	groups[0].anim = anim;
	groups[0].blend_time = 0.f;
	groups[0].state = FLAG_GROUP_ACTIVE;
	groups[0].time = anim->length;
	groups[0].used_group = 0;
	groups[0].prio = 3;

	if(mesh->head.n_groups > 1)
	{
		for(int i = 1; i < mesh->head.n_groups; ++i)
		{
			groups[i].anim = nullptr;
			groups[i].state = 0;
			groups[i].used_group = 0;
			groups[i].time = groups[0].time;
			groups[i].blend_time = groups[0].blend_time;
		}
	}

	need_update = true;

	SetupBones();
}

//=================================================================================================
void MeshInstance::SetToEnd()
{
	groups[0].blend_time = 0.f;
	groups[0].state = FLAG_GROUP_ACTIVE;
	groups[0].time = groups[0].anim->length;
	groups[0].used_group = 0;
	groups[0].prio = 1;

	for(uint i = 1; i < groups.size(); ++i)
	{
		groups[i].state = 0;
		groups[i].used_group = 0;
		groups[i].blend_time = 0;
	}

	need_update = true;

	SetupBones();
}

//=================================================================================================
void MeshInstance::ResetAnimation()
{
	SetupBlending(0);

	groups[0].time = 0.f;
	groups[0].blend_time = 0.f;
	SetBit(groups[0].state, FLAG_BLENDING | FLAG_PLAYING);
}

//=================================================================================================
float MeshInstance::Group::GetBlendT() const
{
	if(IsBlending())
		return blend_time / blend_max;
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
		f << group.used_group;
		if(group.anim)
			f << group.anim->name;
		else
			f.Write0();
		f << group.frame_end;
	}
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
		group.blend_time = 0.f;
		f >> group.state;
		f >> group.prio;
		f >> group.used_group;
		const string& anim_name = f.ReadString1();
		if(anim_name.empty())
			group.anim = nullptr;
		else
			group.anim = mesh->GetAnimation(anim_name.c_str());
		if(version >= 1)
			f >> group.frame_end;
		else
		{
			if(index == 0)
				group.frame_end = frame_end_info;
			else if(index == 1)
				group.frame_end = frame_end_info2;
			else
				group.frame_end = false;
		}
		++index;
	}

	need_update = true;
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
		f.WriteCasted<byte>(group.used_group);
		if(group.anim)
			f << group.anim->name;
		else
			f.Write0();
		f << group.frame_end;
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
		f.ReadCasted<byte>(group.used_group);
		const string& anim_id = f.ReadString1();
		f >> group.frame_end;
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

	need_update = true;
	return true;
}

//=================================================================================================
bool MeshInstance::ApplyPreload(Mesh* mesh)
{
	assert(mesh
		&& preload
		&& !this->mesh
		&& groups.size() == mesh->head.n_groups);

	preload = false;
	this->mesh = mesh;
	mat_bones.resize(mesh->head.n_bones);
	blendb.resize(mesh->head.n_bones);

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
