#ifndef CCUI_ATLAS_H
#define CCUI_ATLAS_H

//use for manager ui altas
//������Tex��Texture2d�Ĺ���ͨ·:
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
	//0�ɹ�...
	int GetUITextueInfo(unsigned id, UITextueInfo &info);


	//����Atlas ��Ӧ��Tex��������Texture2D
	NS_CC::Texture2D *LoadAtlas(unsigned);

	//
	std::unordered_map<unsigned, NS_CC::Texture2D *> _atlasMap;
};

#endif