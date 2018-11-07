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

inline int PixelFormatBits(cocos2d::Texture2D::PixelFormat p)
{
	switch (p)
	{
	case cocos2d::Texture2D::PixelFormat::AUTO:
		break;
	case cocos2d::Texture2D::PixelFormat::BGRA8888:
		return 4 * 8;
	case cocos2d::Texture2D::PixelFormat::RGBA8888:
		return 4 * 8;
	case cocos2d::Texture2D::PixelFormat::RGB888:
		return 3 * 8;
	case cocos2d::Texture2D::PixelFormat::RGB565:
		return 2 * 8;
	case cocos2d::Texture2D::PixelFormat::A8:
		return 8;
	case cocos2d::Texture2D::PixelFormat::I8:
		break;
	case cocos2d::Texture2D::PixelFormat::AI88:
		break;
	case cocos2d::Texture2D::PixelFormat::RGBA4444:
		break;
	case cocos2d::Texture2D::PixelFormat::RGB5A1:
		break;
	case cocos2d::Texture2D::PixelFormat::PVRTC4:
		break;
	case cocos2d::Texture2D::PixelFormat::PVRTC4A:
		break;
	case cocos2d::Texture2D::PixelFormat::PVRTC2:
		break;
	case cocos2d::Texture2D::PixelFormat::PVRTC2A:
		break;
	case cocos2d::Texture2D::PixelFormat::ETC:
		return 4;
	case cocos2d::Texture2D::PixelFormat::S3TC_DXT1:
		return 4;
	case cocos2d::Texture2D::PixelFormat::S3TC_DXT3:
		return 8;
	case cocos2d::Texture2D::PixelFormat::S3TC_DXT5:
		return 8;
	case cocos2d::Texture2D::PixelFormat::ATC_RGB:
		break;
	case cocos2d::Texture2D::PixelFormat::ATC_EXPLICIT_ALPHA:
		break;
	case cocos2d::Texture2D::PixelFormat::ATC_INTERPOLATED_ALPHA:
		break;
	case cocos2d::Texture2D::PixelFormat::NONE:
		break;
	default:
		break;
	}
	assert(false);
	return 0;
}

//use 
unsigned char* getTileBitmap(uint64_t theChar, long &outWidth, long &outHeight, NS_CC::Rect &outRect, int &format);
typedef unsigned char      uint8;
bool PIX_DXT3_DXT3(uint8 *pDst, int pitchs1, int h1, uint8 *pSrc, int w0, int h0);
bool PIX_DXT1_DXT1(uint8 *pDst, int pitchs1, int h1, uint8 *pSrc, int w0, int h0);
bool PIX_ETC1_ETC1(uint8 *pDst, int pitchs1, int h1, uint8 *pSrc, int w0, int h0);
bool PIX_DXT3_RGB888(uint8 *pDst, int pitchs1, int h1, uint8 *pSrc, int w0, int h0);
bool PIX_DXT3_RGBA8888(uint8 *pDst, int pitchs1, int h1, uint8 *pSrc, int w0, int h0);

NS_CC_BEGIN

//TODO: 测试大小是否有效降低mem dc等...
const int TileAtlas::CacheTextureWidth = 1024;
const int TileAtlas::CacheTextureHeight = 1024;
const char* TileAtlas::CMD_PURGE_TILEATLAS = "__cc_PURGE_TILEATLAS";
const char* TileAtlas::CMD_RESET_TILEATLAS = "__cc_RESET_TILEATLAS";

//const int TileAtlas::DEF_FORMAT = (int)(Texture2D::PixelFormat::S3TC_DXT3);//A8

//绘制内存位图(格式转换):
//dst_format: 目标格式,需要调整原图格式...
void renderTileAt(int dst_format, uint8 *dest, int posX, int posY, uint8* bitmap, long bitmapWidth, long bitmapHeight)
{
	const int bits = PixelFormatBits((Texture2D::PixelFormat)dst_format);
	uint8 *pDst = dest + (posY * NS_CC::TileAtlas::CacheTextureWidth + posX) * bits / 8;
	int pitchs1 = NS_CC::TileAtlas::CacheTextureWidth * bits / 8;
	switch ((Texture2D::PixelFormat)dst_format)
	{
	case Texture2D::PixelFormat::RGB888:
		PIX_DXT3_RGB888(pDst, pitchs1, NS_CC::TileAtlas::CacheTextureHeight - posY, bitmap, bitmapWidth, bitmapHeight);
		return;
	case Texture2D::PixelFormat::RGBA8888:
		PIX_DXT3_RGBA8888(pDst, pitchs1, NS_CC::TileAtlas::CacheTextureHeight - posY, bitmap, bitmapWidth, bitmapHeight);
		return;
	case Texture2D::PixelFormat::S3TC_DXT3:
	{
		pitchs1 = (NS_CC::TileAtlas::CacheTextureWidth >> 2) * 16;
		pDst = dest + (posY>>2)*pitchs1 + (posX * 4);
		PIX_DXT3_DXT3(pDst, pitchs1, NS_CC::TileAtlas::CacheTextureHeight - posY, bitmap, bitmapWidth, bitmapHeight);
		return;
	}
	case Texture2D::PixelFormat::S3TC_DXT1:
	{
		pitchs1 = (NS_CC::TileAtlas::CacheTextureWidth >> 2) * 8;
		pDst = dest + (posY >> 2)*pitchs1 + (posX * 2);
		PIX_DXT1_DXT1(pDst, pitchs1, NS_CC::TileAtlas::CacheTextureHeight - posY, bitmap, bitmapWidth, bitmapHeight);
		return;
	}
	case Texture2D::PixelFormat::ETC:
	{
		pitchs1 = NS_CC::TileAtlas::CacheTextureWidth * 2;
		pDst = dest + (posY >> 2)*pitchs1 + (posX * 2);
		PIX_ETC1_ETC1(pDst, pitchs1, NS_CC::TileAtlas::CacheTextureHeight - posY, bitmap, bitmapWidth, bitmapHeight);
		return;
	}
	case Texture2D::PixelFormat::A8:
		break;
	default:
		assert(false);
	}
	

	int iX = posX;
	int iY = posY;
	{
		for (long y = 0; y < bitmapHeight; ++y)
		{
			long bitmap_y = y * bitmapWidth;
			for (int x = 0; x < bitmapWidth; ++x)
			{
				unsigned char cTemp = bitmap[bitmap_y + x];
				// the final pixel
				dest[(iX + (iY * NS_CC::TileAtlas::CacheTextureWidth))] = 0xFF;
				iX += 1;
			}
			iX = posX;
			iY += 1;
		}
	}
}

TileAtlas::TileAtlas(int pix_format)
	: _currentPageData(nullptr)
	, _fontAscender(0)
	, _rendererRecreatedListener(nullptr)
	, _antialiasEnabled(true)
	, _currLineHeight(0)
{
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
#if CC_ENABLE_CACHE_TEXTURE_DATA
	if (_fontFreeType && _rendererRecreatedListener)
	{
		auto eventDispatcher = Director::getInstance()->getEventDispatcher();
		eventDispatcher->removeEventListener(_rendererRecreatedListener);
		_rendererRecreatedListener = nullptr;
	}
#endif

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
	}
}

void TileAtlas::listenRendererRecreated(EventCustom * /*event*/)
{
	purgeTexturesAtlas();
}

void TileAtlas::addLetterDefinition(TileID utf32Char, const TileLetterDefinition &letterDefinition)
{
	_letterDefinitions[utf32Char] = letterDefinition;
}

void TileAtlas::scaleFontLetterDefinition(float scaleFactor)
{
	for (auto&& fontDefinition : _letterDefinitions) {
		auto& letterDefinition = fontDefinition.second;
		letterDefinition.width *= scaleFactor;
		letterDefinition.height *= scaleFactor;
		letterDefinition.offsetX *= scaleFactor;
		letterDefinition.offsetY *= scaleFactor;
	}
}

bool TileAtlas::getLetterDefinitionForChar(TileID utf32Char, TileLetterDefinition &letterDefinition)
{
	auto outIterator = _letterDefinitions.find(utf32Char);

	if (outIterator != _letterDefinitions.end())
	{
		letterDefinition = (*outIterator).second;
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
	long bitmapWidth;
	long bitmapHeight;
	int glyphHeight;
	Rect tempRect;
	TileLetterDefinition tempDef;

	auto scaleFactor = CC_CONTENT_SCALE_FACTOR();

	float startY = _currentPageOrigY;
	int pix_format = 0;
	for (auto&& it : newChars)
	{
		//from TextureManager: TODO: 多帧如何处理？
		unsigned char* bitmap = getTileBitmap(it.id, bitmapWidth, bitmapHeight, tempRect, pix_format);
		if (bitmap && bitmapWidth > 0 && bitmapHeight > 0)
		{
			//check format...
			//assert(matchFormat(pix_format, pixelFormat));

			tempDef.validDefinition = true;
			tempDef.width = tempRect.size.width + _letterPadding + _letterEdgeExtend;
			tempDef.height = tempRect.size.height + _letterPadding + _letterEdgeExtend;
			tempDef.offsetX = tempRect.origin.x - adjustForDistanceMap - adjustForExtend;
			tempDef.offsetY = _fontAscender + tempRect.origin.y - adjustForDistanceMap - adjustForExtend;

			if (_currentPageOrigX + tempDef.width > CacheTextureWidth)
			{
				_currentPageOrigY += _currLineHeight;
				_currLineHeight = 0;
				_currentPageOrigX = 0;
				if (_currentPageOrigY + _lineHeight + _letterPadding + _letterEdgeExtend >= CacheTextureHeight)
				{
					unsigned char *data = _currentPageData;
					if (startY != 0)
					{
						assert(startY == 0);
						data += CacheTextureWidth * (int)startY * PixelFormatBits((Texture2D::PixelFormat)pixelFormat)/8;
					}
					_atlasTextures[_currentPage]->updateWithData(data, 0, startY,
						CacheTextureWidth, CacheTextureHeight - startY);
					startY = 0.0f;
					_currentPageOrigY = 0;
					_currentPage++;

					addOnePage();
				}
			}
			glyphHeight = static_cast<int>(bitmapHeight) + _letterPadding + _letterEdgeExtend;
			if (glyphHeight > _currLineHeight)
			{
				_currLineHeight = glyphHeight;
			}

			renderTileAt(pixelFormat,_currentPageData, _currentPageOrigX + adjustForExtend, _currentPageOrigY + adjustForExtend, bitmap, bitmapWidth, bitmapHeight);

			tempDef.U = _currentPageOrigX;
			tempDef.V = _currentPageOrigY;
			tempDef.textureID = _currentPage;
			_currentPageOrigX += tempDef.width;// + 1
			// take from pixels to points
			tempDef.width = tempDef.width / scaleFactor;
			tempDef.height = tempDef.height / scaleFactor;
			tempDef.U = tempDef.U / scaleFactor;
			tempDef.V = tempDef.V / scaleFactor;
		}
		else {
			if (bitmap)
			{
				assert(false);
				//delete[] bitmap;
			}

			tempDef.validDefinition = false;
			tempDef.width = 0;
			tempDef.height = 0;
			tempDef.U = 0;
			tempDef.V = 0;
			tempDef.offsetX = 0;
			tempDef.offsetY = 0;
			tempDef.textureID = 0;
			_currentPageOrigX += 1;
		}
		_letterDefinitions[it.id] = tempDef;
	}

	//
	unsigned char *data = _currentPageData;
	if (startY != 0)
	{
		assert(false);
		data += CacheTextureWidth * (int)startY * PixelFormatBits((Texture2D::PixelFormat)pixelFormat)/8;
	}
	//partly update...
	int height = _currentPageOrigY - startY + _currLineHeight;//CacheTextureHeight;//
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
