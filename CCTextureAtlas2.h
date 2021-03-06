/****************************************************************************
zz modi for water : 
****************************************************************************/

#ifndef __CCTEXTURE_ATLAS2_H__
#define __CCTEXTURE_ATLAS2_H__

#include <string>

#include "base/ccTypes.h"
#include "base/CCRef.h"
#include "base/ccConfig.h"

NS_CC_BEGIN

class Texture2D;
class EventCustom;
class EventListenerCustom;

#define USE_TEXCOORDS2 1

struct V3F_C4B_T2F_T2F
{
	/// vertices (3F)
	Vec3     vertices;            // 12 bytes
								  /// colors (4B)
	Color4B      colors;              // 4 bytes
									  // tex coords (2F)
	Tex2F        texCoords;           // 8 bytes
#if USE_TEXCOORDS2 == 1
	Tex2F        texCoords2;          // 8 bytes
#endif
};

struct V3F_C4B_T2F_T2F_Quad
{
	/// top left
	V3F_C4B_T2F_T2F    tl;
	/// bottom left
	V3F_C4B_T2F_T2F    bl;
	/// top right
	V3F_C4B_T2F_T2F    tr;
	/// bottom right
	V3F_C4B_T2F_T2F    br;
};


/**
 * @addtogroup _2d
 * @{
 */

/** @brief A class that implements a Texture Atlas.
Supported features:
* The atlas file can be a PVRTC, PNG or any other format supported by Texture2D
* Quads can be updated in runtime
* Quads can be added in runtime
* Quads can be removed in runtime
* Quads can be re-ordered in runtime
* The TextureAtlas2 capacity can be increased or decreased in runtime
* OpenGL component: V3F, C4B, T2F, T2F.
The quads are rendered using an OpenGL ES VBO.
To render the quads using an interleaved vertex array list, you should modify the ccConfig.h file 

@warning If you want to use TextureAtlas2, you'd better setup GL status before it's rendered.
         Otherwise, the effect of TextureAtlas2 will be affected by the GL status of other nodes.
*/
class TextureAtlas2 : public Ref 
{
public:
    /** Creates a TextureAtlas2 with an filename and with an initial capacity for Quads.

     * The TextureAtlas2 capacity can be increased in runtime.
     @param file A null terminated string contains the file path.
     @param capacity Capacity for Quads.
    */
    static TextureAtlas2* create(const std::string& file , ssize_t capacity);

    /** Creates a TextureAtlas2 with a previously initialized Texture2D object, and
     * with an initial capacity for n Quads.

     * The TextureAtlas2 capacity can be increased in runtime.
     @param texture A texture2D object pointer.
     @param capacity Capacity for Quads.
     */
    static TextureAtlas2* createWithTexture(Texture2D *texture, ssize_t capacity);
    /**
     * @js ctor
     */
    TextureAtlas2();
    /**
     * @js NA
     * @lua NA
     */
    virtual ~TextureAtlas2();

    /** Initializes a TextureAtlas2 with a filename and with a certain capacity for Quads.
    * The TextureAtlas2 capacity can be increased in runtime.
    *
     @attention Do not reinitialize the TextureAtlas2 because it will leak memory (issue #706).
     @param file A null terminated string contains the file path.
     @param capacity Capacity for Quads.
    */
    bool initWithFile(const std::string& file, ssize_t capacity);

    /** Initializes a TextureAtlas2 with a previously initialized Texture2D object, and
    * with an initial capacity for Quads. 
    * The TextureAtlas2 capacity can be increased in runtime.
     @attention: Do not reinitialize the TextureAtlas2 because it will leak memory (issue #706).
     @param texture A texture2D object pointer.
     @param capacity Capacity for Quads.
    */
    bool initWithTexture(Texture2D *texture, ssize_t capacity);

    /** Updates a Quad (texture, vertex and color) at a certain index.
    @param quad Quad that are going to be rendered.
    @param index Index must be between 0 and the atlas capacity - 1.
    @since v0.8
    */
    void updateQuad(V3F_C4B_T2F_T2F_Quad* quad, ssize_t index);

    /** Inserts a Quad (texture, vertex and color) at a certain index.
     @param quad Quad that are going to be rendered.
     @param index Index must be between 0 and the atlas capacity - 1.
     @since v0.8
    */
    void insertQuad(V3F_C4B_T2F_T2F_Quad* quad, ssize_t index);

    /** Inserts a c array of quads at a given index.
     @param quads Quad that are going to be rendered.
     @param index Index must be between 0 and the atlas capacity - 1.
     @param amount The quads array amount.
     @attention This method doesn't enlarge the array when amount + index > totalQuads.
     @since v1.1
    */
    void insertQuads(V3F_C4B_T2F_T2F_Quad* quads, ssize_t index, ssize_t amount);

    /** Removes the quad that is located at a certain index and inserts it at a new index.
    This operation is faster than removing and inserting in a quad in 2 different steps.
     @since v0.7.2
    */
    void insertQuadFromIndex(ssize_t fromIndex, ssize_t newIndex);

    /** Removes a quad at a given index number.
    The capacity remains the same, but the total number of quads to be drawn is reduced in 1.
    @since v0.7.2
    */
    void removeQuadAtIndex(ssize_t index);

    /** Removes a amount of quads starting from index.
        @since 1.1
     */
    void removeQuadsAtIndex(ssize_t index, ssize_t amount);
    /** Removes all Quads.
    The TextureAtlas2 capacity remains untouched. No memory is freed.
    The total number of quads to be drawn will be 0.
    @since v0.7.2
    */
    void removeAllQuads();

    /** Resize the capacity of the TextureAtlas2.
    * The new capacity can be lower or higher than the current one.
    * It returns true if the resize was successful.
    * If it fails to resize the capacity it will return false with a new capacity of 0.
     
     @param capacity Capacity for Quads.
    */
    bool resizeCapacity(ssize_t capacity);

    /**
     Used internally by ParticleBatchNode.
     don't use this unless you know what you're doing.
     @since 1.1
    */
    void increaseTotalQuadsWith(ssize_t amount);

    /** Moves an amount of quads from oldIndex at newIndex.
     @since v1.1
     */
    void moveQuadsFromIndex(ssize_t oldIndex, ssize_t amount, ssize_t newIndex);

    /**
     Moves quads from index till totalQuads to the newIndex.
     Used internally by ParticleBatchNode.
     This method doesn't enlarge the array if newIndex + quads to be moved > capacity.
     @since 1.1
    */
    void moveQuadsFromIndex(ssize_t index, ssize_t newIndex);

    /**
     Ensures that after a realloc quads are still empty.
     Used internally by ParticleBatchNode.
     @since 1.1
    */
    void fillWithEmptyQuadsFromIndex(ssize_t index, ssize_t amount);

    /** Draws n quads.
    * N can't be greater than the capacity of the Atlas.
    */
    void drawNumberOfQuads(ssize_t n, bool bAlpha);

    /** Draws n quads from an index (offset).
    N + start can't be greater than the capacity of the atlas.

    @since v1.0
    */
    void drawNumberOfQuads(ssize_t numberOfQuads, ssize_t start, bool bAlpha);

    /** Draws all the Atlas's Quads.
    */
    void drawQuads(bool bAlpha);
    /** Listen the event that renderer was recreated on Android.
     */
    void listenRendererRecreated(EventCustom* event);

    /** Whether or not the array buffer of the VBO needs to be updated.*/
    bool isDirty() { return _dirty; }
    /** Specify if the array buffer of the VBO needs to be updated. */
    void setDirty(bool bDirty) { _dirty = bDirty; }

    /**Get quads total amount.
     * @js NA
     * @lua NA
     */
    virtual std::string getDescription() const;

    /** Gets the quantity of quads that are going to be drawn. */
    ssize_t getTotalQuads() const;
    
    /** Gets the quantity of quads that can be stored with the current texture atlas size. */
    ssize_t getCapacity() const;
    
    /** Gets the texture of the texture atlas. */
    Texture2D* getTexture() const;
    
    /** Sets the texture for the texture atlas. */
    void setTexture(Texture2D* texture);
    
    /** Gets the quads that are going to be rendered. */
    V3F_C4B_T2F_T2F_Quad* getQuads();
    
    /** Sets the quads that are going to be rendered. */
    void setQuads(V3F_C4B_T2F_T2F_Quad* quads);
    
private:
    void renderCommand();

    void setupIndices();
    void mapBuffers();
    void setupVBOandVAO();
    void setupVBO();

protected:
    GLushort*           _indices;
    GLuint              _VAOname;
    GLuint              _buffersVBO[2]; //0: vertex  1: indices
    bool                _dirty; //indicates whether or not the array buffer of the VBO needs to be updated
    /** quantity of quads that are going to be drawn */
    ssize_t _totalQuads;
    /** quantity of quads that can be stored with the current texture atlas size */
    ssize_t _capacity;
    /** Texture of the texture atlas */
    Texture2D* _texture;
    /** Quads that are going to be rendered */
    V3F_C4B_T2F_T2F_Quad* _quads;
    
#if CC_ENABLE_CACHE_TEXTURE_DATA
    EventListenerCustom* _rendererRecreatedListener;
#endif
};

// end of textures group
/// @}

NS_CC_END

#endif //__CCTEXTURE_ATLAS2_H__


