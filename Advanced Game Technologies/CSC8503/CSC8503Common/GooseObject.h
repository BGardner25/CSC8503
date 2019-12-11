#pragma once
#include"GameObject.h"

namespace NCL {
	namespace CSC8503 {

		class NetworkObject;

		class GooseObject : public GameObject {
		public:
			GooseObject(string name = "") : GameObject(name) {}
			virtual ~GooseObject() {}

			virtual void OnCollisionBegin(GameObject* otherObject) {
				//std::cout << "OnCollisionBegin event occured!\n";
			}

			virtual void OnCollisionEnd(GameObject* otherObject) {
				//std::cout << "OnCollisionEnd event occured!\n";
			}

			void SetHasBonusItem(bool hasBonusItem) { this->hasBonusItem = hasBonusItem; }
			bool HasBonusItem() const { return hasBonusItem; }
		protected:
			bool hasBonusItem = false;
		};
	}
}


