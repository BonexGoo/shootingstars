#include <BxScene.hpp>
#include <BxImage.hpp>
#include "adapter.hpp"
#include "command.hpp"
#include "input.hpp"
#include "shootingstars.hpp"

class shootingstars_editor_old : public shootingstars
{
public:
	int MapID;
	int Doc;
	int SelectID;
	BxImage BGImage;
	BxImage HomeImage;
	BxImage UploadImage;
	BxImage EraserImage;
	BxImage PenImage[4];
	BxImage TitleImage;
	BxImage ObjectImage;
	BxImage StarImage;
	BxImage EffectImage;
	int AniFrame;
	int UploadStep;
	const int UploadFramePerStroke;

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
	int EffectFocus;

	id_socket Sock;

public:
	shootingstars_editor_old() : UploadFramePerStroke(5)
	{
		MapID = 0;
		Doc = -1;
		SelectID = 0;
		BGImage.Load("image/bg.png", BxImage::PNG);
		HomeImage.Load("image/home.png", BxImage::PNG, BxImage::PAD7);
		UploadImage.Load("image/upload.png", BxImage::PNG, BxImage::PAD9);
		EraserImage.Load("image/eraser.png", BxImage::PNG, BxImage::PAD1);
		for(int i = 0; i < 4; ++i)
			PenImage[i].Load(BxString("<>:image/crayon_<A:03>.png", BxARG(i)),
				BxImage::PNG, BxImage::PAD3);
		TitleImage.Load("image/title_000.png", BxImage::PNG);
		ObjectImage.Load("image/object_000.png", BxImage::PNG);
		StarImage.Load("image/star.png", BxImage::PNG, BxImage::PAD5);
		EffectImage.Load("image/effect.png", BxImage::PNG, BxImage::PAD5);
		AniFrame = 0;
		UploadStep = -1;
		EffectFocus = EFFECT_LEN - 1;

		Sock = BxCore::Socket::Create();
		string Domain = BxCore::System::GetConfigString("ShootingStars.Domain", "localhost");
		BxCore::Socket::Connect(Sock, Domain, 6566);
	}

	~shootingstars_editor_old()
	{
		BxCore::Socket::Release(Sock);
	}
};

FRAMEWORK_SCENE_HIDDEN(shootingstars_editor_old, "shootingstars_editor_old")

syseventresult OnEvent(shootingstars_editor_old& This, const sysevent& Event)
{
	if(Event.type == syseventtype_init)
	{
		This.ViewOx = Event.init.w / 2.0f;
		This.ViewOy = Event.init.h / 2.0f;
		Command::SetViewRadius(Math::Sqrt(This.ViewOx * This.ViewOx + This.ViewOy * This.ViewOy));

		if(StrSameCount(Event.init.message, "MAPID:") == 6)
			This.MapID = AtoI(Event.init.message + 6);
		This.Doc = Command::Canvas::Create(true);
		Command::Canvas::SetArea(This.Doc, Rect(-DOC_WIDTH_HALF, -DOC_HEIGHT_HALF, DOC_WIDTH_HALF, DOC_HEIGHT_HALF));
		Command::Canvas::SetZoomInRect(This.Doc, Rect(-This.ViewOx, -This.ViewOy, This.ViewOx, This.ViewOy), BoundByOuter);
		Command::Canvas::SetFocus(This.Doc);

		Command::SetThick(25);
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
	else if(Event.type == syseventtype_touch)
	{
		if(This.UploadStep == -1)
		{
			const float X = Event.touch.fx - This.ViewOx;
			const float Y = Event.touch.fy - This.ViewOy;
			Command::SetCurrentTime(BxCore::System::GetTimeMilliSecond());
			switch(Event.touch.type)
			{
			case systouchtype_down:
				Input::ScreenDown(Event.touch.id, X, Y, Event.touch.force, DeviceByTouch);
				break;
			case systouchtype_move:
				Input::ScreenDrag(Event.touch.id, X, Y, Event.touch.force, DeviceByTouch);
				break;
			case systouchtype_up: default:
				Input::ScreenUp(Event.touch.id, DeviceByTouch);
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
						This.EffectFocus = (This.EffectFocus + 1) % shootingstars_editor_old::EFFECT_LEN;
						shootingstars_editor_old::Effect& CurEffect = This.ObjectEffect[This.EffectFocus];
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
			This.SelectID = -1;
			Command::SetStrokeBy("erase");
		}
		else if(StrCmp(Event.button.name, "PEN0") == same)
		{
			This.SelectID = 0;
			Command::SetStrokeBy("aqua");
			Command::SetColor(Color(255, 49, 0, 180, 160));
		}
		else if(StrCmp(Event.button.name, "PEN1") == same)
		{
			This.SelectID = 1;
			Command::SetStrokeBy("aqua");
			Command::SetColor(Color(216, 1, 255, 180, 160));
		}
		else if(StrCmp(Event.button.name, "PEN2") == same)
		{
			This.SelectID = 2;
			Command::SetStrokeBy("aqua");
			Command::SetColor(Color(255, 198, 2, 180, 160));
		}
		else if(StrCmp(Event.button.name, "PEN3") == same)
		{
			This.SelectID = 3;
			Command::SetStrokeBy("aqua");
			Command::SetColor(Color(0, 129, 255, 180, 160));
		}
		else if(StrCmp(Event.button.name, "GOSKY") == same)
		{
			BxScene::SetRequest("shootingstars_server", sceneside_center);
			BxScene::SetRequest("shootingstars_editor_old", sceneside_down);
		}
		else if(This.UploadStep == -1)
		{
			if(StrCmp(Event.button.name, "HOME") == same)
			{
				BxScene::AddRequest("shootingstars_intro", sceneside_left);
				BxScene::SubRequest("shootingstars_editor_old", sceneside_right);
			}
			else if(StrCmp(Event.button.name, "UPLOAD") == same)
			{
				//if(BxCore::Socket::GetState(This.Sock) == socketstate_connected)
					This.UploadStep = 0;
			}
		}
	}
	return syseventresult_pass;
}

sysupdateresult OnUpdate(shootingstars_editor_old& This)
{
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
				Command::ClearShapes(This.Doc);
			}
		}
		++This.UploadStep;
	}
	// 이펙트 진행
	This.RunEffect();
	return sysupdateresult_do_render;
}

void OnRender(shootingstars_editor_old& This, BxDraw& Draw)
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
		const float Bgx = This.BGImage.Width() / 2.0f;
		const float Bgy = This.BGImage.Height() / 2.0f;
		This.DrawImage(Draw, This.BGImage, -Bgx, -Bgy, Bgx, Bgy, DocZoom, This.ViewOx, This.ViewOy);

		// 오브젝트
		const float Objx = This.ObjectImage.Width() / 2.0f;
		const float Objy = This.ObjectImage.Height() / 2.0f;
		This.DrawImage(Draw, This.ObjectImage, -Objx, -Objy, Objx, Objy, DocZoom, This.ViewOx, This.ViewOy);

		// 타이틀
		const float Tx = This.TitleImage.Width() / 2.0f;
		const float Ty = This.TitleImage.Height() / 2.0f;
		This.DrawImage(Draw, This.TitleImage, -Tx + Bgx - 680 + Tx, -Ty - Bgy + 215 - Ty,
			Tx + Bgx - 680 + Tx, Ty - Bgy + 215 - Ty, DocZoom, This.ViewOx, This.ViewOy);

		// 도형들
		int StrokeStep = 0;
		point ScenePos = BxScene::GetPosition("shootingstars_editor_old");
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
							const float LX = (CurMesh[s].lx + Bgx) * DocZoom.scale;
							const float LY = (CurMesh[s].ly + Bgy) * DocZoom.scale;
							const float RX = (CurMesh[s].rx + Bgx) * DocZoom.scale;
							const float RY = (CurMesh[s].ry + Bgy) * DocZoom.scale;
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
	}

	// 홈버튼
	BxTRY(Draw, CLIP(XYWH(0, 0, 220, 220)), "HOME")
	{}//Draw.Rectangle(FILL, Draw.CurrentRect(), COLOR(255, 0, 0) >> OPACITY(128));
	Draw.Area(20, 20, FORM(&This.HomeImage));

	// 업로드
	BxTRY(Draw, CLIP(XYWH(Draw.Width() - 220, 0, 220, 220)), "UPLOAD")
	{}//Draw.Rectangle(FILL, Draw.CurrentRect(), COLOR(255, 0, 0) >> OPACITY(128));
	Draw.Area(Draw.Width() - 20, 20, FORM(&This.UploadImage));

	// 지우개
	BxTRY(Draw, CLIP(XYWH(0, Draw.Height() - 250, 300, 250)), "ERASER")
	{}//Draw.Rectangle(FILL, Draw.CurrentRect(), COLOR(255, 0, 0) >> OPACITY(128));
	Draw.Area(20, Draw.Height() - 20, FORM(&This.EraserImage));

	// 펜
	for(int i = 0; i < 4; ++i)
	{
		const int Pos = Draw.Width() - 100 - 190 * (3 - i);
		BxTRY(Draw, CLIP(XYWH(Pos - 200, Draw.Height() - 180, 190, 180)), BxString("<>:PEN<A>", BxARG(i)))
		{}//Draw.Rectangle(FILL, Draw.CurrentRect(), COLOR(255, 0, 0) >> OPACITY(128));
		Draw.Area(Pos, Draw.Height() + ((This.SelectID == i)? 35 : 65), FORM(&This.PenImage[i]));
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
