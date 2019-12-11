#pragma once
#include "GameObject.h"
#include "GooseObject.h"
namespace NCL {
	namespace CSC8503 {
		class State		{
		public:
			State() {}
			virtual ~State() {}
			virtual void Update() = 0; //Pure virtual base class
		};

		typedef void(*StateFunc)(void*);
		typedef void(*SentryFunc)(GameObject* sentry, GooseObject* player);

		class GenericState : public State		{
		public:
			GenericState(StateFunc someFunc, void* someData) {
				func		= someFunc;
				funcData	= someData;
			}
			virtual void Update() {
				if (funcData != nullptr) {
					func(funcData);
				}
			}
		protected:
			StateFunc func;
			void* funcData;
		};

		class SentryState : public State {
		public:
			SentryState(SentryFunc someFunc, GameObject* sentry, GooseObject* player) {
				func = someFunc;
				this->sentry = sentry;
				this->player = player;
			}

			virtual void Update() {
				if (sentry == nullptr || player == nullptr)
					return;
				func(sentry, player);
			}
		protected:
			SentryFunc func;
			GameObject* sentry;
			GooseObject* player;
		};
	}
}