#include "engine.h"

namespace Engine {
	Core& Core::Instance(){
		static Core core;
		return core;
	}

	Core::~Core(){ 	}

	void Core::Initialize(){
		std::cout << "Initializing core." << std::endl;

		//Enable 3D
		gfxSet3D(true);

		//Initializing Citro3D graphics.
		C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
		
		//Initializing render targets.
		this->leftTarget = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
		this->rightTarget = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);

		//Clearing render targets;
		C3D_RenderTargetSetClear(this->leftTarget, C3D_CLEAR_ALL, COMMON_CLEAR_COLOR, 0);
		C3D_RenderTargetSetClear(this->rightTarget, C3D_CLEAR_ALL, COMMON_CLEAR_COLOR, 0);

		//Setting render outputs.
		C3D_RenderTargetSetOutput(this->leftTarget, GFX_TOP, GFX_LEFT, COMMON_DISPLAY_TRANSFER_FLAGS);
		C3D_RenderTargetSetOutput(this->rightTarget, GFX_TOP, GFX_RIGHT, COMMON_DISPLAY_TRANSFER_FLAGS);

		std::cout << "Initializing scene" << std::endl;

		//Load vertex shader, then create a shader program to bind the vertex shader to.
		//The variables are automatically generated from PICA shader files when using "make" commands.
		this->vertexShader_dvlb = DVLB_ParseFile((u32*) vshader_shbin, vshader_shbin_size);
		shaderProgramInit(&this->program);
		shaderProgramSetVsh(&this->program, &this->vertexShader_dvlb->DVLE[0]);

		//Binding.
		C3D_BindProgram(&this->program);

		//Get location of uniforms used in the vertex shader.
		this->uLoc_projection = shaderInstanceGetUniformLocation(this->program.vertexShader, "projection");
		this->uLoc_view = shaderInstanceGetUniformLocation(this->program.vertexShader, "view");
		this->uLoc_model = shaderInstanceGetUniformLocation(this->program.vertexShader, "model");

		//Initialize attributes, and then configure them for use with vertex shader.
		C3D_AttrInfo* attributeInfo = C3D_GetAttrInfo();
		AttrInfo_Init(attributeInfo);
		AttrInfo_AddLoader(attributeInfo, 0, GPU_FLOAT, 3); //First float array = vertex position.
		AttrInfo_AddLoader(attributeInfo, 1, GPU_FLOAT, 2); //Second float array = texture coordinates.
		AttrInfo_AddLoader(attributeInfo, 2, GPU_FLOAT, 3); //Third float array = normals.

		// Configure the first fragment shading substage to blend the fragment primary color
		// with the fragment secondary color.
		// See https://www.opengl.org/sdk/docs/man2/xhtml/glTexEnv.xml for more insight
		C3D_TexEnv* environment = C3D_GetTexEnv(0);
		C3D_TexEnvSrc(environment, C3D_Both, GPU_FRAGMENT_PRIMARY_COLOR, GPU_FRAGMENT_SECONDARY_COLOR, 0);
		C3D_TexEnvOp(environment, C3D_Both, 0, 0, 0);
		C3D_TexEnvFunc(environment, C3D_Both, GPU_ADD);

		//Lighting setup
		C3D_LightEnvInit(&this->lightEnvironment);
		C3D_LightEnvBind(&this->lightEnvironment);
		C3D_LightEnvMaterial(&this->lightEnvironment, &material);

		LightLut_Phong(&this->lut_Phong, 30);
		C3D_LightEnvLut(&this->lightEnvironment, GPU_LUT_D0, GPU_LUTINPUT_LN, false, &this->lut_Phong);

		C3D_FVec lightVector = { { 6.0, 0.5, 0.0, 0.0 } };

		C3D_LightInit(&this->light, &this->lightEnvironment);
		C3D_LightColor(&this->light, 1.0, 1.0, 1.0);
		C3D_LightPosition(&this->light, &lightVector);

		this->LoadObjects();
	}

	void Core::LoadObjects(){
		std::cout << "Loading objects..." << std::endl;

		//This is how you load game objects with customized components.
		//You first declare a component, with your edited values.
		//Then you add them in via the helper function, AddComponent<T>(), passing in components as arguments.
		//This can be extended to full class object initializations.
		for (int i = 0; i < 3; i++){
			std::shared_ptr<GameObject> temp(new GameObject(vertexList, vertexListSize));
			
			//Physics Component initial setup.
			PhysicsComponent p;
			temp->AddComponent<PhysicsComponent>(p);
			
			//Transform Component initial setup.
			//Not sure what to do with Transform Component.
			TransformComponent t;
			temp->AddComponent<TransformComponent>(t);
			
			//Setting up the initial positions for each game object.
			temp->position.x = 3.0f * i;
			temp->position.y = 5.0f * (i+1);
			
			//Debugging
			if (i == 1){
				temp->debugFlag = true;
			}
			
			this->gameObjects.push_back(temp);
		}
	}

	void Core::Update(u32 downKey, u32 heldKey, u32 upKey, touchPosition touch){
		//Update the player.
		this->player.Update(downKey, heldKey, upKey, touch);
		
		if (this->player.cameraManipulateFlag){
			std::shared_ptr<GameObject> closestObject = this->GetClosestObjectToPosition(this->player.cameraPosition, 4.0f); 
			if (closestObject != nullptr){
				this->player.inHands = closestObject;
				this->player.inHands->isPickedUp = true;
			}
			text(20, 0, "                              ");
			std::cout << "Closest object? " << (closestObject != nullptr ? "True" : "False") << std::endl;
		}
		

		for (size_t i = 0; i < this->gameObjects.size(); i++){
			//This handles updating the game object's properties.
			this->gameObjects[i]->Update();
			
			//This checks if the player is picking up the object within a set distance of 5 units away from the player. Else, we
			//leave it alone.
			if (!this->player.cameraManipulateFlag && this->gameObjects[i]->isPickedUp){
				this->gameObjects[i]->isPickedUp = false;
				this->player.inHands = nullptr;
			}
		}
	}

	void Core::Render(){
		//Fetch Stereoscopic 3D level.
		float slider = osGet3DSliderState();
		//Inter Ocular Distance. We divide by 3.0f to reduce the 3D stereoscopic effects.
		float iod = slider / 3.0f;

		//Rendering scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		{
			C3D_FrameDrawOn(this->leftTarget);
			this->SceneRender(-iod);
			if (iod > 0.0f) {
				C3D_FrameDrawOn(this->rightTarget);
				this->SceneRender(iod);
			}
		}
		C3D_FrameEnd(0);
	}

	void Core::Release(){
		//Releasing memory.
		for (size_t i = 0; i < this->gameObjects.size(); i++){
			this->gameObjects[i]->Release();
		}
	}

	void Core::SceneRender(float interOcularDistance){
		//Declaring reusable model matrix.
		C3D_Mtx modelMatrix;
	
		//Compute projection matrix and update matrix to shader program.                                                                                                               
		Mtx_PerspStereoTilt(&this->projectionMatrix, 40.0f * (std::acos(-1) / 180.0f), 400.0f / 240.0f, 0.01f, 1000.0f, interOcularDistance, 2.0f, false);
		C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, this->uLoc_projection, &this->projectionMatrix);
	
		//Do something about view matrix.
		this->player.RenderUpdate(&this->viewMatrix);
	
		//Compute view matrix and update matrix to shader program.
		C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, this->uLoc_view, &this->viewMatrix);
	
		//Draw the vertex buffer objects.                     
		for (size_t i = 0; i < this->gameObjects.size(); i++) {
			//Calculate model view matrix.
			Mtx_Identity(&modelMatrix);

			//Switch game object buffers
			this->gameObjects[i]->ConfigureBuffer();
	
			//At the moment, there's only 1 object in the scene. This allows the player to "pick" up the object(s) in hand, and manipulate them.
			this->gameObjects[i]->RenderUpdate(this->player.cameraPosition, this->viewMatrix, &modelMatrix);				
				
			//Update to shader program.
			C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, this->uLoc_model, &modelMatrix);
	
			//Render entity.
			this->gameObjects[i]->Render();
		}
	}

	void Core::SceneExit(){
		std::cout << "Exiting scene" << std::endl;

		//Free shader program
		shaderProgramFree(&this->program);
		DVLB_Free(this->vertexShader_dvlb);
	}
	
	//------------------------------------------   Helper functions   ------------------------------------------
	
	std::shared_ptr<GameObject> Core::GetClosestObjectToPosition(C3D_FVec targetPosition, float maximumDistance){
		std::shared_ptr<GameObject> result = nullptr;
		//Setting the minimum distance value as the maximum distance value for accuracy.
		float minimumDistance = maximumDistance; 
		for (size_t i = 0; i < this->gameObjects.size(); i++){
			if (this->gameObjects[i]->debugFlag){
				//We skip game objects marked as debug objects. We don't want it to affect our calculations.
				continue;
			}
			float checkDistance = FVec4_Magnitude(FVec4_Subtract(this->gameObjects[i]->position, targetPosition));
			//It is rare for floating numbers to be equal to the other, but we put it there for math accuracy.
			if (checkDistance <= maximumDistance && checkDistance < minimumDistance){
				result = this->gameObjects[i];
				minimumDistance = checkDistance;
			}
		}
		return result;
	}
};
