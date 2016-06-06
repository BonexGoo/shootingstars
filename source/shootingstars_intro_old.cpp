#include <BxScene.hpp>
#include <BxImage.hpp>

class shootingstars_intro_old
{
public:
	BxImage BGImage;
	BxImage BearImage;
	BxImage Bear1Image;
	BxImage Bear2Image;
	int BearAni;

public:
	shootingstars_intro_old()
	{
		BGImage.Load("image/dark_bg.png", BxImage::PNG, BxImage::PAD5);
		BearImage.Load("image/bear.png", BxImage::PNG);
		Bear1Image.Load("image/bear1.png", BxImage::PNG, BxImage::PAD2);
		Bear2Image.Load("image/bear2.png", BxImage::PNG, BxImage::PAD2);
		BearAni = 0;
	}

	~shootingstars_intro_old()
	{
	}
};

FRAMEWORK_SCENE_HIDDEN(shootingstars_intro_old, "shootingstars_intro_old")

syseventresult OnEvent(shootingstars_intro_old& This, const sysevent& Event)
{
	if(Event.type == syseventtype_touch && Event.touch.type == systouchtype_up)
	{
		This.BearAni = 0;
	}
	else if(Event.type == syseventtype_button && Event.button.type == sysbuttontype_up)
	{
	}
	return syseventresult_pass;
}

sysupdateresult OnUpdate(shootingstars_intro_old& This)
{
	++This.BearAni;
	return sysupdateresult_do_render;
}

void OnRender(shootingstars_intro_old& This, BxDraw& Draw)
{
    Draw.Rectangle(FILL, Draw.CurrentRect(), COLOR(0, 0, 0));
	Draw.Area(Draw.Width() / 2, Draw.Height() / 2, FORM(&This.BGImage));

	if(This.BearAni < 450)
	{
		const int PosV = Max(0, This.BearAni - 400);
		const int PosV2 = PosV * PosV / 5;

		// Drawing Stars
		if(170 < This.BearAni)
		{
			const int BearAni = This.BearAni - 170;
			const int BearAni2 = Max(0, 50 - BearAni);
			const int BearAni3 = Max(0, 80 - BearAni);
			const int BearAni4 = FtoI(BxUtil::Cos(500 + BearAni * 20) * Sqrt(BearAni3) * 3 / 2);
			Draw.Area(Draw.Width() / 2, Draw.Height() - 290 + BearAni2 * BearAni2 / 10 + BearAni4 + PosV2,
				FORM(&This.Bear1Image));
		}

		// Look at the Stars!
		if(230 < This.BearAni)
		{
			const int BearAni = This.BearAni - 230;
			const int BearAni2 = Min(BearAni * 3, 255);
			Draw.Area(Draw.Width() / 2, Draw.Height() - 300 + PosV2, FORM(&This.Bear2Image) >> OPACITY(BearAni2));
		}

		// 곰
		const int BearAni = Max(0, 200 - This.BearAni);
		const int PosW = This.BearImage.Width();
		const int PosH = This.BearImage.Height();
		const int PosX = PosW / 2;
		const int PosY = PosH / 2;
		const int Ani = BearAni * BearAni / 200;
		const int AniY = Abs((Max(15, BearAni) % 30) - 15);
		const rect AreaRect = XYWH(
			Draw.Width() / 2 - PosX * (200 - Ani) / 200,
			Draw.Height() - (PosH - AniY * 3 / 2) * (200 - Ani) / 200 + PosV2,
			PosW * (200 - Ani) / 200,
			PosH * (200 - Ani) / 200);
		Draw.Rectangle(FILL, AreaRect, FORM(&This.BearImage));
		Draw.Rectangle(FILL, AreaRect, FORM(&This.BearImage)
			>> COLOR(0, 0, 0) >> OPACITY(255 * Max(0, Ani - 150) / 50));
	}
}
