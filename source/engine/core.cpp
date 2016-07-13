#include "core.h"

namespace Engine {
	Core::Core(Output* out) {
		//Setting up/Initializing all variables.
		this->output = out;
		this->distZ = 0.0;
		this->uLoc_projection = 0;
		this->uLoc_model = 0;
		this->uLoc_view = 0;
		this->leftTarget = nullptr;
		this->rightTarget = nullptr;
		this->vertexShader_dvlb = nullptr;
		
		float radius = 3.0f;
		
		//Loading entities.
		for (int i = 0; i < 2; i++){
			Entity t(vertexList, vertexListSize);
			t.SetAngleXSpeed(1.0f * i + 1.0f);
			t.SetPositionX(-radius);
			t.SetPositionY(0.0f);
			this->entityList.push_back(t);
		}
		
		//Initializing core engine.
		this->Initialize();
	}
	
	Core::~Core(){
		this->output->Print("Deinitializing core.");
		
		//Exiting scene
		this->SceneExit();
		
		//Deinitializing graphics
		C3D_Fini();
	}
	
	void Core::Initialize(){
		this->output->Print("Initializing core.");
		
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
		
		//Scene initialization
		this->SceneInitialize();
	}
	
	void Core::SceneInitialize(){
		this->output->Print("Initializing scene.");
		
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
		
		C3D_FVec lightVector = {{ 6.0, 0.5, 0.0, 0.0 }};
		
		C3D_LightInit(&this->light, &this->lightEnvironment);
		C3D_LightColor(&this->light, 1.0, 1.0, 1.0);
		C3D_LightPosition(&this->light, &lightVector);
	}
	
	void Core::SceneRender(float interOcularDistance){
		//Compute projection matrix and update matrix to shader program.                                                                                                               
		Mtx_PerspStereoTilt(&this->projectionMatrix, 40.0f * (std::acos(-1) / 180.0f), 400.0f / 240.0f, 0.01f, 1000.0f, interOcularDistance, 2.0f);
		C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, this->uLoc_projection, &this->projectionMatrix);
		                                                   
		//Compute view matrix and update matrix to shader program.
		C3D_Mtx viewMatrix;
		Mtx_Identity(&viewMatrix);
		Mtx_Translate(&viewMatrix, 0.0, 0.0, -10.0 + this->distZ);
		C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, this->uLoc_view, &viewMatrix);
		
		//Declaring reusable model matrix.
		C3D_Mtx modelMatrix;
		
		//Draw the vertex buffer objects.                     
		for (size_t i = 0; i < this->entityList.size(); i++){ 
			//Switch buffers
			this->entityList[i].ConfigureBuffer();
		
			//Calculate model view matrix.
			Mtx_Identity(&modelMatrix);
			this->entityList[i].RenderUpdate(&modelMatrix);
			
			//Update to shader program.
			C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, this->uLoc_model, &modelMatrix);
			
			//Render entity.
			this->entityList[i].Render();
		}
	}
	
	void Core::SceneExit(){
		this->output->Print("Exiting scene.");
		
		//Free shader program
		shaderProgramFree(&this->program);
		DVLB_Free(this->vertexShader_dvlb);
	}
	
	void Core::Update(u32 keyDown, u32 keyHeld, u32 keyUp){
		//This is for game logic, and not for updating game render updates.
		if (keyDown & KEY_L){
			this->distZ--;
		}
		else if (keyDown & KEY_R){
			this->distZ++;
		}
		else if (keyDown & KEY_B){;
			this->output->SetReverseFlag(!this->output->GetReverseFlag());
			this->output->PrintAll();
		}
		
		//Entity updates go here.
		for (size_t i = 0; i < this->entityList.size(); i++){
			this->entityList[i].Update();
		}
	}
	
	void Core::Render() {
		//Fetch Stereoscopic 3D level.
		float slider = osGet3DSliderState();
		//Inter Ocular Distance
		float iod = slider / 3.0f;
		
		//Rendering scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		{
			C3D_FrameDrawOn(this->leftTarget);
			this->SceneRender(-iod);
			if (iod > 0.0f){
				C3D_FrameDrawOn(this->rightTarget);
				this->SceneRender(iod);
			}
		}
		C3D_FrameEnd(0);
	}
	
	Output* Core::GetOutput() const {
		//This is only for fetching the Console.
		return this->output;
	}
};
