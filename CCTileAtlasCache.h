
#ifndef _CCTileAtlasCache_h_
#define _CCTileAtlasCache_h_

 /// @cond DO_NOT_SHOW

#include <unordered_map>
#include "base/ccTypes.h"

NS_CC_BEGIN

class TileAtlas;
class Texture2D;
struct _tileConfig;

//for [small big object] :
class TileAtlasCache
{
public:
	static TileAtlas* getTileAtlasTTF(const _tileConfig* config);
	static bool releaseTileAtlas(TileAtlas *atlas);

	/** Removes cached data.
	 It will purge the textures atlas and if multiple texture exist in one TileAtlas.
	 */
	static void purgeCachedData();

	/** Unload all texture atlas texture create by special file name.
	 CAUTION : All component use this font texture should be reset font name, though the file name is same!
			   otherwise, it will cause program crash!
	*/
	static void unloadTileAtlasTTF(const std::string& fontFileName);

private:
	static std::unordered_map<std::string, TileAtlas *> _atlasMap;
};

NS_CC_END

/// @endcond
#endif
