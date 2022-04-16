#pragma once

//-----------------------------------------------------------------------------
#include "Font.h"
#include "VertexDeclaration.h"
#include "Key.h"

//-----------------------------------------------------------------------------
// Gui events (in comment is new gui meaning)
enum GuiEvent
{
	GuiEvent_GainFocus, // control get focus (old)
	GuiEvent_LostFocus, // control lost focus (old)
	GuiEvent_Moved, // control is moved
	GuiEvent_Resize, // control is resized
	GuiEvent_Show, // control is shown
	GuiEvent_WindowResize, // game window size change, only send to parent controls
	GuiEvent_Close, // window is closed (old)
	GuiEvent_Initialize, // send at control initialization
	GuiEvent_Hide, // control is hidden
	GuiEvent_LostMouseFocus, // control lost mouse focus
	GuiEvent_Custom
};

//-----------------------------------------------------------------------------
struct Hitbox
{
	Rect rect;
	int index, index2;
};

//-----------------------------------------------------------------------------
enum class HitboxOpen
{
	No,
	Yes,
	Group
};

//-----------------------------------------------------------------------------
struct HitboxContext
{
	vector<Hitbox>* hitbox;
	int counter, group_index, group_index2;
	HitboxOpen open;
	Rect region;
};

//-----------------------------------------------------------------------------
enum CursorMode
{
	CURSOR_NORMAL,
	CURSOR_HOVER,
	CURSOR_TEXT,
	CURSOR_MAX
};

//-----------------------------------------------------------------------------
enum GUI_DialogType
{
	DIALOG_OK,
	DIALOG_YESNO,
	DIALOG_CUSTOM
};

//-----------------------------------------------------------------------------
typedef delegate<void(int)> DialogEvent;
typedef delegate<void(int, int)> DialogEvent2;

//-----------------------------------------------------------------------------
enum DialogOrder
{
	ORDER_NORMAL,
	ORDER_TOP,
	ORDER_TOPMOST
};

//-----------------------------------------------------------------------------
struct DialogInfo
{
	DialogInfo() : custom_names(nullptr), img(nullptr), have_tick(false), ticked(false), auto_wrap(false), type(DIALOG_OK), parent(nullptr), order(ORDER_TOP), pause(true)
	{
	}

	string name, text;
	GUI_DialogType type;
	Control* parent;
	DialogEvent event;
	DialogOrder order;
	cstring* custom_names, tick_text;
	Texture* img;
	bool pause, have_tick, ticked, auto_wrap;
};

//-----------------------------------------------------------------------------
class OnCharHandler
{
public:
	virtual void OnChar(char c) {}
};

//-----------------------------------------------------------------------------
struct TextLine
{
	uint begin, end;
	int width;

	TextLine(uint begin, uint end, int width) : begin(begin), end(end), width(width)
	{
	}
};

//-----------------------------------------------------------------------------
namespace layout
{
	struct Gui;
}

//-----------------------------------------------------------------------------
// GUI
class Gui
{
public:
	Gui();
	~Gui();
	void Init();
	void SetText(cstring ok, cstring yes, cstring no, cstring cancel);
	void Draw(bool draw_layers = true, bool draw_dialogs = true);
	bool AddFont(cstring filename);
	Font* GetFont(cstring name, int size, int weight = 4, int outline = 0);
	/* zaawansowane renderowanie tekstu (w por�wnaniu do ID3DXFont)
	zwraca false je�eli by� clipping od do�u (nie kontuuj tekstu w flow)
	Znak $ oznacza jak�� specjaln� czynno�� (o ile jest ustawiona flaga DTF_PARSE_SPECIAL):
		$$ - wstaw $
		$c? - ustaw kolor (r-czerwony, g-zielony, y-��ty, b-czarny, w-bia�y, -przywr�c domy�lny)
		$h+ - informacja o hitboxie
		$h- - koniec hitboxa
		/$b - przerwa w tek�cie
		/$n - nie przerywaj tekstu a� do nast�pnego takiego symbolu (np $njaki� tekst$n - ten tekst nigdy nie zostanie rozdzielony pomi�dzy dwie linijki)
	*/
	bool DrawText(Font* font, Cstring str, uint flags, Color color, const Rect& rect, const Rect* clipping = nullptr,
		vector<Hitbox>* hitboxes = nullptr, int* hitbox_counter = nullptr, const vector<TextLine>* lines = nullptr);
	void Add(Control* ctrl);
	void DrawItem(Texture* t, const Int2& item_pos, const Int2& item_size, Color color, int corner = 16, int size = 64, const Box2d* clip_rect = nullptr);
	void Update(float dt, float mouse_speed);
	void DrawSprite(Texture* t, const Int2& pos, Color color = Color::White, const Rect* clipping = nullptr);
	void OnChar(char c);
	DialogBox* ShowDialog(const DialogInfo& info);
	void ShowDialog(DialogBox* dialog);
	bool CloseDialog(DialogBox* d);
	void CloseDialogInternal(DialogBox* d);
	bool HaveTopDialog(cstring name) const;
	bool HaveDialog() const;
	void DrawSpriteFull(Texture* t, Color color = Color::White);
	void DrawSpriteFullWrap(Texture* t, Color color = Color::White);
	void AddOnCharHandler(OnCharHandler* h) { on_char.push_back(h); }
	void RemoveOnCharHandler(OnCharHandler* h) { RemoveElement(on_char, h); }
	void SimpleDialog(cstring text, Control* parent, cstring name = "simple");
	void DrawSpriteRect(Texture* t, const Rect& rect, Color color = Color::White);
	bool HaveDialog(cstring name);
	bool HaveDialog(DialogBox* dialog);
	bool AnythingVisible() const;
	void OnResize();
	void DrawSpriteRectPart(Texture* t, const Rect& rect, const Rect& part, Color color = Color::White);
	void DrawSpriteTransform(Texture* t, const Matrix& mat, Color color = Color::White);
	void DrawLine(const Vec2& from, const Vec2& to, Color color = Color::Black, float width = 1.f);
	bool NeedCursor();
	bool DrawText3D(Font* font, Cstring text, uint flags, Color color, const Vec3& pos, Rect* text_rect = nullptr);
	bool To2dPoint(const Vec3& pos, Int2& pt);
	static bool Intersect(vector<Hitbox>& hitboxes, const Int2& pt, int* index, int* index2 = nullptr);
	void DrawSpriteTransformPart(Texture* t, const Matrix& mat, const Rect& part, Color color = Color::White);
	void CloseDialogs();
	bool HavePauseDialog() const;
	DialogBox* GetDialog(cstring name);
	void DrawSprite2(Texture* t, const Matrix& mat, const Rect* part = nullptr, const Rect* clipping = nullptr, Color color = Color::White);
	void DrawArea(Color color, const Int2& pos, const Int2& size, const Box2d* clip_rect = nullptr);
	void DrawArea(Color color, const Rect& rect, const Box2d* clip_rect = nullptr)
	{
		DrawArea(color, rect.LeftTop(), rect.Size(), clip_rect);
	}
	void SetLayout(Layout* layout);
	Layout* GetLayout() const { return master_layout; }
	void DrawArea(const Box2d& rect, const AreaLayout& area_layout, const Box2d* clip_rect = nullptr, Color* tint = nullptr);
	void SetOverlay(Overlay* overlay) { this->overlay = overlay; }
	Overlay* GetOverlay() const { return overlay; }
	bool MouseMoved() const { return cursor_pos != prev_cursor_pos; }
	void SetClipboard(cstring text);
	cstring GetClipboard();
	Rect GetSpriteRect(Texture* t, const Matrix& mat, const Rect* part = nullptr, const Rect* clipping = nullptr);
	void UseGrayscale(bool grayscale);
	struct DrawTextOptions
	{
		Font* font;
		cstring str;
		uint flags;
		Color color;
		Rect rect;
		const Rect* clipping;
		vector<Hitbox>* hitboxes;
		int* hitbox_counter;
		const vector<TextLine>* lines;
		Vec2 scale;
		uint lines_start;
		uint lines_end;
		uint str_length;

		DrawTextOptions(Font* font, Cstring str) : font(font), str(str), rect(Rect::Zero), flags(DTF_LEFT), color(Color::Black),
			clipping(nullptr), hitboxes(nullptr), hitbox_counter(nullptr), lines(nullptr), scale(Vec2::One), lines_start(0), lines_end(UINT_MAX),
			str_length(strlen(str))
		{
		}
	};
	bool DrawText2(DrawTextOptions& options);
	bool DoubleClick(Key key)
	{
		assert(key >= Key::LeftButton && key <= Key::X2Button);
		return doubleclk[(int)key - 1];
	}

	Matrix mViewProj;
	Int2 cursor_pos, prev_cursor_pos, wnd_size;
	CursorMode cursor_mode;
	cstring txOk, txYes, txNo, txCancel;
	Control* focused_ctrl;
	float mouse_wheel;

private:
	struct DrawLineContext
	{
		Font* font;
		HitboxContext* hc;
		cstring text;
		VGui* v;
		VGui* v2;
		uint inBuffer;
		uint inBuffer2;
		Vec4 defColor;
		Vec4 currentColor;
		Vec2 scale;
		bool parseSpecial;
	};

	void DrawTextLine(DrawLineContext& ctx, uint line_begin, uint line_end, int x, int y, const Rect* clipping);
	void DrawTextOutline(DrawLineContext& ctx, uint line_begin, uint line_end, int x, int y, const Rect* clipping);
	int Clip(int x, int y, int w, int h, const Rect* clipping);
	void SkipLine(cstring text, uint line_begin, uint line_end, HitboxContext* hc);
	void AddRect(VGui*& v, const Vec2& left_top, const Vec2& right_bottom, const Vec4& color);

	FontLoader* fontLoader;
	GuiShader* shader;
	vector<DialogBox*> created_dialogs;
	Container* layer, *dialog_layer;
	VGui vBuf[256 * 6], vBuf2[256 * 6];
	HitboxContext tmpHitboxContext;
	vector<OnCharHandler*> on_char;
	bool grayscale, doubleclk[5];
	Key lastClick;
	float lastClickTimer;
	Int2 lastClickPos;
	Layout* master_layout;
	layout::Gui* layout;
	Overlay* overlay;
};
