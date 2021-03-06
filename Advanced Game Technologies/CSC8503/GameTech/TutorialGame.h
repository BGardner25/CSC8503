#pragma once
#include "GameTechRenderer.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/State.h"
#include "../CSC8503Common//GooseObject.h"
#include "../CSC8503Common/NavigationGrid.h"
#include <sstream>
#include <iomanip>

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
			void PlayerMovement();
			void InitMisc();
			void ResetGame();
			void UpdateMovingBlocks();
			void SentryStateMachine();
			void Pathfinding();
			//void ParkKeeperStateMachine();

			GameObject* AddFloorToWorld(const Vector3& position, Vector3 dimensions = Vector3(100, 2, 100), 
											string name = "Floor", CollisionType collisionType = CollisionType::FLOOR);
			GameObject* AddOBBFloorToWorld(const Vector3& position, Vector3 dimensions = Vector3(100, 2, 100),
				string name = "Floor", CollisionType collisionType = CollisionType::FLOOR);
			GameObject* AddWallToWorld(const Vector3& position, Vector3 dimensions = Vector3(100, 2, 100), string name = "Wall");
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f, bool hollow = false);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f, bool collectable = false);
			GameObject* AddDynamicCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);
			//IT'S HAPPENING
			GameObject* AddGooseToWorld(const Vector3& position);
			GameObject* AddParkKeeperToWorld(const Vector3& position);
			GameObject* AddCharacterToWorld(const Vector3& position);
			GameObject* AddAppleToWorld(const Vector3& position);
			GameObject* AddLakeToWorld(const Vector3& position, Vector3 dimensions, string name = "Lake");
			GameObject* AddGateToWorld(const Vector3& position, Vector3 dimensions, Quaternion orientation, string name = "Gate");
			GameObject* AddTrampolineToWorld(const Vector3& position, Vector3 dimensions, string name = "Trampoline");
			GameObject* AddPlatformToWorld(const Vector3& position, Vector3 dimensions, string name = "Platform");
			GameObject* AddSpinnerToWorld(const Vector3& position, Vector3 dimensions, string name = "Spinner");


			GameTechRenderer*	renderer;
			PhysicsSystem*		physics;
			GameWorld*			world;

			bool useGravity;
			bool useBroadPhase;
			bool inSelectionMode;
			bool enablePathfinding;

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
			GooseObject* goose = nullptr;
			GameObject* sentry = nullptr;
			GameObject* lake = nullptr;
			GameObject* gate = nullptr; 
			GameObject* parkKeeper = nullptr;
			GameObject* spinner[6];
			GameObject* apple[5];
			GameObject* bonusItem[6];
			GameObject* dynamicCube[3];
			GameObject* trampoline[2];

			const Vector3 GOOSE_SPAWN = Vector3(0, 3, -40);
			const Vector3 SENTRY_SPAWN = Vector3(-180, 3, -470);
			const Vector3 PARK_KEEPER_SPAWN = Vector3(100, 3, -250);

			int appleCount = 0;
			int applesBanked = 0;
			int bonusCount = 0;
			int bonusBanked = 0;
			int totalScore = 0;

			const int appleValue = 1;
			const int bonusValue = 5;
			const int maxValue = (5 * appleValue) + (6 * bonusValue);

			Vector4 originalColour = Vector4(1, 1, 1, 1);

			std::vector<GameObject*> collectedApples;
			std::vector<GameObject*> collectedBonus;

			int timeLeft = 180;
			float timePassed = 0;
			float cubeDirection[3] = { 1.0f, 1.0f, 1.0f };

			StateMachine* stateMachine;

			float sentryToGoose = 0;

			bool canJump = true;
			bool displayObjectInfo = false;

			vector<Vector3> pathNodes;

			float updatePath = 0.51f;

			Vector3 previousPos;
			Vector3 pos;
			Vector3 pathDirectionVec;
		};
	}
}