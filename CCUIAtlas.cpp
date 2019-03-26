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

	//zz释放FrameData数据，对于Tex Atlas，加载完成后，即可释放...
	uint32 theChar = tex_id & 0xFFFFFF;
	ITexture*	 ptex = (ITexture*)g_TexMgr.GetTexImm(theChar);
	if (ptex)
	{
		ptex->ReleaseFrameData();
	}


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
	cocos2d::Texture2D *texture = GetAtlas(TEXID(PACKAGE_atlas0, 40));//
	//texture->setGLProgram(waterp);

	cocos2d::Texture2D *textureMask = GetAtlas(TEXID(PACKAGE_atlas0, 41));//
	texture->setAlphaTexture(textureMask);

	NS_CC::GLProgram* waterp = NS_CC::GLProgramCache::getInstance()->getGLProgram(NS_CC::GLProgram::SHADER_NAME_WATER_POSITION_TEXTURE_COLOR);
	_waterAtlas = cocos2d::TextureAtlas2::createWithTexture(texture, 32);
	_WaterTileBatchNode = NS_CC::WaterTileBatchNode::Create(_waterAtlas);
	_WaterTileBatchNode->retain();
	_WaterTileBatchNode->setGLProgram(waterp);

	NS_CC::GLProgram* water_noalpha = NS_CC::GLProgramCache::getInstance()->getGLProgram(NS_CC::GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR);
	_waterAtlas_NoAlpha = cocos2d::TextureAtlas2::createWithTexture(texture, 32);
	_WaterTileBatchNode_NoAlpha = NS_CC::WaterTileBatchNode::Create(_waterAtlas_NoAlpha);
	_WaterTileBatchNode_NoAlpha->retain();
	_WaterTileBatchNode_NoAlpha->setGLProgram(water_noalpha);
}

//
CCUIAtlas &CCUIAtlas::Instance()
{
	static CCUIAtlas _Inst;
	return _Inst;
}


