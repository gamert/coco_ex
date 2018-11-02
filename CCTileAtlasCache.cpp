
#include "CCTileAtlasCache.h"
//#include "base/CCDirector.h"
#include "CCTileAtlas.h"
//#include "CCTileSceneLayer.h"
//#include "platform/CCFileUtils.h"

NS_CC_BEGIN

std::unordered_map<std::string, TileAtlas *> TileAtlasCache::_atlasMap;
#define ATLAS_MAP_KEY_BUFFER 255

void TileAtlasCache::purgeCachedData()
{
	auto atlasMapCopy = _atlasMap;
	for (auto&& atlas : atlasMapCopy)
	{
		auto refCount = atlas.second->getReferenceCount();
		atlas.second->release();
		if (refCount != 1)
			atlas.second->purgeTexturesAtlas();
	}
	_atlasMap.clear();
}

TileAtlas* TileAtlasCache::getTileAtlasTTF(const _tileConfig* config)
{
	auto realTileFilename = config->tileFilePath;

	char tmp[ATLAS_MAP_KEY_BUFFER];
	snprintf(tmp, ATLAS_MAP_KEY_BUFFER, "%.2f %s", config->tileSize, realTileFilename.c_str());
	std::string atlasName = tmp;
	auto it = _atlasMap.find(atlasName);
	if (it == _atlasMap.end())
	{
		//auto font = FontFreeType::create(realTileFilename, config->fontSize, config->glyphs,
		//	config->customGlyphs, useDistanceField, config->outlineSize);
		//if (font)
		//{
		//	auto tempAtlas = font->createFontAtlas();
		auto tempAtlas = new (std::nothrow) TileAtlas();
		if (tempAtlas)
		{
			_atlasMap[atlasName] = tempAtlas;
			return _atlasMap[atlasName];
		}
		//}
	}
	else
		return it->second;

	return nullptr;
}

bool TileAtlasCache::releaseTileAtlas(TileAtlas *atlas)
{
	if (nullptr != atlas)
	{
		for (auto &item : _atlasMap)
		{
			if (item.second == atlas)
			{
				if (atlas->getReferenceCount() == 1)
				{
					_atlasMap.erase(item.first);
				}
				atlas->release();
				return true;
			}
		}
	}

	return false;
}


void TileAtlasCache::unloadTileAtlasTTF(const std::string& fontFileName)
{
	auto item = _atlasMap.begin();
	while (item != _atlasMap.end())
	{
		if (item->first.find(fontFileName) != std::string::npos)
		{
			CC_SAFE_RELEASE_NULL(item->second);
			item = _atlasMap.erase(item);
		}
		else
			item++;
	}
}

NS_CC_END
