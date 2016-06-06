#include <BxScene.hpp>
#include <BxImage.hpp>
#include "adapter.hpp"
#include "command.hpp"
#include "export.hpp"
#include "input.hpp"
#include "shootingstars.hpp"

class shootingstars_intro : public shootingstars
{
public:
	int MainDoc;
	int Doc[12];
	BxImage BGTitleImage;
	BxImage BGTextImage[2];
	BxImage BGCloseImage[2];
	BxImage GroupImage;
	BxImage GroupEffImage;
	BxImage StarIconImage[12][3];
	BxImage PlanetImage;
	int StarIconAni[12];
	int SelectID;
	int SelectIDAni;
	int AniFrame;
	bool IsZoomMode;
	id_socket ZoomSock;
	bool IsPushedClose;

	int MapIDForZoom;
	Zoom FirstZoom;
	Zoom CurrentZoom;
	Zoom TargetZoom;
	int ZoomAni;
	uhuge LastZoomTime;

public:
	shootingstars_intro()
	{
		MainDoc = -1;
		BGTitleImage.Load("image/dark_title.png", BxImage::PNG, BxImage::PAD2);
		BGTextImage[0].Load("image/text_0.png", BxImage::PNG, BxImage::PAD2);
		BGTextImage[1].Load("image/text_1.png", BxImage::PNG, BxImage::PAD2);
		BGCloseImage[0].Load("image/close_0.png", BxImage::PNG, BxImage::PAD9);
		BGCloseImage[1].Load("image/close_1.png", BxImage::PNG, BxImage::PAD9);
		GroupImage.Load("image/group.png", BxImage::PNG);
		GroupEffImage.Load("image/group_eff.png", BxImage::PNG);
		for(int i = 0; i < 12; ++i)
		{
			StarIconImage[i][0].Load(BxString("<>:image/n<A:02>.png", BxARG(i)), BxImage::PNG);
			StarIconImage[i][1].Load(BxString("<>:image/c<A:02>.png", BxARG(i)), BxImage::PNG);
			StarIconImage[i][2].Load(BxString("<>:image/miniobject_<A:03>.png", BxARG(i)), BxImage::PNG);
			StarIconAni[i] = BxUtil::GetRandom() % 200;
		}
		PlanetImage.Load("image/planet.png", BxImage::PNG);
		SelectID = -1;
		SelectIDAni = 100;
		AniFrame = 0;

		IsZoomMode = false;
		ZoomSock = nullptr;
		IsPushedClose = false;

		MapIDForZoom = 0;
		ZoomAni = 0;
		LastZoomTime = 0;
	}

	~shootingstars_intro()
	{
		BxCore::Socket::Release(ZoomSock);
	}

	void ChangeZoom(const Zoom NewZoom)
	{
		CurrentZoom = Command::Canvas::GetZoom(MainDoc);
		TargetZoom = NewZoom;
		ZoomAni = 1000;
	}

	void UpdateZoom()
	{
		if(0 < ZoomAni)
		{
			ZoomAni = ZoomAni * 9 / 10;
			Zoom NewZoom;
			NewZoom.offset.x = CurrentZoom.offset.x * ZoomAni / 1000 + TargetZoom.offset.x * (1000 - ZoomAni) / 1000;
			NewZoom.offset.y = CurrentZoom.offset.y * ZoomAni / 1000 + TargetZoom.offset.y * (1000 - ZoomAni) / 1000;
			NewZoom.scale = CurrentZoom.scale * ZoomAni / 1000 + TargetZoom.scale * (1000 - ZoomAni) / 1000;
			Command::Canvas::SetZoom(MainDoc, NewZoom);

			for(int i = 0; i < 12; ++i)
			{
				const float Scale = NewZoom.scale / 5;
				Command::Canvas::SetZoom(Doc[i], Zoom(Point(
						(StarPos[i].x + NewZoom.offset.x) / Scale * NewZoom.scale,
						(StarPos[i].y + NewZoom.offset.y) / Scale * NewZoom.scale), Scale));
				if(ZoomAni == 0) Command::Canvas::Update(Doc[i]);
			}
		}
	}
};

FRAMEWORK_SCENE(shootingstars_intro, "shootingstars_intro")

syseventresult OnEvent(shootingstars_intro& This, const sysevent& Event)
{
	if(Event.type == syseventtype_init || Event.type == syseventtype_resize)
	{
		BxScene::SetFlow(sceneflow_decel);
		This.ViewOx = Event.init.w / 2.0f;
		This.ViewOy = Event.init.h / 2.0f;

		const float Bgx = This.DarkBGImage.Width() / 2;
		const float Bgy = This.DarkBGImage.Height() / 2;
		if(Event.type == syseventtype_init)
		{
			This.MainDoc = Command::Canvas::Create(false);
			Command::Canvas::SetArea(This.MainDoc, Rect(-DOC_WIDTH_HALF, -DOC_HEIGHT_HALF, DOC_WIDTH_HALF, DOC_HEIGHT_HALF));
		}
		Command::Canvas::SetZoomInRect(This.MainDoc, Rect(-This.ViewOx, -This.ViewOy, This.ViewOx, This.ViewOy), BoundByOuter);
		This.FirstZoom = Command::Canvas::GetZoom(This.MainDoc);
		Command::SetStrokeBy("hand");

		if(Event.type == syseventtype_init && StrSameCount(Event.init.message, "ZOOM:") == 5)
		{
			This.MapIDForZoom = AtoI(Event.init.message + 5, 8);
			const int DocID = AtoI(Event.init.message + 5 + 8, 8);

			// 별자리
			const float MainScale = This.FirstZoom.scale;
			for(int i = 0; i < 12; ++i)
			{
				if(This.MapIDForZoom == i) This.Doc[i] = DocID;
				else
				{
					This.Doc[i] = Command::Canvas::Create(false);
					Command::Canvas::SetArea(This.Doc[i], Rect(-DOC_WIDTH_HALF, -DOC_HEIGHT_HALF, DOC_WIDTH_HALF, DOC_HEIGHT_HALF));
				}
				const float Scale = MainScale / 5;
				Command::Canvas::SetZoom(This.Doc[i], Zoom(Point(
					This.StarPos[i].x / Scale * MainScale,
					This.StarPos[i].y / Scale * MainScale), Scale));
			}

			This.IsZoomMode = true;
			This.ZoomSock = BxCore::Socket::Create();
			string Domain = BxCore::System::GetConfigString("ShootingStars.Domain", "localhost");
			BxCore::Socket::Connect(This.ZoomSock, Domain, 6566);
		}
	}
	else if(Event.type == syseventtype_quit)
	{
		Command::Canvas::Remove(This.MainDoc);
	}
	else if(Event.type == syseventtype_button)
	{
		if(StrCmp(Event.button.name, "CLOSE") == same)
		{
			if(Event.button.type == sysbuttontype_down)
				This.IsPushedClose = true;
			else if(Event.button.type == sysbuttontype_up)
			{
				This.IsPushedClose = false;
				This.IsZoomMode = false;
				BxCore::Socket::Release(This.ZoomSock);
				This.ZoomSock = nullptr;
				Command::Canvas::SetZoom(This.MainDoc, This.FirstZoom);
				for(int i = 0; i < 12; ++i)
					Command::Canvas::Remove(This.Doc[i]);
			}
		}
		else if(StrSameCount(Event.button.name, "ICON") == 4)
		{
			if(Event.button.type == sysbuttontype_up)
			{
				const int NewSelectID = AtoI(Event.button.name + 4);
				if(This.SelectID != NewSelectID)
				{
					This.SelectID = NewSelectID;
					This.SelectIDAni = 0;
				}
				else
				{
					BxScene::AddRequest("shootingstars_editor", sceneside_right,
						scenelayer_top, BxString("<>:MAPID:<A>", BxARG(NewSelectID)));
					BxScene::SubRequest("shootingstars_intro", sceneside_left);
				}
			}
		}
	}
	else if(Event.type == syseventtype_touch)
	{
		if(Event.touch.type == systouchtype_down)
			This.SelectID = -1;
		if(This.IsZoomMode)
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
				if(This.IsZoomMode)
				{
					const Zoom DocZoom = Command::Canvas::GetZoom(This.MainDoc);
					if(This.ZoomSock)
					{
						const int PacketSize = 16;
						BxCore::Socket::Send(This.ZoomSock, &PacketSize, 4);
						BxCore::Socket::Send(This.ZoomSock, "ZOOM", 4);
						BxCore::Socket::Send(This.ZoomSock, &DocZoom.offset.x, 4);
						BxCore::Socket::Send(This.ZoomSock, &DocZoom.offset.y, 4);
						BxCore::Socket::Send(This.ZoomSock, &DocZoom.scale, 4);
					}
					This.LastZoomTime = BxCore::System::GetTimeMilliSecond();
					This.ChangeZoom(DocZoom);
					This.UpdateZoom();
				}
				break;
			case systouchtype_up: default:
				Input::ScreenUp(Event.touch.id, DeviceByTouch);
				break;
			}
		}
	}
	else if(Event.type == syseventtype_touch && Event.touch.type == systouchtype_down)
		This.SelectID = -1;
	return syseventresult_pass;
}

sysupdateresult OnUpdate(shootingstars_intro& This)
{
	if(This.SelectIDAni < 100) ++This.SelectIDAni;
	++This.AniFrame;

	if(This.IsZoomMode)
	{
		This.UpdateZoom();
		if(0 < This.LastZoomTime && This.LastZoomTime + 5000 < BxCore::System::GetTimeMilliSecond())
		{
			This.LastZoomTime = 0;
			This.ChangeZoom(This.FirstZoom);
		}
	}

	// 이펙트 진행
	This.RunEffect();
	return sysupdateresult_do_render;
}

void OnRender(shootingstars_intro& This, BxDraw& Draw)
{
    Draw.Rectangle(FILL, Draw.CurrentRect(), COLOR(0, 0, 0));
	BxTRY(Draw, HOTSPOT(This.ViewOx, This.ViewOy))
	{
		// 배경화면
		const float Bgx = This.DarkBGImage.Width() / 2.0f;
		const float Bgy = This.DarkBGImage.Height() / 2.0f;
		const Zoom DocZoom = Command::Canvas::GetZoom(This.MainDoc);
		This.DrawImage(Draw, This.DarkBGImage, -Bgx, -Bgy, Bgx, Bgy, DocZoom, This.ViewOx, This.ViewOy);

		// 아이콘
		if(This.IsZoomMode)
		{
			// 별자리
			point ScenePos = BxScene::GetPosition("shootingstars_intro");
			for(int i = 0; i < 12; ++i)
			{
				const Zoom DocZoom = Command::Canvas::GetZoom(This.Doc[i]);
				if(This.MapIDForZoom != i)
				{
					const float Objx = This.StarIconImage[i][2].Width() / 2.0f * 4;
					const float Objy = This.StarIconImage[i][2].Height() / 2.0f * 4;
					This.DrawImage(Draw, This.StarIconImage[i][2], -Objx, -Objy, Objx, Objy, DocZoom, This.ViewOx, This.ViewOy);
				}

				// 도형들
				This.DrawStroke(Draw, This.Doc[i],
					ScenePos.x / DocZoom.scale, ScenePos.y / DocZoom.scale);

				// 행성표식
				BxVarVector<shootingstars::Planet, 4>& PPos = This.PlanetPos[i];
				for(int j = 0, jend = PPos.Length(); j < jend; ++j)
				{
					const int Ani = Abs(((This.AniFrame + PPos[j].a) % 50) - 25);
					const int Ani2 = Ani * Ani;
					BxTRY(Draw, OPACITY(127 + 128 * Ani / 25))
					{
						const int PX = PPos[j].x;
						const int PY = PPos[j].y;
						const int PS = PPos[j].r * (1250 + Ani2) / 1250;
						This.DrawImage(Draw, This.PlanetImage,
							PX - PS, PY - PS, PX + PS, PY + PS,
							DocZoom, This.ViewOx, This.ViewOy);
					}
				}
			}
		}
		else for(int i = 0; i < 12; ++i)
		{
			const int OpaSeed = Max(0, Abs(((This.AniFrame + This.StarIconAni[i]) % 120) - 60) - 10);
			const int Opa = 32 + (255 - 32) * OpaSeed * OpaSeed / 2500;
			const bool IsSelected = (This.SelectID == i);
			const float Six = This.StarIconImage[i][IsSelected].Width() / 2.0f;
			const float Siy = This.StarIconImage[i][IsSelected].Height() / 2.0f;

			const float l = This.StarPos[i].x - Six;
			const float t = This.StarPos[i].y - Siy;
			const float r = This.StarPos[i].x + Six;
			const float b = This.StarPos[i].y + Siy;
			Rect ViewRect = Rect(l, t, r, b) * DocZoom;
			BxTRY(Draw, CLIP(XYXY(ViewRect.l, ViewRect.t, ViewRect.r, ViewRect.b)), BxString("<>:ICON<A:02>", BxARG(i)))
			{} //Draw.Rectangle(FILL, Draw.CurrentRect(), COLOR(255, 0, 0) >> OPACITY(128));

			BxTRY(Draw, OPACITY((IsSelected)? 255 : Opa))
				This.DrawImage(Draw, This.StarIconImage[i][IsSelected],
					This.StarPos[i].x - Six, This.StarPos[i].y - Siy,
					This.StarPos[i].x + Six, This.StarPos[i].y + Siy,
					DocZoom, This.ViewOx, This.ViewOy);

			if(IsSelected)
			{
				const float Gpx = This.GroupImage.Width() / 2.0f;
				const float Gpy = This.GroupImage.Height() / 2.0f;
				const int Ani = Min(This.SelectIDAni, 20);
				BxTRY(Draw, OPACITY(192 * (20 - Ani) / 20))
					This.DrawImage(Draw, This.GroupEffImage,
						This.StarPos[i].x + 2 - Gpx, This.StarPos[i].y - 20 - Gpy,
						This.StarPos[i].x + 2 + Gpx, This.StarPos[i].y - 20 + Gpy,
						DocZoom, This.ViewOx, This.ViewOy);
				BxTRY(Draw, OPACITY(255 * Ani / 20))
					This.DrawImage(Draw, This.GroupImage,
						This.StarPos[i].x + 2 - Gpx, This.StarPos[i].y - 20 - Gpy,
						This.StarPos[i].x + 2 + Gpx, This.StarPos[i].y - 20 + Gpy,
						DocZoom, This.ViewOx, This.ViewOy);
			}
		}

		const int OpaSeed = Max(0, Abs((This.AniFrame % 80) - 40) - 10);
		const int Opa = 255 * OpaSeed * OpaSeed / 900;
		if(This.IsZoomMode)
		{
			Draw.Area(0, 450, FORM(&This.BGTextImage[1]) >> OPACITY(Opa));
			Draw.Area(This.ViewOx, -This.ViewOy, FORM(&This.BGCloseImage[This.IsPushedClose]));
			BxTRY(Draw, CLIP(XYXY(This.ViewOx - 150, -This.ViewOy, This.ViewOx, -This.ViewOy + 150)), "CLOSE")
			{} //Draw.Rectangle(FILL, Draw.CurrentRect(), COLOR(255, 0, 0) >> OPACITY(128));
		}
		else
		{
			// 텍스트
			Draw.Area(-93, 541, FORM(&This.BGTitleImage));
			Draw.Area(0, 330, FORM(&This.BGTextImage[0]) >> OPACITY(Opa));
		}
	}
}
