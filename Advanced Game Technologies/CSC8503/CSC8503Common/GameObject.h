#pragma once
#include "Transform.h"
#include "CollisionVolume.h"

#include "PhysicsObject.h"
#include "RenderObject.h"
#include "NetworkObject.h"

#include <vector>

using std::vector;

namespace NCL {
	namespace CSC8503 {
		// for layer based collision detection
		/*enum class Layer {
			NONE, ONE, TWO, THREE
		};*/

		class NetworkObject;

		class GameObject	{
		public:
			GameObject(string name = "");
			virtual ~GameObject();

			void SetBoundingVolume(CollisionVolume* vol) {
				boundingVolume = vol;
			}

			const CollisionVolume* GetBoundingVolume() const {
				return boundingVolume;
			}

			bool IsActive() const {
				return isActive;
			}

			// @TODO make collectableObject class...
			void SetCollectable(bool isCollectable) { this->isCollectable = isCollectable; }
			bool IsCollectable() const { return isCollectable; }

			void SetCollected(bool collected) { this->collected = collected; }
			bool IsCollected() const { return collected; }

			const Transform& GetConstTransform() const {
				return transform;
			}

			Transform& GetTransform() {
				return transform;
			}

			RenderObject* GetRenderObject() const {
				return renderObject;
			}

			PhysicsObject* GetPhysicsObject() const {
				return physicsObject;
			}

			NetworkObject* GetNetworkObject() const {
				return networkObject;
			}

			void SetRenderObject(RenderObject* newObject) {
				renderObject = newObject;
			}

			void SetPhysicsObject(PhysicsObject* newObject) {
				physicsObject = newObject;
			}

			const string& GetName() const {
				return name;
			}

			virtual void OnCollisionBegin(GameObject* otherObject) {
			}

			virtual void OnCollisionEnd(GameObject* otherObject) {
				//std::cout << "OnCollisionEnd event occured!\n";
			}

			bool GetBroadphaseAABB(Vector3&outsize) const;

			void UpdateBroadphaseAABB();

			void SetCollidedWith(CollisionType collisionType) { this->collisionType = collisionType; }
			CollisionType HasCollidedWith() { return collisionType; }

			/*void SetLayer(Layer layer) { this->layer = layer; }
			Layer GetLayer() const { return layer; }*/

			// @TODO make AIObject class
			void SetStateDescription(string description) { stateDescription = description; }
			string GetStateDescription() const { return stateDescription; }

			void SetSpawnPos(const Vector3& pos) { spawnPos = pos; }
			Vector3 GetSpawnPos() const { return spawnPos; }

		protected:
			Transform			transform;

			CollisionVolume*	boundingVolume;
			PhysicsObject*		physicsObject;
			RenderObject*		renderObject;
			NetworkObject*		networkObject;

			bool	isActive;

			// @TODO put in collectableObj class
			bool	isCollectable;
			bool	collected;

			string	name;

			// @TODO put in AIObj class
			string	stateDescription;

			Vector3 broadphaseAABB;

			CollisionType collisionType;

			Vector3 spawnPos;
			//Layer layer;
		};
	}
}

