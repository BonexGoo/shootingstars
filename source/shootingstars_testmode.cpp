#include <BxScene.hpp>

FRAMEWORK_SCENE(unknown, "shootingstars_testmode")

syseventresult OnEvent(unknown& This, const sysevent& Event)
{
	if(Event.type == syseventtype_init)
	{
		BxScene::AddRequest("shootingstars_intro");
		BxScene::AddRequest("shootingstars_server",
				sceneside_center, scenelayer_top, "TESTMODE");
	}
	return syseventresult_pass;
}

sysupdateresult OnUpdate(unknown& This)
{
	if(BxScene::IsActivity("shootingstars_server"))
	{
		BxScene::SetRequest("shootingstars_server", sceneside_up);
		BxScene::SubRequest("shootingstars_testmode");
	}
	return sysupdateresult_do_render;
}

void OnRender(unknown& This, BxDraw& Draw)
{
}
