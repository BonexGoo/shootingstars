#define DOC_WIDTH_HALF 1000
#define DOC_HEIGHT_HALF 500

class shootingstars
{
public:
	float ViewOx;
	float ViewOy;
	point StarPos[12];

	class Planet
	{
	public:
		int x;
		int y;
		int r;
		int a;
	public:
		Planet()
		{
			x = 0;
			y = 0;
			r = 0;
			a = 0;
		}
		~Planet() {}
	};
	BxVarVector<Planet, 4> PlanetPos[12];

	global_data BxImage SkyBGImage;
	global_data BxImage DrawBGImage;
	global_data BxImage DarkBGImage;

	class Effect
	{
	public:
		bool IsVisible;
		Point pos;
		Point vec;
		float ani;
	public:
		Effect()
		{
			IsVisible = false;
			ani = 1;
		}
		~Effect() {}
	};
	enum {EFFECT_LEN = 30};
	Effect ObjectEffect[EFFECT_LEN];
private:
	int EffectFocus;
	float EffectAni;

public:
	shootingstars();
	virtual ~shootingstars();
	void DrawImage(BxDraw& Draw, BxImage& Img,
		float l, float t, float r, float b, const Zoom& zoom, float vx, float vy);
	void DrawStroke(BxDraw& Draw, int Doc, const float AddX, const float AddY);
	Effect& GetNewEffect();
	void RunEffect();
};
