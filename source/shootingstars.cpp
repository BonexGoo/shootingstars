#include <BxScene.hpp>
#include <BxImage.hpp>
#include "adapter.hpp"
#include "command.hpp"
#include "export.hpp"
#include "input.hpp"
#include "shootingstars.hpp"

BxImage shootingstars::SkyBGImage;
BxImage shootingstars::DrawBGImage;
BxImage shootingstars::DarkBGImage;

shootingstars::shootingstars()
{
	ViewOx = 0;
	ViewOy = 0;
	BxUtil::SetRandom((uint) BxCore::System::GetTimeMilliSecond());
	for(int i = 0; i < 12; ++i)
	{
		BxString Path1("<>:ShootingStars.Map.<A>.", BxARG(i));
		StarPos[i].x = BxCore::System::GetConfigNumber(Path1 + "OX", 0) - 1920 / 2;
		StarPos[i].y = BxCore::System::GetConfigNumber(Path1 + "OY", 0) - 1080 / 2;
		const int Count = BxCore::System::GetConfigNumber(Path1 + "Count", 0);
		for(int j = 0; j < Count; ++j)
		{
			BxString Path2("<>:<A>Planet.<A>.", BxARG(Path1, j));
			PlanetPos[i][j].x = BxCore::System::GetConfigNumber(Path2 + "PX", 0);
			PlanetPos[i][j].y = BxCore::System::GetConfigNumber(Path2 + "PY", 0);
			PlanetPos[i][j].r = BxCore::System::GetConfigNumber(Path2 + "PS", 0);
			PlanetPos[i][j].a = BxUtil::GetValue(0, 1000);
		}
	}

	if(!SkyBGImage.IsExist()) SkyBGImage.Load("image/sky_bg.png", BxImage::PNG);
	if(!DrawBGImage.IsExist()) DrawBGImage.Load("image/draw_bg.png", BxImage::PNG);
	if(!DarkBGImage.IsExist()) DarkBGImage.Load("image/dark_bg.png", BxImage::PNG);

	EffectFocus = EFFECT_LEN - 1;
	EffectAni = 0;
}

shootingstars::~shootingstars()
{
}

void shootingstars::DrawImage(BxDraw& Draw, BxImage& Img,
	float l, float t, float r, float b, const Zoom& zoom, float vx, float vy)
{
	Rect ViewRect = Rect(l, t, r, b) * zoom;
	Rect ImageRect = Rect(0, 0, Img.Width(), Img.Height());
	const float XRate = ViewRect.Width() / Img.Width();
	const float YRate = ViewRect.Height() / Img.Height();
	if(ViewRect.l + XRate < -vx)
	{
		const int Count = (int) (-vx - ViewRect.l) / XRate;
		ViewRect.l += XRate * Count;
		ImageRect.l += Count;
	}
	if(ViewRect.t + YRate < -vy)
	{
		const int Count = (int) (-vy - ViewRect.t) / YRate;
		ViewRect.t += YRate * Count;
		ImageRect.t += Count;
	}
	if(vx < ViewRect.r - XRate)
	{
		const int Count = (int) (ViewRect.r - vx) / XRate;
		ViewRect.r -= XRate * Count;
		ImageRect.r -= Count;
	}
	if(vy < ViewRect.b - YRate)
	{
		const int Count = (int) (ViewRect.b - vy) / YRate;
		ViewRect.b -= YRate * Count;
		ImageRect.b -= Count;
	}
	Draw.Rectangle(FILL, XYXY(ViewRect.l, ViewRect.t, ViewRect.r, ViewRect.b),
		FORM(&Img, XYXY(ImageRect.l, ImageRect.t, ImageRect.r, ImageRect.b)));
}

void shootingstars::DrawStroke(BxDraw& Draw, int Doc, const float AddX, const float AddY)
{
	struct StripData
	{
		float force;
		float lx;
		float ly;
		float rx;
		float ry;
	};
	static int StripCount = 1;
	static StripData* Strips = new StripData[StripCount];

	const Zoom DocZoom = Command::Canvas::GetZoom(Doc);
	const Rect DocArea = Command::Canvas::GetArea(Doc);
	const Rect ViewRect = DocArea * DocZoom;
	linkstep1 doclink = Adapter::GetDocumentLink(Doc);
	if(doclink)
	BxTRY(Draw, CLIP(XYXY(ViewRect.l, ViewRect.t, ViewRect.r, ViewRect.b)))
	for(int l = 0, lend = Adapter::GetLayerCount(doclink); l < lend; ++l)
	{
		linkstep2 layerlink = Adapter::GetLayerLink(doclink, l);
		if(!layerlink || !Adapter::IsLayerVisibled(layerlink))
			continue;
		for(int s = 0, send = Adapter::GetShapeCount(layerlink, false); s < send; ++s)
		{
			linkstep3 shapelink = Adapter::GetShapeLink(layerlink, false, s);
			if(!shapelink) continue;
			if(CONST_STRING("PlatyVGElement::MeshAqua") == Adapter::GetShapeMeshType(shapelink))
			{
				// 울렁울렁 구현
				const int Count = Adapter::GetShapeMeshCount(shapelink);
				const StripData* Datas = (const StripData*) Adapter::GetShapeMeshArray(shapelink);
				if(StripCount < Count)
				{
					StripCount = Count;
					delete[] Strips;
					Strips = new StripData[StripCount];
				}
				for(int i = 0; i < Count; ++i)
				{
					Strips[i].force = Datas[i].force;
					const float LAdd = 4 * Px::Math::Cos((Datas[i].lx + Datas[i].ly) / 16 + EffectAni);
					Strips[i].lx = Datas[i].lx + LAdd;
					Strips[i].ly = Datas[i].ly + LAdd;
					const float RAdd = 4 * Px::Math::Cos((Datas[i].rx + Datas[i].ry) / 16 + EffectAni);
					Strips[i].rx = Datas[i].rx + RAdd;
					Strips[i].ry = Datas[i].ry + RAdd;
				}

				// 랜더링
				color_x888 ShapeColor = RGB32(
					Adapter::GetShapeColorRed(shapelink),
					Adapter::GetShapeColorGreen(shapelink),
					Adapter::GetShapeColorBlue(shapelink));
				BxCore::OpenGL2D::RenderStripDirectly(
					Count, (const void*) Strips,
					Adapter::GetShapeColorAlpha(shapelink),
					Adapter::GetShapeColorAqua(shapelink), ShapeColor,
					DocZoom.offset.x + AddX, DocZoom.offset.y + AddY, DocZoom.scale,
					Adapter::GetShapeMatrixM11(shapelink),
					Adapter::GetShapeMatrixM12(shapelink),
					Adapter::GetShapeMatrixM21(shapelink),
					Adapter::GetShapeMatrixM22(shapelink),
					Adapter::GetShapeMatrixDx(shapelink),
					Adapter::GetShapeMatrixDy(shapelink));
			}
		}
	}
}

shootingstars::Effect& shootingstars::GetNewEffect()
{
	EffectFocus = (EffectFocus + 1) % EFFECT_LEN;
	return ObjectEffect[EffectFocus];
}

void shootingstars::RunEffect()
{
	for(int i = 0; i < EFFECT_LEN; ++i)
	{
		if(!ObjectEffect[i].IsVisible) continue;
		ObjectEffect[i].pos.x += ObjectEffect[i].vec.x;
		ObjectEffect[i].pos.y += ObjectEffect[i].vec.y;
		if((ObjectEffect[i].ani -= 0.033f) < 0)
			ObjectEffect[i].IsVisible = false;
	}
	EffectAni += 0.1f;
}
