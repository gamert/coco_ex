#ifndef WATER_LAYER_H
#define WATER_LAYER_H

#include "CCTextureAtlas2.h"
#include "2d/CCNode.h"
#include "renderer/CCGLProgramCache.h"
#include "renderer/CCGLProgram.h"

NS_CC_BEGIN
#if true


class WaterTileBatchNode;
//�ο�sprite:
class WaterLayerSprite: public Node, public TextureProtocol
{
	enum class RenderMode {
		QUAD,
		POLYGON,
		SLICE9,
		QUAD_BATCHNODE
	};
	RenderMode _renderMode;                 /// render mode used by the Sprite: Quad, Slice9, Polygon or Quad_Batchnode
	Vec2 _stretchFactor;                    /// stretch factor to match the contentSize. for 1- and 9- slice sprites
	Size _originalContentSize;              /// original content size

	// Offset Position (used by Zwoptex)
	Vec2 _offsetPosition;
	Vec2 _unflippedOffsetPositionFromCenter;

	BlendFunc        _blendFunc;            /// It's required for TextureProtocol inheritance
	Texture2D*       _texture;              /// Texture2D object that is used to render the sprite

	Rect _rect,_rectMask;
	V3F_C4B_T2F_T2F_Quad _quad;
	bool _recursiveDirty;
	bool _dirty;
	bool _stretchEnabled;
	bool _flippedX, _flippedY;
	bool _rectRotated;                      /// Whether the texture is rotated
	WaterTileBatchNode *_batchNode;
	TextureAtlas2*   _textureAtlas;			/// SpriteBatchNode texture atlas (weak reference)
	Mat4              _transformToBatch;
	int				 _atlasIndex;
public:

	WaterLayerSprite();
	//
	void Draw()
	{

	}


	/**
	* Gets the weak reference of the TextureAtlas2 when the sprite is rendered using via SpriteBatchNode.
	*/
	TextureAtlas2* getTextureAtlas() const { return _textureAtlas; }

	/**
	* Sets the weak reference of the TextureAtlas2 when the sprite is rendered using via SpriteBatchNode.
	*/
	void setTextureAtlas(TextureAtlas2 *textureAtlas) { _textureAtlas = textureAtlas; }

	/**
	* Returns the index used on the TextureAtlas2.
	*/
	ssize_t getAtlasIndex() const { return _atlasIndex; }

	/**
	* Sets the index used on the TextureAtlas2.
	*
	* @warning Don't modify this value unless you know what you are doing.
	*/
	void setAtlasIndex(ssize_t atlasIndex) { _atlasIndex = atlasIndex; }

	//
	void setTexture(Texture2D *texture) override;

	Texture2D* getTexture() const override
	{
		return _texture;
	}
	/// @name Functions inherited from TextureProtocol.
	/**
	*@code
	*When this function bound into js or lua,the parameter will be changed.
	*In js: var setBlendFunc(var src, var dst).
	*In lua: local setBlendFunc(local src, local dst).
	*@endcode
	*/
	void setBlendFunc(const BlendFunc &blendFunc) override { _blendFunc = blendFunc; }
	/**
	* @js  NA
	* @lua NA
	*/
	const BlendFunc& getBlendFunc() const override { return _blendFunc; }
	/// @}



	void setPosition(float x, float y) override
	{
		Node::setPosition(x, y);
		setDirty(true);
	}
	//void setPosition(const Vec2& pos)
	//{
	//	Node::setPosition(pos);
	//	setDirty(true);
	//}

	void setTextureRect(const Rect& rect)
	{
		setTextureRect(rect, false, rect.size);
	}

	//���������С����Content��С:
	void setTextureRect(const Rect& rect, bool rotated, const Size& untrimmedSize);

	void updatePoly();

	void setMaskRect(const Rect& rect)
	{
		_rectMask = rect;
	}
	// override this method to generate "double scale" sprites
	void setVertexRect(const Rect& rect)
	{
		_rect = rect;
	}

	void setTextureCoords(const Rect& rectInPoints)
	{
		setTextureCoords(rectInPoints, &_quad);
	}

	//
	void setTextureCoords(const Rect& rectInPoints, V3F_C4B_T2F_T2F_Quad* outQuad);
	void setTexture2Coords(const Rect& rectInPoints, V3F_C4B_T2F_T2F_Quad* outQuad);

	void setVertexCoords(const Rect& rect, V3F_C4B_T2F_T2F_Quad* outQuad);

	void setDirty(bool dirty)
	{
		_dirty = dirty;
	}
	bool isDirty() { return _dirty; }

	//
	void setBatchNode(WaterTileBatchNode *spriteBatchNode);
	bool _shouldBeHidden;
	void updateTransform(void) override;

	V3F_C4B_T2F_T2F_Quad &getQuad() { return _quad; }
};



class WaterTileBatchNode : public Node, public TextureProtocol
{
public:
	WaterTileBatchNode();
	void Init(TextureAtlas2 * textureAtlas);
	static WaterTileBatchNode *Create(TextureAtlas2 * textureAtlas);

	WaterLayerSprite *_reusedLetter;
	TextureAtlas2 * _textureAtlas;

	/**
	* Gets the weak reference of the TextureAtlas2 when the sprite is rendered using via SpriteBatchNode.
	*/
	TextureAtlas2* getTextureAtlas() const { return _textureAtlas; }

	/**
	* Sets the weak reference of the TextureAtlas2 when the sprite is rendered using via SpriteBatchNode.
	*/
	void setTextureAtlas(TextureAtlas2 *textureAtlas) { _textureAtlas = textureAtlas; }


	//@_reusedLetter
	//@_reusedRect: ��ʾ��Atlas�ϵ�Rect
	//@px, py: ��Ļ����
	void AddWaterSprite(Rect &_reusedRect, float px, float py, Rect &maskRect)
	{
		//����һ��Rect
		_reusedLetter->setMaskRect(maskRect);
		_reusedLetter->setTextureRect(_reusedRect, false, _reusedRect.size);
		//float letterPositionX = _lettersInfo[ctr].positionX + _linesOffsetX[_lettersInfo[ctr].lineIndex];
		//����λ��
		_reusedLetter->setPosition(px, py);
		int index = _textureAtlas->getTotalQuads();
		insertQuadFromSprite(_reusedLetter, index);
	}
	//
	void Flush(bool bAlphaMask);

	void insertQuadFromSprite(WaterLayerSprite *sprite, ssize_t index)
	{
		// make needed room
		while (index >= _textureAtlas->getCapacity() || _textureAtlas->getCapacity() == _textureAtlas->getTotalQuads())
		{
			this->increaseAtlasCapacity();
		}

		//
		// update the quad directly. Don't add the sprite to the scene graph
		//
		sprite->setBatchNode(this);
		sprite->setAtlasIndex(index);

		V3F_C4B_T2F_T2F_Quad quad = sprite->getQuad();
		_textureAtlas->insertQuad(&quad, index);

		// FIXME:: updateTransform will update the textureAtlas too, using updateQuad.
		// FIXME:: so, it should be AFTER the insertQuad
		sprite->setDirty(true);
		sprite->updateTransform();

	}

	void increaseAtlasCapacity()
	{
		// if we're going beyond the current TextureAtlas2's capacity,
		// all the previously initialized sprites will need to redo their texture coords
		// this is likely computationally expensive
		ssize_t quantity = (_textureAtlas->getCapacity() + 1) * 4 / 3;

		CCLOG("cocos2d: SpriteBatchNode: resizing TextureAtlas2 capacity from [%d] to [%d].",
			static_cast<int>(_textureAtlas->getCapacity()),
			static_cast<int>(quantity));

		if (!_textureAtlas->resizeCapacity(quantity))
		{
			// serious problems
			//CCLOGWARN("cocos2d: WARNING: Not enough memory to resize the atlas");
			CCASSERT(false, "Not enough memory to resize the atlas");
		}
	}


	////
	BlendFunc        _blendFunc;            /// It's required for TextureProtocol inheritance
	Texture2D*       _texture;              /// Texture2D object that is used to render the sprite

	//
	void setTexture(Texture2D *texture) override
	{
		if (_texture != texture)
		{
			CC_SAFE_RETAIN(texture);
			CC_SAFE_RELEASE(_texture);
			_texture = texture;
			//updateBlendFunc();
		}
	}

	Texture2D* getTexture() const override
	{
		return _texture;
	}

	/// @name Functions inherited from TextureProtocol.
	/**
	*@code
	*When this function bound into js or lua,the parameter will be changed.
	*In js: var setBlendFunc(var src, var dst).
	*In lua: local setBlendFunc(local src, local dst).
	*@endcode
	*/
	void setBlendFunc(const BlendFunc &blendFunc) override { _blendFunc = blendFunc; }
	/**
	* @js  NA
	* @lua NA
	*/
	const BlendFunc& getBlendFunc() const override { return _blendFunc; }
	/// @}
};



#endif
NS_CC_END

#endif