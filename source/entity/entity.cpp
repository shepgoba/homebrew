#include "entity.h"

namespace Engine {
	Entity::Entity(const Vertex list[], int size) {
		//Setting array element size.
		this->listElementSize = size;
		
		//Setting list size.
		this->vertexListSize = size * sizeof(Vertex);
		
		//Initializing vertex list.
		//Create vertex buffer objects.
		this->vertexBuffer = linearAlloc(this->vertexListSize);
		std::memcpy(this->vertexBuffer, list, this->vertexListSize);
		
		//Enabling rendering flag.
		this->renderFlag = true;
		//Enabling updating flag.
		this->updateFlag = true;
		
		//Other remaining stuffs.
		this->angleXOffset = 0.0f;
		this->angleXSpeed = 0.0f;
		this->posX = 0.0f;
		this->posY = 0.0f;
		this->posZ = 0.0f;
		this->rotX = 0.0f;
		this->rotY = 0.0f;
		this->rotZ = 0.0f;
		this->scaleX = 0.0f;
		this->scaleY = 0.0f;
		this->scaleZ = 0.0f;
	}
	
	Entity::Entity(const Entity& copy){
		//Copying everything over.
		this->posX = copy.posX;
		this->posY = copy.posY;
		this->posZ = copy.posZ;
		this->rotX = copy.rotX;
		this->rotY = copy.rotY;
		this->rotZ = copy.rotZ;
		this->scaleX = copy.scaleX;
		this->scaleY = copy.scaleY;
		this->scaleZ = copy.scaleZ;
		this->listElementSize = copy.listElementSize;
		this->vertexListSize = copy.vertexListSize;
		this->renderFlag = copy.renderFlag;
		this->updateFlag = copy.updateFlag;
		this->angleXOffset = copy.angleXOffset;
		this->angleXSpeed = copy.angleXSpeed;
		this->vertexBuffer = linearAlloc(copy.vertexListSize);
		std::memcpy(this->vertexBuffer, copy.vertexBuffer, copy.vertexListSize);
	}
	
	Entity::~Entity(){
		linearFree(this->vertexBuffer);
	}
	
	void Entity::Update(){
		//This is for game logic updates, for each individual entities in the engine.
		if (this->updateFlag){
			//Do stuff here....
			this->rotX += this->angleXSpeed * radian;
			this->rotX += this->angleXOffset * radian;
			if (this->rotX > degToRad(180.0f)){
				this->rotX = degToRad(-180.0f);
			}
			this->posX = 3.0f * cosf(this->rotX);
			this->posY = 3.0f * sinf(this->rotX);
		}
	}
	
	void Entity::RenderUpdate(C3D_Mtx* modelMatrix) {
		//This update function will update entity properties.
		Mtx_Translate(modelMatrix, this->posX, this->posY, 0.0f);
	}
	
	void Entity::Render(){
		if (this->renderFlag){
			//Since the entity object uses up the full vertex buffer, we start from the
			//beginning of the vertex buffer, and go through to the end of it.
			C3D_DrawArrays(GPU_TRIANGLES, 0, this->listElementSize);
		}
	}
	
	void Entity::ConfigureBuffer(){
		//Initialize and configure buffers.
		//The Buffer Info needs to be reset every time a new buffer is to take its place.
		//In other words, BufInfo_Init() is frequently used.
		C3D_BufInfo* bufferInfo = C3D_GetBufInfo();                  
		BufInfo_Init(bufferInfo);                                    
		BufInfo_Add(bufferInfo, this->vertexBuffer, sizeof(Vertex), 3, 0x210); 
	}
	
	u32 Entity::GetListSize() const {
		return this->listElementSize;
	}
	
	u32 Entity::GetTotalSize() const {
		return this->vertexListSize;
	}
	
	void* Entity::GetVertexBuffer() const {
		return this->vertexBuffer;
	}
	
	void Entity::SetUpdateFlag(bool value){
		this->updateFlag = value;
	}
	
	void Entity::SetRenderFlag(bool value){
		this->renderFlag = value;
	}
	
	void Entity::SetAngleXSpeed(float value) {
		this->angleXSpeed = value;
	}
	
	void Entity::SetAngleXOffset(float value){
		this->angleXOffset = value;
	}
	
//	void Entity::SetPositionX(float value) {
//		this->posX = value;
//	}
//	
//	void Entity::SetPositionY(float value) {
//		this->posY = value;
//	}
//	
//	void Entity::SetPositionZ(float value){
//		this->posZ = value;
//	}
//	
//	void Entity::SetRotationX(float value){
//		this->rotX = value;
//	}
//	
//	void Entity::SetRotationY(float value){
//		this->rotY = value;
//	}
//	
//	void Entity::SetRotationZ(float value){
//		this->rotZ = value;
//	}
//	
//	void Entity::SetScaleX(float value){
//		this->scaleX = value;
//	}
//	
//	void Entity::SetScaleY(float value){
//		this->scaleY = value;
//	}
//	
//	void Entity::SetScaleZ(float value){
//		this->scaleZ = value;
//	}
	
	bool Entity::IsRenderEnabled() const {
		return this->renderFlag;
	}
	
	bool Entity::IsUpdateEnabled() const {
		return this->updateFlag;
	}
};
