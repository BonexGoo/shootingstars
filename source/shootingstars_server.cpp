#include <BxScene.hpp>
#include <BxImage.hpp>
#include "adapter.hpp"
#include "command.hpp"
#include "export.hpp"
#include "input.hpp"
#include "shootingstars.hpp"

class shootingstars_server : public shootingstars
{
public:
	int MainDoc;
	BxImage BGImage;
	int Doc[12];
	//BxImage ObjectImage[12];
	BxImage PlanetImage;
	BxImage RingImage;
	id_server Serv;
	Zoom FirstZoom;
	Zoom CurrentZoom;
	Zoom TargetZoom;
	int ZoomAni;
	uhuge LastZoomTime;
	int AniFrame;

public:
	shootingstars_server()
	{
		MainDoc = -1;
		BGImage.Load("image/sky_bg.png", BxImage::PNG);
		for(int i = 0; i < 12; ++i)
		{
			Doc[i] = -1;
			//ObjectImage[i].Load(BxString("<>:image/miniobject_<A:03>.png", BxARG(i)), BxImage::PNG);
		}
		PlanetImage.Load("image/planet.png", BxImage::PNG);
		RingImage.Load("image/ring.png", BxImage::PNG);
		Serv = BxCore::Server::Create(true);
		ZoomAni = 0;
		LastZoomTime = 0;
		AniFrame = 0;
		const bool IsSuccess = BxCore::Server::Listen(Serv, 6566);
		BxASSERT("서버등록에 실패하였습니다", IsSuccess);
	}

	~shootingstars_server()
	{
		BxCore::Server::Release(Serv);
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
				if(ZoomAni == 0)
					Command::Canvas::Update(Doc[i]);
			}
		}
	}
};

FRAMEWORK_SCENE(shootingstars_server, "shootingstars_server")

syseventresult OnEvent(shootingstars_server& This, const sysevent& Event)
{
	if(Event.type == syseventtype_init || Event.type == syseventtype_resize)
	{
		This.ViewOx = Event.init.w / 2.0f;
		This.ViewOy = Event.init.h / 2.0f;
		Command::SetViewRadius(Math::Sqrt(This.ViewOx * This.ViewOx + This.ViewOy * This.ViewOy));
		// 배경
		const float Bgx = This.BGImage.Width() / 2;
		const float Bgy = This.BGImage.Height() / 2;
		if(Event.type == syseventtype_init)
		{
			This.MainDoc = Command::Canvas::Create(false);
			Command::Canvas::SetArea(This.MainDoc, Rect(-DOC_WIDTH_HALF, -DOC_HEIGHT_HALF, DOC_WIDTH_HALF, DOC_HEIGHT_HALF));
		}
		Command::Canvas::SetZoomInRect(This.MainDoc, Rect(-This.ViewOx, -This.ViewOy, This.ViewOx, This.ViewOy));
		This.FirstZoom = Command::Canvas::GetZoom(This.MainDoc);
		// 별자리
		const float MainScale = This.FirstZoom.scale;
		for(int i = 0; i < 12; ++i)
		{
			if(Event.type == syseventtype_init)
			{
				This.Doc[i] = Command::Canvas::Create(false);
				Command::Canvas::SetArea(This.Doc[i], Rect(-DOC_WIDTH_HALF, -DOC_HEIGHT_HALF, DOC_WIDTH_HALF, DOC_HEIGHT_HALF));
			}
			const float Scale = MainScale / 5;
			Command::Canvas::SetZoom(This.Doc[i], Zoom(Point(
				This.StarPos[i].x / Scale * MainScale,
				This.StarPos[i].y / Scale * MainScale), Scale));
		}
	}
	else if(Event.type == syseventtype_quit)
	{
		Command::Canvas::Remove(This.MainDoc);
		for(int i = 0; i < 12; ++i)
			Command::Canvas::Remove(This.Doc[i]);
	}
	else if(Event.type == syseventtype_touch)
	{
		if(BxScene::GetSide("shootingstars_server") == sceneside_center)
			return syseventresult_done;
	}
	else if(Event.type == syseventtype_button && Event.button.type == sysbuttontype_up)
	{
		if(StrCmp(Event.button.name, "GOLAND") == same)
		{
			BxScene::SetRequest("shootingstars_server", sceneside_up);
			BxScene::SetRequest("shootingstars_editor", sceneside_center);
		}
	}
	return syseventresult_pass;
}

sysupdateresult OnUpdate(shootingstars_server& This)
{
	++This.AniFrame;
	This.UpdateZoom();
	if(0 < This.LastZoomTime && This.LastZoomTime + 5000 < BxCore::System::GetTimeMilliSecond())
	{
		This.LastZoomTime = 0;
		This.ChangeZoom(This.FirstZoom);
	}

	struct PacketStruct
	{
		uint Type;
		union
		{
			float Zoom[3];
			struct
			{
				int MapID;
				byte Buffer[1];
			};
		};
	};
	while(BxCore::Server::TryNextPacket(This.Serv))
	{
		if(BxCore::Server::GetPacketKind(This.Serv) == peerpacketkind_message)
		{
			huge PacketSize = 0;
			const PacketStruct* Packet = (PacketStruct*)
				BxCore::Server::GetPacketBuffer(This.Serv, &PacketSize);
			if(Packet->Type == *((const uint*) "ZOOM"))
			{
				This.LastZoomTime = BxCore::System::GetTimeMilliSecond();
				This.ChangeZoom(Zoom(Point(Packet->Zoom[0], Packet->Zoom[1]), Packet->Zoom[2]));
				This.UpdateZoom();
			}
            else if(Packet->Type == *((const uint*) "CLS_"))
				Command::ClearShapes(This.Doc[Packet->MapID]);
            else if(Packet->Type == *((const uint*) "LINE"))
				Export::LoadShape(This.Doc[Packet->MapID], 0, Packet->Buffer, (sint32) PacketSize - 8);
		}
	}

	// 이펙트 진행
	This.RunEffect();
	return sysupdateresult_do_render;
}

void OnRender(shootingstars_server& This, BxDraw& Draw)
{
    Draw.Rectangle(FILL, Draw.CurrentRect(), COLOR(0, 0, 0));
	BxTRY(Draw, HOTSPOT(This.ViewOx, This.ViewOy))
	{
		// 배경화면
		const float Bgx = This.BGImage.Width() / 2.0f;
		const float Bgy = This.BGImage.Height() / 2.0f;
		const Zoom MainDocZoom = Command::Canvas::GetZoom(This.MainDoc);
		This.DrawImage(Draw, This.BGImage, -Bgx, -Bgy, Bgx, Bgy, MainDocZoom, This.ViewOx, This.ViewOy);

		// 별자리
		point ScenePos = BxScene::GetPosition("shootingstars_server");
		for(int i = 0; i < 12; ++i)
		{
			const Zoom DocZoom = Command::Canvas::GetZoom(This.Doc[i]);
			//const float Objx = This.ObjectImage[i].Width() / 2.0f * 4;
			//const float Objy = This.ObjectImage[i].Height() / 2.0f * 4;
			//This.DrawImage(Draw, This.ObjectImage[i], -Objx, -Objy, Objx, Objy, DocZoom, This.ViewOx, This.ViewOy);

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

			// 링이펙트
			for(int j = 0, jend = PPos.Length(); j < jend; ++j)
			{
				const int Ani = (This.AniFrame + PPos[j].a) % 600;
				const int Ani2 = Ani * Ani;
				if(Ani < 30)
				BxTRY(Draw, OPACITY(255 - 224 * Ani / 30))
				{
					const int PX = PPos[j].x;
					const int PY = PPos[j].y;
					const int PS = PPos[j].r * (90 + Ani2) / 90;
					This.DrawImage(Draw, This.RingImage,
						PX - PS, PY - PS, PX + PS, PY + PS,
						DocZoom, This.ViewOx, This.ViewOy);
				}
			}
		}
	}

	// 클라화면보기
	if(BxScene::IsActivity("shootingstars_editor"))
	BxTRY(Draw, CLIP(XYWH(Draw.Width() - 120, 0, 120, 30)), "GOLAND")
	{
		Draw.Rectangle(FILL, Draw.CurrentRect(), COLOR(255, 0, 255) >> OPACITY(64));
		Draw.TextW(BxCore::Font::Default(), (wstring)(void*) L"클라화면보기 ▼",
			Draw.CurrentCenter(), textalign_center_middle, COLOR(255, 0, 255));
	}
}
