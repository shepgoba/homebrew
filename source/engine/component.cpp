#include "component.h"

namespace Entity {
	Component::Component() {
		this->type = ComponentType::AbstractComponent;
	}
	
	Component::~Component(){ }
	
	void Component::SetParent(GameObject* parent){
		this->parent = std::shared_ptr<GameObject>(parent);
	}

	//------------------------------------------------------------------------------------

	PhysicsComponent::PhysicsComponent() {
		this->type = ComponentType::PhysicsComponent;
		ax = ay = az = vx = vy = vz = 0.0f;
		std::cout << "PhysicsComponent has been created." << std::endl;
	}
	
	PhysicsComponent::PhysicsComponent(PhysicsComponent& copy) {
		this->type = copy.type;
		this->ax = copy.ax;
		this->ay = copy.ay;
		this->az = copy.az;
		this->vx = copy.vx;
		this->vy = copy.vy;
		this->vz = copy.vz;
	}
	
	void PhysicsComponent::Initialize() { }

	void PhysicsComponent::Update(){
		if (this->parent->position.y < 0.0f) {
			this->ay *= -0.8f;
			this->vy *= -0.8f;
			if (std::abs(this->ay) < std::numeric_limits<float>::epsilon()){
				this->ay = 0.0f;
			}
		}
		else if (this->ay > this->GravityY){
			this->ay += this->GravityY / 30.0f;
		}

		this->vx += this->ax;
		this->vy += this->ay;
		this->vz += this->az;
		this->parent->position.x += this->vx;
		this->parent->position.y += this->vy;
		this->parent->position.z += this->vz;

		this->vx *= 0.2f;
		this->vy *= 0.2f;
		this->vz *= 0.2f;
	}

	void PhysicsComponent::RenderUpdate(C3D_Mtx& viewMatrix, C3D_Mtx* modelMatrix){
		Mtx_Translate(modelMatrix, this->parent->position.x, this->parent->position.y, this->parent->position.z, true);
	}

	void PhysicsComponent::Out(){
		std::cout << std::setprecision(3) << az << "    " << vz << "     " << this->parent->position.z << std::endl;
	}

	//------------------------------------------------------------------------------------
	
	TransformComponent::TransformComponent() {
		this->type = ComponentType::TransformComponent;
		this->testAngle = 0.0f;
	}
	
	TransformComponent::TransformComponent(TransformComponent& copy){
		this->type = copy.type;
		this->scale.x = copy.scale.x;
		this->scale.y = copy.scale.y;
		this->scale.z = copy.scale.z;
		this->testAngle = copy.testAngle;
	}
	
	void TransformComponent::Initialize() {
		if (this->parent != nullptr){
			this->parent->scale.x = this->parent->scale.y = this->parent->scale.z = 1.0f;
			this->parent->rotation = Quat_Identity();
		}
	}
	
	void TransformComponent::Update(){ 
	}
	
	void TransformComponent::RenderUpdate(C3D_Mtx& viewMatrix, C3D_Mtx* modelMatrix){
	}
	
	void TransformComponent::Out() { }
}
