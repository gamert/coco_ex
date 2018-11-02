
#ifndef _CCTileScene_H_
#define _CCTileScene_H_

#include "2d/CCNode.h"
#include "renderer/CCCustomCommand.h"
#include "renderer/CCQuadCommand.h"
#include "CCTileAtlas.h"
#include "base/ccTypes.h"

NS_CC_BEGIN

/**
*/

#define CC_DEFAULT_FONT_LABEL_SIZE  12

/**
* @struct TILEConfig
* @see `GlyphCollection`
*/
typedef struct _tileConfig
{
	float fontSize;
	_tileConfig(float size = CC_DEFAULT_FONT_LABEL_SIZE):fontSize(size)
	{
	}
} TILEConfig;



class Sprite;
class SpriteBatchNode;
class DrawNode;
class EventListenerCustom;

/**
* @brief TileScene is a subclass of Node that knows how to render tile labels.
* @js NA
*/
class TileScene : public Node, public LabelProtocol, public BlendProtocol
{
public:
	/// @name Creators
	/// @{

	/**
	* Allocates and initializes a TileScene, with default settings.
	*
	* @return An automatically released TileScene object.
	*/
	static TileScene* create();

	bool setTILEConfig(const TILEConfig& ttfConfig);
	const TILEConfig& getTILEConfig() const { return _fontConfig; }
	/**
	* Sets the text color of TileScene.
	* The text color is different from the color of Node.
	* @warning Limiting use to only when the TileScene created with true type font or system font.
	*/
	virtual void setTextColor(const Color4B &color);

	/** Returns the text color of the TileScene.*/
	const Color4B& getTextColor() const { return _textColor; }

	/**
	* Makes the TileScene exactly this untransformed width.
	*
	* The TileScene's width be used for text align if the value not equal zero.
	*/
	void setWidth(float width) { setDimensions(width, _labelHeight); }
	float getWidth() const { return _labelWidth; }

	/**
	* Makes the TileScene exactly this untransformed height.
	*
	* The TileScene's height be used for text align if the value not equal zero.
	* The text will display incomplete if the size of TileScene is not large enough to display all text.
	*/
	void setHeight(float height) { setDimensions(_labelWidth, height); }
	float getHeight() const { return _labelHeight; }

	/** Sets the untransformed size of the TileScene in a more efficient way. */
	void setDimensions(float width, float height);
	const Size& getDimensions() const { return _labelDimensions; }

	/** Update content immediately.*/
	virtual void updateContent();

	/**
	* Provides a way to treat each character like a Sprite.
	* @warning No support system font.
	*/
	virtual Sprite * getLetter(int lettetIndex);

	/** Clips upper and lower margin to reduce height of TileScene.*/
	void setClipMarginEnabled(bool clipEnabled) { _clipEnabled = clipEnabled; }

	bool isClipMarginEnabled() const { return _clipEnabled; }

	/** Sets the line height of the TileScene.
	* @warning Not support system font.
	* @since v3.2.0
	*/
	void setLineHeight(float height);

	/**
	* Returns the line height of this TileScene.
	* @warning Not support system font.
	* @since v3.2.0
	*/
	float getLineHeight() const;


	TileAtlas* getTileAtlas() { return _fontAtlas; }

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
	virtual void setGlobalZOrder(float globalZOrder) override;


CC_CONSTRUCTOR_ACCESS:
	/**
	* Constructor of TileScene.
	* @js NA
	*/
	TileScene();

	/**
	* Destructor of TileScene.
	* @js NA
	* @lua NA
	*/
	virtual ~TileScene();

protected:
	struct LetterInfo
	{
		char32_t utf32Char;
		bool valid;
		float positionX;
		float positionY;
		int atlasIndex;
		int lineIndex;
	};

	virtual void setTileAtlas(TileAtlas* atlas, bool distanceFieldEnabled = false, bool useA8Shader = false);
	bool getTileLetterDef(char32_t character, TileDefinition& letterDef) const;

	void computeStringNumLines();

	void onDraw(const Mat4& transform, bool transformUpdated);
	void drawSelf(bool visibleByCamera, Renderer* renderer, uint32_t flags);

	void rescaleWithOriginalTileSize();

	void updateLabelLetters();
	virtual bool alignText();

	void recordLetterInfo(const cocos2d::Vec2& point, char32_t utf32Char, int letterIndex, int lineIndex);
	void recordPlaceholderInfo(int letterIndex, char32_t utf16Char);

	bool updateQuads();

	void createSpriteForSystemTile(const TileDefinition& fontDef);
	void createShadowSpriteForSystemTile(const TileDefinition& fontDef);

	virtual void updateShaderProgram();
	bool setTILEConfigInternal(const TILEConfig& ttfConfig);
	void scaleTileSizeDown(float fontSize);
	void restoreTileSize();
	void updateLetterSpriteScale(Sprite* sprite);
	int getFirstCharLen(const TileString& utf32Text, int startIndex, int textLen) const;
	int getFirstWordLen(const TileString& utf32Text, int startIndex, int textLen) const;

	void reset();

	TileDefinition _getTileDefinition() const;

	virtual void updateColor() override;

	bool _contentDirty;
	TileString _utf32Text;		//zz save tile list here...

	TileAtlas* _fontAtlas;
	Vector<SpriteBatchNode*> _batchNodes;
	std::vector<LetterInfo> _lettersInfo;

	//! used for optimization
	Sprite *_reusedLetter;
	Rect _reusedRect;
	int _lengthOfString;

	//layout relevant properties.
	float _lineHeight;
	float _lineSpacing;
	float _maxLineWidth;
	Size _labelDimensions;
	float _labelWidth;
	float _labelHeight;

	TILEConfig _fontConfig;
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
	float _originalTileSize;

private:
	CC_DISALLOW_COPY_AND_ASSIGN(TileScene);
};

// end group
/// @}

NS_CC_END

#endif /*__COCOS2D_CCLABEL_H */
