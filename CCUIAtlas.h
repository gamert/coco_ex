#ifndef CCUI_ATLAS_H
#define CCUI_ATLAS_H

//use for manager ui altas
//建立从Tex到Texture2d的管理通路:
/// @cond DO_NOT_SHOW

#include <unordered_map>
#include "../DeviceFormat2cocos.h"

//namespace cocos2d {
//	class Texture2D;
//}



class CCUIAtlas
{
public:
	static CCUIAtlas &Instance();
	//0成功...
	int GetUITextueInfo(unsigned id, UITextueInfo &info);


	//载入Atlas 对应的Tex，并生成Texture2D
	NS_CC::Texture2D *LoadAtlas(unsigned);

	//
	std::unordered_map<unsigned, NS_CC::Texture2D *> _atlasMap;
};

#endif