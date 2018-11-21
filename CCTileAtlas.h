#ifndef _CCTileAtlas_h_
#define _CCTileAtlas_h_


#include "platform/CCPlatformMacros.h"
#include "base/CCRef.h"
#include "platform/CCStdC.h" // ssize_t on windows
#include <string>
#include <vector>
#include <unordered_map>

NS_CC_BEGIN

class Texture2D;
class EventCustom;
class EventListenerCustom;

typedef float UAxis;

//ref : 
struct TileLetterDefinition
{
	UAxis U;		//纹理在atlas上的位置.
	UAxis V;
	UAxis width;	//宽高
	UAxis height;
	UAxis offsetX;	//
	UAxis offsetY;
	int textureID;
	bool validDefinition;
};

/**
*/
#define CC_DEFAULT_TILE_LABEL_SIZE  32
/**
* @struct TILEConfig
* @see `GlyphCollection`
*/
typedef struct _tileConfig
{
	std::string tileFilePath;
	float tileSize;
	int  pixelFormat;
	_tileConfig(float size = CC_DEFAULT_TILE_LABEL_SIZE) :tileSize(size)
	{
	}
} TILEConfig;

//
typedef unsigned int TileID;
struct TileChar
{
	TileChar(TileID _id,int _x,int _y):id(_id), x(_x), y(_y)
	{
	}
	TileID id;	//TODO: FrameIndex,PackIndex,TexID
	int x,y;
};
typedef std::vector<TileChar> TileString;
//typedef std::u32string TileString;	//
typedef std::unordered_map<ssize_t, Texture2D*> TTexture2DMap;
//ref : FontFreeType
//TODO : use global buffer
class TileAtlas : public Ref
{
public:
	static const int CacheTextureWidth;
	static const int CacheTextureHeight;
	static const char* CMD_PURGE_TILEATLAS;
	static const char* CMD_RESET_TILEATLAS;
	/**
	* @js ctor
	*/
	TileAtlas(int pix_format);
	/**
	* @js NA
	* @lua NA
	*/
	virtual ~TileAtlas();

	void addLetterDefinition(TileID utf32Char, const TileLetterDefinition &letterDefinition);
	bool getLetterDefinitionForChar(TileID utf32Char, TileLetterDefinition &letterDefinition);

	bool prepareLetterDefinitions(const TileString& utf16String);

	const TTexture2DMap& getTextures() const { return _atlasTextures; }
	void  addTexture(Texture2D *texture, int slot);
	float getLineHeight() const { return _lineHeight; }
	void  setLineHeight(float newHeight) { _lineHeight = newHeight; }

	std::string getTileName() const;

	Texture2D* getTexture(int slot);

	/** listen the event that renderer was recreated on Android/WP8
	It only has effect on Android and WP8.
	*/
	void listenRendererRecreated(EventCustom *event);

	/** Removes textures atlas.
	It will purge the textures atlas and if multiple texture exist in the TileAtlas.
	*/
	void purgeTexturesAtlas();

	/** sets font texture parameters:
	- GL_TEXTURE_MIN_FILTER = GL_LINEAR
	- GL_TEXTURE_MAG_FILTER = GL_LINEAR
	*/
	void setAntiAliasTexParameters();

	/** sets font texture parameters:
	- GL_TEXTURE_MIN_FILTER = GL_NEAREST
	- GL_TEXTURE_MAG_FILTER = GL_NEAREST
	*/
	void setAliasTexParameters();

protected:
	void reset();
	void reinit();
	void releaseTextures();
	void findNewCharacters(const TileString& u32Text, TileString &newChars);

	Texture2D *addOnePage();
	/**
	* Scale each font letter by scaleFactor.
	*
	* @param scaleFactor A float scale factor for scaling font letter info.
	*/
	void scaleFontLetterDefinition(float scaleFactor);

	TTexture2DMap _atlasTextures;
	std::unordered_map<TileID, TileLetterDefinition> _letterDefinitions;
	float _lineHeight;

	// Dynamic GlyphCollection related stuff
	int  pixelFormat;//Texture2D::PixelFormat

	int _currentPage;
	unsigned char *_currentPageData;
	int _currentPageDataSize;
	float _currentPageOrigX;
	float _currentPageOrigY;

	int _letterPadding;
	int _letterEdgeExtend;

	int _fontAscender;
	EventListenerCustom* _rendererRecreatedListener;
	bool _antialiasEnabled;	//
	int _currLineHeight;

	friend class TileSceneLayer;
};






////管理Tile的快速绘制
//class CSceneTile
//{
//public:
//
//	//// 显示大地表
//	//if (sMap[i].wTile)
//	//{
//	//	pTex = GetTex(PACKAGE_Tiles, sMap[i].wTile, nPriority);
//	//	if (pTex)
//	//		sprite->DrawTexture(x0, y0, pTex);
//	//}
//
//
//};

NS_CC_END

/// @endcond
#endif /* defined() */