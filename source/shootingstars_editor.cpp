#include <BxScene.hpp>
#include <BxImage.hpp>
#include "adapter.hpp"
#include "command.hpp"
#include "input.hpp"
#include "export.hpp"
#include "shootingstars.hpp"

#define DEBUGLINE false

class shootingstars_editor : public shootingstars
{
public:
	int MapID;
	int Doc;
	int SelectID;
	int SelectIDAni;
	BxImage HomeImage;
	BxImage UploadImage;
	BxImage EraserImage;
	BxImage EraserEffImage;
	BxImage StarPenImage[3];
	BxImage StarPenEffImage;
	BxImage StarTitleImage;
	BxImage ObjectImage;
	BxImage StarImage;
	BxImage EffectImage;
	BxImage PlanetImage;
	int AniFrame;
	int UploadStep;
	const int UploadFramePerStroke;

	id_socket Sock;

public:
	shootingstars_editor() : UploadFramePerStroke(5)
	{
		MapID = 0;
		Doc = -1;
		SelectID = 0;
		SelectIDAni = 100;
		HomeImage.Load("image/home.png", BxImage::PNG, BxImage::PAD7);
		UploadImage.Load("image/upload.png", BxImage::PNG, BxImage::PAD9);
		EraserImage.Load("image/eraser.png", BxImage::PNG, BxImage::PAD1);
		EraserEffImage.Load("image/eraser_eff.png", BxImage::PNG, BxImage::PAD1);
		for(int i = 0; i < 3; ++i)
			StarPenImage[i].Load(BxString("<>:image/starpen_<A:03>.png", BxARG(i)),
				BxImage::PNG, BxImage::PAD3);
		StarPenEffImage.Load("image/starpen_eff.png", BxImage::PNG, BxImage::PAD3);
		StarImage.Load("image/star.png", BxImage::PNG, BxImage::PAD5);
		EffectImage.Load("image/effect.png", BxImage::PNG, BxImage::PAD5);
		PlanetImage.Load("image/planet.png", BxImage::PNG);
		AniFrame = 0;
		UploadStep = -1;

		Sock = BxCore::Socket::Create();
		string Domain = BxCore::System::GetConfigString("ShootingStars.Domain", "localhost");
		BxCore::Socket::Connect(Sock, Domain, 6566);
	}

	~shootingstars_editor()
	{
		BxCore::Socket::Release(Sock);
	}
};

FRAMEWORK_SCENE_HIDDEN(shootingstars_editor, "shootingstars_editor")

syseventresult OnEvent(shootingstars_editor& This, const sysevent& Event)
{
	/*좌표따기: static bool IsTestMode = false;
	if(Event.type == syseventtype_key)
	{
		if(Event.key.code == keykind_space)
		{
			if(Event.key.type == syskeytype_down)
				IsTestMode = true;
			else if(Event.key.type == syskeytype_up)
				IsTestMode = false;
		}
	}
	else*/
	if(Event.type == syseventtype_init)
	{
		This.ViewOx = Event.init.w / 2.0f;
		This.ViewOy = Event.init.h / 2.0f;
		Command::SetViewRadius(Math::Sqrt(This.ViewOx * This.ViewOx + This.ViewOy * This.ViewOy));

		if(StrSameCount(Event.init.message, "MAPID:") == 6)
			This.MapID = AtoI(Event.init.message + 6);
		This.StarTitleImage.Load(BxString("<>:image/startitle_<A:03>.png", BxARG(This.MapID)), BxImage::PNG, BxImage::PAD7);
		This.ObjectImage.Load(BxString("<>:image/object_<A:03>.png", BxARG(This.MapID)), BxImage::PNG);

		This.Doc = Command::Canvas::Create(true);
		Command::Canvas::SetArea(This.Doc, Rect(-DOC_WIDTH_HALF, -DOC_HEIGHT_HALF, DOC_WIDTH_HALF, DOC_HEIGHT_HALF));
		Command::Canvas::SetZoomInRect(This.Doc, Rect(-This.ViewOx, -This.ViewOy, This.ViewOx, This.ViewOy), BoundByOuter);
		Command::Canvas::SetFocus(This.Doc);

		Command::SetThick(20);
		Input::SetDeviceSpeedyForceOn(DeviceByTouch, 500);

		Command::SetStrokeBy("aqua");
		Command::SetColor(Color(255, 49, 0, 180, 160));
	}
	else if(Event.type == syseventtype_resize)
	{
		This.ViewOx = Event.resize.w / 2.0f;
		This.ViewOy = Event.resize.h / 2.0f;
		Command::SetViewRadius(Math::Sqrt(This.ViewOx * This.ViewOx + This.ViewOy * This.ViewOy), true);
	}
	else if(Event.type == syseventtype_quit)
	{
		Command::Canvas::Remove(This.Doc);
	}
	else if(Event.type == syseventtype_touch)
	{
		//좌표따기: static float SavedX = 0;
		//좌표따기: static float SavedY = 0;
		if(This.UploadStep == -1)
		{
			const float X = Event.touch.fx - This.ViewOx;
			const float Y = Event.touch.fy - This.ViewOy;
			Command::SetCurrentTime(BxCore::System::GetTimeMilliSecond());
			switch(Event.touch.type)
			{
			case systouchtype_down:
				Input::ScreenDown(Event.touch.id, X, Y, Event.touch.force, DeviceByTouch);
				/*좌표따기: if(IsTestMode)
				{
					const Zoom DocZoom = Command::Canvas::GetZoom(This.Doc);
					SavedX = X / DocZoom.scale - DocZoom.offset.x;
					SavedY = Y / DocZoom.scale - DocZoom.offset.y;
					BxTRACE("<>:[@5=<A> @6=<A>",
						BxARG((int) (SavedX + 0.5), (int) (SavedY + 0.5)));
				}*/
				break;
			case systouchtype_move:
				Input::ScreenDrag(Event.touch.id, X, Y, Event.touch.force, DeviceByTouch);
				break;
			case systouchtype_up: default:
				Input::ScreenUp(Event.touch.id, DeviceByTouch);
				/*좌표따기: if(IsTestMode)
				{
					const Zoom DocZoom = Command::Canvas::GetZoom(This.Doc);
					const float DistX = (X / DocZoom.scale - DocZoom.offset.x) - SavedX;
					const float DistY = (Y / DocZoom.scale - DocZoom.offset.y) - SavedY;
					const float Dist = Math::Sqrt(DistX * DistX + DistY * DistY);
					BxTRACE("<>:@7=<A>]", BxARG((int) (Dist + 0.5)));
				}*/
				break;
			}
			// 이펙트 추가
			if(This.SelectID != -1)
			if(Event.touch.type == systouchtype_down || Event.touch.type == systouchtype_move)
			{
				static uint SeedValue = 0;
				BxUtil::SetRandom(++SeedValue);
				if(BxUtil::GetValue(0, 99) < 20)
				{
					const int Count = BxUtil::GetValue(1, 2);
					for(int i = 0; i < Count; ++i)
					{
						shootingstars::Effect& CurEffect = This.GetNewEffect();
						CurEffect.IsVisible = true;
						CurEffect.pos.x = Event.touch.fx;
						CurEffect.pos.y = Event.touch.fy;
						const int Angle = BxUtil::GetValue(0, 1023);
						const int Speed = BxUtil::GetValue(2, 6);
						CurEffect.vec.x = Speed * BxUtil::Cos(Angle) / (1 << 16);
						CurEffect.vec.y = Speed * BxUtil::Sin(Angle) / (1 << 16);
						CurEffect.ani = 1;
					}
				}
			}
		}
		return syseventresult_done;
	}
	else if(Event.type == syseventtype_button && Event.button.type == sysbuttontype_up)
	{
		if(StrCmp(Event.button.name, "ERASER") == same)
		{
			if(This.SelectID != -1)
			{
				This.SelectID = -1;
				This.SelectIDAni = 0;
				Command::SetStrokeBy("erase");
			}
		}
		else if(StrCmp(Event.button.name, "PEN0") == same)
		{
			if(This.SelectID != 0)
			{
				This.SelectID = 0;
				This.SelectIDAni = 0;
				Command::SetStrokeBy("aqua");
				Command::SetColor(Color(255, 49, 0, 180, 160));
			}
		}
		else if(StrCmp(Event.button.name, "PEN1") == same)
		{
			if(This.SelectID != 1)
			{
				This.SelectID = 1;
				This.SelectIDAni = 0;
				Command::SetStrokeBy("aqua");
				Command::SetColor(Color(216, 1, 255, 180, 160));
			}
		}
		else if(StrCmp(Event.button.name, "PEN2") == same)
		{
			if(This.SelectID != 2)
			{
				This.SelectID = 2;
				This.SelectIDAni = 0;
				Command::SetStrokeBy("aqua");
				Command::SetColor(Color(255, 198, 2, 180, 160));
			}
		}
		else if(StrCmp(Event.button.name, "GOSKY") == same)
		{
			BxScene::SetRequest("shootingstars_server", sceneside_center);
			BxScene::SetRequest("shootingstars_editor", sceneside_down);
		}
		else if(This.UploadStep == -1)
		{
			if(StrCmp(Event.button.name, "HOME") == same)
			{
				BxScene::AddRequest("shootingstars_intro", sceneside_left);
				BxScene::SubRequest("shootingstars_editor", sceneside_right);
			}
			else if(StrCmp(Event.button.name, "UPLOAD") == same)
			{
				This.UploadStep = 0;
			}
		}
	}
	return syseventresult_pass;
}

sysupdateresult OnUpdate(shootingstars_editor& This)
{
	if(This.SelectIDAni < 100) ++This.SelectIDAni;
	++This.AniFrame;

	if(-1 < This.UploadStep)
	{
		if(This.UploadStep == 0)
		{
			const int PacketSize = 8;
			BxCore::Socket::Send(This.Sock, &PacketSize, 4);
			BxCore::Socket::Send(This.Sock, "CLS_", 4);
			BxCore::Socket::Send(This.Sock, &This.MapID, 4);
		}
		else if((This.UploadStep % This.UploadFramePerStroke) == 0)
		{
			bool HasFoundShape = false;
			int iCount = This.UploadStep / This.UploadFramePerStroke;
			linkstep1 doclink = Adapter::GetDocumentLink(This.Doc);
			if(doclink)
			for(int l = 0, lend = Adapter::GetLayerCount(doclink); l < lend; ++l)
			{
				linkstep2 layerlink = Adapter::GetLayerLink(doclink, l);
				if(!layerlink || !Adapter::IsLayerVisibled(layerlink))
					continue;
				for(int s = 0, send = Adapter::GetShapeCount(layerlink, false); s < send; ++s)
				{
					HasFoundShape = true;
					linkstep3 shapelink = Adapter::GetShapeLink(layerlink, false, s);
					if(shapelink && --iCount == 0)
					{
						const int BufferSize = Adapter::GetShapeBufferSize(shapelink);
						const int PacketSize = 8 + BufferSize;
						BxCore::Socket::Send(This.Sock, &PacketSize, 4);
						BxCore::Socket::Send(This.Sock, "LINE", 4);
						BxCore::Socket::Send(This.Sock, &This.MapID, 4);
						BxCore::Socket::Send(This.Sock, Adapter::GetShapeBuffer(shapelink), BufferSize);
					}
				}
			}
			if(!HasFoundShape || iCount == 0)
			{
				This.UploadStep = -2;				
				int NewDoc = Command::Canvas::Create(false);
				Command::Canvas::SetArea(NewDoc, Rect(-DOC_WIDTH_HALF, -DOC_HEIGHT_HALF, DOC_WIDTH_HALF, DOC_HEIGHT_HALF));

				if(doclink)
				for(int l = 0, lend = Adapter::GetLayerCount(doclink); l < lend; ++l)
				{
					linkstep2 layerlink = Adapter::GetLayerLink(doclink, l);
					if(!layerlink || !Adapter::IsLayerVisibled(layerlink))
						continue;
					for(int s = 0, send = Adapter::GetShapeCount(layerlink, false); s < send; ++s)
					{
						linkstep3 shapelink = Adapter::GetShapeLink(layerlink, false, s);
						Export::LoadShape(NewDoc, 0, Adapter::GetShapeBuffer(shapelink),
							Adapter::GetShapeBufferSize(shapelink));
					}
				}
				Command::ClearShapes(This.Doc);

				BxScene::AddRequest("shootingstars_intro", sceneside_left, scenelayer_top,
					BxString("<>:ZOOM:<A:08><A:08>", BxARG(This.MapID, NewDoc)));
				BxScene::SubRequest("shootingstars_editor", sceneside_right);
			}
		}
		++This.UploadStep;
	}
	// 이펙트 진행
	This.RunEffect();
	return sysupdateresult_do_render;
}

void OnRender(shootingstars_editor& This, BxDraw& Draw)
{
    Draw.Rectangle(FILL, Draw.CurrentRect(), COLOR(0, 0, 0));
	linkstep1 doclink = Adapter::GetDocumentLink(This.Doc);
	BxASSERT("Document가 nullptr입니다", doclink);

	BxTRY(Draw, HOTSPOT(This.ViewOx, This.ViewOy))
	{
		const Zoom DocZoom = Command::Canvas::GetZoom(This.Doc);
		const Rect DocArea = Command::Canvas::GetArea(This.Doc);
		const Rect ViewRect = DocArea * DocZoom;

		// 배경화면
		const float Bgx = This.SkyBGImage.Width() / 2.0f;
		const float Bgy = This.SkyBGImage.Height() / 2.0f;
		This.DrawImage(Draw, This.SkyBGImage, -Bgx, -Bgy, Bgx, Bgy, DocZoom, This.ViewOx, This.ViewOy);

		// 오브젝트
		const float Objx = This.ObjectImage.Width() / 2.0f;
		const float Objy = This.ObjectImage.Height() / 2.0f;
		This.DrawImage(Draw, This.ObjectImage, -Objx, -Objy, Objx, Objy, DocZoom, This.ViewOx, This.ViewOy);

		// 도형들
		int StrokeStep = 0;
		point ScenePos = BxScene::GetPosition("shootingstars_editor");
		ScenePos.x /= DocZoom.scale;
		ScenePos.y /= DocZoom.scale;
		BxTRY(Draw, CLIP(XYXY(ViewRect.l, ViewRect.t, ViewRect.r, ViewRect.b)))
		for(int l = 0, lend = Adapter::GetLayerCount(doclink); l < lend; ++l)
		{
			linkstep2 layerlink = Adapter::GetLayerLink(doclink, l);
			if(!layerlink || !Adapter::IsLayerVisibled(layerlink))
				continue;
			for(int dyn = 0; dyn < 2; ++dyn)
			for(int s = 0, send = Adapter::GetShapeCount(layerlink, dyn != 0); s < send; ++s)
			{
				linkstep3 shapelink = Adapter::GetShapeLink(layerlink, dyn != 0, s);
				if(!shapelink) continue;
				if(CONST_STRING("PlatyVGElement::MeshAqua") == Adapter::GetShapeMeshType(shapelink))
				{
					const int CurMeshCount = Adapter::GetShapeMeshCount(shapelink);
					if(This.UploadStep / This.UploadFramePerStroke < ++StrokeStep && 2 <= CurMeshCount)
					{
						const int AniOpa = 255 * (This.UploadFramePerStroke - MinMax(0,
							This.UploadStep - (StrokeStep - 1) * This.UploadFramePerStroke,
							This.UploadFramePerStroke)) / This.UploadFramePerStroke;
						const MeshAqua* CurMesh = (const MeshAqua*) Adapter::GetShapeMeshArray(shapelink);
						color_x888 ShapeColor = RGB32(
							Adapter::GetShapeColorRed(shapelink),
							Adapter::GetShapeColorGreen(shapelink),
							Adapter::GetShapeColorBlue(shapelink));
						BxCore::OpenGL2D::RenderStripDirectly(
							CurMeshCount, CurMesh,
							Adapter::GetShapeColorAlpha(shapelink) * AniOpa / 255,
							Adapter::GetShapeColorAqua(shapelink), ShapeColor,
							DocZoom.offset.x + ScenePos.x, DocZoom.offset.y + ScenePos.y, DocZoom.scale,
							Adapter::GetShapeMatrixM11(shapelink),
							Adapter::GetShapeMatrixM12(shapelink),
							Adapter::GetShapeMatrixM21(shapelink),
							Adapter::GetShapeMatrixM22(shapelink),
							Adapter::GetShapeMatrixDx(shapelink),
							Adapter::GetShapeMatrixDy(shapelink));
						// 별모양
						BxUtil::SetRandom(s);
						const int Step = Max(1, 4 * DocZoom.scale);
						BxTRY(Draw, FORM(&This.StarImage) >> ((AniOpa < 255)? COLOR(ShapeColor) : StyleStack::Zero()))
						for(int s = Step, so = 0; s < CurMeshCount; s += Step * 2, ++so)
						{
							const float LX = (CurMesh[s].lx + DOC_WIDTH_HALF) * DocZoom.scale;
							const float LY = (CurMesh[s].ly + DOC_HEIGHT_HALF) * DocZoom.scale;
							const float RX = (CurMesh[s].rx + DOC_WIDTH_HALF) * DocZoom.scale;
							const float RY = (CurMesh[s].ry + DOC_HEIGHT_HALF) * DocZoom.scale;
							const int MX = (int) ((LX + RX) / 2 + 0.5f);
							const int MY = (int) ((LY + RY) / 2 + 0.5f);
							const int MR = Sqrt(Pow((int) (LX - RX + 0.5f)) + Pow((int) (LY - RY + 0.5f))) / 2;
							const int Opa = 160 * Pow(((This.AniFrame + BxUtil::GetRandom(so & 0xFF)) % 60) - 30) / 900;
							Draw.Rectangle(FILL, XYR(MX, MY, MR + MR * 9 * (255 - AniOpa) / 255),
								OPACITY(Opa * AniOpa / 255));
						}
					}
				}
			}
		}

		// 행성표식
		BxVarVector<shootingstars::Planet, 4>& PPos = This.PlanetPos[This.MapID];
		for(int i = 0, iend = PPos.Length(); i < iend; ++i)
		{
			const int Ani = Abs(((This.AniFrame + PPos[i].a) % 100) - 50);
			const int Ani2 = Ani * Ani;
			BxTRY(Draw, OPACITY(127 + 128 * Ani / 50))
			{
				const int PX = PPos[i].x;
				const int PY = PPos[i].y;
				const int PS = PPos[i].r * (5000 + Ani2) / 5000;
				This.DrawImage(Draw, This.PlanetImage,
					PX - PS, PY - PS, PX + PS, PY + PS,
					DocZoom, This.ViewOx, This.ViewOy);
			}
		}
	}

	// 그림틀
	Draw.Rectangle(FILL, XYWH(0, 0, Draw.Width(), Draw.Height()), FORM(&This.DrawBGImage));
	Draw.Area(330, 0, FORM(&This.StarTitleImage));

	// 홈버튼
	BxTRY(Draw, CLIP(XYWH(0, 0, 220, 220)), "HOME")
	{if(DEBUGLINE) Draw.Rectangle(FILL, Draw.CurrentRect(), COLOR(255, 0, 0) >> OPACITY(128));}
	Draw.Area(20, 20, FORM(&This.HomeImage));

	// 업로드
	BxTRY(Draw, CLIP(XYWH(Draw.Width() - 220, 0, 220, 220)), "UPLOAD")
	{if(DEBUGLINE) Draw.Rectangle(FILL, Draw.CurrentRect(), COLOR(255, 0, 0) >> OPACITY(128));}
	Draw.Area(Draw.Width() - 20, 20, FORM(&This.UploadImage));

	// 지우개
	BxTRY(Draw, CLIP(XYWH(0, Draw.Height() - 250, 300, 250)), "ERASER")
	{if(DEBUGLINE) Draw.Rectangle(FILL, Draw.CurrentRect(), COLOR(255, 0, 0) >> OPACITY(128));}
	if(This.SelectID == -1 && This.SelectIDAni < 30)
	{
		const int W = This.EraserEffImage.Width();
		const int H = This.EraserEffImage.Height();
		Draw.Rectangle(FILL, XYXY(
			20 - This.SelectIDAni * 2,
			Draw.Height() - 20 - H - This.SelectIDAni * 2,
			20 + W + This.SelectIDAni * 2,
			Draw.Height() - 20 + This.SelectIDAni * 2),
			FORM(&This.EraserEffImage) >>
			OPACITY((255 - 64) * (30 - This.SelectIDAni) / 30));
	}
	Draw.Area(20, Draw.Height() - 20, FORM(&This.EraserImage));

	// 펜
	for(int i = 0; i < 3; ++i)
	{
		const int Pos = Draw.Width() - 100 - 220 * (2 - i);
		BxTRY(Draw, CLIP(XYWH(Pos - 215, Draw.Height() - 190, 220, 190)), BxString("<>:PEN<A>", BxARG(i)))
		{if(DEBUGLINE) Draw.Rectangle(FILL, Draw.CurrentRect(), COLOR(255, 0, 0) >> OPACITY(128));}
		if(This.SelectID == i && This.SelectIDAni < 30)
		{
			const int W = This.StarPenImage[i].Width();
			const int H = This.StarPenImage[i].Height();
			Draw.Rectangle(FILL, XYXY(
				Pos - W - This.SelectIDAni * 2,
				Draw.Height() - 25 - H - This.SelectIDAni * 2,
				Pos + This.SelectIDAni * 2,
				Draw.Height() - 25 + This.SelectIDAni * 2),
				FORM(&This.StarPenEffImage) >>
				OPACITY((255 - 64) * (30 - This.SelectIDAni) / 30));
		}
		Draw.Area(Pos, Draw.Height() - ((This.SelectID == i)? 25 : 5), FORM(&This.StarPenImage[i]));
	}

	// 이펙트
	for(int i = 0; i < shootingstars::EFFECT_LEN; ++i)
	{
		if(!This.ObjectEffect[i].IsVisible)
			continue;
		Draw.Rectangle(FILL, XYR(
			(int) (This.ObjectEffect[i].pos.x + 0.5f),
			(int) (This.ObjectEffect[i].pos.y + 0.5f),
			(int) (5 + 20 * (1 - This.ObjectEffect[i].ani) + 0.5f)),
			FORM(&This.EffectImage) >>
			OPACITY((int) (255 * This.ObjectEffect[i].ani)));
	}

	// 서버화면보기
	if(BxScene::IsActivity("shootingstars_server"))
	BxTRY(Draw, CLIP(XYWH(Draw.Width() - 120, 0, 120, 30)), "GOSKY")
	{
		Draw.Rectangle(FILL, Draw.CurrentRect(), COLOR(255, 0, 255) >> OPACITY(64));
		Draw.TextW(BxCore::Font::Default(), (wstring)(void*) L"서버화면보기 ▲",
			Draw.CurrentCenter(), textalign_center_middle, COLOR(255, 0, 255));
	}
}
