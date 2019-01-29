#include "CCTileAtlas.h"
#if CC_TARGET_PLATFORM != CC_PLATFORM_WIN32 && CC_TARGET_PLATFORM != CC_PLATFORM_WINRT && CC_TARGET_PLATFORM != CC_PLATFORM_ANDROID
#include <iconv.h>
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include "platform/android/jni/Java_org_cocos2dx_lib_Cocos2dxHelper.h"
#endif

#include "base/CCDirector.h"
#include "base/CCEventListenerCustom.h"
#include "base/CCEventDispatcher.h"
#include "base/CCEventType.h"
#include "../DeviceFormat2cocos.h"

#ifdef USE_RBP_PACK
#include "../../../../SDK/juj_RectangleBinPack/MaxRectsBinPack.h"
#endif

NS_CC_BEGIN

TileLetterDefinition *TileLetterDefinition::New()
{
	TileLetterDefinition *p = new TileLetterDefinition();
	memset(p,0,sizeof(TileLetterDefinition));
	return p;
}

//TODO: ���Դ�С�Ƿ���Ч����mem dc��...
const int TileAtlas::CacheTextureWidth = 1024*2;
const int TileAtlas::CacheTextureHeight = 1024;
const char* TileAtlas::CMD_PURGE_TILEATLAS = "__cc_PURGE_TILEATLAS";
const char* TileAtlas::CMD_RESET_TILEATLAS = "__cc_RESET_TILEATLAS";

//const int TileAtlas::DEF_FORMAT = (int)(Texture2D::PixelFormat::S3TC_DXT3);//A8



TileAtlas::TileAtlas(int pix_format)
	: _currentPageData(nullptr)
	, _fontAscender(0)
	, _rendererRecreatedListener(nullptr)
	, _antialiasEnabled(true)
	, _currLineHeight(0)
{

#ifdef USE_RBP_PACK
	m_pMaxRectsBinPack = new rbp::MaxRectsBinPack();
	m_pMaxRectsBinPack->Init(TileAtlas::CacheTextureWidth, TileAtlas::CacheTextureHeight,false);
#endif


	if (true)
	{
		_lineHeight = 0;
		_fontAscender = 0;
		_currentPage = 0;
		_currentPageOrigX = 0;
		_currentPageOrigY = 0;
		_letterEdgeExtend = 0;
		_letterPadding = 0;

		pixelFormat = (int)pix_format;	//ETC

		reinit();

#if CC_ENABLE_CACHE_TEXTURE_DATA
		auto eventDispatcher = Director::getInstance()->getEventDispatcher();

		_rendererRecreatedListener = EventListenerCustom::create(EVENT_RENDERER_RECREATED, CC_CALLBACK_1(TileAtlas::listenRendererRecreated, this));
		eventDispatcher->addEventListenerWithFixedPriority(_rendererRecreatedListener, 1);
#endif
	}
}

void TileAtlas::reinit()
{
	if (_currentPageData)
	{
		delete[]_currentPageData;
		_currentPageData = nullptr;
	}
	_currentPageDataSize = CacheTextureWidth * CacheTextureHeight * PixelFormatBits((Texture2D::PixelFormat)pixelFormat);
	_currentPageDataSize = _currentPageDataSize / 8;
	_currentPageData = new (std::nothrow) unsigned char[_currentPageDataSize];

	assert(_currentPage == 0);
	addOnePage();
}


Texture2D *TileAtlas::addOnePage()
{
	memset(_currentPageData, 0, _currentPageDataSize);

	Texture2D *tex = new (std::nothrow) Texture2D;
	//if (_antialiasEnabled)
	//{
	//	tex->setAntiAliasTexParameters();
	//}
	//else
	//{
	//	tex->setAliasTexParameters();
	//}
	tex->initWithData(_currentPageData, _currentPageDataSize,
		(Texture2D::PixelFormat)pixelFormat, CacheTextureWidth, CacheTextureHeight, Size(CacheTextureWidth, CacheTextureHeight));
	addTexture(tex, _currentPage);
	tex->release();
	return tex;
}

TileAtlas::~TileAtlas()
{
//#if CC_ENABLE_CACHE_TEXTURE_DATA
//	if (_fontFreeType && _rendererRecreatedListener)
//	{
//		auto eventDispatcher = Director::getInstance()->getEventDispatcher();
//		eventDispatcher->removeEventListener(_rendererRecreatedListener);
//		_rendererRecreatedListener = nullptr;
//	}
//#endif

	releaseTextures();

	delete[]_currentPageData;

#if CC_TARGET_PLATFORM != CC_PLATFORM_WIN32 && CC_TARGET_PLATFORM != CC_PLATFORM_WINRT && CC_TARGET_PLATFORM != CC_PLATFORM_ANDROID
	if (_iconv)
	{
		iconv_close(_iconv);
		_iconv = nullptr;
	}
#endif
}

void TileAtlas::reset()
{
	releaseTextures();
	_currLineHeight = 0;
	_currentPage = 0;
	_currentPageOrigX = 0;
	_currentPageOrigY = 0;
	_letterDefinitions.clear();

	reinit();
}

void TileAtlas::releaseTextures()
{
	for (auto &item : _atlasTextures)
	{
		item.second->release();
	}
	_atlasTextures.clear();
}

void TileAtlas::purgeTexturesAtlas()
{
	if (true)
	{
		reset();
		auto eventDispatcher = Director::getInstance()->getEventDispatcher();
		eventDispatcher->dispatchCustomEvent(CMD_PURGE_TILEATLAS, this);
		eventDispatcher->dispatchCustomEvent(CMD_RESET_TILEATLAS, this);

#ifdef USE_RBP_PACK	
		//for
		if (m_pMaxRectsBinPack)
		{
			m_pMaxRectsBinPack->Init(TileAtlas::CacheTextureWidth, TileAtlas::CacheTextureHeight, false);
		}
#endif

	}
}

void TileAtlas::listenRendererRecreated(EventCustom * /*event*/)
{
	purgeTexturesAtlas();
}

//void TileAtlas::addLetterDefinition(TileID utf32Char, const TileLetterDefinition &letterDefinition)
//{
//	_letterDefinitions[utf32Char] = letterDefinition;
//}

void TileAtlas::scaleFontLetterDefinition(float scaleFactor)
{
	for (auto&& fontDefinition : _letterDefinitions) {
		auto& letterDefinition = fontDefinition.second;
		letterDefinition->width *= scaleFactor;
		letterDefinition->height *= scaleFactor;
		letterDefinition->offsetX *= scaleFactor;
		letterDefinition->offsetY *= scaleFactor;
	}
}

bool TileAtlas::getLetterDefinitionForChar(TileID utf32Char, TileLetterDefinition &letterDefinition)
{
	auto outIterator = _letterDefinitions.find(utf32Char);

	if (outIterator != _letterDefinitions.end())
	{
		letterDefinition = *(outIterator->second);
		return letterDefinition.validDefinition;
	}
	else
	{
		return false;
	}
}


void TileAtlas::findNewCharacters(const TileString& u32Text, TileString &newChars)
{
	if (_letterDefinitions.empty())
	{
		newChars = u32Text;
	}
	else
	{
		auto length = u32Text.size();
		newChars.reserve(length);
		for (size_t i = 0; i < length; ++i)
		{
			auto outIterator = _letterDefinitions.find(u32Text[i].id);
			if (outIterator == _letterDefinitions.end())
			{
				newChars.push_back(u32Text[i]);
			}
		}
	}
}

//
bool matchFormat(int src, int dst)
{
	if (src == 4)//TPT_DXT1
		return dst == (int)Texture2D::PixelFormat::S3TC_DXT1;
	if (src == 5)//TPT_DXT1
		return dst == (int)Texture2D::PixelFormat::S3TC_DXT3;
	return false;
}

#ifdef USE_RBP_PACK
bool TileAtlas::prepareTexDef(const TileString& utf32Text)
{
	TileString newChars;
	findNewCharacters(utf32Text, newChars);
	if (newChars.empty())
	{
		return false;
	}

	int adjustForDistanceMap = _letterPadding / 2;
	int adjustForExtend = _letterEdgeExtend / 2;
	int bitmapWidth;
	int bitmapHeight;
	//int glyphHeight;
	//Rect tempRect;

	auto scaleFactor = CC_CONTENT_SCALE_FACTOR();

	TileLetterDefinition *preDef;// = NULL;
	int pix_format = 0;
	const int MAX_TBS = 64;
	TileBitmaps_t tbs[MAX_TBS];
	for (auto&& it : newChars)
	{
		//from TextureManager: 1. ��֡
		int count = getTileBitmaps(it.id, tbs, MAX_TBS);
		//unsigned char* bitmap = getTileBitmap(it.id, bitmapWidth, bitmapHeight, tempRect, pix_format);
		for (int k = 0; k < count; k++)
		{
			TileLetterDefinition *tempDef = TileLetterDefinition::New();

			unsigned char* bitmap = tbs[k].pData;
			bitmapWidth = tbs[k].outWidth;
			bitmapHeight = tbs[k].outHeight;
			pix_format = tbs[k].format;
			Rect &tempRect = tbs[k].outRect;
			bool shouldDelData = tbs[k].shouldDelData;
			if (bitmap && bitmapWidth > 0 && bitmapHeight > 0)
			{
				//check format...
				//assert(matchFormat(pix_format, pixelFormat));

				tempDef->validDefinition = true;
				tempDef->width = tempRect.size.width + _letterPadding + _letterEdgeExtend;
				tempDef->height = tempRect.size.height + _letterPadding + _letterEdgeExtend;
				tempDef->offsetX = tempRect.origin.x - adjustForDistanceMap - adjustForExtend;
				tempDef->offsetY = _fontAscender + tempRect.origin.y - adjustForDistanceMap - adjustForExtend;
				rbp::Rect rcResult = m_pMaxRectsBinPack->Insert(bitmapWidth, bitmapHeight, rbp::MaxRectsBinPack::RectBestShortSideFit);
				if (rcResult.width == 0 || rcResult.height == 0)
				{
					unsigned char *data = _currentPageData;
					_atlasTextures[_currentPage]->updateWithData(data, 0, 0, CacheTextureWidth, CacheTextureHeight - 0);
					_currentPage++;

					addOnePage();
					m_pMaxRectsBinPack->Init(TileAtlas::CacheTextureWidth, TileAtlas::CacheTextureHeight, false);
					rcResult = m_pMaxRectsBinPack->Insert(bitmapWidth, bitmapHeight, rbp::MaxRectsBinPack::RectBestShortSideFit);
				}
				assert(rcResult.width != 0 && rcResult.height != 0);

				//
				PixWriter_t pw(pixelFormat, _currentPageData, NS_CC::TileAtlas::CacheTextureWidth, NS_CC::TileAtlas::CacheTextureHeight);
				pw.renderTileAt(rcResult.x, rcResult.y , bitmap, bitmapWidth, bitmapHeight, pix_format);

				tempDef->U = rcResult.x;
				tempDef->V = rcResult.y;
				tempDef->textureID = _currentPage;

				// take from pixels to points
				tempDef->width = tempDef->width / scaleFactor;
				tempDef->height = tempDef->height / scaleFactor;
				tempDef->U = tempDef->U / scaleFactor;
				tempDef->V = tempDef->V / scaleFactor;

				if (shouldDelData)
				{
					delete[] bitmap;
				}
			}
			else {
				if (bitmap)
				{
					assert(false);
					//delete[] bitmap;
				}

				tempDef->validDefinition = false;
				tempDef->width = 0;
				tempDef->height = 0;
				tempDef->U = 0;
				tempDef->V = 0;
				tempDef->offsetX = 0;
				tempDef->offsetY = 0;
				tempDef->textureID = 0;
				_currentPageOrigX += 1;
			}
			//Register or Link Def
			if (k == 0)
			{
				_letterDefinitions[it.id] = tempDef;
			}
			else
			{
				preDef->pNext = tempDef;
			}
			preDef = tempDef;
			if (tempDef->validDefinition == false)
			{
				Assert(false);
				break;
			}
		}//end count
	}//end chars

	//partly update...	//
	unsigned char *data = _currentPageData;
	_atlasTextures[_currentPage]->updateWithData(data, 0, 0, CacheTextureWidth, CacheTextureHeight);
	return true;
}

#endif
//muti tile prepare
bool TileAtlas::prepareLetterDefinitions(const TileString& utf32Text)
{
	TileString newChars;
	findNewCharacters(utf32Text, newChars);
	if (newChars.empty())
	{
		return false;
	}

	int adjustForDistanceMap = _letterPadding / 2;
	int adjustForExtend = _letterEdgeExtend / 2;
	int bitmapWidth;
	int bitmapHeight;
	int glyphHeight;
	//Rect tempRect;

	auto scaleFactor =  CC_CONTENT_SCALE_FACTOR();

	TileLetterDefinition *preDef;// = NULL;
	float startY = _currentPageOrigY;
	int pix_format = 0;
	const int MAX_TBS = 64;
	TileBitmaps_t tbs[MAX_TBS];
	for (auto&& it : newChars)
	{
		//from TextureManager: 1. ��֡
		int count = getTileBitmaps(it.id, tbs, MAX_TBS);
		//unsigned char* bitmap = getTileBitmap(it.id, bitmapWidth, bitmapHeight, tempRect, pix_format);
		for (int k = 0; k < count; k++)
		{
			TileLetterDefinition *tempDef = TileLetterDefinition::New();

			unsigned char* bitmap = tbs[k].pData;
			bitmapWidth = tbs[k].outWidth;
			bitmapHeight= tbs[k].outHeight;
			pix_format = tbs[k].format;
			Rect &tempRect = tbs[k].outRect;
			bool shouldDelData = tbs[k].shouldDelData;
			if (bitmap && bitmapWidth > 0 && bitmapHeight > 0)
			{
				//check format...
				//assert(matchFormat(pix_format, pixelFormat));

				tempDef->validDefinition = true;
				tempDef->width = tempRect.size.width + _letterPadding + _letterEdgeExtend;
				tempDef->height = tempRect.size.height + _letterPadding + _letterEdgeExtend;
				tempDef->offsetX = tempRect.origin.x - adjustForDistanceMap - adjustForExtend;
				tempDef->offsetY = _fontAscender + tempRect.origin.y - adjustForDistanceMap - adjustForExtend;

				//�п�����
				if (_currentPageOrigX + tempDef->width > CacheTextureWidth)
				{
					_currentPageOrigY += _currLineHeight;
					_currLineHeight = 0;
					_currentPageOrigX = 0;
				}
				//�и߳�����
				if (_currentPageOrigY + bitmapHeight + _letterPadding + _letterEdgeExtend >= CacheTextureHeight)
				{
					unsigned char *data = _currentPageData;
					if (startY != 0)
					{
						//assert(startY == 0);
						data += CacheTextureWidth * (int)startY * PixelFormatBits((Texture2D::PixelFormat)pixelFormat) / 8;
					}
					_atlasTextures[_currentPage]->updateWithData(data, 0, startY,
						CacheTextureWidth, CacheTextureHeight - startY);
					startY = 0.0f;
					_currentPageOrigY = 0;
					_currentPage++;

					addOnePage();
				}

				glyphHeight = static_cast<int>(bitmapHeight) + _letterPadding + _letterEdgeExtend;
				if (glyphHeight > _currLineHeight)
				{
					_currLineHeight = glyphHeight;
				}
				//
				PixWriter_t pw(pixelFormat, _currentPageData, NS_CC::TileAtlas::CacheTextureWidth, NS_CC::TileAtlas::CacheTextureHeight);
				pw.renderTileAt(_currentPageOrigX + adjustForExtend, _currentPageOrigY + adjustForExtend, bitmap, bitmapWidth, bitmapHeight, pix_format);

				tempDef->U = _currentPageOrigX;
				tempDef->V = _currentPageOrigY;
				tempDef->textureID = _currentPage;
				_currentPageOrigX += tempDef->width;// + 1
				// take from pixels to points
				tempDef->width = tempDef->width / scaleFactor;
				tempDef->height = tempDef->height / scaleFactor;
				tempDef->U = tempDef->U / scaleFactor;
				tempDef->V = tempDef->V / scaleFactor;

				if (shouldDelData)
				{
					delete[] bitmap;
				}
			}
			else {
				if (bitmap)
				{
					assert(false);
					//delete[] bitmap;
				}

				tempDef->validDefinition = false;
				tempDef->width = 0;
				tempDef->height = 0;
				tempDef->U = 0;
				tempDef->V = 0;
				tempDef->offsetX = 0;
				tempDef->offsetY = 0;
				tempDef->textureID = 0;
				_currentPageOrigX += 1;
			}
			//Register or Link Def
			if (k == 0)
			{
				_letterDefinitions[it.id] = tempDef;
			}
			else
			{
				preDef->pNext = tempDef;
			}
			preDef = tempDef;
			if (tempDef->validDefinition == false)
			{
				Assert(false);
				break;
			}
		}//end count
	}//end chars

	//partly update...
	int height = _currentPageOrigY - startY + _currLineHeight;//CacheTextureHeight;//
	//
	unsigned char *data = _currentPageData;
	if (startY != 0)
	{
		//assert(false);
		data += CacheTextureWidth * (int)startY * PixelFormatBits((Texture2D::PixelFormat)pixelFormat)/8;

		assert(startY + height <= CacheTextureHeight);
	}
	_atlasTextures[_currentPage]->updateWithData(data, 0, startY, CacheTextureWidth, height);
	return true;
}

void TileAtlas::addTexture(Texture2D *texture, int slot)
{
	texture->retain();
	_atlasTextures[slot] = texture;
}

Texture2D* TileAtlas::getTexture(int slot)
{
	return _atlasTextures[slot];
}

std::string TileAtlas::getTileName() const
{
	std::string fontName = "";//_fontFreeType ? _fontFreeType->getFontName() : 
	if (fontName.empty()) return fontName;
	auto idx = fontName.rfind("/");
	if (idx != std::string::npos) { return fontName.substr(idx + 1); }
	idx = fontName.rfind("\\");
	if (idx != std::string::npos) { return fontName.substr(idx + 1); }
	return fontName;
}

void TileAtlas::setAliasTexParameters()
{
	if (_antialiasEnabled)
	{
		_antialiasEnabled = false;
		for (const auto & tex : _atlasTextures)
		{
			tex.second->setAliasTexParameters();
		}
	}
}

void TileAtlas::setAntiAliasTexParameters()
{
	if (!_antialiasEnabled)
	{
		_antialiasEnabled = true;
		for (const auto & tex : _atlasTextures)
		{
			tex.second->setAntiAliasTexParameters();
		}
	}
}

NS_CC_END
