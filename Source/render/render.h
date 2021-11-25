#pragma once

#include <d3d11.h>

#define SAFE_RELEASE(x)  if(x) { x->Release(); x = nullptr; }
#define SAFE_DESTROY(x)  if(x) { x->destroy(); x = nullptr; }

// To assign a name to each DX object
#define setDXName(dx_obj,new_name) \
        { HRESULT nameSet = (dx_obj)->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(new_name), new_name); \
		assert(SUCCEEDED(nameSet)); }

class CRender {

	IDXGISwapChain* swap_chain = nullptr;
	ID3D11Texture2D* depth_stencil = nullptr;
	ID3D11Texture2D* depth_stencil_irradiance = nullptr;
	int                     width = 0;
	int                     height = 0;
	bool					  fullscreen;
	bool					  vsync;

	ID3D11Debug* debug;

	bool createDevice(HWND hWnd);
	bool createDepthBuffer();

public:

	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* ctx = nullptr;
	ID3D11RenderTargetView* render_target_view = nullptr;
	ID3D11DepthStencilView* depth_stencil_view = nullptr;
	ID3D11DepthStencilView* depth_stencil_view_irradiance = nullptr;
	// To allow shader access the depth data assocaited to the texture depth_stencil
	ID3D11ShaderResourceView* depth_srv = nullptr;
	ID3D11ShaderResourceView* depth_srv_irradiance = nullptr;

	bool create(HWND hWnd, bool fullscreen, bool vsync);
	void destroy();

	int getWidth() const { return width; }
	int getHeight() const { return height; }

	void activateBackBuffer();
	void swapFrames();
	void setFullScreenMode(bool mode);
	void setVSyncEnabled(bool v) { vsync = v; };
	void toggleFullScreen();
};

extern CRender Render;
///#define Render CRender::get()

#include "render/gpu_trace.h"
#include "render/colors.h"
#include "render/vertex_declaration.h"
#include "render/shaders/pixel_shader.h"
#include "render/shaders/vertex_shader.h"
#include "render/meshes/mesh.h"
#include "render/shaders/pipeline_state.h"
#include "render/shaders/shader_cte.h"
#include "../bin/data/shaders/constants.h"
#include "render/textures/texture.h"
#include "render/textures/material.h"
