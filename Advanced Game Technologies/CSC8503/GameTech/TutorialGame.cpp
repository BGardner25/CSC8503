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

	forceMagnitude	= 10.0f;
	useGravity		= false;
	useBroadPhase	= true;
	inSelectionMode = false;

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
	loadFunc("goose.msh"	 , &gooseMesh);
	loadFunc("CharacterA.msh", &keeperMesh);
	loadFunc("CharacterM.msh", &charA);
	loadFunc("CharacterF.msh", &charB);
	loadFunc("Apple.msh"	 , &appleMesh);

	basicTex	= (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	InitCamera();
	InitWorld();
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

	delete home;
	delete goose;
	delete lake;
	delete land;
	delete gate;
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
	// TEMPORARY
	appleCount = physics->physAppleCount;
	UpdateKeys();

	if (useGravity) {
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
	}

	renderer->DrawString("Apples:" + std::to_string(appleCount), Vector2(10, 0));

	SelectObject();
	MoveSelectedObject();
	PlayerMovement();

	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);

	Debug::FlushRenderables();
	renderer->Render();
}

void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::B)) {
		useBroadPhase = !useBroadPhase;
		physics->UseBroadPhase(useBroadPhase);
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

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		selectionObject->GetPhysicsObject()->AddForce(-rightAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		selectionObject->GetPhysicsObject()->AddForce(rightAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		selectionObject->GetPhysicsObject()->AddForce(fwdAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		selectionObject->GetPhysicsObject()->AddForce(-fwdAxis);
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
		float scale = 200.0f;
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::W)) {
			Quaternion orientation = Quaternion(Vector3(0, 1, 0), 0.0f);
			orientation.Normalise();
			goose->GetPhysicsObject()->AddForce(Vector3(0, 0, -1) * scale);
			goose->GetTransform().SetLocalOrientation(orientation);
		}
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::A)) {
			Quaternion orientation = Quaternion(Vector3(0, 1, 0), -1.0f);
			orientation.Normalise();
			goose->GetPhysicsObject()->AddForce(Vector3(-1, 0, 0) * scale);
			goose->GetTransform().SetLocalOrientation(orientation);
			//goose->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
			//std::cout << goose->GetTransform().GetWorldOrientation() << std::endl;
		}
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::S)) {
			Quaternion orientation = Quaternion(Vector3(0, 1, 0), 180.0f);
			orientation.Normalise();
			goose->GetPhysicsObject()->AddForce(Vector3(0, 0, 1) * scale);
			goose->GetTransform().SetLocalOrientation(orientation);
		}
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::D)) {
			Quaternion orientation = Quaternion(Vector3(0, 1, 0), 1.0f);
			orientation.Normalise();
			goose->GetPhysicsObject()->AddForce(Vector3(1, 0, 0) * scale);
			goose->GetTransform().SetLocalOrientation(orientation);
		}
		if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::SPACE))
			goose->GetPhysicsObject()->AddForce(Vector3(0, 1, 0) * scale * 20);
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::SHIFT))
			goose->GetPhysicsObject()->AddForce(Vector3(0, -1, 0) * scale * 0.4);
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
				return true;
			}
			else {
				return false;
			}
		}
		if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
			if (selectionObject) {
				if (lockedObject == selectionObject) {
					lockedObject = nullptr;
				}
				else {
					lockedObject = selectionObject;
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
	if (inSelectionMode) {
		/*float scale = 200.0f;
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::W))
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -1) * scale);
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::A))
			selectionObject->GetPhysicsObject()->AddForce(Vector3(-1, 0, 0) * scale);
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::S))
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 1) * scale);
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::D))
			selectionObject->GetPhysicsObject()->AddForce(Vector3(1, 0, 0) * scale);
		if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::SPACE))
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 1, 0) * scale * 20);
		if (Window::GetKeyboard()->KeyDown(NCL::KeyboardKeys::SHIFT))
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -1, 0) * scale * 0.4);*/
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

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();

	/****************LEVEL FOUNDATION*****************/
	home = AddFloorToWorld(Vector3(0, 2, -40), Vector3(10, 0.5, 10));
	lake = AddLakeToWorld(Vector3(0, 0, -40), Vector3(40, 1, 50));

	AddWallToWorld(Vector3(0, 4, 12), Vector3(40, 6, 2));
	AddWallToWorld(Vector3(-41, 4, -40), Vector3(1, 6, 50));
	AddWallToWorld(Vector3(41, 4, -40), Vector3(1, 6, 50));

	land = AddFloorToWorld(Vector3(0, 0, -290), Vector3(200, 2, 200));
	// left wall
	AddWallToWorld(Vector3(-202, 8, -290), Vector3(2, 10, 200));
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
	goose = AddGooseToWorld(GOOSE_SPAWN);
	sentry = AddCharacterToWorld(SENTRY_SPAWN);

	// gate area
	apple[0] = AddAppleToWorld(Vector3(120, 3, -150));
	// maze
	apple[1] = AddAppleToWorld(Vector3(-95, 3, -225));
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
	gate = AddGateToWorld(Vector3(50, 4, -150), Vector3(0.5, 2, 4), "Gate");
	AddWallToWorld(Vector3(50, 5, -118), Vector3(1, 3, 28));
	AddWallToWorld(Vector3(50, 5, -169), Vector3(1, 3, 15));
	AddWallToWorld(Vector3(124.5, 5, -185), Vector3(75.5, 3, 1));
	/*************************************************/

	/*******************MAZE AREA*********************/
	AddWallToWorld(Vector3(-120, 5, -160), Vector3(60, 3, 1));
	AddWallToWorld(Vector3(-61, 5, -246), Vector3(1, 3, 85));

	AddWallToWorld(Vector3(-160, 5, -180), Vector3(40, 3, 1));
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

	AddWallToWorld(Vector3(-81, 5, -266), Vector3(1, 3, 30));
	AddWallToWorld(Vector3(-90, 5, -235), Vector3(10, 3, 1));
	AddWallToWorld(Vector3(-101, 5, -226), Vector3(1, 3, 10));

	dynamicCube[0] = AddCubeToWorld(Vector3(-71, 6, -170), Vector3(8, 4, 8));
	dynamicCube[1] = AddCubeToWorld(Vector3(-191, 6, -200), Vector3(8, 4, 8));
	dynamicCube[2] = AddCubeToWorld(Vector3(-71, 6, -315), Vector3(8, 4, 8));
	/*************************************************/

	/*******************TRAMPOLINE AREA***************/
	AddWallToWorld(Vector3(140, 9, -350), Vector3(60, 7, 1));
	AddWallToWorld(Vector3(81, 9, -420.5), Vector3(1, 7, 69.5));
	
	trampoline[0] = AddTrampolineToWorld(Vector3(100, 2.01, -335), Vector3(8, 0.01, 8));
	trampoline[1] = AddTrampolineToWorld(Vector3(95, 2.01, -365), Vector3(8, 0.01, 8));

	AddWallToWorld(Vector3(100, 5, -450), Vector3(1, 3, 10));
	AddWallToWorld(Vector3(105, 5, -461), Vector3(6, 3, 1));
	AddWallToWorld(Vector3(110, 5, -450), Vector3(1, 3, 10));

	// @TODO make this harder to push
	AddCubeToWorld(Vector3(105.5, 4, -436.5), Vector3(5, 2, 3));
	/*************************************************/

	/*******************JUMPING PUZZLE****************/
	AddPlatformToWorld(Vector3(60, 3, -400), Vector3(5, 0.25, 5));
	AddPlatformToWorld(Vector3(60, 4, -415), Vector3(5, 0.25, 5));
	AddPlatformToWorld(Vector3(50, 6, -435), Vector3(5, 0.25, 5));
	AddPlatformToWorld(Vector3(30, 8, -435), Vector3(5, 0.25, 5));
	AddPlatformToWorld(Vector3(30, 10, -455), Vector3(5, 0.25, 5));
	AddPlatformToWorld(Vector3(50, 12, -465), Vector3(5, 0.25, 5));
	/*************************************************/

	/*InitMixedGridWorld(10, 10, 3.5f, 3.5f);
	AddGooseToWorld(Vector3(30, 2, 0));
	AddAppleToWorld(Vector3(35, 2, 0));

	AddParkKeeperToWorld(Vector3(40, 2, 0));
	AddCharacterToWorld(Vector3(45, 2, 0));

	AddCubeToWorld(Vector3(0, 5, 0), Vector3(1, 2, 4));
	AddSphereToWorld(Vector3(0, 10, -10), 2.0f, 10.0f, true);
	AddSphereToWorld(Vector3(0, 10, -20), 2.0f, 10.0f, false);

	
	GameObject* rubberSphere = AddSphereToWorld(Vector3(-10, 20, -10), 2.0f, 10.0f, false);
	GameObject* steelSphere = AddSphereToWorld(Vector3(-10, 20, -20), 2.0f, 10.0f, false);

	PhysicsObject* rPhys = rubberSphere->GetPhysicsObject();
	PhysicsObject* sPhys = steelSphere->GetPhysicsObject();

	rPhys->SetElasticity(0.95);
	sPhys->SetElasticity(0.20);
	*/

	// second floor... initmixedgridworld adds floor also
	//AddFloorToWorld(Vector3(0, -2, 0));
}

//From here on it's functions to add in objects to the world!

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position, Vector3 dimensions, string name) {
	GameObject* floor = new GameObject(name);

	AABBVolume* volume = new AABBVolume(dimensions);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform().SetWorldScale(dimensions);
	floor->GetTransform().SetWorldPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader, Vector4(0.0f, 1.0f, 0.0f, 1.0f)));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

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

	world->AddGameObject(wall);

	return wall;
}

GameObject* TutorialGame::AddGateToWorld(const Vector3& position, Vector3 dimensions, string name) {
	GameObject* gate = new GameObject(name);

	AABBVolume* volume = new AABBVolume(dimensions);
	gate->SetBoundingVolume((CollisionVolume*)volume);
	gate->GetTransform().SetWorldScale(dimensions);
	gate->GetTransform().SetWorldPosition(position);

	gate->SetRenderObject(new RenderObject(&gate->GetTransform(), cubeMesh, basicTex, basicShader, Vector4(1.0f, 0.6f, 0.2f, 1.0f)));
	gate->SetPhysicsObject(new PhysicsObject(&gate->GetTransform(), gate->GetBoundingVolume()));

	gate->GetPhysicsObject()->SetInverseMass(10);
	gate->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(gate);

	return gate;
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

	if(collectable)
		cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader, Vector4(0.0f, 0.5f, 0.5f, 1.0f)));
	else
		cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

GameObject* TutorialGame::AddGooseToWorld(const Vector3& position)
{
	float size			= 1.0f;
	float inverseMass	= 1.0f;

	GameObject* goose = new GameObject("Goose");


	SphereVolume* volume = new SphereVolume(size);
	goose->SetBoundingVolume((CollisionVolume*)volume);

	goose->GetTransform().SetWorldScale(Vector3(size,size,size) );
	goose->GetTransform().SetWorldPosition(position);

	goose->SetRenderObject(new RenderObject(&goose->GetTransform(), gooseMesh, nullptr, basicShader));
	goose->SetPhysicsObject(new PhysicsObject(&goose->GetTransform(), goose->GetBoundingVolume()));

	goose->GetPhysicsObject()->SetInverseMass(inverseMass);
	goose->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(goose);

	return goose;
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

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddAppleToWorld(const Vector3& position) {
	GameObject* apple = new GameObject("Apple");

	SphereVolume* volume = new SphereVolume(0.7f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform().SetWorldScale(Vector3(4, 4, 4));
	apple->GetTransform().SetWorldPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), appleMesh, nullptr, basicShader, Vector4(1.0f, 0.0f, 0.0f, 1.0f)));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

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

