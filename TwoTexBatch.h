
#ifndef TWOTEXBATCH_H_
#define TWOTEXBATCH_H_

#include "2d/CCNode.h"
#include "renderer/CCCustomCommand.h"
#include "renderer/CCQuadCommand.h"
#include "CCTileAtlas.h"
#include "base/ccTypes.h"
#include <vector>

namespace zzcc {

#define _SP_ARRAY_DECLARE_TYPE(name, itemType) \
	typedef struct name { int size; int capacity; itemType* items; } name; \
	name* name##_create(int initialCapacity); \
	void name##_dispose(name* self); \
	void name##_clear(name* self); \
	name* name##_setSize(name* self, int newSize); \
	void name##_ensureCapacity(name* self, int newCapacity); \
	void name##_add(name* self, itemType value); \
	void name##_addAll(name* self, name* other); \
	void name##_addAllValues(name* self, itemType* values, int offset, int count); \
	void name##_removeAt(name* self, int index); \
	int name##_contains(name* self, itemType value); \
	itemType name##_pop(name* self); \
	itemType name##_peek(name* self);

	_SP_ARRAY_DECLARE_TYPE(spUnsignedShortArray, unsigned short)

	struct V3F_C4B_T2F_T2F {
		cocos2d::Vec3 position;
		cocos2d::Color4B color;
		cocos2d::Tex2F texCoords;
		cocos2d::Tex2F texCoords2;	//the second 
	};
	
	struct TwoTexTriangles {
		V3F_C4B_T2F_T2F* verts;
		unsigned short* indices;
		int vertCount;
		int indexCount;
	};
	
	class TwoTexTrianglesCommand : public cocos2d::CustomCommand {
	public:
		TwoTexTrianglesCommand();
		
		~TwoTexTrianglesCommand();
	
		void init(float globalOrder, GLuint textureID, cocos2d::GLProgramState* glProgramState, cocos2d::BlendFunc blendType, const TwoTexTriangles& triangles, const cocos2d::Mat4& mv, uint32_t flags);
		
		void useMaterial() const;
		
		inline uint32_t getMaterialID() const { return _materialID; }
		
		inline GLuint getTextureID() const { return _textureID; }
		
		inline const TwoTexTriangles& getTriangles() const { return _triangles; }
		
		inline ssize_t getVertexCount() const { return _triangles.vertCount; }
		
		inline ssize_t getIndexCount() const { return _triangles.indexCount; }
		
		inline const V3F_C4B_T2F_T2F* getVertices() const { return _triangles.verts; }
		
		inline const unsigned short* getIndices() const { return _triangles.indices; }
		
		inline cocos2d::GLProgramState* getGLProgramState() const { return _glProgramState; }
		
		inline cocos2d::BlendFunc getBlendType() const { return _blendType; }
		
		inline const cocos2d::Mat4& getModelView() const { return _mv; }
		
		void draw ();
		
		void setForceFlush (bool forceFlush) { _forceFlush = forceFlush; }
		
		bool isForceFlush () { return _forceFlush; };
		
	protected:
		void generateMaterialID();
		uint32_t _materialID;
		GLuint _textureID;
		cocos2d::GLProgramState* _glProgramState;
		cocos2d::GLProgram* _glProgram;
		cocos2d::BlendFunc _blendType;
		TwoTexTriangles _triangles;
		cocos2d::Mat4 _mv;
		GLuint _alphaTextureID;
		bool _forceFlush;
	};

    class CCTwoTexBatch {
    public:
        static CCTwoTexBatch* getInstance ();

        static void destroyInstance ();

        void update (float delta);

		V3F_C4B_T2F_T2F* allocateVertices(uint32_t numVertices);
		void deallocateVertices(uint32_t numVertices);
		
		unsigned short* allocateIndices(uint32_t numIndices);
		void deallocateIndices(uint32_t numIndices);

		TwoTexTrianglesCommand* addCommand(cocos2d::Renderer* renderer, float globalOrder, GLuint textureID, cocos2d::GLProgramState* glProgramState, cocos2d::BlendFunc blendType, const TwoTexTriangles& triangles, const cocos2d::Mat4& mv, uint32_t flags);

		cocos2d::GLProgramState* getTwoTexTintProgramState () { return _twoColorTintShaderState; }
		
		void batch (TwoTexTrianglesCommand* command);
		
		void flush (TwoTexTrianglesCommand* materialCommand);
		
		uint32_t getNumBatches () { return _numBatches; };
		
    protected:
        CCTwoTexBatch ();
        virtual ~CCTwoTexBatch ();

		void reset ();

		TwoTexTrianglesCommand* nextFreeCommand ();

		// pool of commands
		std::vector<TwoTexTrianglesCommand*> _commandsPool;
		uint32_t _nextFreeCommand;

		// pool of vertices
		std::vector<V3F_C4B_T2F_T2F> _vertices;
		uint32_t _numVertices;
		
		// pool of indices
		spUnsignedShortArray* _indices;
		
		// two color tint shader and state
		cocos2d::GLProgram* _twoColorTintShader;
		cocos2d::GLProgramState* _twoColorTintShaderState;
		
		// VBO handles & attribute locations
		GLuint _vertexBufferHandle;
		V3F_C4B_T2F_T2F* _vertexBuffer;
		uint32_t _numVerticesBuffer;
		GLuint _indexBufferHandle;
		uint32_t _numIndicesBuffer;
		unsigned short* _indexBuffer;
		GLint _positionAttributeLocation;
		GLint _colorAttributeLocation;
		GLint _texCoordsAttributeLocation;
		GLint _tex2CoordsAttributeLocation;	//第2张纹理的坐标...

		// last batched command, needed for flushing to set material
		TwoTexTrianglesCommand* _lastCommand;
		
		// number of batches in the last frame
		uint32_t _numBatches;
	};
}

#endif // TWOTEXBATCH_H_
