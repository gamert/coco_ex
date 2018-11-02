
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

NS_CC_BEGIN

const int TileAtlas::CacheTextureWidth = 512;
const int TileAtlas::CacheTextureHeight = 512;
const char* TileAtlas::CMD_PURGE_TILEATLAS = "__cc_PURGE_TILEATLAS";
const char* TileAtlas::CMD_RESET_TILEATLAS = "__cc_RESET_TILEATLAS";

unsigned char* getTileBitmap(uint64_t theChar, long &outWidth, long &outHeight, Rect &outRect);

void renderTileAt(unsigned char *dest, int posX, int posY, unsigned char* bitmap, long bitmapWidth, long bitmapHeight)
{
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
				dest[(iX + (iY * TileAtlas::CacheTextureWidth))] = cTemp;

				iX += 1;
			}

			iX = posX;
			iY += 1;
		}
	}
}


TileAtlas::TileAtlas()
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
		_letterEdgeExtend = 2;
		_letterPadding = 0;

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
	auto texture = new (std::nothrow) Texture2D;

	_currentPageDataSize = CacheTextureWidth * CacheTextureHeight;

	_currentPageData = new (std::nothrow) unsigned char[_currentPageDataSize];
	memset(_currentPageData, 0, _currentPageDataSize);

	auto  pixelFormat = Texture2D::PixelFormat::A8;
	texture->initWithData(_currentPageData, _currentPageDataSize,
		pixelFormat, CacheTextureWidth, CacheTextureHeight, Size(CacheTextureWidth, CacheTextureHeight));

	addTexture(texture, 0);
	texture->release();
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

void TileAtlas::addLetterDefinition(TileID utf32Char, const TileDefinition &letterDefinition)
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

bool TileAtlas::getLetterDefinitionForChar(TileID utf32Char, TileDefinition &letterDefinition)
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


void TileAtlas::findNewCharacters(const TileString& u32Text, TCharCodeMap& charCodeMap)
{
	TileString newChars;
	if (_letterDefinitions.empty())
	{
		newChars.append(u32Text);
	}
	else
	{
		auto length = u32Text.length();
		newChars.reserve(length);
		for (size_t i = 0; i < length; ++i)
		{
			auto outIterator = _letterDefinitions.find(u32Text[i]);
			if (outIterator == _letterDefinitions.end())
			{
				newChars.push_back(u32Text[i]);
			}
		}
	}
}

//muti tile prepare
bool TileAtlas::prepareLetterDefinitions(const TileString& utf32Text)
{
	TCharCodeMap codeMapOfNewChar;
	findNewCharacters(utf32Text, codeMapOfNewChar);
	if (codeMapOfNewChar.empty())
	{
		return false;
	}

	int adjustForDistanceMap = _letterPadding / 2;
	int adjustForExtend = _letterEdgeExtend / 2;
	long bitmapWidth;
	long bitmapHeight;
	int glyphHeight;
	Rect tempRect;
	TileDefinition tempDef;

	auto scaleFactor = CC_CONTENT_SCALE_FACTOR();
	auto  pixelFormat = Texture2D::PixelFormat::A8;

	float startY = _currentPageOrigY;

	for (auto&& it : codeMapOfNewChar)
	{
		//from TextureManager
		unsigned char* bitmap = getTileBitmap(it.second, bitmapWidth, bitmapHeight, tempRect);
		if (bitmap && bitmapWidth > 0 && bitmapHeight > 0)
		{
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
					unsigned char *data = nullptr;
					data = _currentPageData + CacheTextureWidth * (int)startY;

					_atlasTextures[_currentPage]->updateWithData(data, 0, startY,
						CacheTextureWidth, CacheTextureHeight - startY);

					startY = 0.0f;

					_currentPageOrigY = 0;
					memset(_currentPageData, 0, _currentPageDataSize);
					_currentPage++;
					auto tex = new (std::nothrow) Texture2D;
					if (_antialiasEnabled)
					{
						tex->setAntiAliasTexParameters();
					}
					else
					{
						tex->setAliasTexParameters();
					}
					tex->initWithData(_currentPageData, _currentPageDataSize,
						pixelFormat, CacheTextureWidth, CacheTextureHeight, Size(CacheTextureWidth, CacheTextureHeight));
					addTexture(tex, _currentPage);
					tex->release();
				}
			}
			glyphHeight = static_cast<int>(bitmapHeight) + _letterPadding + _letterEdgeExtend;
			if (glyphHeight > _currLineHeight)
			{
				_currLineHeight = glyphHeight;
			}

			renderTileAt(_currentPageData, _currentPageOrigX + adjustForExtend, _currentPageOrigY + adjustForExtend, bitmap, bitmapWidth, bitmapHeight);

			tempDef.U = _currentPageOrigX;
			tempDef.V = _currentPageOrigY;
			tempDef.textureID = _currentPage;
			_currentPageOrigX += tempDef.width + 1;
			// take from pixels to points
			tempDef.width = tempDef.width / scaleFactor;
			tempDef.height = tempDef.height / scaleFactor;
			tempDef.U = tempDef.U / scaleFactor;
			tempDef.V = tempDef.V / scaleFactor;
		}
		else {
			if (bitmap)
				delete[] bitmap;

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

		_letterDefinitions[it.first] = tempDef;
	}

	unsigned char *data = nullptr;
	data = _currentPageData + CacheTextureWidth * (int)startY;
	_atlasTextures[_currentPage]->updateWithData(data, 0, startY, CacheTextureWidth, _currentPageOrigY - startY + _currLineHeight);
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
