#include "common.h"

#include "Font.h"
#include "Pad.h"
#include "Text.h"
#include "main.h"
#include "Timer.h"
#include "DMAudio.h"
#include "FileMgr.h"
#include "Streaming.h"
#include "TxdStore.h"
#include "General.h"
#include "Camera.h"
#include "Vehicle.h"
#include "Bike.h"
#include "PlayerSkin.h"
#include "PlayerInfo.h"
#include "World.h"
#include "Renderer.h"
#include "AnimManager.h"
#include "AnimBlendAssocGroup.h"
#include "AnimViewer.h"
#include "PlayerPed.h"
#include "Pools.h"
#include "References.h"
#include "PathFind.h"
#include "HandlingMgr.h"
#include "TempColModels.h"
#include "Particle.h"
#include "CdStream.h"
#include "Messages.h"
#include "CarCtrl.h"
#include "FileLoader.h"
#include "ModelIndices.h"
#include "Clock.h"
#include "Timecycle.h"
#include "RpAnimBlend.h"
#include "AnimBlendAssociation.h"
#include "Shadows.h"
#include "Radar.h"
#include "Hud.h"
#include "debugmenu.h"

int CAnimViewer::animTxdSlot = 0;
CEntity *CAnimViewer::pTarget = nil;

void
CAnimViewer::Render(void) {
	if (pTarget) {
#ifdef FIX_BUGS
		if(pTarget->IsPed())
			((CPed*)pTarget)->UpdateRpHAnim();
#endif
		// Check if entity has valid RwObject before accessing clump
		if (pTarget->m_rwObject) {
			RpClump *clump = pTarget->GetClump();
			if (clump) {
				pTarget->Render();
				CRenderer::RenderOneNonRoad(pTarget);
			}
		}
	}
}

void
CAnimViewer::Initialise(void) {

	// we need messages, messages needs hud, hud needs those
	int hudSlot = CTxdStore::AddTxdSlot("hud");
	CTxdStore::LoadTxd(hudSlot, "MODELS/HUD.TXD");
	CHud::m_Wants_To_Draw_Hud = false;

	animTxdSlot = CTxdStore::AddTxdSlot("generic");
	CTxdStore::Create(animTxdSlot);
	int particleSlot = CTxdStore::AddTxdSlot("particle");
	CTxdStore::LoadTxd(particleSlot, "MODELS/PARTICLE.TXD");
	CTxdStore::SetCurrentTxd(animTxdSlot);
	CPools::Initialise();
	CReferences::Init();
	TheCamera.Init();
	TheCamera.SetRwCamera(Scene.camera);
	TheCamera.Cams[TheCamera.ActiveCam].Distance = 5.0f;
	ThePaths.Init();
	ThePaths.AllocatePathFindInfoMem(4500);
	CCollision::Init();
	CWorld::Initialise();
	mod_HandlingManager.Initialise();
	CTempColModels::Initialise();
	CAnimManager::Initialise();
	CModelInfo::Initialise();
	CParticle::Initialise();
	CCarCtrl::Init();
	CPedStats::Initialise();
	CMessages::Init();
	CdStreamAddImage("MODELS\\GTA3.IMG");
	CFileLoader::LoadLevel("DATA\\DEFAULT.DAT");
	CFileLoader::LoadLevel("DATA\\ANIMVIEWER.DAT");
	CStreaming::Init();
	for(int i = 0; i < MODELINFOSIZE; i++)
		if(CModelInfo::GetModelInfo(i))
			CModelInfo::GetModelInfo(i)->ConvertAnimFileIndex();
	CStreaming::LoadInitialPeds();
	CStreaming::RequestSpecialModel(MI_PLAYER, "player", STREAMFLAGS_DONT_REMOVE);
	CStreaming::LoadAllRequestedModels(false);
	CRenderer::Init();
	CVehicleModelInfo::LoadVehicleColours();
#ifdef FIX_BUGS
	CVehicleModelInfo::LoadEnvironmentMaps();
#endif
	CAnimManager::LoadAnimFiles();
	CWorld::PlayerInFocus = 0;
	CWeapon::InitialiseWeapons();
	CPed::Initialise();
	CTimer::Initialise();
	CClock::Initialise(60000);
	CTimeCycle::Initialise();
	CCarCtrl::Init();
	CPlayerPed *player = new CPlayerPed();
	player->SetPosition(1000.0f, 1000.0f, 1000.0f);
	CWorld::Players[0].m_pPed = player;
	CDraw::SetFOV(120.0f);
	CDraw::ms_fLODDistance = 500.0f;

	int fd = CFileMgr::OpenFile("DATA\\SPECIAL.TXT", "r");
	char animGroup[32], modelName[32];
	if (fd) {
		for (int lineId = 0; lineId < NUM_OF_SPECIAL_CHARS; lineId++) {
			if (!CFileMgr::ReadLine(fd, gString, 255))
				break;

			sscanf(gString, "%s %s", modelName, animGroup);
			int groupId;
			for (groupId = 0; groupId < NUM_ANIM_ASSOC_GROUPS; groupId++) {
				if (!strcmp(animGroup, CAnimManager::GetAnimGroupName((AssocGroupId)groupId)))
					break;
			}

			if (groupId != NUM_ANIM_ASSOC_GROUPS)
				((CPedModelInfo*)CModelInfo::GetModelInfo(MI_SPECIAL01 + lineId))->m_animGroup = groupId;

			CStreaming::RequestSpecialChar(lineId, modelName, STREAMFLAGS_DONT_REMOVE);
		}
		CFileMgr::CloseFile(fd);
	} else {
		// TODO? maybe request some special models here so the thing doesn't crash
	}

	// From LCS. idk if needed
	int vanBlock = CAnimManager::GetAnimationBlockIndex("van");
	int bikesBlock = CAnimManager::GetAnimationBlockIndex("bikes");
	int bikevBlock = CAnimManager::GetAnimationBlockIndex("bikev");
	int bikehBlock = CAnimManager::GetAnimationBlockIndex("bikeh");
	int bikedBlock = CAnimManager::GetAnimationBlockIndex("biked");
	CStreaming::FlushRequestList();
	CStreaming::RequestAnim(vanBlock, STREAMFLAGS_DEPENDENCY);
	CStreaming::RequestAnim(bikesBlock, STREAMFLAGS_DEPENDENCY);
	CStreaming::RequestAnim(bikevBlock, STREAMFLAGS_DEPENDENCY);
	CStreaming::RequestAnim(bikehBlock, STREAMFLAGS_DEPENDENCY);
	CStreaming::RequestAnim(bikedBlock, STREAMFLAGS_DEPENDENCY);
	CStreaming::LoadAllRequestedModels(false);
	CAnimManager::AddAnimBlockRef(vanBlock);
	CAnimManager::AddAnimBlockRef(bikesBlock);
	CAnimManager::AddAnimBlockRef(bikevBlock);
	CAnimManager::AddAnimBlockRef(bikehBlock);
	CAnimManager::AddAnimBlockRef(bikedBlock);
}

int
LastPedModelId(int modelId)
{
	CBaseModelInfo *model;
	for(;;){
		assert(modelId < MODELINFOSIZE);
		model = CModelInfo::GetModelInfo(modelId);
		if (model && model->GetModelType() == MITYPE_PED)
			break;
		modelId--;
	}
	return modelId;
}

int
FirstCarModelId(int modelId)
{
	CBaseModelInfo *model;
	for(;;){
		assert(modelId < MODELINFOSIZE);
		model = CModelInfo::GetModelInfo(modelId);
		if (model && model->GetModelType() == MITYPE_VEHICLE)
			break;
		modelId++;
	}
	return modelId;
}


int
NextModelId(int modelId, int wantedChange)
{
	// Max. 2 trials wasn't here, it's me that added it.

	int tryCount = 2;
	int ogModelId = modelId;

	while(tryCount != 0) {
		modelId += wantedChange;
		if (modelId < 0 || modelId >= MODELINFOSIZE) {
			tryCount--;
			wantedChange = -wantedChange;
		} else if (modelId != 5 && modelId != 6 && modelId != 405) {
			CBaseModelInfo *model = CModelInfo::GetModelInfo(modelId);
			if (model)
			{
				//int type = model->m_type;
				return modelId;
			}
		}
	}
	return ogModelId;
}

void
PlayAnimation(RpClump *clump, AssocGroupId animGroup, AnimationId anim)
{
	CAnimBlendAssociation *currentAssoc = RpAnimBlendClumpGetAssociation(clump, anim);

	if (currentAssoc && currentAssoc->IsPartial())
		delete currentAssoc;

	RpAnimBlendClumpSetBlendDeltas(clump, ASSOC_PARTIAL, -8.0f);

	CAnimBlendAssociation *animAssoc = CAnimManager::BlendAnimation(clump, animGroup, anim, 8.0f);
	animAssoc->flags |= ASSOC_DELETEFADEDOUT;
	animAssoc->SetCurrentTime(0.0f);
	animAssoc->SetRun();
}

void
CAnimViewer::Update(void)
{
	static int modelId = 0;
	static int animId = 0;
	static bool reloadIFP = false;

	AssocGroupId animGroup = ASSOCGRP_STD;
	int nextModelId = modelId;
	CBaseModelInfo *modelInfo = CModelInfo::GetModelInfo(modelId);

	if (modelInfo->GetModelType() == MITYPE_PED) {
		int animGroup = ((CPedModelInfo*)modelInfo)->m_animGroup;

		if (animId > ANIM_STD_IDLE)
			animGroup = ASSOCGRP_STD;

		if (reloadIFP) {
			if (pTarget) {
				CWorld::Remove(pTarget);
				if (pTarget)
					delete pTarget;
			}
			pTarget = nil;
			
			// These calls were inside of LoadIFP function.
			CAnimManager::Shutdown();
			CAnimManager::Initialise();
			CAnimManager::LoadAnimFiles();

			reloadIFP = false;
		}
	} else {
		animGroup = ASSOCGRP_STD;
	}
	CPad::UpdatePads();
	CPad* pad = CPad::GetPad(0);
#ifdef DEBUGMENU
	DebugMenuProcess();
#endif

	CStreaming::UpdateForAnimViewer();
	CStreaming::RequestModel(modelId, 0);
	if (CStreaming::HasModelLoaded(modelId)) {

		if (!pTarget) {

			// Verify model has valid RwObject before creating entity
			if (!CModelInfo::GetModelInfo(modelId)->GetRwObject()) {
				printf("[AnimViewer] Model %d has no RwObject, skipping to next\n", modelId);
				// Auto-skip to next valid model
				nextModelId = NextModelId(modelId, 1);
				modelId = nextModelId;
				return;
			}

			if (modelInfo->GetModelType() == MITYPE_VEHICLE) {

				CVehicleModelInfo* veh = (CVehicleModelInfo*)modelInfo;
				if (veh->m_vehicleType == VEHICLE_TYPE_CAR) {
					pTarget = new CAutomobile(modelId, RANDOM_VEHICLE);
				} else if (veh->m_vehicleType == VEHICLE_TYPE_BOAT) {
					pTarget = new CBoat(modelId, RANDOM_VEHICLE);
				} else if (veh->m_vehicleType == VEHICLE_TYPE_BIKE) {
					pTarget = new CBike(modelId, RANDOM_VEHICLE);
				} else {
					pTarget = new CObject(modelId, true);
					if (!modelInfo->GetColModel()) {
						modelInfo->SetColModel(&CTempColModels::ms_colModelWheel1);
					}
				}
				pTarget->SetStatus(STATUS_ABANDONED);
			} else if (modelInfo->GetModelType() == MITYPE_PED) {
				pTarget = new CPed(PEDTYPE_CIVMALE);
				pTarget->SetModelIndex(modelId);
			} else {
				pTarget = new CObject(modelId, true);
				if (!modelInfo->GetColModel())
				{
					modelInfo->SetColModel(&CTempColModels::ms_colModelWheel1);
				}
				pTarget->SetStatus(STATUS_ABANDONED);
			}
			
			// Final check: verify the created entity has a valid clump
			if (!pTarget->GetClump()) {
				printf("[AnimViewer] Created entity for model %d has no clump, deleting and skipping\n", modelId);
				delete pTarget;
				pTarget = nil;
				nextModelId = NextModelId(modelId, 1);
				modelId = nextModelId;
				return;
			}
			
			pTarget->SetPosition(0.0f, 0.0f, 0.0f);
			CWorld::Add(pTarget);
			TheCamera.TakeControl(pTarget, CCam::MODE_MODELVIEW, JUMP_CUT, CAMCONTROL_SCRIPT);
		}
		// Verify the clump is still valid after creation
		// Must check m_rwObject before calling GetClump() to avoid crash
		if (!pTarget->m_rwObject) {
			printf("[AnimViewer] Model %d lost its RwObject after creation, cleaning up\n", modelId);
			CWorld::Remove(pTarget);
			if (pTarget)
				delete pTarget;
			pTarget = nil;
			nextModelId = NextModelId(modelId, 1);
			modelId = nextModelId;
			return;
		}
		
		RpClump *targetClump = pTarget->GetClump();
		if (!targetClump) {
			printf("[AnimViewer] Model %d has invalid clump, cleaning up\n", modelId);
			CWorld::Remove(pTarget);
			if (pTarget)
				delete pTarget;
			pTarget = nil;
			nextModelId = NextModelId(modelId, 1);
			modelId = nextModelId;
			return;
		}

		if (pTarget->IsVehicle() || pTarget->IsPed() || pTarget->IsObject()) {
			((CPhysical*)pTarget)->m_vecMoveSpeed = CVector(0.0f, 0.0f, 0.0f);
		}
#ifdef FIX_BUGS
		// so we don't end up in the water
		pTarget->GetMatrix().GetPosition().z = 10.0f;
#else
		pTarget->GetMatrix().GetPosition().z = 0.0f;
#endif

		if (modelInfo->GetModelType() == MITYPE_PED) {
			((CPed*)pTarget)->bKindaStayInSamePlace = true;

			// Triangle in mobile
			if (pad->GetSquareJustDown()) {
				reloadIFP = true;
				AsciiToUnicode("IFP reloaded", gUString);
				CMessages::AddMessage(gUString, 1000, 0);

			} else if (pad->GetCrossJustDown()) {
				PlayAnimation(targetClump, animGroup, (AnimationId)animId);
				AsciiToUnicode("Animation restarted", gUString);
				CMessages::AddMessage(gUString, 1000, 0);

			} else if (pad->GetCircleJustDown()) {
				PlayAnimation(targetClump, animGroup, ANIM_STD_IDLE);
				AsciiToUnicode("Idle animation playing", gUString);
				CMessages::AddMessage(gUString, 1000, 0);

			} else if (pad->GetDPadUpJustDown()) {
				animId--;
				if (animId < 0) {
					animId = ANIM_STD_NUM - 1;
				}
				PlayAnimation(targetClump, animGroup, (AnimationId)animId);

				sprintf(gString, "Current anim: %d", animId);
				AsciiToUnicode(gString, gUString);
				CMessages::AddMessage(gUString, 1000, 0);

			} else if (pad->GetDPadDownJustDown()) {
				animId = (animId == (ANIM_STD_NUM - 1) ? 0 : animId + 1);
				PlayAnimation(targetClump, animGroup, (AnimationId)animId);

				sprintf(gString, "Current anim: %d", animId);
				AsciiToUnicode(gString, gUString);
				CMessages::AddMessage(gUString, 1000, 0);

			} else if (pad->GetStartJustDown()) {

			} else if (pad->GetLeftShoulder1JustDown()) {
				nextModelId = FirstCarModelId(modelId);
				AsciiToUnicode("Switched to vehicles", gUString);
				CMessages::AddMessage(gUString, 1000, 0);
				// Originally it was GetPad(1)->LeftShoulder2
			} else if (pad->NewState.Triangle) {
				((CPedModelInfo *)CModelInfo::GetModelInfo(pTarget->GetModelIndex()))->AnimatePedColModelSkinned(targetClump);
				AsciiToUnicode("Ped Col model will be animated as long as you hold the button", gUString);
				CMessages::AddMessage(gUString, 100, 0);
			}

			// From LCS
			if (CAnimManager::GetAnimAssocGroups()[animGroup].numAssociations <= animId)
				animId = 0;

		} else if (modelInfo->GetModelType() == MITYPE_VEHICLE) {

			if (pad->GetLeftShoulder1JustDown()) {
				nextModelId = LastPedModelId(modelId);
				AsciiToUnicode("Switched to peds", gUString);
				CMessages::AddMessage(gUString, 1000, 0);
				// Start in mobile
			} else if (pad->GetSquareJustDown()) {
				CVehicleModelInfo::LoadVehicleColours();
				AsciiToUnicode("Carcols.dat reloaded", gUString);
				CMessages::AddMessage(gUString, 1000, 0);
			}
		}
	}

	// Model navigation with keyboard modifiers
	// Q+E: jump 100 models, E only: jump 10 models, None: jump 1 model
	int jumpAmount = 1;
	bool isEPressed = pad->NewKeyState.VK_KEYS['E'];
	bool isQPressed = pad->NewKeyState.VK_KEYS['Q'];
	
	if (isQPressed && isEPressed) {
		jumpAmount = 100;
	} else if (isEPressed) {
		jumpAmount = 10;
	}

	if (pad->GetDPadLeftJustDown()) {
		// Jump backwards by jumpAmount valid models
		int stepsRemaining = jumpAmount;
		nextModelId = modelId;
		while (stepsRemaining > 0) {
			nextModelId = NextModelId(nextModelId, -1);
			stepsRemaining--;
		}

		if (jumpAmount > 1) {
			sprintf(gString, "Current model ID: %d (-%d)", nextModelId, jumpAmount);
		} else {
			sprintf(gString, "Current model ID: %d", nextModelId);
		}
		AsciiToUnicode(gString, gUString);
		CMessages::AddMessage(gUString, 1000, 0);

	} else if (pad->GetDPadRightJustDown()) {
		// Jump forwards by jumpAmount valid models
		int stepsRemaining = jumpAmount;
		nextModelId = modelId;
		while (stepsRemaining > 0) {
			nextModelId = NextModelId(nextModelId, 1);
			stepsRemaining--;
		}

		if (jumpAmount > 1) {
			sprintf(gString, "Current model ID: %d (+%d)", nextModelId, jumpAmount);
		} else {
			sprintf(gString, "Current model ID: %d", nextModelId);
		}
		AsciiToUnicode(gString, gUString);
		CMessages::AddMessage(gUString, 1000, 0);
	}
	// There were extra codes here to let us change model id by 50, but xbox CPad struct is different, so I couldn't port.

	if (nextModelId != modelId) {
		modelId = nextModelId;
		if (pTarget) {
			CWorld::Remove(pTarget);
			if (pTarget)
				delete pTarget;
		}
		pTarget = nil;
		return;
	}

	CTimeCycle::Update();
	CWorld::Process();
	if (pTarget)
		TheCamera.Process();
}

void
CAnimViewer::Shutdown(void)
{
	if (CWorld::Players[0].m_pPed)
		delete CWorld::Players[0].m_pPed;

	CWorld::ShutDown();
	CModelInfo::ShutDown();
	CAnimManager::Shutdown();
	CTimer::Shutdown();
	CStreaming::Shutdown();
	CTxdStore::RemoveTxdSlot(animTxdSlot);
}
