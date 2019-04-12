#ifndef CCUI_ATLAS_H
#define CCUI_ATLAS_H

//use for manager ui altas
//建立从Tex到Texture2d的管理通路:
/// @cond DO_NOT_SHOW

#include <unordered_map>
#include "../DeviceFormat2cocos.h"
#include "math/CCGeometry.h"

namespace cocos2d {
	class TextureAtlas2;
	class WaterTileBatchNode;
	class SpriteBatchNode;
	class Sprite;


	class SpriteBatchNodeDraw
	{
	public:
		SpriteBatchNode *_batNode;
		Sprite	*_reusedLetter;
		SpriteBatchNodeDraw(Texture2D *tex);

		void DrawSprite(NS_CC::Rect &rc, float x, float y);
		void Flush();
	};
}




class CCUIAtlas
{
public:
	CCUIAtlas();
	static CCUIAtlas &Instance();
	//0成功...
	int GetUITextueInfo(unsigned id, UITextueInfo &info);
	//
	NS_CC::SpriteBatchNodeDraw *GetTileBatchNode(unsigned id);
	//void DrawAtlasSprite(unsigned id, NS_CC::Rect &rc, float x, float y);

	//载入Atlas 对应的Tex，并生成Texture2D
	NS_CC::Texture2D *GetAtlas(unsigned);
	NS_CC::TextureAtlas2 *_waterAtlas;
	NS_CC::WaterTileBatchNode *_WaterTileBatchNode;

	NS_CC::TextureAtlas2 *_waterAtlas_NoAlpha;
	NS_CC::WaterTileBatchNode *_WaterTileBatchNode_NoAlpha;

protected:
	//for atlas sprite batch:
	std::unordered_map<unsigned, NS_CC::SpriteBatchNodeDraw *> _atlasBatchMap;
	//for all Texture2D:
	std::unordered_map<unsigned, NS_CC::Texture2D *> _atlasMap;
};

#endif