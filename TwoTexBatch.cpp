//zz add for water : color tex and alpha tex
//TWO_TEX_TINT_VERTEX_SHADER

#include <algorithm>
#include "TwoTexBatch.h"
#include "base/ccUtils.h"
#include "renderer/CCRenderer.h"
#include "base/CCDirector.h"
#include "base/CCEventDispatcher.h"

USING_NS_CC;
#define EVENT_AFTER_DRAW_RESET_POSITION "director_after_draw"
using std::max;
#define INITIAL_SIZE (10000)
#define MAX_VERTICES 64000
#define MAX_INDICES 64000

#define STRINGIFY(A)  #A

namespace zzcc {

	TwoTexTrianglesCommand::TwoTexTrianglesCommand() :_materialID(0), _textureID(0), _glProgramState(nullptr), _glProgram(nullptr), _blendType(BlendFunc::DISABLE), _alphaTextureID(0) {
	_type = RenderCommand::Type::CUSTOM_COMMAND;
	func = [this]() { draw(); };
}

void TwoTexTrianglesCommand::init(float globalOrder, GLuint textureID, GLProgramState* glProgramState, BlendFunc blendType, const TwoTexTriangles& triangles, const Mat4& mv, uint32_t flags) {
	CCASSERT(glProgramState, "Invalid GLProgramState");
	CCASSERT(glProgramState->getVertexAttribsFlags() == 0, "No custom attributes are supported in QuadCommand");

	RenderCommand::init(globalOrder, mv, flags,0);

	_triangles = triangles;
	if(_triangles.indexCount % 3 != 0) {
	int count = _triangles.indexCount;
		_triangles.indexCount = count / 3 * 3;
		CCLOGERROR("Resize indexCount from %d to %d, size must be multiple times of 3", count, _triangles.indexCount);
	}
	_mv = mv;

	if( _textureID != textureID || _blendType.src != blendType.src || _blendType.dst != blendType.dst ||
		_glProgramState != glProgramState ||
		_glProgram != glProgramState->getGLProgram()) {
		_textureID = textureID;
		_blendType = blendType;
		_glProgramState = glProgramState;
		_glProgram = glProgramState->getGLProgram();

		generateMaterialID();
	}
}

TwoTexTrianglesCommand::~TwoTexTrianglesCommand() {
}

void TwoTexTrianglesCommand::generateMaterialID() {
	// do not batch if using custom uniforms (since we cannot batch) it
	if(_glProgramState->getUniformCount() > 0) {
		_materialID = Renderer::MATERIAL_ID_DO_NOT_BATCH;
		setSkipBatching(true);
	}
	else {
		int glProgram = (int)_glProgram->getProgram();		
		_materialID = glProgram + (int)_textureID + (int)_blendType.src + (int)_blendType.dst;
	}
}

void TwoTexTrianglesCommand::useMaterial() const {
	//Set texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _textureID);
	
	if (_alphaTextureID > 0)
    {
		// ANDROID ETC1 ALPHA supports.
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, _alphaTextureID);
	}
	//set blend mode
    cocos2d::utils::setBlending(_blendType.src, _blendType.dst);
	
	_glProgramState->apply(_mv);
}
	
void TwoTexTrianglesCommand::draw() {
	CCTwoTexBatch::getInstance()->batch(this);
}

const char* TWO_TEX_TINT_VERTEX_SHADER = STRINGIFY(
attribute vec4 a_position;
attribute vec4 a_color;
attribute vec4 a_color2;
attribute vec2 a_texCoords;

\n#ifdef GL_ES\n
varying lowp vec4 v_light;
varying lowp vec4 v_dark;
varying mediump vec2 v_texCoord;
\n#else\n
varying vec4 v_light;
varying vec4 v_dark;
varying vec2 v_texCoord;

\n#endif\n

void main() {
	v_light = a_color;
	v_dark = a_color2;
	v_texCoord = a_texCoords;
	gl_Position = CC_PMatrix * a_position;
}
);

const char* TWO_TEX_TINT_FRAGMENT_SHADER = STRINGIFY(
\n#ifdef GL_ES\n
precision lowp float;
\n#endif\n

varying vec4 v_light;
varying vec4 v_dark;
varying vec2 v_texCoord;

void main() {
	vec4 texColor = texture2D(CC_Texture0, v_texCoord);
	float alpha = texColor.a * v_light.a;
	gl_FragColor.a = alpha;
	gl_FragColor.rgb = (1.0 - texColor.rgb) * v_dark.rgb * alpha + texColor.rgb * v_light.rgb;	
}
);


static CCTwoTexBatch* instance = nullptr;

CCTwoTexBatch* CCTwoTexBatch::getInstance () {
	if (!instance) instance = new CCTwoTexBatch();
	return instance;
}

void CCTwoTexBatch::destroyInstance () {
	if (instance) {
		delete instance;
		instance = nullptr;
	}
}

CCTwoTexBatch::CCTwoTexBatch () {
	for (unsigned int i = 0; i < INITIAL_SIZE; i++) {
		_commandsPool.push_back(new TwoTexTrianglesCommand());
	}
	
	_indices = spUnsignedShortArray_create(8);
	
	reset ();
	
	// callback after drawing is finished so we can clear out the batch state
	// for the next frame
	Director::getInstance()->getEventDispatcher()->addCustomEventListener(EVENT_AFTER_DRAW_RESET_POSITION, [this](EventCustom* eventCustom){
		this->update(0);
	});;
	
	_twoColorTintShader = cocos2d::GLProgram::createWithByteArrays(TWO_TEX_TINT_VERTEX_SHADER, TWO_TEX_TINT_FRAGMENT_SHADER);
	_twoColorTintShaderState = GLProgramState::getOrCreateWithGLProgram(_twoColorTintShader);
	_twoColorTintShaderState->retain();
	
	glGenBuffers(1, &_vertexBufferHandle);
	_vertexBuffer = new V3F_C4B_T2F_T2F[MAX_VERTICES];
	glGenBuffers(1, &_indexBufferHandle);
	_indexBuffer = new unsigned short[MAX_INDICES];
	_positionAttributeLocation = _twoColorTintShader->getAttribLocation("a_position");
	_colorAttributeLocation = _twoColorTintShader->getAttribLocation("a_color");
	_tex2CoordsAttributeLocation = _twoColorTintShader->getAttribLocation("a_texCoords2");
	_texCoordsAttributeLocation = _twoColorTintShader->getAttribLocation("a_texCoords");
}

CCTwoTexBatch::~CCTwoTexBatch () {
	Director::getInstance()->getEventDispatcher()->removeCustomEventListeners(EVENT_AFTER_DRAW_RESET_POSITION);
	
	spUnsignedShortArray_dispose(_indices);
	
	for (unsigned int i = 0; i < _commandsPool.size(); i++) {
		delete _commandsPool[i];
		_commandsPool[i] = nullptr;
	}
	_twoColorTintShader->release();
	delete _vertexBuffer;
	delete _indexBuffer;
}

void CCTwoTexBatch::update (float delta) {	
	reset();
}

V3F_C4B_T2F_T2F* CCTwoTexBatch::allocateVertices(uint32_t numVertices) {
	if (_vertices.size() - _numVertices < numVertices) {
		V3F_C4B_T2F_T2F* oldData = _vertices.data();
		_vertices.resize((_vertices.size() + numVertices) * 2 + 1);
		V3F_C4B_T2F_T2F* newData = _vertices.data();
		for (uint32_t i = 0; i < this->_nextFreeCommand; i++) {
			TwoTexTrianglesCommand* command = _commandsPool[i];
			TwoTexTriangles& triangles = (TwoTexTriangles&)command->getTriangles();
			triangles.verts = newData + (triangles.verts - oldData);
		}
	}
	
	V3F_C4B_T2F_T2F* vertices = _vertices.data() + _numVertices;
	_numVertices += numVertices;
	return vertices;
}
	
	
void CCTwoTexBatch::deallocateVertices(uint32_t numVertices) {
	_numVertices -= numVertices;
}


unsigned short* CCTwoTexBatch::allocateIndices(uint32_t numIndices) {
	if (_indices->capacity - _indices->size < (int)numIndices) {
		unsigned short* oldData = _indices->items;
		int oldSize =_indices->size;
		spUnsignedShortArray_ensureCapacity(_indices, _indices->size + numIndices);
		unsigned short* newData = _indices->items;
		for (uint32_t i = 0; i < this->_nextFreeCommand; i++) {
			TwoTexTrianglesCommand* command = _commandsPool[i];
			TwoTexTriangles& triangles = (TwoTexTriangles&)command->getTriangles();
			if (triangles.indices >= oldData && triangles.indices < oldData + oldSize) {
				triangles.indices = newData + (triangles.indices - oldData);
			}
		}
	}
	
	unsigned short* indices = _indices->items + _indices->size;
	_indices->size += numIndices;
	return indices;
}

void CCTwoTexBatch::deallocateIndices(uint32_t numIndices) {
	_indices->size -= numIndices;
}

TwoTexTrianglesCommand* CCTwoTexBatch::addCommand(cocos2d::Renderer* renderer, float globalOrder, GLuint textureID, cocos2d::GLProgramState* glProgramState, cocos2d::BlendFunc blendType, const TwoTexTriangles& triangles, const cocos2d::Mat4& mv, uint32_t flags) {
	TwoTexTrianglesCommand* command = nextFreeCommand();
	command->init(globalOrder, textureID, glProgramState, blendType, triangles, mv, flags);
	renderer->addCommand(command);	
	return command;
}
	
void CCTwoTexBatch::batch (TwoTexTrianglesCommand* command) {
	if (_numVerticesBuffer + command->getTriangles().vertCount >= MAX_VERTICES || _numIndicesBuffer + command->getTriangles().indexCount >= MAX_INDICES) {
		flush(_lastCommand);
	}
	
	memcpy(_vertexBuffer + _numVerticesBuffer, command->getTriangles().verts, sizeof(V3F_C4B_T2F_T2F) * command->getTriangles().vertCount);
	const Mat4& modelView = command->getModelView();
	for (int i = _numVerticesBuffer; i < (int)_numVerticesBuffer + command->getTriangles().vertCount; i++) {
		modelView.transformPoint(&_vertexBuffer[i].position);
	}
	
	unsigned short vertexOffset = (unsigned short)_numVerticesBuffer;
	unsigned short* indices = command->getTriangles().indices;
	for (int i = 0, j = _numIndicesBuffer; i < command->getTriangles().indexCount; i++, j++) {
		_indexBuffer[j] = indices[i] + vertexOffset;
	}
	
	_numVerticesBuffer += command->getTriangles().vertCount;
	_numIndicesBuffer += command->getTriangles().indexCount;
	
	uint32_t materialID = command->getMaterialID();
	
	if ((_lastCommand && _lastCommand->getMaterialID() != materialID) || command->isForceFlush()) {
		flush(_lastCommand);
	}
	_lastCommand = command;
}
	
void CCTwoTexBatch::flush (TwoTexTrianglesCommand* materialCommand) {
	if (!materialCommand)
		return;
	
	materialCommand->useMaterial();
	
	glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(V3F_C4B_T2F_T2F) * _numVerticesBuffer , _vertexBuffer, GL_DYNAMIC_DRAW);
	
	glEnableVertexAttribArray(_positionAttributeLocation);
	glEnableVertexAttribArray(_colorAttributeLocation);
	glEnableVertexAttribArray(_texCoordsAttributeLocation);
	glEnableVertexAttribArray(_tex2CoordsAttributeLocation);

	glVertexAttribPointer(_positionAttributeLocation, 3, GL_FLOAT, GL_FALSE, sizeof(V3F_C4B_T2F_T2F), (GLvoid*)offsetof(V3F_C4B_T2F_T2F, position));
	glVertexAttribPointer(_colorAttributeLocation, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(V3F_C4B_T2F_T2F), (GLvoid*)offsetof(V3F_C4B_T2F_T2F, color));
	glVertexAttribPointer(_texCoordsAttributeLocation, 2, GL_FLOAT, GL_FALSE, sizeof(V3F_C4B_T2F_T2F), (GLvoid*)offsetof(V3F_C4B_T2F_T2F, texCoords));
	glVertexAttribPointer(_tex2CoordsAttributeLocation, 2, GL_FLOAT, GL_FALSE, sizeof(V3F_C4B_T2F_T2F), (GLvoid*)offsetof(V3F_C4B_T2F_T2F, texCoords2));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBufferHandle);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * _numIndicesBuffer, _indexBuffer, GL_STATIC_DRAW);
	
	glDrawElements(GL_TRIANGLES, (GLsizei)_numIndicesBuffer, GL_UNSIGNED_SHORT, 0);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	_numVerticesBuffer = 0;
	_numIndicesBuffer = 0;
	_numBatches++;
}

void CCTwoTexBatch::reset() {
	_nextFreeCommand = 0;
	_numVertices = 0;
	_indices->size = 0;
	_numVerticesBuffer = 0;
	_numIndicesBuffer = 0;
	_lastCommand = nullptr;
	_numBatches = 0;
}

TwoTexTrianglesCommand* CCTwoTexBatch::nextFreeCommand() {
	if (_commandsPool.size() <= _nextFreeCommand) {
		size_t newSize = _commandsPool.size() * 2 + 1;
		for (size_t i = _commandsPool.size();  i < newSize; i++) {
			_commandsPool.push_back(new TwoTexTrianglesCommand());
		}
	}
	TwoTexTrianglesCommand* command = _commandsPool[_nextFreeCommand++];
	command->setForceFlush(false);
	return command;
}
}
