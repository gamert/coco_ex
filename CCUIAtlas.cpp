#include "CCUIAtlas.h"
#include "../DeviceFormat2cocos.h"
#include "../MTexture.h"
#include "../MTextureManager.h"
using namespace RENDERSYSTEM;


NS_CC::Texture2D *CCUIAtlas::LoadAtlas(unsigned tex_id)
{
	auto it = _atlasMap.find(tex_id);
	if (it != _atlasMap.end())
	{
		return it->second;
	}
	//
	NS_CC::Texture2D *tex = getTileTexture2D(tex_id);
	_atlasMap[tex_id] = tex;
	return tex;
}

//根据UI需要填充的数据..
int CCUIAtlas::GetUITextueInfo(unsigned id, UITextueInfo &info)
{
	const ITexture*	 tex = g_TexMgr.GetTexImm(id);//51, index+1
	if (NULL == tex)
	{
		MISS_TEX(id);
		return 0;
	}
	MTexture *pTex = (MTexture *)tex;
	return pTex->GetUITextueInfo(&info);
}

//
CCUIAtlas &CCUIAtlas::Instance()
{
	static CCUIAtlas _Inst;
	return _Inst;
}


