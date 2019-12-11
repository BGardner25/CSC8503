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
				//std::cout << "OnCollisionBegin event occured!\n";
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

			void SetStateDescription(string description) { stateDescription = description; }
			string GetStateDescription() const { return stateDescription; }

		protected:
			Transform			transform;

			CollisionVolume*	boundingVolume;
			PhysicsObject*		physicsObject;
			RenderObject*		renderObject;
			NetworkObject*		networkObject;

			bool	isActive;
			bool	isCollectable;
			bool	collected;
			string	name;
			string	stateDescription;

			Vector3 broadphaseAABB;

			CollisionType collisionType;
			//Layer layer;
		};
	}
}

