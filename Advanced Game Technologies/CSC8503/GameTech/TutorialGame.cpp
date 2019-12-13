#include "TutorialGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"

#include "../CSC8503Common/PositionConstraint.h"

using namespace NCL;
using namespace CSC8503;

TutorialGame::TutorialGame()	{
	world		= new GameWorld();
	renderer	= new GameTechRenderer(*world);
	physics		= new PhysicsSystem(*world);
	stateMachine = new StateMachine();

	forceMagnitude	= 10.0f;
	useGravity		= false;
	useBroadPhase	= true;
	inSelectionMode = true;
	enablePathfinding = false;

	Debug::SetRenderer(renderer);
	
	InitialiseAssets();
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh"		 , &cubeMesh);
	loadFunc("sphere.msh"	 , &sphereMesh);
	loadFunc("CenteredGoose.msh", &gooseMesh);
	loadFunc("CharacterA.msh", &keeperMesh);
	loadFunc("CharacterM.msh", &charA);
	loadFunc("CharacterF.msh", &charB);
	loadFunc("Apple.msh"	 , &appleMesh);

	basicTex	= (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	InitCamera();
	InitWorld();
	InitMisc();
}

TutorialGame::~TutorialGame()	{
	delete cubeMesh;
	delete sphereMesh;
	delete gooseMesh;
	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;
	delete stateMachine;

	delete home;
	delete goose;
	delete sentry;
	delete lake;
	delete gate;
	delete parkKeeper;
	delete[] spinner;
	delete[] apple;
	delete[] bonusItem;
	delete[] dynamicCube;
	delete[] trampoline;
}

void TutorialGame::UpdateGame(float dt) {
	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}
	if (lockedObject != nullptr) {
		LockedCameraMovement();
	}
	
	UpdateKeys();

	/*if (useGravity) {
		Debug::Print("(G)ravity on", Vector2(10, 40));
	}
	else {
		Debug::Print("(G)ravity off", Vector2(10, 40));
	}
	if (useBroadPhase) {
		Debug::Print("Broadphase on", Vector2(20, 60));
	}
	else {
		Debug::Print("Broadphase off", Vector2(20, 60));
	}*/

	renderer->DrawString("Apples:" + std::to_string(appleCount), Vector2(10, 42));
	renderer->DrawString("Bonus Items:" + std::to_string(bonusCount), Vector2(10, 21));
	renderer->DrawString("Total Score:" + std::to_string(totalScore), Vector2(10, 0));
	renderer->DrawString("Time Left:" + std::to_string(timeLeft), Vector2(0, 63));
	renderer->DrawString("Obj Pos: ", Vector2(0, renderer->GetHeight() - 20));
	renderer->DrawString("Obj Orientation: ", Vector2(0, renderer->GetHeight() - 40));
	renderer->DrawString("Obj AI State: ", Vector2(0, renderer->GetHeight() - 60));

	SelectObject();
	//MoveSelectedObject();

	goose->HasCollidedWith() == CollisionType::LAKE ? goose->GetPhysicsObject()->SetInverseMass(0.35f) : goose->GetPhysicsObject()->SetInverseMass(1.0f);
	PlayerMovement();

	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);
	stateMachine->Update();

	updatePath += dt;
	
	if (enablePathfinding)
		Pathfinding();
		
	sentryToGoose = (goose->GetTransform().GetWorldPosition() - sentry->GetTransform().GetWorldPosition()).Length();

	timePassed += dt;
	if (timePassed >= 1.0f && timeLeft > 0) {
		timeLeft--;
		timePassed = 0;
	}
	if (timeLeft <= 0 || totalScore >= 35) {
		// @TODO menu to ask if quit or play again
		ResetGame();
	}

	for (GameObject* i : apple) {
		if (i->IsCollected()) {
			appleCount++;
			collectedApples.emplace_back(i);
			i->SetCollected(false);
		}
	}
	for (GameObject* i : bonusItem) {
		if (i->IsCollected()) {
			bonusCount++;
			collectedBonus.emplace_back(i);
			goose->SetHasBonusItem(true);
			i->SetCollected(false);
		}
	}
	if (goose->HasCollidedWith() == CollisionType::HOME && (appleCount > 0 || bonusCount > 0)) {
		totalScore += appleCount;
		totalScore += bonusCount * bonusValue;
		appleCount = bonusCount = 0;
		goose->SetHasBonusItem(false);
		sentry->GetTransform().SetWorldPosition(SENTRY_SPAWN);
		parkKeeper->GetTransform().SetWorldPosition(PARK_KEEPER_SPAWN);
		collectedApples.clear();
		collectedBonus.clear();
	}
	if (goose->HasCollidedWith() == CollisionType::TRAMPOLINE) {
		goose->SetCollidedWith(CollisionType::DEFAULT);
		goose->GetPhysicsObject()->AddForce(Vector3(0.0f, 10000.0f, 0.0f));
	}
	if (goose->HasCollidedWith() == CollisionType::AI) {
		goose->SetCollidedWith(CollisionType::DEFAULT);
		goose->SetHasBonusItem(false);
		for (GameObject* i : collectedBonus)
			i->GetTransform().SetWorldPosition(i->GetSpawnPos());
		collectedBonus.clear();
		bonusCount = 0;
	}
	// display selected object position, orientation and AI state... if any
	if (displayObjectInfo) {
		std::ostringstream s;
		s << selectionObject->GetTransform().GetWorldPosition();
		renderer->DrawString(s.str(), Vector2(250, renderer->GetHeight() - 20));
		// empty s
		s.str(string());
		s << selectionObject->GetTransform().GetWorldOrientation();
		renderer->DrawString(s.str(), Vector2(480, renderer->GetHeight() - 40));
		renderer->DrawString(selectionObject->GetStateDescription(), Vector2(390, renderer->GetHeight() - 60));
	}

	UpdateMovingBlocks();
	for(GameObject* i : spinner)
		i->GetPhysicsObject()->AddTorque(Vector3(0.0, 150000.0, 0.0));

	// @TODO spawn new keeper every 30 secs

	Debug::FlushRenderables();
	renderer->Render();
}

void TutorialGame::UpdateMovingBlocks() {
	// move blocks. if they hit a wall, move in the other direction
	if (dynamicCube[0]->HasCollidedWith() == CollisionType::WALL) {
		cubeDirection[0] *= -1.0f;
		dynamicCube[0]->SetCollidedWith(CollisionType::DEFAULT);
	}
	dynamicCube[0]->GetPhysicsObject()->AddForce(Vector3(-100000.0f * cubeDirection[0], 0.0f, 0.0f));

	for (int i = 1; i < 3; ++i) {
		if (dynamicCube[i]->HasCollidedWith() == CollisionType::WALL) {
			cubeDirection[i] *= -1.0f;
			dynamicCube[i]->SetCollidedWith(CollisionType::DEFAULT);
		}
		dynamicCube[i]->GetPhysicsObject()->AddForce(Vector3(0.0f, 0.0f, 100000.0f * cubeDirection[i]));
	}
}

void TutorialGame::ResetGame() {
	selectionObject = nullptr;
	delete stateMachine;
	stateMachine = new StateMachine();
	enablePathfinding = false;
	InitWorld();
	InitMisc();
}

void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		ResetGame();
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}
	/*if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::B)) {
		useBroadPhase = !useBroadPhase;
		physics->UseBroadPhase(useBroadPhase);
	}*/
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::P)) {
		enablePathfinding = !enablePathfinding;
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}

	if (lockedObject) {
		LockedObjectMovement();
	}
	else {
		DebugObjectMovement();
	}
}

void TutorialGame::LockedObjectMovement() {
	Matrix4 view		= world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld	= view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);

	float scale = 100.0f;

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		selectionObject->GetPhysicsObject()->AddForce(-rightAxis * scale);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		selectionObject->GetPhysicsObject()->AddForce(rightAxis * scale);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		selectionObject->GetPhysicsObject()->AddForce(fwdAxis * scale);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		selectionObject->GetPhysicsObject()->AddForce(-fwdAxis * scale);
	}
}

void  TutorialGame::LockedCameraMovement() {
	if (lockedObject != nullptr) {
		Vector3 objPos = lockedObject->GetTransform().GetWorldPosition();
		Vector3 camPos = objPos + lockedOffset;

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0, 1, 0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); //nearly there now!

		world->GetMainCamera()->SetPosition(camPos + Vector3(0,10,10));
		world->GetMainCamera()->SetPitch(angles.x);
		world->GetMainCamera()->SetYaw(angles.y);
	}
}


void TutorialGame::DebugObjectMovement() {
//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}
}

void TutorialGame::PlayerMovement() {
	if (inSelectionMode) {
		float speed = 150.0f;
		/*Quaternion orientation;
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::W)) {
			orientation = Quaternion(Vector3(0, 1, 0), 0.0f);
			orientation.Normalise();
			goose->GetPhysicsObject()->AddForce(Vector3(0, 0, -speed));
			goose->GetTransform().SetLocalOrientation(orientation);
		}
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::A)) {
			orientation = Quaternion(Vector3(0, 1, 0), -1.0f);
			orientation.Normalise();
			goose->GetPhysicsObject()->AddForce(Vector3(-speed, 0, 0));
			goose->GetTransform().SetLocalOrientation(orientation);
		}
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::S)) {
			orientation = Quaternion(Vector3(0, 1, 0), 180.0f);
			orientation.Normalise();
			goose->GetPhysicsObject()->AddForce(Vector3(0, 0, speed));
			goose->GetTransform().SetLocalOrientation(orientation);
		}
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::D)) {
			orientation = Quaternion(Vector3(0, 1, 0), 1.0f);
			orientation.Normalise();
			goose->GetPhysicsObject()->AddForce(Vector3(speed, 0, 0));
			goose->GetTransform().SetLocalOrientation(orientation);
		}*/

		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::W)) {
			goose->GetPhysicsObject()->AddForce(Vector3(0, 0, -speed));
		}
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::A)) {
			goose->GetPhysicsObject()->AddForce(Vector3(-speed, 0, 0));
		}
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::S)) {
			goose->GetPhysicsObject()->AddForce(Vector3(0, 0, speed));
		}
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::D)) {
			goose->GetPhysicsObject()->AddForce(Vector3(speed, 0, 0));
		}

		// direction goose is facing
		Vector3 directionVec =  goose->GetTransform().GetWorldPosition() - goose->GetPhysicsObject()->GetAngularVelocity();
		directionVec.Normalise();
		float gooseAngle = atan2(directionVec.x, directionVec.z);
		Quaternion orientation = Quaternion(0.0f, sin(gooseAngle * 0.5f), 0.0f, cos(gooseAngle * 0.5f));
		orientation.Normalise();
		goose->GetTransform().SetLocalOrientation(orientation);

		// can only jump if on the floor
		if ((goose->HasCollidedWith() == CollisionType::FLOOR || goose->HasCollidedWith() == CollisionType::HOME ||
			goose->HasCollidedWith() == CollisionType::LAKE))
			canJump = true;
		else
			canJump = false;
		if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::SPACE) && canJump == true) {
			goose->GetPhysicsObject()->AddForce(Vector3(0, 100.0f, 0) * 20);
			canJump = false;
			goose->SetCollidedWith(CollisionType::NONE);
		}
	}
}

/*

Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around.

*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		//renderer->DrawString("Press Q to change to camera mode!", Vector2(10, 0));
		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(originalColour);
				selectionObject = nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;
				originalColour = selectionObject->GetRenderObject()->GetColour();
				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
				displayObjectInfo = true;
				return true;
			}
			else {
				return false;
			}
		}
		if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
			if (goose) {
				if (lockedObject == goose) {
					lockedObject = nullptr;
				}
				else {
					lockedObject = goose;
				}
			}
		}
	}
	else {
		//renderer->DrawString("Press Q to change to select mode!", Vector2(10, 0));
	}
	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/

void TutorialGame::MoveSelectedObject() {
	// draw text at 10, 20
	renderer->DrawString("Click Force: " + std::to_string(forceMagnitude), Vector2(10, 20));
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject)
		return;	// nothing has been selected

	// push the selected object
	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true))
			if (closestCollision.node == selectionObject)
				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
	}
}

void TutorialGame::InitMisc() {
	// lock camera to goose and allow goose movement
	selectionObject = goose;
	lockedObject = goose;

	if (inSelectionMode) {
		Window::GetWindow()->ShowOSPointer(true);
		Window::GetWindow()->LockMouseToWindow(false);
	}
}

void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.5f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
}

void TutorialGame::SentryStateMachine() {	

	SentryFunc idleFunc = [](GameObject* sentry, GooseObject* goose) {
		sentry->SetStateDescription("idle");
		Vector3 directionVec = goose->GetTransform().GetWorldPosition() - sentry->GetTransform().GetWorldPosition();
		//http://lolengine.net/blog/2013/09/18/beautiful-maths-quaternion-from-vectors
		float gooseAngle = atan2(directionVec.x, directionVec.z);
		// only rotate around y axis
		Quaternion orientation = Quaternion(0.0f, sin(gooseAngle * 0.5f), 0.0f, cos(gooseAngle * 0.5f));
		sentry->GetTransform().SetLocalOrientation(orientation);
	};

	SentryFunc chaseFunc = [](GameObject* sentry, GooseObject* goose) {
		sentry->SetStateDescription("chasing/preparing to chase");
		Vector3 directionVec = goose->GetTransform().GetWorldPosition() - sentry->GetTransform().GetWorldPosition();
		//http://lolengine.net/blog/2013/09/18/beautiful-maths-quaternion-from-vectors
		float gooseAngle = atan2(directionVec.x, directionVec.z);
		// only rotate around y axis
		Quaternion orientation = Quaternion(0.0f, sin(gooseAngle * 0.5f), 0.0f, cos(gooseAngle * 0.5f));
		sentry->GetTransform().SetLocalOrientation(orientation);
		if (goose->HasBonusItem())
			sentry->GetPhysicsObject()->AddForce(directionVec * 5.0f);
	};

	SentryState* idleState = new SentryState(idleFunc, sentry, goose);
	SentryState* chaseState = new SentryState(chaseFunc, sentry, goose);

	stateMachine->AddState(idleState);
	stateMachine->AddState(chaseState);

	// if distance between sentry and goose < 40 transition to chase state
	GenericTransition<float&, float>* toChase = new GenericTransition<float&, float>(
														GenericTransition<float&, float>::LessThanTransition, 
															sentryToGoose, 40.0f, idleState, chaseState);

	// if distance between sentry and goose > 60 transition to idle state
	GenericTransition<float&, float>* toIdle = new GenericTransition<float&, float>(
		GenericTransition<float&, float>::GreaterThanTransition,
		sentryToGoose, 60.0f, chaseState, idleState);

	stateMachine->AddTransition(toIdle);
	stateMachine->AddTransition(toChase);
}

/*void TutorialGame::ParkKeeperStateMachine() {
	KeeperFunc idleFunc = [](GameObject* keeper, GooseObject* goose) {
		keeper->SetStateDescription("idle");
	};

	KeeperFunc chaseFunc = [](GameObject* keeper, GooseObject* goose) {
		keeper->SetStateDescription("chasing");
	};

	KeeperState* idleState = new KeeperState(idleFunc, sentry, goose);
	KeeperState* chaseState = new KeeperState(chaseFunc, sentry, goose);

	stateMachine->AddState(idleState);
	stateMachine->AddState(chaseState);

	GenericTransition<float&, float>* toChase = new GenericTransition<float&, float>(
		GenericTransition<float&, float>::LessThanTransition,
		sentryToGoose, 40.0f, idleState, chaseState);

	GenericTransition<float&, float>* toIdle = new GenericTransition<float&, float>(
		GenericTransition<float&, float>::GreaterThanTransition,
		sentryToGoose, 60.0f, chaseState, idleState);

	stateMachine->AddTransition(toIdle);
	stateMachine->AddTransition(toChase);
}*/

void TutorialGame::Pathfinding() {
	NavigationGrid grid("CourseworkMap.txt");

	NavigationPath outPath;

	Vector3 difference = Vector3(210.0f, 0.0f, 500.0f);

	Vector3 startPos = parkKeeper->GetTransform().GetWorldPosition() + difference;
	Vector3 endPos = goose->GetTransform().GetWorldPosition() + difference;

	bool found = false;
	// check for new path every 0.5 seconds
	if (updatePath > 0.50f) {
		found = grid.FindPath(startPos, endPos, outPath);
	}
	
	if (found) {
		updatePath = 0.0f;
		outPath.PopWaypoint(previousPos);
		outPath.PopWaypoint(pos);

		//Vector3 copyPrevPos = previousPos;
		//Vector3 copyPos = pos;

		pathDirectionVec = pos - previousPos;
		pathDirectionVec.Normalise();

		/*while (outPath.PopWaypoint(pos)) {
			Debug::DrawLine(copyPreviousPos, copyPos, Vector4(0.0f, 0.0f, 1.0f, 1.0f));
			copyPreviousPos = copyPos;
		}*/
	}
	if (updatePath > 0.50f)
		parkKeeper->GetPhysicsObject()->ClearForces();
	else
		parkKeeper->GetPhysicsObject()->AddForce(pathDirectionVec * 150.0f);
}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();
	
	// reset scoring
	appleCount = 0;
	applesBanked = 0;
	bonusCount = 0;
	bonusBanked = 0;
	totalScore = 0;
	timeLeft = 180;

	/****************LEVEL FOUNDATION*****************/
	home = AddFloorToWorld(Vector3(0, 1.2, -40), Vector3(10, 0.2, 10), "Home", CollisionType::HOME);
	lake = AddLakeToWorld(Vector3(0, 0, -40), Vector3(40, 1, 50));

	AddWallToWorld(Vector3(0, 4, 12), Vector3(40, 6, 2));
	AddWallToWorld(Vector3(-41, 4, -40), Vector3(1, 6, 50));
	AddWallToWorld(Vector3(41, 4, -40), Vector3(1, 6, 50));

	AddFloorToWorld(Vector3(0, 0, -290), Vector3(220, 2, 200));
	// left wall
	AddWallToWorld(Vector3(-202, 12, -290), Vector3(2, 10, 200));
	// right wall
	AddWallToWorld(Vector3(202, 8, -290), Vector3(2, 10, 200));
	// back wall
	AddWallToWorld(Vector3(0, 8, -492), Vector3(200, 10, 2));
	// left front wall
	AddWallToWorld(Vector3(-122, 8, -88), Vector3(80, 10, 2));
	// right front wall
	AddWallToWorld(Vector3(122, 8, -88), Vector3(80, 10, 2));
	/*************************************************/

	/***************OBJECTS***************************/
	goose = (GooseObject*)AddGooseToWorld(GOOSE_SPAWN);
	sentry = AddCharacterToWorld(SENTRY_SPAWN);

	// gate area
	apple[0] = AddAppleToWorld(Vector3(120, 3, -150));
	// maze
	apple[1] = AddAppleToWorld(Vector3(-88, 3, -225));
	// trampoline area
	apple[2] = AddAppleToWorld(Vector3(150, 3, -420));
	// jumping puzzle
	apple[3] = AddAppleToWorld(Vector3(30, 9, -435));
	// near sentry AI
	apple[4] = AddAppleToWorld(Vector3(-160, 3, -450));

	// near home
	bonusItem[0] = AddCubeToWorld(Vector3(35, 2, -5), Vector3(0.8, 0.8, 0.8), 10.0f, true);
	// gate area
	bonusItem[1] = AddCubeToWorld(Vector3(190, 2, -120), Vector3(0.8, 0.8, 0.8), 10.0f, true);
	// maze
	bonusItem[2] = AddCubeToWorld(Vector3(-64, 2, -329), Vector3(0.8, 0.8, 0.8), 10.0f, true);
	// jump puzzle
	bonusItem[3] = AddCubeToWorld(Vector3(50, 14, -465), Vector3(0.8, 0.8, 0.8), 10.0f, true);
	// near sentry AI
	bonusItem[4] = AddCubeToWorld(Vector3(-175, 2, -450), Vector3(0.8, 0.8, 0.8), 10.0f, true);
	// trampoline area
	bonusItem[5] = AddCubeToWorld(Vector3(105, 2, -455), Vector3(0.8, 0.8, 0.8), 10.0f, true);
	/*************************************************/

	/******************GATE AREA**********************/
	gate = AddGateToWorld(Vector3(53, 4, -148), Vector3(0.5, 2, 4), Quaternion(Vector3(0, -0.8, 0), 0.34f));
	AddWallToWorld(Vector3(50, 4, -115), Vector3(1, 2, 26));
	AddWallToWorld(Vector3(50, 4, -166), Vector3(1, 2, 15));
	AddWallToWorld(Vector3(124.5, 4, -182), Vector3(75.5, 2, 1));
	
	spinner[0] = AddSpinnerToWorld(Vector3(90, 4, -151), Vector3(0.5, 2, 4));
	spinner[1] = AddSpinnerToWorld(Vector3(90, 4, -121), Vector3(0.5, 2, 4));
	spinner[2] = AddSpinnerToWorld(Vector3(110, 4, -131), Vector3(0.5, 2, 4));
	spinner[3] = AddSpinnerToWorld(Vector3(140, 4, -151), Vector3(0.5, 2, 4));
	spinner[4] = AddSpinnerToWorld(Vector3(140, 4, -121), Vector3(0.5, 2, 4));
	spinner[5] = AddSpinnerToWorld(Vector3(160, 4, -131), Vector3(0.5, 2, 4));
	/*************************************************/

	/*******************MAZE AREA*********************/
	AddWallToWorld(Vector3(-120, 5, -160), Vector3(60, 3, 1));
	AddWallToWorld(Vector3(-61, 5, -246), Vector3(1, 3, 85));

	AddWallToWorld(Vector3(-160, 5, -180), Vector3(40, 3, 1));
	// maze block stopper
	AddWallToWorld(Vector3(-180, 5, -178), Vector3(1, 3, 1));
	AddWallToWorld(Vector3(-121, 5, -186), Vector3(1, 3, 5));
	AddWallToWorld(Vector3(-111, 5, -192), Vector3(11, 3, 1));
	AddWallToWorld(Vector3(-101, 5, -186), Vector3(1, 3, 5));
	AddWallToWorld(Vector3(-92, 5, -180), Vector3(10, 3, 1));

	AddWallToWorld(Vector3(-122, 5, -215), Vector3(60, 3, 1));
	AddWallToWorld(Vector3(-181, 5, -246), Vector3(1, 3, 30));
	AddWallToWorld(Vector3(-172, 5, -277), Vector3(10, 3, 1));
	AddWallToWorld(Vector3(-161, 5, -286), Vector3(1, 3, 10));
	AddWallToWorld(Vector3(-131, 5, -297), Vector3(51, 3, 1));

	AddWallToWorld(Vector3(-140, 5, -310), Vector3(60, 3, 1));
	AddWallToWorld(Vector3(-81, 5, -321), Vector3(1, 3, 10));
	AddWallToWorld(Vector3(-71, 5, -332), Vector3(11, 3, 1));
	AddWallToWorld(Vector3(-74, 5, -329), Vector3(6, 3, 2));

	AddWallToWorld(Vector3(-81, 5, -266), Vector3(1, 3, 30));
	AddWallToWorld(Vector3(-90, 5, -235), Vector3(10, 3, 1));
	AddWallToWorld(Vector3(-101, 5, -226), Vector3(1, 3, 10));
	// ramp
	AddOBBFloorToWorld(Vector3(-101, 4, -224), Vector3(8, 1, 9));
	// maze block stopper
	AddWallToWorld(Vector3(-79, 5, -235), Vector3(1, 3, 1));
	
	dynamicCube[0] = AddDynamicCubeToWorld(Vector3(-71, 6, -170), Vector3(8, 4, 8), 0.001f);
	dynamicCube[1] = AddDynamicCubeToWorld(Vector3(-191, 6, -200), Vector3(8, 4, 8), 0.001f);
	dynamicCube[2] = AddDynamicCubeToWorld(Vector3(-71, 6, -305), Vector3(8, 4, 8), 0.001f);
	/*************************************************/

	/*******************TRAMPOLINE AREA***************/
	AddWallToWorld(Vector3(140, 9, -350), Vector3(60, 7, 1));
	AddWallToWorld(Vector3(81, 9, -420.5), Vector3(1, 7, 69.5));
	
	trampoline[0] = AddTrampolineToWorld(Vector3(100, 2.01, -335), Vector3(8, 0.01, 8));
	trampoline[1] = AddTrampolineToWorld(Vector3(95, 2.01, -365), Vector3(8, 0.01, 8));

	AddWallToWorld(Vector3(100, 5, -450), Vector3(1, 3, 10));
	AddWallToWorld(Vector3(105, 5, -461), Vector3(6, 3, 1));
	AddWallToWorld(Vector3(110, 5, -450), Vector3(1, 3, 10));

	AddCubeToWorld(Vector3(105.5, 4, -436.5), Vector3(5, 2, 3), 0.1f);
	/*************************************************/

	/*******************JUMPING PUZZLE****************/
	AddPlatformToWorld(Vector3(60, 4, -415), Vector3(5, 0.25, 5));
	AddPlatformToWorld(Vector3(50, 6, -435), Vector3(5, 0.25, 5));
	AddPlatformToWorld(Vector3(30, 8, -435), Vector3(5, 0.25, 5));
	AddPlatformToWorld(Vector3(30, 10, -455), Vector3(5, 0.25, 5));
	AddPlatformToWorld(Vector3(50, 12, -465), Vector3(5, 0.25, 5));
	/*************************************************/

	parkKeeper = AddParkKeeperToWorld(PARK_KEEPER_SPAWN);

	SentryStateMachine();
}

//From here on it's functions to add in objects to the world!

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position, Vector3 dimensions, string name, CollisionType collisionType) {
	GameObject* floor = new GameObject(name);

	AABBVolume* volume = new AABBVolume(dimensions);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform().SetWorldScale(dimensions);
	floor->GetTransform().SetWorldPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader, Vector4(0.0f, 1.0f, 0.0f, 1.0f)));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();
	floor->GetPhysicsObject()->SetCollisionType(collisionType);

	world->AddGameObject(floor);

	return floor;
}

GameObject* TutorialGame::AddOBBFloorToWorld(const Vector3& position, Vector3 dimensions, string name, CollisionType collisionType) {
	GameObject* floor = new GameObject(name);

	OBBVolume* volume = new OBBVolume(dimensions);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform().SetWorldScale(dimensions);
	floor->GetTransform().SetWorldPosition(position);
	Quaternion orientation = Quaternion(Vector3(1.0f, 0.0f, 0.0f), 0.18f);
	orientation.Normalise();
	floor->GetTransform().SetLocalOrientation(orientation);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader, Vector4(1.0f, 0.5f, 0.1f, 1.0f)));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();
	floor->GetPhysicsObject()->SetCollisionType(collisionType);

	world->AddGameObject(floor);

	return floor;
}

GameObject* TutorialGame::AddPlatformToWorld(const Vector3& position, Vector3 dimensions, string name) {
	GameObject* platform = new GameObject(name);

	AABBVolume* volume = new AABBVolume(dimensions);
	platform->SetBoundingVolume((CollisionVolume*)volume);
	platform->GetTransform().SetWorldScale(dimensions);
	platform->GetTransform().SetWorldPosition(position);

	platform->SetRenderObject(new RenderObject(&platform->GetTransform(), cubeMesh, basicTex, basicShader, Vector4(1.0f, 0.0f, 1.0f, 1.0f)));
	platform->SetPhysicsObject(new PhysicsObject(&platform->GetTransform(), platform->GetBoundingVolume()));

	platform->GetPhysicsObject()->SetInverseMass(0);
	platform->GetPhysicsObject()->InitCubeInertia();
	platform->GetPhysicsObject()->SetCollisionType(CollisionType::FLOOR);

	world->AddGameObject(platform);

	return platform;
}

GameObject* TutorialGame::AddTrampolineToWorld(const Vector3& position, Vector3 dimensions, string name) {
	GameObject* trampoline = new GameObject(name);

	AABBVolume* volume = new AABBVolume(dimensions);
	trampoline->SetBoundingVolume((CollisionVolume*)volume);
	trampoline->GetTransform().SetWorldScale(dimensions);
	trampoline->GetTransform().SetWorldPosition(position);

	trampoline->SetRenderObject(new RenderObject(&trampoline->GetTransform(), cubeMesh, basicTex, basicShader, Vector4(1.0f, 0.0f, 0.0f, 1.0f)));
	trampoline->SetPhysicsObject(new PhysicsObject(&trampoline->GetTransform(), trampoline->GetBoundingVolume()));

	trampoline->GetPhysicsObject()->SetInverseMass(0);
	trampoline->GetPhysicsObject()->InitCubeInertia();
	trampoline->GetPhysicsObject()->SetCollisionType(CollisionType::TRAMPOLINE);

	world->AddGameObject(trampoline);

	return trampoline;
}

GameObject* TutorialGame::AddLakeToWorld(const Vector3& position, Vector3 dimensions, string name) {
	GameObject* lake = new GameObject(name);

	AABBVolume* volume = new AABBVolume(dimensions);
	lake->SetBoundingVolume((CollisionVolume*)volume);
	lake->GetTransform().SetWorldScale(dimensions);
	lake->GetTransform().SetWorldPosition(position);

	lake->SetRenderObject(new RenderObject(&lake->GetTransform(), cubeMesh, basicTex, basicShader, Vector4(0.0f, 0.0f, 1.0f, 1.0f)));
	lake->SetPhysicsObject(new PhysicsObject(&lake->GetTransform(), lake->GetBoundingVolume()));

	lake->GetPhysicsObject()->SetInverseMass(0);
	lake->GetPhysicsObject()->InitCubeInertia();
	lake->GetPhysicsObject()->SetCollisionType(CollisionType::LAKE);

	world->AddGameObject(lake);

	return lake;
}

GameObject* TutorialGame::AddWallToWorld(const Vector3& position, Vector3 dimensions, string name) {
	GameObject* wall = new GameObject(name);

	AABBVolume* volume = new AABBVolume(dimensions);
	wall->SetBoundingVolume((CollisionVolume*)volume);
	wall->GetTransform().SetWorldScale(dimensions);
	wall->GetTransform().SetWorldPosition(position);

	wall->SetRenderObject(new RenderObject(&wall->GetTransform(), cubeMesh, basicTex, basicShader, Vector4(1.0f, 0.5f, 0.1f, 1.0f)));
	wall->SetPhysicsObject(new PhysicsObject(&wall->GetTransform(), wall->GetBoundingVolume()));

	wall->GetPhysicsObject()->SetInverseMass(0);
	wall->GetPhysicsObject()->InitCubeInertia();
	wall->GetPhysicsObject()->SetCollisionType(CollisionType::WALL);

	world->AddGameObject(wall);

	return wall;
}

GameObject* TutorialGame::AddGateToWorld(const Vector3& position, Vector3 dimensions, Quaternion orientation, string name) {
	GameObject* gate = new GameObject(name);

	OBBVolume* volume = new OBBVolume(dimensions);
	gate->SetBoundingVolume((CollisionVolume*)volume);
	gate->GetTransform().SetWorldScale(dimensions);
	gate->GetTransform().SetWorldPosition(position);
	orientation.Normalise();
	gate->GetTransform().SetLocalOrientation(orientation);

	gate->SetRenderObject(new RenderObject(&gate->GetTransform(), cubeMesh, basicTex, basicShader, Vector4(1.0f, 0.6f, 0.2f, 1.0f)));
	gate->SetPhysicsObject(new PhysicsObject(&gate->GetTransform(), gate->GetBoundingVolume()));

	gate->GetPhysicsObject()->SetInverseMass(0.1);
	gate->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(gate);

	return gate;
}

GameObject* TutorialGame::AddSpinnerToWorld(const Vector3& position, Vector3 dimensions, string name) {
	GameObject* spinner = new GameObject(name);

	OBBVolume* volume = new OBBVolume(dimensions);
	spinner->SetBoundingVolume((CollisionVolume*)volume);
	spinner->GetTransform().SetWorldScale(dimensions);
	spinner->GetTransform().SetWorldPosition(position);

	spinner->SetRenderObject(new RenderObject(&spinner->GetTransform(), cubeMesh, basicTex, basicShader, Vector4(1.0f, 0.6f, 0.2f, 1.0f)));
	spinner->SetPhysicsObject(new PhysicsObject(&spinner->GetTransform(), spinner->GetBoundingVolume()));

	spinner->GetPhysicsObject()->SetInverseMass(0.001f);
	spinner->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(spinner);

	return spinner;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass, bool hollow) {
	GameObject* sphere = new GameObject("Sphere");

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);
	sphere->GetTransform().SetWorldScale(sphereSize);
	sphere->GetTransform().SetWorldPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);

	if (hollow)
		sphere->GetPhysicsObject()->InitHollowSphereInertia();
	else
		sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass, bool collectable) {
	string name = "Cube";
	if (collectable)
		name = "CollectableCube";

	GameObject* cube = new GameObject(name);

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform().SetWorldPosition(position);
	cube->GetTransform().SetWorldScale(dimensions);

	cube->SetSpawnPos(position);

	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));
	if (collectable) {
		cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader, Vector4(0.0f, 0.5f, 0.5f, 1.0f)));
		cube->GetPhysicsObject()->SetCollisionType(CollisionType::COLLECTABLE);
	}
	else
		cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

GameObject* TutorialGame::AddDynamicCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	string name = "DynamicCube";

	GameObject* cube = new GameObject(name);

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform().SetWorldPosition(position);
	cube->GetTransform().SetWorldScale(dimensions);

	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));
	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();
	cube->GetPhysicsObject()->SetCollisionType(CollisionType::IMMOVABLE);

	world->AddGameObject(cube);

	return cube;
}

GameObject* TutorialGame::AddGooseToWorld(const Vector3& position) {
	float size			= 1.0f;
	float inverseMass	= 1.0f;
	float elasticity	= 0.2f;

	GooseObject* goose = new GooseObject("Goose");

	SphereVolume* volume = new SphereVolume(size);
	goose->SetBoundingVolume((CollisionVolume*)volume);

	goose->GetTransform().SetWorldScale(Vector3(size,size,size) );
	goose->GetTransform().SetWorldPosition(position);

	goose->SetRenderObject(new RenderObject(&goose->GetTransform(), gooseMesh, nullptr, basicShader));
	goose->SetPhysicsObject(new PhysicsObject(&goose->GetTransform(), goose->GetBoundingVolume()));

	goose->SetSpawnPos(GOOSE_SPAWN);

	goose->GetPhysicsObject()->SetInverseMass(inverseMass);
	goose->GetPhysicsObject()->SetElasticity(elasticity);
	goose->GetPhysicsObject()->InitSphereInertia();
	goose->GetPhysicsObject()->SetCollisionType(CollisionType::PLAYER);

	world->AddGameObject(goose);

	return (GameObject*)goose;
}

GameObject* TutorialGame::AddParkKeeperToWorld(const Vector3& position)
{
	float meshSize = 4.0f;
	float inverseMass = 0.5f;

	GameObject* keeper = new GameObject("Park Keeper");

	AABBVolume* volume = new AABBVolume(Vector3(0.3, 0.9f, 0.3) * meshSize);
	keeper->SetBoundingVolume((CollisionVolume*)volume);

	keeper->GetTransform().SetWorldScale(Vector3(meshSize, meshSize, meshSize));
	keeper->GetTransform().SetWorldPosition(position);

	keeper->SetRenderObject(new RenderObject(&keeper->GetTransform(), keeperMesh, nullptr, basicShader));
	keeper->SetPhysicsObject(new PhysicsObject(&keeper->GetTransform(), keeper->GetBoundingVolume()));

	keeper->GetPhysicsObject()->SetInverseMass(inverseMass);
	keeper->GetPhysicsObject()->InitCubeInertia();
	keeper->GetPhysicsObject()->SetCollisionType(CollisionType::AI);

	world->AddGameObject(keeper);

	return keeper;
}

GameObject* TutorialGame::AddCharacterToWorld(const Vector3& position) {
	float meshSize = 4.0f;
	float inverseMass = 0.5f;

	auto pos = keeperMesh->GetPositionData();

	Vector3 minVal = pos[0];
	Vector3 maxVal = pos[0];

	for (auto& i : pos) {
		maxVal.y = max(maxVal.y, i.y);
		minVal.y = min(minVal.y, i.y);
	}

	GameObject* character = new GameObject("Character");

	float r = rand() / (float)RAND_MAX;


	AABBVolume* volume = new AABBVolume(Vector3(0.3, 0.9f, 0.3) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform().SetWorldScale(Vector3(meshSize, meshSize, meshSize));
	character->GetTransform().SetWorldPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), r > 0.5f ? charA : charB, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitCubeInertia();
	character->GetPhysicsObject()->SetCollisionType(CollisionType::AI);

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddAppleToWorld(const Vector3& position) {
	GameObject* apple = new GameObject("Apple");

	SphereVolume* volume = new SphereVolume(0.7f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform().SetWorldScale(Vector3(4, 4, 4));
	apple->GetTransform().SetWorldPosition(position);
	apple->SetCollectable(true);

	apple->SetSpawnPos(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), appleMesh, nullptr, basicShader, Vector4(1.0f, 0.0f, 0.0f, 1.0f)));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();
	apple->GetPhysicsObject()->SetCollisionType(CollisionType::COLLECTABLE);

	world->AddGameObject(apple);

	return apple;
}

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
			}
			else {
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols+1; ++x) {
		for (int z = 1; z < numRows+1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(8, 8, 8);

	float	invCubeMass = 5;
	int		numLinks	= 25;
	float	maxDistance	= 30;
	float	cubeDistance = 20;

	Vector3 startPos = Vector3(500, 1000, 500);

	GameObject* start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0);

	GameObject* end = AddCubeToWorld(startPos + Vector3((numLinks + 2) * cubeDistance, 0, 0), cubeSize, 0);

	GameObject* previous = start;

	for (int i = 0; i < numLinks; ++i) {
		GameObject* block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, invCubeMass);
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
	}

	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);
}

void TutorialGame::SimpleGJKTest() {
	Vector3 dimensions		= Vector3(5, 5, 5);
	Vector3 floorDimensions = Vector3(100, 2, 100);

	GameObject* fallingCube = AddCubeToWorld(Vector3(0, 20, 0), dimensions, 10.0f);
	GameObject* newFloor	= AddCubeToWorld(Vector3(0, 0, 0), floorDimensions, 0.0f);

	delete fallingCube->GetBoundingVolume();
	delete newFloor->GetBoundingVolume();

	fallingCube->SetBoundingVolume((CollisionVolume*)new OBBVolume(dimensions));
	newFloor->SetBoundingVolume((CollisionVolume*)new OBBVolume(floorDimensions));

}