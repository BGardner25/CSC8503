#pragma once
#include "../CSC8503Common/GameWorld.h"
#include <set>

namespace NCL {
	namespace CSC8503 {
		class PhysicsSystem	{
		public:
			PhysicsSystem(GameWorld& g);
			~PhysicsSystem();

			void Clear();

			void Update(float dt);

			void UseGravity(bool state) {
				applyGravity = state;
			}

			void UseBroadPhase(bool state) {
				useBroadPhase = state;
			}

			void SetGlobalDamping(float d) {
				globalDamping = d;
			}

			void SetGravity(const Vector3& g);
			// @TODO TEMPORARY
			int physAppleCount = 0;
		protected:
			void BasicCollisionDetection();
			void BroadPhase();
			void NarrowPhase();

			void ClearForces();

			void IntegrateAccel(float dt);
			void IntegrateVelocity(float dt);

			void UpdateConstraints(float dt);

			void UpdateCollisionList();
			void UpdateObjectAABBs();

			void ImpulseResolveCollision(GameObject& a , GameObject&b, CollisionDetection::ContactPoint& p) const;
			void CollectableCollision(GameObject& collectableObject);
			void TrampolineCollision(GameObject& playerObject, GameObject& trampoline, CollisionDetection::ContactPoint& p);
			void LakeCollision(GameObject& playerObject);
			// moving cube collision

			GameWorld& gameWorld;

			bool	applyGravity;
			Vector3 gravity;
			float	dTOffset;
			float	globalDamping;
			float	frameDT;

			std::set<CollisionDetection::CollisionInfo> allCollisions;
			std::set<CollisionDetection::CollisionInfo>		broadphaseCollisions;
			std::vector<CollisionDetection::CollisionInfo>	broadphaseCollisionsVec;
			bool useBroadPhase		= true;
			int numCollisionFrames	= 5;
		};
	}
}

