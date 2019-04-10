
#ifndef _CCTileScene_H_
#define _CCTileScene_H_

#include "2d/CCNode.h"
#include "renderer/CCCustomCommand.h"
#include "renderer/CCQuadCommand.h"
#include "CCTileAtlas.h"
#include "base/ccTypes.h"

NS_CC_BEGIN



/** @struct TileDefinition
 * Font attributes.
 */
struct TileDefinition
{
public:
	/**
	 * @js NA
	 * @lua NA
	 */
	TileDefinition()
		: _tileSize(0)
		, _dimensions(Size::ZERO)
		, _tileFillColor(Color3B::WHITE)
		, _tileAlpha(255)
	{}
	/// tile name
	std::string           _tileName;
	/// tile size
	int                   _tileSize;
	/// rendering box
	Size                  _dimensions;
	/// tile color
	Color3B               _tileFillColor;
	/// tile alpha
	GLubyte               _tileAlpha;
};




class Sprite;
class SpriteBatchNode;
class DrawNode;
class EventListenerCustom;

/**
* @brief TileSceneLayer is a subclass of Node that knows how to render tile labels.
* @js NA
*/
class TileSceneLayer : public Node, public BlendProtocol
{
public:
	/// @name Creators
	/// @{

	/**
	* Allocates and initializes a TileSceneLayer, with default settings.
	*
	* @return An automatically released TileSceneLayer object.
	*/
	static TileSceneLayer* create();

	/**
	* init.
	*/
	bool setTILEConfig(const TILEConfig& ttfConfig);
	const TILEConfig& getTILEConfig() const { return _tileConfig; }
	/**
	* set content data.
	*/
	void setTileString(TileString &_tiles)
	{
		_utf32Text = _tiles;
		_contentDirty = true;
	}
	void addTile(TileID id, int x, int y,int z, unsigned int Color, float scaleX)
	{
		_utf32Text.push_back(TileChar(id,x, y, z, Color, scaleX));
		_contentDirty = true;
	}
	void clearTiles()
	{
		_utf32Text.clear();
		_contentDirty = true;
	}
	/**
	* Return length of string.
	*/
	int getStringLength();

	/**
	* Sets the text color of TileSceneLayer.
	* The text color is different from the color of Node.
	* @warning Limiting use to only when the TileSceneLayer created with true type tile or system tile.
	*/
	virtual void setTextColor(const Color4B &color);

	/** Returns the text color of the TileSceneLayer.*/
	const Color4B& getTextColor() const { return _textColor; }

	/**
	* Makes the TileSceneLayer exactly this untransformed width.
	*
	* The TileSceneLayer's width be used for text align if the value not equal zero.
	*/
	void setWidth(float width) { setDimensions(width, _labelHeight); }
	float getWidth() const { return _labelWidth; }

	/**
	* Makes the TileSceneLayer exactly this untransformed height.
	*
	* The TileSceneLayer's height be used for text align if the value not equal zero.
	* The text will display incomplete if the size of TileSceneLayer is not large enough to display all text.
	*/
	void setHeight(float height) { setDimensions(_labelWidth, height); }
	float getHeight() const { return _labelHeight; }

	/** Sets the untransformed size of the TileSceneLayer in a more efficient way. */
	void setDimensions(float width, float height);
	const Size& getDimensions() const { return _labelDimensions; }

	/** Update content immediately.*/
	virtual void updateContent();

	/**
	* Provides a way to treat each character like a Sprite.
	* @warning No support system tile.
	*/
	virtual Sprite * getLetter(int lettetIndex);

	/** Clips upper and lower margin to reduce height of TileSceneLayer.*/
	void setClipMarginEnabled(bool clipEnabled) { _clipEnabled = clipEnabled; }

	bool isClipMarginEnabled() const { return _clipEnabled; }

	/**
	Returns tile size
	*/
	float getRenderingTileSize()const;

	TileAtlas* getTileAtlas() { return _tileAtlas; }

	virtual const BlendFunc& getBlendFunc() const override { return _blendFunc; }
	virtual void setBlendFunc(const BlendFunc &blendFunc) override;

	virtual bool isOpacityModifyRGB() const override { return _isOpacityModifyRGB; }
	virtual void setOpacityModifyRGB(bool isOpacityModifyRGB) override;
	virtual void updateDisplayedColor(const Color3B& parentColor) override;
	virtual void updateDisplayedOpacity(GLubyte parentOpacity) override;

	virtual std::string getDescription() const override;

	virtual const Size& getContentSize() const override;
	virtual Rect getBoundingBox() const override;

	virtual void visit(Renderer *renderer, const Mat4 &parentTransform, uint32_t parentFlags) override;
	virtual void draw(Renderer *renderer, const Mat4 &transform, uint32_t flags) override;

	virtual void setCameraMask(unsigned short mask, bool applyChildren = true) override;

	virtual void removeAllChildrenWithCleanup(bool cleanup) override;
	virtual void removeChild(Node* child, bool cleanup = true) override;


CC_CONSTRUCTOR_ACCESS:
	/**
	* Constructor of TileSceneLayer.
	* @js NA
	*/
	TileSceneLayer();

	/**
	* Destructor of TileSceneLayer.
	* @js NA
	* @lua NA
	*/
	virtual ~TileSceneLayer();

protected:
	struct LetterInfo
	{
		TileID utf32Char;
		bool valid;
		float positionX;
		float positionY;
		float positionZ;
		int atlasIndex;
		unsigned int color;
		float scaleX;
		//int lineIndex;
	};

	virtual void setTileAtlas(TileAtlas* atlas, bool distanceFieldEnabled = false, bool useA8Shader = false);
	bool getTileLetterDef(TileID character, TileLetterDefinition& letterDef) const;

	void onDraw(const Mat4& transform, bool transformUpdated);
	void drawSelf(bool visibleByCamera, Renderer* renderer, uint32_t flags);

	void rescaleWithOriginalTileSize();

	void updateLabelLetters();
	virtual bool alignText();

	void recordLetterInfo(const cocos2d::Vec3& point, TileID utf32Char, int letterIndex, int lineIndex,unsigned int color ,float scaleX /*= 1.0f*/);
	void recordPlaceholderInfo(int letterIndex, TileID utf16Char);

	bool updateQuads();

	virtual void updateShaderProgram();
	bool setTILEConfigInternal(const TILEConfig& ttfConfig);
	void scaleTileSizeDown(float tileSize);
	void restoreTileSize();
	void updateLetterSpriteScale(Sprite* sprite);

	void reset();

	TileDefinition _getTileDefinition() const;

	virtual void updateColor() override;

	bool _contentDirty;
	TileString _utf32Text;		//from map request

	TileAtlas* _tileAtlas;		//resource 
	Vector<SpriteBatchNode*> _batchNodes;	//for batch
	std::vector<LetterInfo> _lettersInfo;	//drawing letters

	//! used for optimization
	Sprite *_reusedLetter;
	Rect _reusedRect;
	int _lengthOfString;

	//layout relevant properties.
	Size _labelDimensions;
	float _labelWidth;		//for clip... 
	float _labelHeight;

	TILEConfig _tileConfig;
	Color4B _textColor;
	Color4F _textColorF;

	QuadCommand _quadCommand;
	CustomCommand _customCommand;
	GLint _uniformTextColor;
	bool _useA8Shader;

	bool _clipEnabled;
	bool _blendFuncDirty;
	BlendFunc _blendFunc;

	/// whether or not the label was inside bounds the previous frame
	bool _insideBounds;

	bool _isOpacityModifyRGB;

	std::unordered_map<int, Sprite*> _letters;

	EventListenerCustom* _purgeTextureListener;
	EventListenerCustom* _resetTextureListener;

#if CC_LABEL_DEBUG_DRAW
	DrawNode* _debugDrawNode;
#endif
	float _bmtileScale;
	float _originalTileSize;

private:
	CC_DISALLOW_COPY_AND_ASSIGN(TileSceneLayer);
};

// end group
/// @}

NS_CC_END

#endif /*__COCOS2D_CCLABEL_H */
