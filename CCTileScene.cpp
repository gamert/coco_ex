
#include <algorithm>

#include "CCTileScene.h"
#include "CCTileAtlas.h"
#include "2d/CCSprite.h"
#include "2d/CCSpriteBatchNode.h"
#include "2d/CCDrawNode.h"
#include "2d/CCCamera.h"
#include "platform/CCFileUtils.h"
#include "renderer/CCRenderer.h"
#include "base/CCDirector.h"
#include "base/CCEventListenerCustom.h"
#include "base/CCEventDispatcher.h"
#include "base/CCEventCustom.h"
#include "base/ccUtils.h"


NS_CC_BEGIN

/**
* TileLetter used to update the quad in texture atlas without SpriteBatchNode.
*/
class TileLetter : public Sprite
{
public:
	TileLetter()
	{
		_textureAtlas = nullptr;
		_letterVisible = true;
	}

	static TileLetter* createWithTexture(Texture2D *texture, const Rect& rect, bool rotated = false)
	{
		auto letter = new (std::nothrow) TileLetter();
		if (letter && letter->initWithTexture(texture, rect, rotated))
		{
			letter->Sprite::setVisible(false);
			letter->autorelease();
			return letter;
		}
		CC_SAFE_DELETE(letter);
		return nullptr;
	}

	CREATE_FUNC(TileLetter);

	virtual void updateTransform() override
	{
		if (isDirty())
		{
			_transformToBatch = getNodeToParentTransform();
			Size &size = _rect.size;

			float x1 = _offsetPosition.x;
			float y1 = _offsetPosition.y;
			float x2 = x1 + size.width;
			float y2 = y1 + size.height;

			// issue #17022: don't flip, again, the letter since they are flipped in sprite's code
			//if (_flippedX) std::swap(x1, x2);
			//if (_flippedY) std::swap(y1, y2);

			float x = _transformToBatch.m[12];
			float y = _transformToBatch.m[13];

			float cr = _transformToBatch.m[0];
			float sr = _transformToBatch.m[1];
			float cr2 = _transformToBatch.m[5];
			float sr2 = -_transformToBatch.m[4];
			float ax = x1 * cr - y1 * sr2 + x;
			float ay = x1 * sr + y1 * cr2 + y;

			float bx = x2 * cr - y1 * sr2 + x;
			float by = x2 * sr + y1 * cr2 + y;
			float cx = x2 * cr - y2 * sr2 + x;
			float cy = x2 * sr + y2 * cr2 + y;
			float dx = x1 * cr - y2 * sr2 + x;
			float dy = x1 * sr + y2 * cr2 + y;

			_quad.bl.vertices.set(SPRITE_RENDER_IN_SUBPIXEL(ax), SPRITE_RENDER_IN_SUBPIXEL(ay), _positionZ);
			_quad.br.vertices.set(SPRITE_RENDER_IN_SUBPIXEL(bx), SPRITE_RENDER_IN_SUBPIXEL(by), _positionZ);
			_quad.tl.vertices.set(SPRITE_RENDER_IN_SUBPIXEL(dx), SPRITE_RENDER_IN_SUBPIXEL(dy), _positionZ);
			_quad.tr.vertices.set(SPRITE_RENDER_IN_SUBPIXEL(cx), SPRITE_RENDER_IN_SUBPIXEL(cy), _positionZ);

			if (_textureAtlas)
			{
				_textureAtlas->updateQuad(&_quad, _atlasIndex);
			}

			_recursiveDirty = false;
			setDirty(false);
		}

		Node::updateTransform();
	}

	virtual void updateColor() override
	{
		if (_textureAtlas == nullptr)
		{
			return;
		}

		auto displayedOpacity = _displayedOpacity;
		if (!_letterVisible)
		{
			displayedOpacity = 0.0f;
		}
		Color4B color4(_displayedColor.r, _displayedColor.g, _displayedColor.b, displayedOpacity);
		// special opacity for premultiplied textures
		if (_opacityModifyRGB)
		{
			color4.r *= displayedOpacity / 255.0f;
			color4.g *= displayedOpacity / 255.0f;
			color4.b *= displayedOpacity / 255.0f;
		}
		_quad.bl.colors = color4;
		_quad.br.colors = color4;
		_quad.tl.colors = color4;
		_quad.tr.colors = color4;

		_textureAtlas->updateQuad(&_quad, _atlasIndex);
	}

	void setVisible(bool visible) override
	{
		_letterVisible = visible;
		updateColor();
	}

	bool isVisible() const override
	{
		return _letterVisible;
	}

	//TileLetter doesn't need to draw directly.
	void draw(Renderer* /*renderer*/, const Mat4 & /*transform*/, uint32_t /*flags*/) override
	{
	}

private:
	bool _letterVisible;
};


TileScene* TileScene::create()
{
	auto ret = new (std::nothrow) TileScene;

	if (ret)
	{
		ret->autorelease();
	}

	return ret;
}

TileScene::TileScene()
	: _fontAtlas(nullptr)
	, _reusedLetter(nullptr)
{
	setAnchorPoint(Vec2::ANCHOR_MIDDLE);
	reset();

#if CC_LABEL_DEBUG_DRAW
	_debugDrawNode = DrawNode::create();
	addChild(_debugDrawNode);
#endif

	_purgeTextureListener = EventListenerCustom::create(TileAtlas::CMD_PURGE_TILEATLAS, [this](EventCustom* event) {
		if (_fontAtlas && event->getUserData() == _fontAtlas)
		{
			for (auto&& it : _letters)
			{
				it.second->setTexture(nullptr);
			}
			_batchNodes.clear();

			if (_fontAtlas)
			{
				TileAtlasCache::releaseTileAtlas(_fontAtlas);
			}
		}
	});
	_eventDispatcher->addEventListenerWithFixedPriority(_purgeTextureListener, 1);

	_resetTextureListener = EventListenerCustom::create(TileAtlas::CMD_RESET_TILEATLAS, [this](EventCustom* event) {
		if (_fontAtlas && event->getUserData() == _fontAtlas)
		{
			_fontAtlas = nullptr;
			auto lineHeight = _lineHeight;
			for (auto&& it : _letters)
			{
				getLetter(it.first);
			}
		}
	});
	_eventDispatcher->addEventListenerWithFixedPriority(_resetTextureListener, 2);
}

TileScene::~TileScene()
{
	if (_fontAtlas)
	{
		Node::removeAllChildrenWithCleanup(true);
		CC_SAFE_RELEASE_NULL(_reusedLetter);
		_batchNodes.clear();
		TileAtlasCache::releaseTileAtlas(_fontAtlas);
	}
	_eventDispatcher->removeEventListener(_purgeTextureListener);
	_eventDispatcher->removeEventListener(_resetTextureListener);
}

void TileScene::reset()
{
	Node::removeAllChildrenWithCleanup(true);
	CC_SAFE_RELEASE_NULL(_reusedLetter);
	_letters.clear();
	_batchNodes.clear();
	_lettersInfo.clear();
	if (_fontAtlas)
	{
		TileAtlasCache::releaseTileAtlas(_fontAtlas);
		_fontAtlas = nullptr;
	}


	_contentDirty = false;
	_lengthOfString = 0;

	_lineHeight = 0.f;
	_lineSpacing = 0.f;
	_maxLineWidth = 0.f;
	_labelDimensions.width = 0.f;
	_labelDimensions.height = 0.f;
	_labelWidth = 0.f;
	_labelHeight = 0.f;

	_textColor = Color4B::WHITE;
	_textColorF = Color4F::WHITE;
	setColor(Color3B::WHITE);

	_uniformTextColor = -1;

	_useA8Shader = false;
	_clipEnabled = false;
	_blendFuncDirty = false;
	_blendFunc = BlendFunc::ALPHA_PREMULTIPLIED;
	_isOpacityModifyRGB = false;
	_insideBounds = true;
	_originalTileSize = 0.0f;
	setRotationSkewX(0);        // reverse italics
}

//  ETC1 ALPHA supports, for LabelType::BMTILE & LabelType::CHARMAP
static Texture2D* _getTexture(TileScene* label)
{
	auto fontAtlas = label->getTileAtlas();
	Texture2D* texture = nullptr;
	if (fontAtlas != nullptr) {
		auto textures = fontAtlas->getTextures();
		if (!textures.empty()) {
			texture = textures.begin()->second;
		}
	}
	return texture;
}

void TileScene::updateShaderProgram()
{
	_uniformTextColor = glGetUniformLocation(getGLProgram()->getProgram(), "u_textColor");
}

void TileScene::setTileAtlas(TileAtlas* atlas, bool distanceFieldEnabled /* = false */, bool useA8Shader /* = false */)
{
	if (atlas == _fontAtlas)
		return;

	CC_SAFE_RETAIN(atlas);
	if (_fontAtlas)
	{
		_batchNodes.clear();
		TileAtlasCache::releaseTileAtlas(_fontAtlas);
	}
	_fontAtlas = atlas;

	if (_reusedLetter == nullptr)
	{
		_reusedLetter = Sprite::create();
		_reusedLetter->setOpacityModifyRGB(_isOpacityModifyRGB);
		_reusedLetter->retain();
		_reusedLetter->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
	}

	if (_fontAtlas)
	{
		_lineHeight = _fontAtlas->getLineHeight();
		_contentDirty = true;
	}
	_useA8Shader = useA8Shader;
	updateShaderProgram();
}

bool TileScene::setTILEConfig(const TILEConfig& ttfConfig)
{
	_originalTileSize = ttfConfig.fontSize;
	return setTILEConfigInternal(ttfConfig);
}


void TileScene::setDimensions(float width, float height)
{
	if (height != _labelHeight || width != _labelWidth)
	{
		_labelWidth = width;
		_labelHeight = height;
		_labelDimensions.width = width;
		_labelDimensions.height = height;

		_maxLineWidth = width;
		_contentDirty = true;
	}
}

void TileScene::restoreTileSize()
{
	auto ttfConfig = this->getTILEConfig();
	ttfConfig.fontSize = _originalTileSize;
	this->setTILEConfigInternal(ttfConfig);
}


void TileScene::updateLabelLetters()
{
	if (_letters.empty())
	{
		return;
	}
	Rect uvRect;
	TileLetter* letterSprite;
	int letterIndex;

	for (auto it = _letters.begin(); it != _letters.end();)
	{
		letterIndex = it->first;
		letterSprite = (TileLetter*)it->second;

		if (letterIndex >= _lengthOfString)
		{
			Node::removeChild(letterSprite, true);
			it = _letters.erase(it);
		}
		else
		{
			auto& letterInfo = _lettersInfo[letterIndex];
			if (letterInfo.valid)
			{
				auto& letterDef = _fontAtlas->_letterDefinitions[letterInfo.utf32Char];
				uvRect.size.height = letterDef.height;
				uvRect.size.width = letterDef.width;
				uvRect.origin.x = letterDef.U;
				uvRect.origin.y = letterDef.V;

				auto batchNode = _batchNodes.at(letterDef.textureID);
				letterSprite->setTextureAtlas(batchNode->getTextureAtlas());
				letterSprite->setTexture(_fontAtlas->getTexture(letterDef.textureID));
				if (letterDef.width <= 0.f || letterDef.height <= 0.f)
				{
					letterSprite->setTextureAtlas(nullptr);
				}
				else
				{
					letterSprite->setTextureRect(uvRect, false, uvRect.size);
					letterSprite->setTextureAtlas(_batchNodes.at(letterDef.textureID)->getTextureAtlas());
					letterSprite->setAtlasIndex(_lettersInfo[letterIndex].atlasIndex);
				}

				auto px = letterInfo.positionX + letterDef.width / 2 + _linesOffsetX[letterInfo.lineIndex];
				auto py = letterInfo.positionY - letterDef.height / 2 + _letterOffsetY;
				letterSprite->setPosition(px, py);
			}
			else
			{
				letterSprite->setTextureAtlas(nullptr);
			}
			this->updateLetterSpriteScale(letterSprite);
			++it;
		}
	}
}

bool TileScene::alignText()
{
	if (_fontAtlas == nullptr || _utf32Text.empty())
	{
		setContentSize(Size::ZERO);
		return true;
	}

	bool ret = true;
	do {
		_fontAtlas->prepareLetterDefinitions(_utf32Text);
		auto& textures = _fontAtlas->getTextures();
		auto size = textures.size();
		if (size > static_cast<size_t>(_batchNodes.size()))
		{
			for (auto index = static_cast<size_t>(_batchNodes.size()); index < size; ++index)
			{
				auto batchNode = SpriteBatchNode::createWithTexture(textures.at(index));
				if (batchNode)
				{
					_isOpacityModifyRGB = batchNode->getTexture()->hasPremultipliedAlpha();
					_blendFunc = batchNode->getBlendFunc();
					batchNode->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
					batchNode->setPosition(Vec2::ZERO);
					_batchNodes.pushBack(batchNode);
				}
			}
		}
		if (_batchNodes.empty())
		{
			return true;
		}
		// optimize for one-texture-only scenario
		// if multiple textures, then we should count how many chars
		// are per texture
		if (_batchNodes.size() == 1)
			_batchNodes.at(0)->reserveCapacity(_utf32Text.size());

		_reusedLetter->setBatchNode(_batchNodes.at(0));
		_lengthOfString = 0;

		if (!updateQuads()) {
			ret = false;
			break;
		}
		updateLabelLetters();
		updateColor();
	} while (0);

	return ret;
}


bool TileScene::updateQuads()
{
	bool ret = true;
	for (auto&& batchNode : _batchNodes)
	{
		batchNode->getTextureAtlas()->removeAllQuads();
	}

	for (int ctr = 0; ctr < _lengthOfString; ++ctr)
	{
		if (_lettersInfo[ctr].valid)
		{
			//z: get from atlas
			auto& letterDef = _fontAtlas->_letterDefinitions[_lettersInfo[ctr].utf32Char];

			_reusedRect.size.height = letterDef.height;
			_reusedRect.size.width = letterDef.width;
			_reusedRect.origin.x = letterDef.U;
			_reusedRect.origin.y = letterDef.V;

			auto py = _lettersInfo[ctr].positionY ;//+ _letterOffsetY
			//
			if (_labelHeight > 0.f) {
				if (py > _tailoredTopY)
				{
					auto clipTop = py - _tailoredTopY;
					_reusedRect.origin.y += clipTop;
					_reusedRect.size.height -= clipTop;
					py -= clipTop;
				}
				if (py - letterDef.height * _bmfontScale < _tailoredBottomY)
				{
					_reusedRect.size.height = (py < _tailoredBottomY) ? 0.f : (py - _tailoredBottomY);
				}
			}

			auto lineIndex = _lettersInfo[ctr].lineIndex;
			auto px = _lettersInfo[ctr].positionX + letterDef.width / 2 * _bmfontScale + _linesOffsetX[lineIndex];

			//´°¿ÚµÄ¿í¶È£¿
			if (_labelWidth > 0.f) {
				if (this->isHorizontalClamped(px, lineIndex)) {
					if (_overflow == Overflow::CLAMP) {
						_reusedRect.size.width = 0;
					}
					else if (_overflow == Overflow::SHRINK) {
						if (_contentSize.width > letterDef.width) {
							ret = false;
							break;
						}
						else {
							_reusedRect.size.width = 0;
						}

					}
				}
			}


			if (_reusedRect.size.height > 0.f && _reusedRect.size.width > 0.f)
			{
				_reusedLetter->setTextureRect(_reusedRect, false, _reusedRect.size);
				float letterPositionX = _lettersInfo[ctr].positionX + _linesOffsetX[_lettersInfo[ctr].lineIndex];
				_reusedLetter->setPosition(letterPositionX, py);
				auto index = static_cast<int>(_batchNodes.at(letterDef.textureID)->getTextureAtlas()->getTotalQuads());
				_lettersInfo[ctr].atlasIndex = index;

				this->updateLetterSpriteScale(_reusedLetter);

				_batchNodes.at(letterDef.textureID)->insertQuadFromSprite(_reusedLetter, index);
			}
		}
	}


	return ret;
}

bool TileScene::setTILEConfigInternal(const TILEConfig& ttfConfig)
{
	TileAtlas *newAtlas = TileAtlasCache::getTileAtlasTTF(&ttfConfig);

	if (!newAtlas)
	{
		reset();
		return false;
	}
	setTileAtlas(newAtlas, false, true);
	_fontConfig = ttfConfig;
	updateShaderProgram();
	return true;
}



void TileScene::scaleTileSizeDown(float fontSize)
{
	bool shouldUpdateContent = true;
	
	auto ttfConfig = this->getTILEConfig();
	ttfConfig.fontSize = fontSize;
	this->setTILEConfigInternal(ttfConfig);

	if (shouldUpdateContent) {
		this->updateContent();
	}
}


void TileScene::setCameraMask(unsigned short mask, bool applyChildren)
{
	Node::setCameraMask(mask, applyChildren);
}


void TileScene::updateContent()
{
	bool updateFinished = true;
	if (_fontAtlas)
	{
		updateFinished = alignText();
	}

	if (updateFinished) {
		_contentDirty = false;
	}

#if CC_LABEL_DEBUG_DRAW
	_debugDrawNode->clear();
	Vec2 vertices[4] =
	{
		Vec2::ZERO,
		Vec2(_contentSize.width, 0),
		Vec2(_contentSize.width, _contentSize.height),
		Vec2(0, _contentSize.height)
	};
	_debugDrawNode->drawPoly(vertices, 4, true, Color4F::WHITE);
#endif
}

void TileScene::onDraw(const Mat4& transform, bool /*transformUpdated*/)
{
	auto glprogram = getGLProgram();
	glprogram->use();
	utils::setBlending(_blendFunc.src, _blendFunc.dst);

	glprogram->setUniformsForBuiltins(transform);
	for (auto&& it : _letters)
	{
		it.second->updateTransform();
	}
	//glprogram->setUniformLocationWith4f(_uniformTextColor,
	//_textColorF.r, _textColorF.g, _textColorF.b, _textColorF.a);

	for (auto&& batchNode : _batchNodes)
	{
		batchNode->getTextureAtlas()->drawQuads();
	}
}

void TileScene::draw(Renderer *renderer, const Mat4 &transform, uint32_t flags)
{
	if (_batchNodes.empty() || _lengthOfString <= 0)
	{
		return;
	}
	// Don't do calculate the culling if the transform was not updated
	bool transformUpdated = flags & FLAGS_TRANSFORM_DIRTY;
#if CC_USE_CULLING
	auto visitingCamera = Camera::getVisitingCamera();
	auto defaultCamera = Camera::getDefaultCamera();
	if (visitingCamera == defaultCamera) {
		_insideBounds = (transformUpdated || visitingCamera->isViewProjectionUpdated()) ? renderer->checkVisibility(transform, _contentSize) : _insideBounds;
	}
	else
	{
		_insideBounds = renderer->checkVisibility(transform, _contentSize);
	}

	if (_insideBounds)
#endif
	{
		_customCommand.init(_globalZOrder, transform, flags);
		_customCommand.func = CC_CALLBACK_0(TileScene::onDraw, this, transform, transformUpdated);

		renderer->addCommand(&_customCommand);
	}
}

void TileScene::visit(Renderer *renderer, const Mat4 &parentTransform, uint32_t parentFlags)
{
	if (!_visible || (_utf32Text.empty() && _children.empty()))
	{
		return;
	}

	if (_contentDirty)
	{
		updateContent();
	}

	uint32_t flags = processParentFlags(parentTransform, parentFlags);

	bool visibleByCamera = isVisitableByVisitingCamera();
	if (_children.empty() && !visibleByCamera)
	{
		return;
	}

	// IMPORTANT:
	// To ease the migration to v3.0, we still support the Mat4 stack,
	// but it is deprecated and your code should not rely on it
	_director->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
	_director->loadMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, _modelViewTransform);

	if (!_children.empty())
	{
		sortAllChildren();

		int i = 0;
		// draw children zOrder < 0
		for (auto size = _children.size(); i < size; ++i)
		{
			auto node = _children.at(i);

			if (node && node->getLocalZOrder() < 0)
				node->visit(renderer, _modelViewTransform, flags);
			else
				break;
		}

		this->drawSelf(visibleByCamera, renderer, flags);

		for (auto it = _children.cbegin() + i, itCend = _children.cend(); it != itCend; ++it)
		{
			(*it)->visit(renderer, _modelViewTransform, flags);
		}

	}
	else
	{
		this->drawSelf(visibleByCamera, renderer, flags);
	}

	_director->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
}

void TileScene::drawSelf(bool visibleByCamera, Renderer* renderer, uint32_t flags)
{
	if (visibleByCamera && !_utf32Text.empty())
	{
		draw(renderer, _modelViewTransform, flags);
	}
}




///// PROTOCOL STUFF
Sprite* TileScene::getLetter(int letterIndex)
{
	Sprite* letter = nullptr;
	do
	{
		if (_systemTileDirty || _currentLabelType == LabelType::STRING_TEXTURE)
		{
			break;
		}

		auto contentDirty = _contentDirty;
		if (contentDirty)
		{
			updateContent();
		}

		if (_textSprite == nullptr && letterIndex < _lengthOfString)
		{
			const auto &letterInfo = _lettersInfo[letterIndex];
			if (!letterInfo.valid || letterInfo.atlasIndex<0)
			{
				break;
			}

			if (_letters.find(letterIndex) != _letters.end())
			{
				letter = _letters[letterIndex];
			}

			if (letter == nullptr)
			{
				auto& letterDef = _fontAtlas->_letterDefinitions[letterInfo.utf32Char];
				auto textureID = letterDef.textureID;
				Rect uvRect;
				uvRect.size.height = letterDef.height;
				uvRect.size.width = letterDef.width;
				uvRect.origin.x = letterDef.U;
				uvRect.origin.y = letterDef.V;

				if (letterDef.width <= 0.f || letterDef.height <= 0.f)
				{
					letter = TileLetter::create();
				}
				else
				{
					this->updateBMTileScale();
					letter = TileLetter::createWithTexture(_fontAtlas->getTexture(textureID), uvRect);
					letter->setTextureAtlas(_batchNodes.at(textureID)->getTextureAtlas());
					letter->setAtlasIndex(letterInfo.atlasIndex);
					auto px = letterInfo.positionX + _bmfontScale * uvRect.size.width / 2 + _linesOffsetX[letterInfo.lineIndex];
					auto py = letterInfo.positionY - _bmfontScale * uvRect.size.height / 2 + _letterOffsetY;
					letter->setPosition(px, py);
					letter->setOpacity(_realOpacity);
					this->updateLetterSpriteScale(letter);
				}

				addChild(letter);
				_letters[letterIndex] = letter;
			}
		}
	} while (false);

	return letter;
}

void TileScene::setLineHeight(float height)
{
	CCASSERT(_currentLabelType != LabelType::STRING_TEXTURE, "Not supported system font!");

	if (_lineHeight != height)
	{
		_lineHeight = height;
		_contentDirty = true;
	}
}

float TileScene::getLineHeight() const
{
	return _lineHeight * _bmfontScale;
}



int TileScene::getStringLength()
{
	_lengthOfString = static_cast<int>(_utf32Text.length());
	return _lengthOfString;
}

// RGBA protocol
void TileScene::setOpacityModifyRGB(bool isOpacityModifyRGB)
{
	if (isOpacityModifyRGB != _isOpacityModifyRGB)
	{
		_isOpacityModifyRGB = isOpacityModifyRGB;
		updateColor();
	}
}

void TileScene::updateDisplayedColor(const Color3B& parentColor)
{
	Node::updateDisplayedColor(parentColor);
	for (auto&& it : _letters)
	{
		it.second->updateDisplayedColor(_displayedColor);
	}
}

void TileScene::updateDisplayedOpacity(GLubyte parentOpacity)
{
	Node::updateDisplayedOpacity(parentOpacity);
	for (auto&& it : _letters)
	{
		it.second->updateDisplayedOpacity(_displayedOpacity);
	}
}

// FIXME: it is not clear what is the difference between setTextColor() and setColor()
// if setTextColor() only changes the text and nothing but the text (no glow, no outline, not underline)
// that's fine but it should be documented
void TileScene::setTextColor(const Color4B &color)
{
	_textColor = color;
	_textColorF.r = _textColor.r / 255.0f;
	_textColorF.g = _textColor.g / 255.0f;
	_textColorF.b = _textColor.b / 255.0f;
	_textColorF.a = _textColor.a / 255.0f;
}

void TileScene::updateColor()
{
	if (_batchNodes.empty())
	{
		return;
	}

	Color4B color4(_displayedColor.r, _displayedColor.g, _displayedColor.b, _displayedOpacity);

	// special opacity for premultiplied textures
	if (_isOpacityModifyRGB)
	{
		color4.r *= _displayedOpacity / 255.0f;
		color4.g *= _displayedOpacity / 255.0f;
		color4.b *= _displayedOpacity / 255.0f;
	}

	cocos2d::TextureAtlas* textureAtlas;
	V3F_C4B_T2F_Quad *quads;
	for (auto&& batchNode : _batchNodes)
	{
		textureAtlas = batchNode->getTextureAtlas();
		quads = textureAtlas->getQuads();
		auto count = textureAtlas->getTotalQuads();

		for (int index = 0; index < count; ++index)
		{
			quads[index].bl.colors = color4;
			quads[index].br.colors = color4;
			quads[index].tl.colors = color4;
			quads[index].tr.colors = color4;
			textureAtlas->updateQuad(&quads[index], index);
		}
	}
}

std::string TileScene::getDescription() const
{
	char tmp[50];
	sprintf(tmp, "<TileScene | Tag = %d, TileScene = >", _tag);
	std::string ret = tmp;
	//ret += _utf8Text;

	return ret;
}

const Size& TileScene::getContentSize() const
{
	if (_contentDirty)
	{
		const_cast<TileScene*>(this)->updateContent();
	}
	return _contentSize;
}

Rect TileScene::getBoundingBox() const
{
	const_cast<TileScene*>(this)->getContentSize();

	return Node::getBoundingBox();
}

void TileScene::setBlendFunc(const BlendFunc &blendFunc)
{
	_blendFunc = blendFunc;
	_blendFuncDirty = true;
}

void TileScene::removeAllChildrenWithCleanup(bool cleanup)
{
	Node::removeAllChildrenWithCleanup(cleanup);
	_letters.clear();
}

void TileScene::removeChild(Node* child, bool cleanup /* = true */)
{
	Node::removeChild(child, cleanup);
	for (auto&& it : _letters)
	{
		if (it.second == child)
		{
			_letters.erase(it.first);
			break;
		}
	}
}

TileDefinition TileScene::_getTileDefinition() const
{
	TileDefinition systemTileDef;

	std::string fontName = _systemTile;
	if (_fontAtlas && !_fontAtlas->getTileName().empty()) fontName = _fontAtlas->getTileName();

	systemTileDef._fontName = fontName;
	systemTileDef._fontSize = _systemTileSize;
	systemTileDef._dimensions.width = _labelWidth;
	systemTileDef._dimensions.height = _labelHeight;
	systemTileDef._fontFillColor.r = _textColor.r;
	systemTileDef._fontFillColor.g = _textColor.g;
	systemTileDef._fontFillColor.b = _textColor.b;
	systemTileDef._fontAlpha = _textColor.a;

	return systemTileDef;
}


float TileScene::getRenderingTileSize()const
{
	float fontSize = this->getTILEConfig().fontSize;
	return fontSize;
}


void TileScene::rescaleWithOriginalTileSize()
{
	auto renderingTileSize = this->getRenderingTileSize();
	if (_originalTileSize - renderingTileSize >= 1) {
		this->scaleTileSizeDown(_originalTileSize);
	}
}


NS_CC_END
