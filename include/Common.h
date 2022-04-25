#pragma once

// Enums
enum VertexDeclarationId;
enum class ImageFormat;

// Engine types
struct Billboard;
struct Camera;
struct Decal;
struct DebugNode;
struct DynamicTexture;
struct GlowNode;
struct Light;
struct Lights;
struct MeshInstance;
struct ParticleEmitter;
struct PhysicsDrawer;
struct PostEffect;
struct Scene;
struct SceneBatch;
struct SceneNode;
struct SceneNodeGroup;
struct SimpleMesh;
struct TaskData;
struct Terrain;
struct TrailParticleEmitter;
class App;
class BasicShader;
class Engine;
class FontLoader;
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
struct MusicList;
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
union Cell;
class Button;
class CheckBox;
class Container;
class Control;
class DialogBox;
class DrawBox;
class Grid;
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
struct _GUID;
typedef void* HANDLE;
typedef HWND__* HWND;
typedef _GUID GUID;

// DirectX types
struct _D3D_SHADER_MACRO;
struct D3D11_INPUT_ELEMENT_DESC;
struct ID3D11BlendState;
struct ID3D10Blob;
struct ID3D11Buffer;
struct ID3D11DepthStencilState;
struct ID3D11DepthStencilView;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11InputLayout;
struct ID3D11PixelShader;
struct ID3D11RasterizerState;
struct ID3D11RenderTargetView;
struct ID3D11SamplerState;
struct ID3D11ShaderResourceView;
struct ID3D11Texture2D;
struct ID3D11VertexShader;
struct IDXGIAdapter;
struct IDXGIFactory;
struct IDXGISwapChain;
typedef _D3D_SHADER_MACRO D3D_SHADER_MACRO;
typedef ID3D10Blob ID3DBlob;
typedef ID3D11ShaderResourceView* TEX;

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
class btGhostPairCallback;
class btHeightfieldTerrainShape;
class btTriangleIndexVertexArray;

// Globals
namespace app
{
	extern App* app;
	extern Engine* engine;
	extern Gui* gui;
	extern Input* input;
	extern Physics* physics;
	extern Render* render;
	extern ResourceManager* res_mgr;
	extern SceneManager* scene_mgr;
	extern SoundManager* sound_mgr;
}
