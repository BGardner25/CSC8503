#pragma once
#include "GameTechRenderer.h"
#include "../CSC8503Common/PhysicsSystem.h"


namespace NCL {
	namespace CSC8503 {
		class TutorialGame		{
		public:
			TutorialGame();
			~TutorialGame();

			virtual void UpdateGame(float dt);

		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys();

			void InitWorld();

			/*
			These are some of the world/object creation functions I created when testing the functionality
			in the module. Feel free to mess around with them to see different objects being created in different
			test scenarios (constraints, collision types, and so on). 
			*/
			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			void BridgeConstraintTest();
			void SimpleGJKTest();

			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();
			void LockedCameraMovement();
			void PlayerMovement(float forceScale = 1.0f);
			void InitMisc();
			void ResetCollectables();
			void ResetGame();

			GameObject* AddFloorToWorld(const Vector3& position, Vector3 dimensions = Vector3(100, 2, 100), 
											string name = "Floor", CollisionType collisionType = CollisionType::FLOOR);
			GameObject* AddWallToWorld(const Vector3& position, Vector3 dimensions = Vector3(100, 2, 100), string name = "Wall");
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f, bool hollow = false);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f, bool collectable = false);
			//IT'S HAPPENING
			GameObject* AddGooseToWorld(const Vector3& position);
			GameObject* AddParkKeeperToWorld(const Vector3& position);
			GameObject* AddCharacterToWorld(const Vector3& position);
			GameObject* AddAppleToWorld(const Vector3& position);
			GameObject* AddLakeToWorld(const Vector3& position, Vector3 dimensions, string name = "Lake");
			GameObject* AddGateToWorld(const Vector3& position, Vector3 dimensions, string name = "Gate");
			GameObject* AddTrampolineToWorld(const Vector3& position, Vector3 dimensions, string name = "Trampoline");
			GameObject* AddPlatformToWorld(const Vector3& position, Vector3 dimensions, string name = "Platform");


			GameTechRenderer*	renderer;
			PhysicsSystem*		physics;
			GameWorld*			world;

			bool useGravity;
			bool useBroadPhase;
			bool inSelectionMode;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;

			OGLMesh*	cubeMesh	= nullptr;
			OGLMesh*	sphereMesh	= nullptr;
			OGLTexture* basicTex	= nullptr;
			OGLShader*	basicShader = nullptr;

			//Coursework Meshes
			OGLMesh*	gooseMesh	= nullptr;
			OGLMesh*	keeperMesh	= nullptr;
			OGLMesh*	appleMesh	= nullptr;
			OGLMesh*	charA		= nullptr;
			OGLMesh*	charB		= nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 14, 20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}

			GameObject* home = nullptr;
			GameObject* goose = nullptr;
			GameObject* sentry = nullptr;
			GameObject* lake = nullptr;
			GameObject* gate = nullptr;
			GameObject* apple[5];
			GameObject* bonusItem[6];
			GameObject* dynamicCube[3];
			GameObject* trampoline[2];

			// fence, gate, trampoline etc
			const Vector3 GOOSE_SPAWN = Vector3(0, 3, -40);
			const Vector3 SENTRY_SPAWN = Vector3(-180, 3, -470);

			int appleCount = 0;
			int applesBanked = 0;
			int bonusCount = 0;
			int bonusBanked = 0;
			int totalScore = 0;

			Vector4 originalColour = Vector4(1, 1, 1, 1);

			std::vector<GameObject*> collectedObjects;

			int timeLeft = 180;
			float timePassed = 0;
			bool canJump = true;
		};
	}
}