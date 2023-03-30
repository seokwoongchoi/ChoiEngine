#pragma once

#define MAX_SKELETAL_ACTOR_COUNT 3
//#ifdef _DEBUG
//#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
//#endif

#include <Windows.h>

#include <assert.h>

//STL
#include <fstream>      // std::ifstream
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <functional>
#include <iterator>
#include <thread>
#include <mutex>
#include <queue>
#include <algorithm>
#include <chrono>
#include <utility>
#include <process.h>


using namespace std;

//Direct3D

#include <d3dcompiler.h>
#include <d3d11_4.h>
#include <d3dx10math.h>
#include <d3dx11async.h>

//#pragma comment(lib,"D3D12.lib");
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx10.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "d3dcompiler.lib")
//#pragma comment(lib, "dxguid.lib")

//DirectXTex
#include <DirectXTex.h>
#pragma comment(lib, "directxtex.lib")

#define Check(hr) { assert(SUCCEEDED(hr)); }


#define SafeRelease(p){ if(p){ (p)->Release(); (p) = NULL; } }
#define SafeDelete(p){ if(p){ delete (p); (p) = NULL; } }
#define SafeDeleteArray(p){ if(p){ delete [] (p); (p) = NULL; } }
typedef unsigned int uint;
typedef unsigned long ulong;

typedef D3DXVECTOR2 Vector2;
typedef D3DXVECTOR3 Vector3;
typedef D3DXVECTOR4 Vector4;

typedef D3DXCOLOR Color;
typedef D3DXMATRIX Matrix;
typedef D3DXQUATERNION Quaternion;

typedef D3DXPLANE Plane;

//Math
#include "./Math/Math.h"

//Global Data
#include "GlobalDatas/GlobalData.h"
//Mainsystem
#include "Core/D3D11/D3D.h"


//SubSystems
#include "Systems/Keyboard.h"
#include "Systems/Mouse.h"
#include "Systems/Time.h"
#include "Systems/Thread.h"
//Viewer
#include "Viewer/Viewport.h"
#include "Viewer/Camera.h"


//D3D11 Wrapper Class
#include "Renders/BasicData/Geometry.h"
#include "Renders/BasicData/Vertex.h"
#include "Renders/InputLayout.h"

//Utillity
#include "Utility/GeometryUtility.h"
#include "Utility/BinaryFile.h"
#include "Utility/Path.h"
//#include "Utility/FileStream.h"
//#include "Utility/FileSystem.h"
#include "Utility/String.h"


//Resource
#include "Resources/Textures.h"
#include "Resources/Material.h"