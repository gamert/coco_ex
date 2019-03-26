#ifndef CCUI_ATLAS_H
#define CCUI_ATLAS_H

//use for manager ui altas
//������Tex��Texture2d�Ĺ���ͨ·:
/// @cond DO_NOT_SHOW

#include <unordered_map>
#include "../DeviceFormat2cocos.h"

namespace cocos2d {
	class TextureAtlas2;
	class WaterTileBatchNode;
}

class CCUIAtlas
{
public:
	CCUIAtlas();
	static CCUIAtlas &Instance();
	//0�ɹ�...
	int GetUITextueInfo(unsigned id, UITextueInfo &info);


	//����Atlas ��Ӧ��Tex��������Texture2D
	NS_CC::Texture2D *GetAtlas(unsigned);
	NS_CC::TextureAtlas2 *_waterAtlas;
	NS_CC::WaterTileBatchNode *_WaterTileBatchNode;

	NS_CC::TextureAtlas2 *_waterAtlas_NoAlpha;
	NS_CC::WaterTileBatchNode *_WaterTileBatchNode_NoAlpha;

	//
	std::unordered_map<unsigned, NS_CC::Texture2D *> _atlasMap;
};

#endif