#include "CCUIAtlas.h"
#include "../DeviceFormat2cocos.h"
#include "../MTexture.h"
#include "../MTextureManager.h"

#include "CCTextureAtlas2.h"
#include "WaterLayer.h"

using namespace RENDERSYSTEM;


NS_CC::Texture2D *CCUIAtlas::GetAtlas(unsigned tex_id)
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
		if (id != 0)
		{
			MISS_TEX(id);
		}
		return 0;
	}
	MTexture *pTex = (MTexture *)tex;
	return pTex->GetUITextueInfo(&info);
}


CCUIAtlas::CCUIAtlas()
{
	//初始化水
	cocos2d::Texture2D *texture = GetAtlas(TEXID(PACKAGE_atlas0, 40));//51, index+1
	_waterAtlas = cocos2d::TextureAtlas2::createWithTexture(texture, 40);
	_WaterTileBatchNode = NS_CC::WaterTileBatchNode::Create(_waterAtlas);
	_WaterTileBatchNode->retain();
}

//
CCUIAtlas &CCUIAtlas::Instance()
{
	static CCUIAtlas _Inst;
	return _Inst;
}


