#pragma once

#include "Core.h"
#include "FileFormat.h"

// Engine types
struct Billboard;
struct Camera;
struct DynamicTexture;
struct Light;
struct Lights;
struct MeshInstance;
struct ParticleEmitter;
struct PostEffect;
struct Scene;
struct SceneBatch;
struct SceneNode;
struct SceneNodeGroup;
struct TaskData;
struct Terrain;
struct TrailParticleEmitter;
class App;
class BasicShader;
class CustomCollisionWorld;
class Engine;
class GlowShader;
class GrassShader;
class Gui;
class GuiShader;
class Input;
class Pak;
class ParticleShader;
class Physics;
class PostfxShader;
class Render;
class RenderTarget;
class ResourceManager;
class SceneManager;
class ShaderHandler;
class SkyboxShader;
class SoundManager;
class SuperShader;
class TerrainShader;

// Resource types
struct Font;
struct Mesh;
struct Music;
struct Resource;
struct Sound;
struct Texture;
struct VertexData;
typedef Font* FontPtr;
typedef Mesh* MeshPtr;
typedef Music* MusicPtr;
typedef Sound* SoundPtr;
typedef Texture* TexturePtr;
typedef VertexData* VertexDataPtr;

// Gui types
struct AreaLayout;
struct Notification;
class Button;
class CheckBox;
class Container;
class Control;
class DialogBox;
class DrawBox;
class GuiDialog;
class GuiElement;
class Label;
class Layout;
class ListBox;
class MenuBar;
class MenuList;
class MenuStrip;
class Notifications;
class Overlay;
class Panel;
class PickItemDialog;
class TextBox;
class TreeView;

// Windows types
struct HWND__;
typedef HWND__* HWND;

// DirectX types
struct _D3DPRESENT_PARAMETERS_;
struct _D3DXMACRO;
struct ID3DXEffect;
struct ID3DXEffectPool;
struct ID3DXFont;
struct ID3DXMesh;
struct ID3DXSprite;
struct IDirect3D9;
struct IDirect3DDevice9;
struct IDirect3DIndexBuffer9;
struct IDirect3DSurface9;
struct IDirect3DTexture9;
struct IDirect3DVertexBuffer9;
struct IDirect3DVertexDeclaration9;
typedef _D3DPRESENT_PARAMETERS_ D3DPRESENT_PARAMETERS;
typedef _D3DXMACRO D3DXMACRO;
typedef const char* D3DXHANDLE;
typedef ID3DXFont* FONT;
typedef IDirect3DIndexBuffer9* IB;
typedef IDirect3DSurface9* SURFACE;
typedef IDirect3DTexture9* TEX;
typedef IDirect3DVertexBuffer9* VB;

// FMod types
namespace FMOD
{
	class Channel;
	class ChannelGroup;
	class Sound;
	class System;
}
typedef FMOD::Sound* SOUND;

// Bullet physics types
class btBvhTriangleMeshShape;
class btCollisionObject;
class btCollisionShape;
class btHeightfieldTerrainShape;
class btTriangleIndexVertexArray;

// Globals
namespace app
{
	extern App* app;
	extern Engine* engine;
	extern Gui* gui;
	extern Input* input;
	extern Render* render;
	extern ResourceManager* res_mgr;
	extern SceneManager* scene_mgr;
	extern SoundManager* sound_mgr;
}

// Misc functions
void RegisterCrashHandler(cstring title, cstring version, cstring url, cstring log_file, int minidump_level);
