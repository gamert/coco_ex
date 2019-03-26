#include "WaterLayer.h"


NS_CC_BEGIN
#if true

#if CC_SPRITEBATCHNODE_RENDER_SUBPIXEL
#define SPRITE_RENDER_IN_SUBPIXEL
#else
#define SPRITE_RENDER_IN_SUBPIXEL(__ARGS__) (ceil(__ARGS__))
#endif

//参考sprite:

WaterLayerSprite::WaterLayerSprite() :_recursiveDirty(false), _dirty(true), _texture(NULL), _batchNode(NULL), _textureAtlas(NULL)
{
	_stretchEnabled = false;
	_flippedX = _flippedY = false;
	_rectRotated = false;
	_blendFunc = BlendFunc::ALPHA_PREMULTIPLIED;

	// clean the Quad
	memset(&_quad, 0, sizeof(_quad));

	// Atlas: Color
	_quad.bl.colors = Color4B::WHITE;
	_quad.br.colors = Color4B::WHITE;
	_quad.tl.colors = Color4B::WHITE;
	_quad.tr.colors = Color4B::WHITE;
}

//
void WaterLayerSprite::setTexture(Texture2D *texture)
{
	if (_glProgramState == nullptr)
	{
		setGLProgramState(GLProgramState::getOrCreateWithGLProgramName(GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR_NO_MVP, texture));
	}
	// If batchnode, then texture id should be the same
	CCASSERT(!_batchNode || (texture &&  texture->getName() == _batchNode->getTexture()->getName()), "CCSprite: Batched sprites should use the same texture as the batchnode");
	// accept texture==nil as argument
	CCASSERT(!texture || dynamic_cast<Texture2D*>(texture), "setTexture expects a Texture2D. Invalid argument");

	if (texture == nullptr)
	{
		assert(false);
	}

	if ((_renderMode != RenderMode::QUAD_BATCHNODE) && (_texture != texture))
	{
		CC_SAFE_RETAIN(texture);
		CC_SAFE_RELEASE(_texture);
		_texture = texture;
		//updateBlendFunc();
	}
}

//根据纹理大小设置Content大小:
void WaterLayerSprite::setTextureRect(const Rect& rect, bool rotated, const Size& untrimmedSize)
{
	_rectRotated = rotated;

	Node::setContentSize(untrimmedSize);
	_originalContentSize = untrimmedSize;

	setVertexRect(rect);
	//updateStretchFactor();
	updatePoly();
}

void WaterLayerSprite::updatePoly()
{
	Rect copyRect;
	if (_stretchEnabled) {
		// case B)
		copyRect = Rect(0, 0, _rect.size.width * _stretchFactor.x, _rect.size.height * _stretchFactor.y);
	}
	else {
		// case A)
		// modify origin to put the sprite in the correct offset
		copyRect = Rect((_contentSize.width - _originalContentSize.width) / 2.0f,
			(_contentSize.height - _originalContentSize.height) / 2.0f,
			_rect.size.width,
			_rect.size.height);
	}
	setTextureCoords(_rect, &_quad);
	setVertexCoords(copyRect, &_quad);

	//_polyInfo.setQuad(&_quad);
}


//
void WaterLayerSprite::setTextureCoords(const Rect& rectInPoints, V3F_C4B_T2F_T2F_Quad* outQuad)
{
	Texture2D *tex = _textureAtlas->getTexture();
	if (tex == nullptr)
	{
		return;
	}

	const auto rectInPixels = CC_RECT_POINTS_TO_PIXELS(rectInPoints);

	const float atlasWidth = (float)tex->getPixelsWide();
	const float atlasHeight = (float)tex->getPixelsHigh();

	float rw = rectInPixels.size.width;
	float rh = rectInPixels.size.height;

	// if the rect is rotated, it means that the frame is rotated 90 degrees (clockwise) and:
	//  - rectInpoints: origin will be the bottom-left of the frame (and not the top-right)
	//  - size: represents the unrotated texture size
	//
	// so what we have to do is:
	//  - swap texture width and height
	//  - take into account the origin
	//  - flip X instead of Y when flipY is enabled
	//  - flip Y instead of X when flipX is enabled

	if (_rectRotated)
		std::swap(rw, rh);

#if CC_FIX_ARTIFACTS_BY_STRECHING_TEXEL
	float left = (2 * rectInPixels.origin.x + 1) / (2 * atlasWidth);
	float right = left + (rw * 2 - 2) / (2 * atlasWidth);
	float top = (2 * rectInPixels.origin.y + 1) / (2 * atlasHeight);
	float bottom = top + (rh * 2 - 2) / (2 * atlasHeight);
#else
	float left = rectInPixels.origin.x / atlasWidth;
	float right = (rectInPixels.origin.x + rw) / atlasWidth;
	float top = rectInPixels.origin.y / atlasHeight;
	float bottom = (rectInPixels.origin.y + rh) / atlasHeight;
#endif // CC_FIX_ARTIFACTS_BY_STRECHING_TEXEL


	if ((!_rectRotated && _flippedX) || (_rectRotated && _flippedY))
	{
		std::swap(left, right);
	}

	if ((!_rectRotated && _flippedY) || (_rectRotated && _flippedX))
	{
		std::swap(top, bottom);
	}

	if (_rectRotated)
	{
		outQuad->bl.texCoords.u = left;
		outQuad->bl.texCoords.v = top;
		outQuad->br.texCoords.u = left;
		outQuad->br.texCoords.v = bottom;
		outQuad->tl.texCoords.u = right;
		outQuad->tl.texCoords.v = top;
		outQuad->tr.texCoords.u = right;
		outQuad->tr.texCoords.v = bottom;
	}
	else
	{
		outQuad->bl.texCoords.u = left;
		outQuad->bl.texCoords.v = bottom;
		outQuad->br.texCoords.u = right;
		outQuad->br.texCoords.v = bottom;
		outQuad->tl.texCoords.u = left;
		outQuad->tl.texCoords.v = top;
		outQuad->tr.texCoords.u = right;
		outQuad->tr.texCoords.v = top;
	}
}

void WaterLayerSprite::setVertexCoords(const Rect& rect, V3F_C4B_T2F_T2F_Quad* outQuad)
{
	float relativeOffsetX = _unflippedOffsetPositionFromCenter.x;
	float relativeOffsetY = _unflippedOffsetPositionFromCenter.y;

	// issue #732
	if (_flippedX)
	{
		relativeOffsetX = -relativeOffsetX;
	}
	if (_flippedY)
	{
		relativeOffsetY = -relativeOffsetY;
	}

	_offsetPosition.x = relativeOffsetX + (_originalContentSize.width - _rect.size.width) / 2;
	_offsetPosition.y = relativeOffsetY + (_originalContentSize.height - _rect.size.height) / 2;

	// FIXME: Stretching should be applied to the "offset" as well
	// but probably it should be calculated in the caller function. It will be tidier
	if (_renderMode == RenderMode::QUAD) {
		_offsetPosition.x *= _stretchFactor.x;
		_offsetPosition.y *= _stretchFactor.y;
	}

	// rendering using batch node
	if (_renderMode == RenderMode::QUAD_BATCHNODE)
	{
		// update dirty_, don't update recursiveDirty_
		setDirty(true);
	}
	else
	{
		// self rendering

		// Atlas: Vertex
		const float x1 = 0.0f + _offsetPosition.x + rect.origin.x;
		const float y1 = 0.0f + _offsetPosition.y + rect.origin.y;
		const float x2 = x1 + rect.size.width;
		const float y2 = y1 + rect.size.height;

		// Don't update Z.
		outQuad->bl.vertices.set(x1, y1, 0.0f);
		outQuad->br.vertices.set(x2, y1, 0.0f);
		outQuad->tl.vertices.set(x1, y2, 0.0f);
		outQuad->tr.vertices.set(x2, y2, 0.0f);
	}
}


void WaterLayerSprite::updateTransform(void)
{
	CCASSERT(_renderMode == RenderMode::QUAD_BATCHNODE, "updateTransform is only valid when Sprite is being rendered using an SpriteBatchNode");

	// recalculate matrix only if it is dirty
	if (isDirty()) {

		// If it is not visible, or one of its ancestors is not visible, then do nothing:
		if (!_visible || (_parent && _parent != _batchNode && static_cast<WaterLayerSprite*>(_parent)->_shouldBeHidden))
		{
			_quad.br.vertices.setZero();
			_quad.tl.vertices.setZero();
			_quad.tr.vertices.setZero();
			_quad.bl.vertices.setZero();
			_shouldBeHidden = true;
		}
		else
		{
			_shouldBeHidden = false;

			if (!_parent || _parent == _batchNode)
			{
				_transformToBatch = getNodeToParentTransform();
			}
			else
			{
				CCASSERT(dynamic_cast<Sprite*>(_parent), "Logic error in Sprite. Parent must be a Sprite");
				const Mat4 &nodeToParent = getNodeToParentTransform();
				Mat4 &parentTransform = static_cast<WaterLayerSprite*>(_parent)->_transformToBatch;
				_transformToBatch = parentTransform * nodeToParent;
			}

			//
			// calculate the Quad based on the Affine Matrix
			//

			Size &size = _rect.size;

			float x1 = _offsetPosition.x;
			float y1 = _offsetPosition.y;

			float x2 = x1 + size.width;
			float y2 = y1 + size.height;

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
			setTextureCoords(_rect);
		}

		// MARMALADE CHANGE: ADDED CHECK FOR nullptr, TO PERMIT SPRITES WITH NO BATCH NODE / TEXTURE ATLAS
		if (_textureAtlas)
		{
			_textureAtlas->updateQuad(&_quad, _atlasIndex);
		}

		_recursiveDirty = false;
		setDirty(false);
	}

	// MARMALADE CHANGED
	// recursively iterate over children
	/*    if( _hasChildren )
	{
	// MARMALADE: CHANGED TO USE Node*
	// NOTE THAT WE HAVE ALSO DEFINED virtual Node::updateTransform()
	arrayMakeObjectsPerformSelector(_children, updateTransform, Sprite*);
	}*/
	Node::updateTransform();
}



#endif
NS_CC_END