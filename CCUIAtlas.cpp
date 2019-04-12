#include "CCUIAtlas.h"
#include "../DeviceFormat2cocos.h"
#include "../MTexture.h"
#include "../MTextureManager.h"

#include "CCTextureAtlas2.h"
#include "WaterLayer.h"
#include "2d/CCSpriteBatchNode.h"
#include "2d/CCSprite.h"
#include "math/Vec2.h"
//#include "platform/CCStdC.h"
#include "base/ccUtils.h"

using namespace RENDERSYSTEM;

namespace cocos2d {

	//
	SpriteBatchNodeDraw::SpriteBatchNodeDraw(NS_CC::Texture2D *tex):_reusedLetter(NULL)
	{
		_batNode = NS_CC::SpriteBatchNode::createWithTexture(tex);

		NS_CC::GLProgram* shader = NS_CC::GLProgramCache::getInstance()->getGLProgram(NS_CC::GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR);
		_batNode->retain();
		_batNode->setGLProgram(shader);

		if (_reusedLetter == nullptr)
		{
			_reusedLetter = NS_CC::Sprite::create();
			//_reusedLetter->setOpacityModifyRGB(_isOpacityModifyRGB);
			_reusedLetter->retain();
			_reusedLetter->setAnchorPoint(NS_CC::Vec2::ANCHOR_TOP_LEFT);
			_reusedLetter->setTextureAtlas(_batNode->getTextureAtlas());
		}

	}

	//
	void SpriteBatchNodeDraw::DrawSprite(NS_CC::Rect &rc, float x, float y)
	{
		_reusedLetter->setTextureRect(rc, false, rc.size);
		//float letterPositionX = _lettersInfo[ctr].positionX + _linesOffsetX[_lettersInfo[ctr].lineIndex];
		_reusedLetter->setPosition(x, y);

		TextureAtlas* _textureAtlas = _batNode->getTextureAtlas();
		int index = _textureAtlas->getTotalQuads();
		_batNode->insertQuadFromSprite(_reusedLetter, index);
	}

	//
	void SpriteBatchNodeDraw::Flush()
	{
		TextureAtlas* _textureAtlas = _batNode->getTextureAtlas();
		if (_textureAtlas)
		{
			_batNode->getGLProgram()->use();
			_batNode->getGLProgram()->setUniformsForBuiltins();//_mv

			const BlendFunc&_blendFunc = _batNode->getBlendFunc();
			utils::setBlending(_blendFunc.src, _blendFunc.dst);

			_textureAtlas->drawQuads();
			_textureAtlas->removeAllQuads();
		}
	}
}



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
	//if (id == 200717)
	//{
	//	int i = 0; i++;
	//}
	MTexture *pTex = (MTexture *)tex;
	return pTex->GetUITextueInfo(&info);
}

//根据ID获取通用的Atlas SpriteBatchNode
NS_CC::SpriteBatchNodeDraw *CCUIAtlas::GetTileBatchNode(unsigned tex_id)
{
	auto it = _atlasBatchMap.find(tex_id);
	if (it != _atlasBatchMap.end())
	{
		return it->second;
	}
	
	//
	NS_CC::Texture2D *tex = getTileTexture2D(tex_id);
	NS_CC::SpriteBatchNodeDraw *pp = new NS_CC::SpriteBatchNodeDraw(tex);
	_atlasBatchMap[tex_id] = pp;
	return pp;
}

//void CCUIAtlas::DrawAtlasSprite(unsigned id, NS_CC::Rect &rc, float x, float y)
//{
//	//NS_CC::SpriteBatchNode *p = GetTileBatchNode(pT1->GetAtlasID());
//	//if (p)
//	//{
//	//	//p->
//	//}
//}




CCUIAtlas::CCUIAtlas()
{
	//初始化水
    NS_CC::Texture2D *texture = GetAtlas(TEXID(PACKAGE_atlas0, 40));//
	//texture->setGLProgram(waterp);

    NS_CC::Texture2D *textureMask = GetAtlas(TEXID(PACKAGE_atlas0, 41));//
	texture->setAlphaTexture(textureMask);

	NS_CC::GLProgram* waterp = NS_CC::GLProgramCache::getInstance()->getGLProgram(NS_CC::GLProgram::SHADER_NAME_WATER_POSITION_TEXTURE_COLOR);
	_waterAtlas = NS_CC::TextureAtlas2::createWithTexture(texture, 32);
	_WaterTileBatchNode = NS_CC::WaterTileBatchNode::Create(_waterAtlas);
	_WaterTileBatchNode->retain();
	_WaterTileBatchNode->setGLProgram(waterp);

	NS_CC::GLProgram* water_noalpha = NS_CC::GLProgramCache::getInstance()->getGLProgram(NS_CC::GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR);
	_waterAtlas_NoAlpha = NS_CC::TextureAtlas2::createWithTexture(texture, 32);
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


