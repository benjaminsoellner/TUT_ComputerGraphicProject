/*** OHJ-2700 exercise 3
Name: Benjamin Söllner
Number: 206342
Email: benjamin.sollner@tut.fi
***/

// Textures can be downloaded from: http://www.students.tut.fi/~sollner/cgassign3/textures.zip

#include <iostream>
#include <cmath>
#include <set>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <GL/glut.h>

using namespace std;


/***************************************************************************************
 ************      SMALL STUFF                                              ************
 ***************************************************************************************/


/**********************************************************
 *                   struct Point
 * a simple struct to combine x, y, z coordinates
 **********************************************************/

struct Point {
	float x, y, z;
};


/**********************************************************
 *                auxiliary functions
 * provide tools such as reading a certain number of bytes
 * from a stream, loading an image from a file, loading
 * a texture from a file or drawing a rectangle
 **********************************************************/

/*
 * reads nbrBytes from the pointer addr and returns
 * the number, which is computed by these bytes
 */

int glauxReadBytes(unsigned char* &addr, int nbrBytes) {
	int sum = 0;
    addr += nbrBytes;
    for (int i = 0; i < nbrBytes; i++)
        sum = *(addr-i-1) + 256*sum;
    return sum;
}

/*
 * loads an image from the file filename - this can be a .tga or a .bmp image
 * (only supports 24 and 32 bit images without compression). changes the
 * contents of width, height and bytesPerPixel according to the image
 */

void *glauxLoadImage(const string& filename, int& width, int& height, int& bytesPerPixel) {
	const int bmpSizeHeaderSize = 14, bmpImageHeaderSize = 40, tgaHeaderSize = 18;
	unsigned char *imageData;
	ifstream file; 
	unsigned char bmpFileHeader[bmpSizeHeaderSize];
	unsigned char bmpImageHeader[bmpImageHeaderSize];
	unsigned char tgaHeader[tgaHeaderSize];
	int size, offset;

	file.open(filename.c_str(), ios::in | ios::binary);
	if (!file) {
		cerr << "Cannot open " << filename << endl;
		return NULL;
	}

	// BMP image
	if (filename.substr(filename.size()-4, 4) == ".bmp") {
		file.read((char*)bmpFileHeader,  bmpSizeHeaderSize);  // reading BMP file header
		file.read((char*)bmpImageHeader, bmpImageHeaderSize); // reading BMP image header
		if (file.bad()) {
			cerr << "Cannot read file!" << endl;
			file.close();
			return NULL;
		}
		// BMP file header fields
		unsigned char *pBmpFileHeader = bmpFileHeader;
		unsigned char magicNumber1 = glauxReadBytes(pBmpFileHeader,1);
		unsigned char magicNumber2 = glauxReadBytes(pBmpFileHeader,1);
		           /* fileSize    */ glauxReadBytes(pBmpFileHeader,4);
		           /* headerSize  */ glauxReadBytes(pBmpFileHeader,4);
		unsigned int offsetToData  = glauxReadBytes(pBmpFileHeader,4);
		// BMP image header fields
		unsigned char *pBmpImageHeader = bmpImageHeader;
		          /* headerSize        */ glauxReadBytes(pBmpImageHeader,4);
		unsigned int widthBMP           = glauxReadBytes(pBmpImageHeader,4);
		unsigned int heightBMP          = glauxReadBytes(pBmpImageHeader,4);
		unsigned short int planes       = glauxReadBytes(pBmpImageHeader,2);
		unsigned short int bitsPerPixel = glauxReadBytes(pBmpImageHeader,2);
		unsigned int compression        = glauxReadBytes(pBmpImageHeader,4);
		unsigned int imageSize          = glauxReadBytes(pBmpImageHeader,4);
		          /* xResolution       */ glauxReadBytes(pBmpImageHeader,4);
		          /* yResolution       */ glauxReadBytes(pBmpImageHeader,4);
		          /* nbrColours        */ glauxReadBytes(pBmpImageHeader,4);
		          /* importantColours  */ glauxReadBytes(pBmpImageHeader,4);
		if (magicNumber1 != 'B' || magicNumber2 != 'M') {
			cerr << filename << " not a BMP file!" << endl;
			file.close();
			return NULL;
		}
		if (planes != 1 || compression != 0 || (bitsPerPixel != 24 && bitsPerPixel != 32)) {
			cerr << filename << ": Only support uncompressed 24- or 32-bit RGB files!" << endl;
			cerr << "planes: " << planes << ", compression: " << compression << "bitsPerPixel: " << bitsPerPixel << endl;
			file.close();
			return NULL;
		}
		width = widthBMP;
		height = heightBMP;
		bytesPerPixel = bitsPerPixel/8;
		offset = offsetToData;
		size = imageSize;
  
	// TGA image
	} else if (filename.substr(filename.size()-4, 4) == ".tga") {
		file.read((char*)&tgaHeader, tgaHeaderSize);
		if (file.bad()) {
			cerr << "Cannot read file!" << endl;
			file.close();
			return NULL;
		}
		// TGA header fields
		unsigned char *pTgaHeader = tgaHeader;
		           /* identsize       */ glauxReadBytes(pTgaHeader,1);
		           /* colourmaptype   */ glauxReadBytes(pTgaHeader,1);
		unsigned char imagetype        = glauxReadBytes(pTgaHeader,1);
		           /* colourmapstart  */ glauxReadBytes(pTgaHeader,2);
		           /* colourmaplength */ glauxReadBytes(pTgaHeader,2);
		           /* colourmapbits   */ glauxReadBytes(pTgaHeader,1);
		           /* xorigin         */ glauxReadBytes(pTgaHeader,2);
		           /* yorigin         */ glauxReadBytes(pTgaHeader,2);
		unsigned short widthTGA        = glauxReadBytes(pTgaHeader,2);
		unsigned short heightTGA       = glauxReadBytes(pTgaHeader,2);
		unsigned char bitsPerPixel     = glauxReadBytes(pTgaHeader,1);
		           /* descriptor      */ glauxReadBytes(pTgaHeader,1);
		if (imagetype != 2 || (bitsPerPixel != 24 && bitsPerPixel != 32)) {
			cerr << filename << ": Only support uncompressed 24- or 32-bit RGB files!" << endl;
			file.close();
			return NULL;
		}
		width = widthTGA;
		height = heightTGA;
		bytesPerPixel = bitsPerPixel/8;
		offset = tgaHeaderSize;
		size = width * height * bytesPerPixel;
	} else {
		cerr << "Only support BMP and TGA files!" << endl;
		file.close();
		return NULL;
	}
	file.seekg(offset, ios::beg);
	imageData = new unsigned char[size];
	file.read((char*)imageData, size);
	file.close();
	return imageData;
}

/*
 * loads a texture from the file provided by filename and returns the
 * openGL texture handle
 */

GLuint glauxLoadTexture(const string& filename) {
    GLuint texture;
	int width, height, bytesperpixel;
	unsigned char *buffer = (unsigned char*) glauxLoadImage(filename, width, height, bytesperpixel);
    glGenTextures(1, &texture);             // allocate texture ID
    glBindTexture(GL_TEXTURE_2D, texture);  // select our current texture
	gluBuild2DMipmaps(GL_TEXTURE_2D, bytesperpixel, width, height, (bytesperpixel == 4 ? GL_RGBA : GL_RGB), GL_UNSIGNED_BYTE, buffer);
	// texture parameters
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    delete buffer;
    return texture;
}

/*
 * draws a rectangle to the screen, given by the x,y,z-coordinates of the 
 * corner points provided with tl, tr, bl, br (top-left, top-right etc.)
 * sets the UV coordinates for the corners being startU at the top points
 * endU at the bottom points, startV right and endV left
 */

void glauxRectangle(float tlX, float tlY, float tlZ,
					 float trX, float trY, float trZ,
					 float brX, float brY, float brZ,
					 float blX, float blY, float blZ,
					 float startU, float startV, float endU, float endV) {
	glBegin(GL_QUADS);
		glTexCoord2f(startU, startV); glVertex3f(tlX, tlY, tlZ);
		glTexCoord2f(startU, endV);   glVertex3f(trX, trY, trZ);
		glTexCoord2f(endU,   endV);   glVertex3f(brX, brY, brZ);
		glTexCoord2f(endU,   startV); glVertex3f(blX, blY, blZ);
	glEnd();
}


/***************************************************************************************
 ************      WALLS   (collision detection)                            ************
 ***************************************************************************************/


/**********************************************************
 *                  class WallPlane
 * represents one of the three basic planes in the 
 * coordinate system
 **********************************************************/

enum WallPlane {
	WALL_XY_PLANE=0, WALL_YZ_PLANE=1, WALL_XZ_PLANE=2
};

/**********************************************************
 *                      class Wall
 * represents a (invisible) wall, which can be used for
 * collision testing. a wall can be parallel to the 
 * XY-, the YZ or the XZ-plane (see enum WallPlane). A 
 * wall has two start- and end-values and a limit. E.g.
 * for a XY-parallel wall, the limit would represent the 
 * location of the wall on the Z-axis and start- and end-
 * values would represent the area in X- and Y-dimension,
 * where the wall is located.
 **********************************************************/

class Wall {
	private:
		float start1, end1;
		float start2, end2;
		float limit;
		WallPlane parallelTo;
	public:
		Wall(WallPlane parallelTo, float limit, float start1, float end1, float start2, float end2);
		bool canIMove(Point from, Point to);
		void draw();
};

Wall::Wall(WallPlane parallelTo, float limit, float start1, float end1, float start2, float end2):
	start1(start1), end1(end1), start2(start2), end2(end2), limit(limit), parallelTo(parallelTo) {}

/*
 * returns, if a line from the Point from to the Point to
 * can be drawen without intersecting the wall
 */

bool Wall::canIMove(Point from, Point to) {
	bool steppedOverLimit = false;
	bool inFrontOfWall    = false;
	if (this->parallelTo == WALL_XY_PLANE) {
		steppedOverLimit = (from.z <= this->limit && to.z >= this->limit) ||
						   (from.z >= this->limit && to.z <= this->limit);
		inFrontOfWall = ((from.x >= this->start1 && from.x <= this->end1) &&
						(from.y >= this->start2 && from.y <= this->end2)) ||
						((to.x >= this->start1 && to.x <= this->end1) &&
						(to.y >= this->start2 && to.y <= this->end2));
	} else if (this->parallelTo == WALL_YZ_PLANE) {
		steppedOverLimit = (from.x <= this->limit && to.x >= this->limit ||
							from.x >= this->limit && to.x <= this->limit );
		inFrontOfWall = ((from.y >= this->start1 && from.y <= this->end1) &&
						(from.z >= this->start2 && from.z <= this->end2)) ||
						((to.y >= this->start1 && to.y <= this->end1) &&
						(to.z >= this->start2 && to.z <= this->end2));
	} else if (this->parallelTo == WALL_XZ_PLANE) {
		steppedOverLimit = (from.y <= this->limit && to.y >= this->limit ||
							from.y >= this->limit && to.y <= this->limit );
		inFrontOfWall = ((from.x >= this->start1 && from.x <= this->end1) &&
						(from.z >= this->start2 && from.z <= this->end2)) ||
						((to.x >= this->start1 && to.x <= this->end1) &&
						(to.z >= this->start2 && to.z <= this->end2));

	}
	return !inFrontOfWall || !steppedOverLimit;
}

/*
 * draws the wall with a random color. only used for testing
 * for rendering, the RoomWall class is used.
 */

void Wall::draw() {
	float tlX = 0.0; float trX = 0.0; float blX = 0.0; float brX = 0.0;
	float tlY = 0.0; float trY = 0.0; float blY = 0.0; float brY = 0.0;
	float tlZ = 0.0; float trZ = 0.0; float blZ = 0.0; float brZ = 0.0;
	if (this->parallelTo == WALL_XY_PLANE) {
		trZ = tlZ = blZ = brZ = this->limit;
		tlX = trX = this->start1; blX = brX = this->end1;
		tlY = blY = this->start2; trY = brY = this->end2;
	} else if (this->parallelTo == WALL_YZ_PLANE) {
		trX = tlX = blX = brX = this->limit;
		tlY = trY = this->start1; blY = brY = this->end1;
		tlZ = blZ = this->start2; trZ = brZ = this->end2;
	} else if (this->parallelTo == WALL_XZ_PLANE) {
		trY = tlY = blY = brY = this->limit;
		tlX = trX = this->start1; blX = brX = this->end1;
		tlZ = blZ = this->start2; trZ = brZ = this->end2;
	}
	// draw border points
	glColor3f((rand()%100)*0.01f, (rand()%100)*0.01f, (rand()%100)*0.01f);
	glBegin(GL_QUADS);
		glVertex3f(trX, trY, trZ);
		glVertex3f(tlX, tlY, tlZ);
		glVertex3f(blX, blY, blZ);
		glVertex3f(brX, brY, brZ);
	glEnd();
}


/**********************************************************
 *                    class WallList
 * represents a collection of walls. collision testing can
 * so be made to all walls at once.
 **********************************************************/

class WallList {
	private:
		set<Wall*> walls;
	public:
		WallList();
		~WallList();
		Wall* add(WallPlane parallelTo, float limit, float start, float end, float top, float bottom);
		void remove(Wall* wall);
		bool canIMove(Point from, Point to);
		void draw();
};

WallList::WallList(): walls() {
}

WallList::~WallList() {
	// free memory for all allocated walls
	for (set<Wall*>::iterator it = this->walls.begin();
		 it != this->walls.end(); it++)
		delete (*it);
}

/*
 * adding & removing walls from the list
 */

Wall* WallList::add(WallPlane parallelTo, float limit, float start, float end, float top, float bottom) {
	Wall* w = new Wall(parallelTo, limit, start, end, top, bottom);
	this->walls.insert(w);
	return w;
}

void WallList::remove(Wall* wall) {
	this->walls.erase(wall);
}

/*
 * does collision testing to all the walls
 */

bool WallList::canIMove(Point from, Point to) {
	for (set<Wall*>::iterator it = this->walls.begin();
		 it != this->walls.end(); it++) 
		if (! (*it)->canIMove(from, to)) return false;
	return true;
}

/*
 * draws all the walls. only used for testing
 */

void WallList::draw() {
	for (set<Wall*>::iterator it = this->walls.begin();
		 it != this->walls.end(); it++) 
		(*it)->draw();
}


/***************************************************************************************
 ************      RENDERABLES                                              ************
 ***************************************************************************************/


/**********************************************************
 *                   class Renderable
 * is an interface which is implemented by all objects,
 * which have to be rendered in the openGL window
 * provides the hooks initScene, renderScene and 
 * disposeScene, which are called before the first
 * rendering, on every rendering and on exiting the program
 * respectively.
 **********************************************************/

class Renderable {
	public: 
		virtual void initScene() {}
		virtual void renderScene() {}
		virtual void disposeScene() {}
};


/**********************************************************
 *                class RenderableList
 * provides the possibility to group multiple Renderables
 * together and init/render/dispose them alltogether.
 * RenderableList is itself a Renderable, so one can even
 * build trees out of it (composite pattern)
 **********************************************************/

class RenderableList : Renderable {
	private:
		set<Renderable*> renderables;
	public:
		RenderableList();
		void add(Renderable* r);
		void remove(Renderable* r);
		virtual void initScene();
		virtual void renderScene();
		virtual void disposeScene();
};

RenderableList::RenderableList(): renderables() {}

/*
 * adding and removing Renderables from the list
 */

void RenderableList::add(Renderable* r) {
	this->renderables.insert(r);
}

void RenderableList::remove(Renderable* r) {
	this->renderables.erase(r);
}

/*
 * initializing, rendering or disposing all objects from the list
 */

void RenderableList::initScene() {
	for (set<Renderable*>::iterator it = this->renderables.begin();
			it != this->renderables.end(); it++)
		(*it)->initScene();
}

void RenderableList::renderScene() {
	for (set<Renderable*>::iterator it = this->renderables.begin();
			it != this->renderables.end(); it++)
		(*it)->renderScene();
}

void RenderableList::disposeScene() {
	for (set<Renderable*>::iterator it = this->renderables.begin();
			it != this->renderables.end(); it++) {
		(*it)->disposeScene();
		delete (*it); // disposing also frees the memory
	}
}


/***************************************************************************************
 ************      ROOMS                                                    ************
 ***************************************************************************************/


/**********************************************************
 *                   class RoomWall
 * represents a wall of a room. this can be the ceiling,
 * floor, left/right/front/back wall. a RoomWall has, similar
 * to a Wall, dimensions (limit and 2 times start/end), but
 * also features for rendering (a texture or a color). 
 * Furthermore a RoomWall can have a door inside of the wall
 * with start end endpoints in both dimensions. A RoomWall
 * can be connected to a WallList object. That way,
 * the RoomWall object will take care, that the adequate
 * walls will be added to the WallList object for collision
 * testing.
 **********************************************************/

class RoomWall : public Renderable {
	private: 
		static const float WALL_DISTANCE;
		static const float RENDER_DISTANCE;
		static bool classInitialized;
		WallPlane parallelTo;
		WallList* wallList;
		Wall *beforeDoor, *afterDoor, *aboveDoor, *belowDoor;
		Wall *befInDoor, *aftInDoor, *aboInDoor, *belInDoor;
		float colorR, colorG, colorB;
		float start1, start2, end1, end2, limit;
		float doorStart1, doorStart2, doorEnd1, doorEnd2;
		bool direction;
		GLuint textureMap;
		GLuint textureBump;
		void removeWalls();
		void createWalls();
		static void auxSwap4Floats(float& a1, float& a2, float& b1, float& b2, float& c1, float& c2, float& d1, float& d2) {
			float dummy;
			dummy = a1; a1 = a2; a2 = dummy;
			dummy = b1; b1 = b2; b2 = dummy;
			dummy = c1; c1 = c2; c2 = dummy;
			dummy = d1; d1 = d2; d2 = dummy;
		}
		static void auxRectangle(WallPlane parallelTo, float limit, float start1, float start2, float end1, float end2, float startUV1, float startUV2, float endUV1, float endUV2) {
			float tlX = 0.0; float trX = 0.0; float blX = 0.0; float brX = 0.0;
			float tlY = 0.0; float trY = 0.0; float blY = 0.0; float brY = 0.0;
			float tlZ = 0.0; float trZ = 0.0; float blZ = 0.0; float brZ = 0.0;
			float u1 = 0.0; float u2 = 0.0; float v1 = 0.0; float v2 = 0.0;
			if (parallelTo == WALL_XY_PLANE) {
				trZ = tlZ = blZ = brZ = limit;
				tlX = trX = start1; blX = brX = end1;
				tlY = blY = start2; trY = brY = end2;
				u1 = startUV1; u2 = endUV1; v1 = startUV2; v2 = endUV2;
			} else if (parallelTo == WALL_YZ_PLANE) {
				trX = tlX = blX = brX = limit;
				tlY = trY = start1; blY = brY = end1;
				tlZ = blZ = start2; trZ = brZ = end2;
				u1 = startUV1; u2 = endUV1; v1 = startUV2; v2 = endUV2;
			} else if (parallelTo == WALL_XZ_PLANE) {
				trY = tlY = blY = brY = limit;
				tlX = trX = start1; blX = brX = end1;
				tlZ = blZ = start2; trZ = brZ = end2;
				v1 = startUV1; v2 = endUV1; u1 = startUV2; u2 = endUV2;
			}
			glauxRectangle(tlX,tlY,tlZ, trX,trY,brZ, brX,brY,brZ, blX,blY,blZ, u1,v1, u2,v2);
		}

	public:
		RoomWall(WallPlane parallelTo, float limit, float start1, float end1, float start2, float end2, 
			WallList* wallList, bool orientation);
		~RoomWall();
		void setDoor(float doorStart1, float doorStart2, float doorEnd1, float doorEnd2);
		void setColor(float r, float g, float b);
		void setTextureMap(const string& filename);
		void setTextureBump(const string& filename);
		bool hasDoor();
		virtual void renderScene();
		static void initClass() {
			// upon construction of the first object of this
			// class, we have to activate depth test automatically.
			if (! RoomWall::classInitialized) {
				RoomWall::classInitialized = true;
				glShadeModel(GL_SMOOTH);
				glEnable(GL_DEPTH_TEST);
			}
		}
};

bool RoomWall::classInitialized = false;
// this is used to render the wall a bit further away from its original limit, 
// so we do not get problems with overlapping walls. the direction member tells,
// in which direction the wall is moved.
const float RoomWall::RENDER_DISTANCE = 0.1;
// this is used to do collision control a bit further away from the wall, to not
// get "clipping errors". the direction member tells, in which the padding is added.
const float RoomWall::WALL_DISTANCE   = 1.0;

RoomWall::RoomWall(WallPlane parallelTo, float limit, float start1, float end1, float start2, float end2, WallList* wallList, bool direction):
		parallelTo(parallelTo), wallList(wallList), 
		beforeDoor(NULL), afterDoor(NULL), aboveDoor(NULL), belowDoor(NULL),
		befInDoor(NULL), aftInDoor(NULL), aboInDoor(NULL), belInDoor(NULL),
		colorR(1.0f), colorG(1.0f), colorB(1.0f), 
		start1(start1), start2(start2), end1(end1), end2(end2), limit(limit), 
		doorStart1(-1.0), doorStart2(-1.0), doorEnd1(-1.0), doorEnd2(-1.0), 
		direction(direction), textureMap((GLuint) -1), textureBump((GLuint) -1) {
	RoomWall::initClass();
	this->createWalls(); 
}

RoomWall::~RoomWall() {
	this->removeWalls();
}

/*
 * adds the adequate Wall objects to the WallList provided in the wallList member
 */

void RoomWall::createWalls() {
	this->removeWalls();
	if (this->wallList != NULL) {
		if (this->hasDoor()) {
			float start1 = this->start1; float end1 = this->end1;
			float start2 = this->start2; float end2 = this->end2;
			float doorStart1 = this->doorStart1; float doorStart2 = this->doorStart2;
			float doorEnd1   = this->doorStart1; float doorEnd2   = this->doorEnd2;
			if (this->parallelTo == WALL_YZ_PLANE)
				RoomWall::auxSwap4Floats(start1, start2, end1, end2, doorStart1, doorStart2, doorEnd1, doorEnd2);
			// the boundaries of the collision testing walls
			float beforeLimit = 0.0; float beforeStart1 = 0.0; float beforeEnd1 = 0.0; float beforeStart2 = 0.0; float beforeEnd2 = 0.0;
			float afterLimit  = 0.0; float afterStart1  = 0.0; float afterEnd1  = 0.0; float afterStart2  = 0.0; float afterEnd2  = 0.0;
			float aboveLimit  = 0.0; float aboveStart1  = 0.0; float aboveEnd1  = 0.0; float aboveStart2  = 0.0; float aboveEnd2  = 0.0;
			float belowLimit  = 0.0; float belowStart1  = 0.0; float belowEnd1  = 0.0; float belowStart2  = 0.0; float belowEnd2  = 0.0;
			float befInLimit  = 0.0; float befInStart1  = 0.0; float befInEnd1  = 0.0; float befInStart2  = 0.0; float befInEnd2  = 0.0;
			float aftInLimit  = 0.0; float aftInStart1  = 0.0; float aftInEnd1  = 0.0; float aftInStart2  = 0.0; float aftInEnd2  = 0.0;
			float aboInLimit  = 0.0; float aboInStart1  = 0.0; float aboInEnd1  = 0.0; float aboInStart2  = 0.0; float aboInEnd2  = 0.0;
			float belInLimit  = 0.0; float belInStart1  = 0.0; float belInEnd1  = 0.0; float belInStart2  = 0.0; float belInEnd2  = 0.0;
			// calculate their position and width
			beforeLimit = afterLimit = aboveLimit = belowLimit = befInStart2 = aftInStart2 = belInStart2 = aboInStart2 =
																 this->limit + (this->direction ? 1 : -1) * RoomWall::WALL_DISTANCE;
			befInEnd2 = aftInEnd2 = belInEnd2 = aboInEnd2      = this->limit - (this->direction ? 1 : -1) * RoomWall::WALL_DISTANCE;
			beforeStart1                                                                    = this->start1     + RoomWall::WALL_DISTANCE;
			beforeEnd1 = aboveStart1 = belowStart1 = befInLimit = belInStart1 = aboInStart1 = this->doorStart1 + RoomWall::WALL_DISTANCE;
			aboveEnd1 = belowEnd1 = afterStart1 = aftInLimit = belInEnd1 = aboInEnd1        = this->doorEnd1   - RoomWall::WALL_DISTANCE;
			afterEnd1                                                                       = this->end1       - RoomWall::WALL_DISTANCE;
			beforeStart2 = belowStart2 = afterStart2                                        = this->start2     + RoomWall::WALL_DISTANCE;
			belowEnd2 = belInLimit = befInStart1 = aftInStart1                              = this->doorStart2 + RoomWall::WALL_DISTANCE;
			aboveStart2 = aboInLimit = befInEnd1 = aftInEnd1                                = this->doorEnd2   - RoomWall::WALL_DISTANCE;
			beforeEnd2 = aboveEnd2 = afterEnd2                                              = this->end2       - RoomWall::WALL_DISTANCE;
			// the inner walls of the door frame have a different orientation
			// - this could yield to the fact, that the two wall dimensions (start1/start2 etc.) have to be swapped. this is done by auxSwap3Floats
			WallPlane befInPlane = this->parallelTo;
			WallPlane aftInPlane = this->parallelTo;
			WallPlane aboInPlane = this->parallelTo;
			WallPlane belInPlane = this->parallelTo;
			switch (this->parallelTo) {
				case WALL_XY_PLANE: 
					befInPlane = aftInPlane = WALL_YZ_PLANE;
					belInPlane = aboInPlane = WALL_XZ_PLANE;
				break; case WALL_YZ_PLANE:
					befInPlane = aftInPlane = WALL_XZ_PLANE;
					RoomWall::auxSwap4Floats(befInStart1, befInStart2, befInEnd1, befInEnd2, aftInStart1, aftInStart2, aftInEnd1, aftInEnd2);
					belInPlane = aboInPlane = WALL_XY_PLANE;
					RoomWall::auxSwap4Floats(belInStart1, belInStart2, belInEnd1, belInEnd2, aboInStart1, aboInStart2, aboInEnd1, aboInEnd2);
				break; case WALL_XZ_PLANE:
					befInPlane = aftInPlane = WALL_YZ_PLANE;
					RoomWall::auxSwap4Floats(befInStart1, befInStart2, befInEnd1, befInEnd2, aftInStart1, aftInStart2, aftInEnd1, aftInEnd2);
					belInPlane = aboInPlane = WALL_XY_PLANE;
			}
			// add the walls, but only if they do not have zero width/height
			if (beforeEnd1-beforeStart1 != 0 && beforeEnd2-beforeStart2 != 0)  this->beforeDoor = this->wallList->add(this->parallelTo, beforeLimit, beforeStart1, beforeEnd1, beforeStart2, beforeEnd2);
			if (afterEnd1-afterStart1 != 0 && afterEnd2-afterStart2 != 0)      this->afterDoor  = this->wallList->add(this->parallelTo, afterLimit,  afterStart1,  afterEnd1,  afterStart2,  afterEnd2);
			if (aboveEnd1-aboveStart1 != 0 && aboveEnd2-aboveStart2 != 0)      this->aboveDoor  = this->wallList->add(this->parallelTo, aboveLimit,  aboveStart1,  aboveEnd1,  aboveStart2,  aboveEnd2);
			if (belowEnd1-belowStart1 != 0 && aboveEnd2-belowStart2 != 0)      this->belowDoor  = this->wallList->add(this->parallelTo, belowLimit,  belowStart1,  belowEnd1,  belowStart2,  belowEnd2);
			if (befInEnd1-befInStart1 != 0 && befInEnd2-befInStart2 != 0)      this->befInDoor  = this->wallList->add(befInPlane,       befInLimit,  befInStart1,  befInEnd1,  befInStart2,  befInEnd2);
			if (aftInEnd1-aftInStart1 != 0 && aftInEnd2-aftInStart2 != 0)      this->aftInDoor  = this->wallList->add(aftInPlane,       aftInLimit,  aftInStart1,  aftInEnd1,  aftInStart2,  aftInEnd2);
			if (aboInEnd1-aboInStart1 != 0 && aboInEnd2-aboInStart2 != 0)      this->aboInDoor = this->wallList->add(aboInPlane,       aboInLimit,  aboInStart1,  aboInEnd1,  aboInStart2,  aboInEnd2);
			if (belInEnd1-belInStart1 != 0 && belInEnd2-belInStart2 != 0)      this->belInDoor = this->wallList->add(belInPlane,       belInLimit,  belInStart1,  belInEnd1,  belInStart2,  belInEnd2);
		} else {
			// having no doors at all is much simpler. just draw one collision detection wall
			this->beforeDoor = this->wallList->add(this->parallelTo,
					this->limit + (this->direction ? 1 : -1) * RoomWall::WALL_DISTANCE,
					this->start1+RoomWall::WALL_DISTANCE, this->end1-RoomWall::WALL_DISTANCE,
					this->start2+RoomWall::WALL_DISTANCE, this->end2-RoomWall::WALL_DISTANCE
				);
		}
	}
}


/*
 * removes all the walls from the WallList provided in the wallList member
 */

void RoomWall::removeWalls() {
	if (this->beforeDoor != NULL) { delete this->beforeDoor; this->wallList->remove(this->beforeDoor); this->beforeDoor = NULL; }
	if (this->afterDoor  != NULL) { delete this->afterDoor;  this->wallList->remove(this->afterDoor);  this->afterDoor  = NULL; }
	if (this->belowDoor  != NULL) { delete this->belowDoor;  this->wallList->remove(this->belowDoor);  this->belowDoor  = NULL; }
	if (this->aboveDoor  != NULL) { delete this->aboveDoor;  this->wallList->remove(this->aboveDoor);  this->aboveDoor  = NULL; }
	if (this->befInDoor  != NULL) { delete this->befInDoor;  this->wallList->remove(this->befInDoor);  this->befInDoor  = NULL; }
	if (this->aftInDoor  != NULL) { delete this->aftInDoor;  this->wallList->remove(this->aftInDoor);  this->aftInDoor  = NULL; }
	if (this->belInDoor  != NULL) { delete this->belInDoor;  this->wallList->remove(this->belInDoor);  this->belInDoor  = NULL; }
	if (this->aboInDoor  != NULL) { delete this->aboInDoor;  this->wallList->remove(this->aboInDoor);  this->aboInDoor  = NULL; }
}


/*
 * adds a door to the wall and creates the wall objects for
 * collision testing. giving (-1, -1, -1, -1) will remove the door.
 */

void RoomWall::setDoor(float doorStart1, float doorStart2, float doorEnd1, float doorEnd2) {
	this->removeWalls();
	this->doorStart1 = doorStart1;
	this->doorStart2 = doorStart2;
	this->doorEnd1 = doorEnd1;
	this->doorEnd2 = doorEnd2;
	this->createWalls();
}

/*
 * loads a texture (map or bump) for the wall from the file called filename. 
 */

void RoomWall::setTextureMap(const string& filename) {
	if ((signed int) this->textureMap != -1) glDeleteTextures(1, &this->textureMap);
	this->textureMap = glauxLoadTexture(filename);
}

void RoomWall::setTextureBump(const string& filename) {
	if ((signed int) this->textureBump != -1) glDeleteTextures(1, &this->textureBump);
	this->textureBump = glauxLoadTexture(filename);
	// This function works, but the bump map will not show up,
	// since bump map was not implemented in the renderScene method.
}

/*
 * sets the color of the wall. use the range [0.0,1.0].
 */

void RoomWall::setColor(float r, float g, float b) {
	this->colorR = r; this->colorG = g; this->colorB = b;
}

/*
 * returns true, if a door was added to the wall.
 */

bool RoomWall::hasDoor() {
	return (this->doorStart1 != -1 || this->doorStart2 != -1 || this->doorEnd1 != -1 || this->doorEnd2 != -1);
}

/*
 * the implemented hooks from Renderable
 */

void RoomWall::renderScene() {
	float beforeLimit = 0.0; float beforeStart1 = 0.0; float beforeEnd1 = 0.0; float beforeStart2 = 0.0; float beforeEnd2 = 0.0;
	float afterLimit  = 0.0; float afterStart1  = 0.0; float afterEnd1  = 0.0; float afterStart2  = 0.0; float afterEnd2  = 0.0;
	float aboveLimit  = 0.0; float aboveStart1  = 0.0; float aboveEnd1  = 0.0; float aboveStart2  = 0.0; float aboveEnd2  = 0.0;
	float belowLimit  = 0.0; float belowStart1  = 0.0; float belowEnd1  = 0.0; float belowStart2  = 0.0; float belowEnd2  = 0.0;
	float beforeUVStart1 = 0.0; float beforeUVEnd1 = 0.0; float beforeUVStart2 = 0.0; float beforeUVEnd2 = 0.0;
	float afterUVStart1  = 0.0; float afterUVEnd1  = 0.0; float afterUVStart2  = 0.0; float afterUVEnd2  = 0.0;
	float aboveUVStart1  = 0.0; float aboveUVEnd1  = 0.0; float aboveUVStart2  = 0.0; float aboveUVEnd2  = 0.0;
	float belowUVStart1  = 0.0; float belowUVEnd1  = 0.0; float belowUVStart2  = 0.0; float belowUVEnd2  = 0.0;
	if (this->hasDoor()) {
		// calculate position of rectangles
		beforeLimit = afterLimit = aboveLimit = belowLimit   = this->limit + (this->direction ? 1 : -1) * RoomWall::RENDER_DISTANCE;
		beforeStart1                                         = this->start1     + RoomWall::RENDER_DISTANCE;
		beforeEnd1 = aboveStart1 = belowStart1               = this->doorStart1 + RoomWall::RENDER_DISTANCE;
		aboveEnd1 = belowEnd1 = afterStart1                  = this->doorEnd1   - RoomWall::RENDER_DISTANCE;
		afterEnd1                                            = this->end1       - RoomWall::RENDER_DISTANCE;
		beforeStart2 = belowStart2 = afterStart2             = this->start2     + RoomWall::RENDER_DISTANCE;
		belowEnd2                                            = this->doorStart2 + RoomWall::RENDER_DISTANCE;
		aboveStart2                                          = this->doorEnd2   - RoomWall::RENDER_DISTANCE;
		beforeEnd2 = aboveEnd2 = afterEnd2                   = this->end2       - RoomWall::RENDER_DISTANCE;
		// calculate uv-coordinates
		beforeUVStart1 = 0.0;
		beforeUVEnd1   = (beforeEnd1-beforeStart1)  / (afterEnd1-beforeStart1);
		aboveUVStart1  = (aboveStart1-beforeStart1) / (afterEnd1-beforeStart1);
		aboveUVEnd1    = (aboveEnd1-beforeStart1)   / (afterEnd1-beforeStart1);
		belowUVStart1  = (belowStart1-beforeStart1) / (afterEnd1-beforeStart1);
		belowUVEnd1    = (belowEnd1-beforeStart1)   / (afterEnd1-beforeStart1);
		afterUVStart1  = (afterStart1-beforeStart1) / (afterEnd1-beforeStart1);
		afterUVEnd1    = 1.0;
		beforeUVStart2 = 0.0;
		beforeUVEnd2   = (beforeEnd2-beforeStart2)  / (afterEnd2-beforeStart2);
		aboveUVStart2  = (aboveStart2-beforeStart2) / (afterEnd2-beforeStart2);
		aboveUVEnd2    = (aboveEnd2-beforeStart2)   / (afterEnd2-beforeStart2);
		belowUVStart2  = (belowStart2-beforeStart2) / (afterEnd2-beforeStart2);
		belowUVEnd2    = (belowEnd2-beforeStart2)   / (afterEnd2-beforeStart2);
		afterUVStart2  = (afterStart2-beforeStart2) / (afterEnd2-beforeStart2);
		afterUVEnd2    = 1.0;
	} else {
		beforeStart1 = this->start1 + RoomWall::RENDER_DISTANCE;
		beforeStart2 = this->start2 + RoomWall::RENDER_DISTANCE;
		beforeEnd1   = this->end1   + RoomWall::RENDER_DISTANCE;
		beforeEnd2   = this->end2   + RoomWall::RENDER_DISTANCE;
		beforeLimit  = this->limit + (this->direction ? 1 : -1) * RoomWall::RENDER_DISTANCE;
		beforeUVStart1 = beforeUVStart2 = 0.0;
		beforeUVEnd1   = beforeUVEnd2   = 1.0;
	}

	// colors and materials
	float diffuseColour[] = { this->colorR, this->colorG, this->colorB, 1.0 };
	float specularColour[] = { this->colorR, this->colorG, this->colorB, 1.0 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, diffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularColour);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0);
	glColor4f(this->colorR, this->colorG, this->colorB, 1.0);
	// textures
	if ((signed int) this->textureMap != -1 || (signed int) this->textureBump != -1) {
		glEnable(GL_TEXTURE_2D);
		if ((signed int) this->textureMap != -1) {
			glBindTexture(GL_TEXTURE_2D, this->textureMap);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}
		if ((signed int) this->textureBump != -1) {
			// Bump maps not implemented yet.
		}
	}

	switch (this->parallelTo) {
		case WALL_XY_PLANE: glNormal3f(0.0, 0.0, 1.0); break;
		case WALL_XZ_PLANE: glNormal3f(0.0, 1.0, 0.0); break;
		case WALL_YZ_PLANE: glNormal3f(1.0, 0.0, 0.0); break;
	}

	// draw points
	RoomWall::auxRectangle(this->parallelTo, beforeLimit,
						   beforeStart1, beforeStart2, beforeEnd1, beforeEnd2, 
						   beforeUVStart1, beforeUVStart2, beforeUVEnd1, beforeUVEnd2);
	if (this->hasDoor()) {
		RoomWall::auxRectangle(this->parallelTo, afterLimit, 
							   afterStart1, afterStart2, afterEnd1, afterEnd2,
							   afterUVStart1, afterUVStart2, afterUVEnd1, afterUVEnd2);
		RoomWall::auxRectangle(this->parallelTo, aboveLimit, 
							   aboveStart1, aboveStart2, aboveEnd1, aboveEnd2, 
							   aboveUVStart1, aboveUVStart2, aboveUVEnd1, aboveUVEnd2);
		RoomWall::auxRectangle(this->parallelTo, belowLimit,
							   belowStart1, belowStart2, belowEnd1, belowEnd2,
							   belowUVStart1, belowUVStart2, belowUVEnd1, belowUVEnd2);
	}

	// end of texture
	if ((signed int) this->textureMap != -1 || (signed int) this->textureBump != -1) {
		glDisable(GL_TEXTURE_2D);
	}
}


/**********************************************************
 *                  class RoomLight
 * represents a light in openGL with a position, a diffuse
 * and specular color and linear/quadratic attenuation.
 * the number of lights is limited, since upon construction
 * the openGL-light-identifier (GL_LIGHT0 .. GL_LIGHT7) has
 * to be given, which is used to maintain the light.
 **********************************************************/

class RoomLight : public Renderable {
	private: 
		static bool classInitialized;
		const GLuint lightId;
		float positionX, positionY, positionZ;
		float diffuseR,  diffuseG,  diffuseB;
		float specularR, specularG, specularB;
		float attenuationLinear, attenuationQuadratic;
	public:
		RoomLight(GLuint lightId);
		void setPosition(float x, float y, float z);
		void setDiffuseColor(float r, float g, float b);
		void setSpecularColor(float r, float g, float b);
		void setColors(float r, float g, float b);
		void setAttenuation(float linear, float quadratic);
		virtual void initScene();
		static void classInitialize() {
			// upon creation of the first light, we have
			// to enable lightning
			if (!RoomLight::classInitialized) {
				RoomLight::classInitialized = true;
				glEnable(GL_LIGHTING);
			}
		};
};

bool RoomLight::classInitialized = false;

RoomLight::RoomLight(GLuint lightId): 
			lightId(lightId), 
			positionX(0.0), positionY(0.0), positionZ(0.0),
			diffuseR(1.0), diffuseG(1.0), diffuseB(1.0),
			specularR(1.0), specularG(1.0), specularB(1.0),
			attenuationLinear(0.001), attenuationQuadratic(0.001) {
	RoomLight::classInitialize();
}

/*
 * setters for position, diffuse and specular color (setColors sets
 * both colors to the same value) and attenuation
 */

void RoomLight::setPosition(float x, float y, float z) {
	this->positionX = x; this->positionY = y; this->positionZ = z;
}

void RoomLight::setDiffuseColor(float r, float g, float b) {
	this->diffuseR = r; this->diffuseG = g; this->diffuseB = b;
}

void RoomLight::setSpecularColor(float r, float g, float b) {
	this->specularR = r; this->specularG = g; this->specularB = b;
}

void RoomLight::setColors(float r, float g, float b) {
	this->setDiffuseColor(r, g, b);
	this->setSpecularColor(r, g, b);
}

void RoomLight::setAttenuation(float linear, float quadratic) {
	this->attenuationLinear = linear;
	this->attenuationQuadratic = quadratic;
}

/*
 * the implemented hooks of Renderable
 */

void RoomLight::initScene() {
	glEnable(this->lightId);
	float position[3] = { this->positionX, this->positionY, this->positionZ };
	float diffuse[3]  = { this->diffuseR,  this->diffuseG,  this->diffuseB  };
	float specular[3] = { this->specularR, this->specularG, this->specularB };
	glLightfv(this->lightId, GL_POSITION, position);
	glLightfv(this->lightId, GL_DIFFUSE,  diffuse);
    glLightfv(this->lightId, GL_SPECULAR, specular);
    glLightf(this->lightId, GL_LINEAR_ATTENUATION,    this->attenuationLinear);
    glLightf(this->lightId, GL_QUADRATIC_ATTENUATION, this->attenuationQuadratic);
}


/**********************************************************
 *                     class Room
 * a room is a Renderable consisting of 6 walls (left, 
 * right, top, bottom, front, back) and a RenderableList
 * of Light objects. The room itself has 6 coordinates
 * (like a Box, both min/max of X/Y/Z). Room provides
 * methods to access the members from each of the 6 walls 
 * and methods to add and remove lights to the room
 * Room will init/render/dispose all the walls and lights
 **********************************************************/

class Room : public Renderable {
	private:
		RenderableList lights;
		RoomWall leftWall;
		RoomWall rightWall;
		RoomWall frontWall;
		RoomWall backWall;
		RoomWall topWall;
		RoomWall bottomWall;
	public:
		Room(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, WallList* wallList);
		void addLight(RoomLight* light);
		void removeLight(RoomLight* light);
		RoomWall* getLeftWall();
		RoomWall* getRightWall();
		RoomWall* getFrontWall();
		RoomWall* getBackWall();
		RoomWall* getTopWall();
		RoomWall* getBottomWall();
		virtual void initScene();
		virtual void renderScene();
		virtual void disposeScene();
};

Room::Room(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, WallList* wallList):
		  leftWall(WALL_YZ_PLANE, minX, minY, maxY, minZ, maxZ, wallList, true),
		 rightWall(WALL_YZ_PLANE, maxX, minY, maxY, minZ, maxZ, wallList, false),
		 frontWall(WALL_XY_PLANE, maxZ, minX, maxX, minY, maxY, wallList, false),
		  backWall(WALL_XY_PLANE, minZ, minX, maxX, minY, maxY, wallList, true),
		   topWall(WALL_XZ_PLANE, maxY, minX, maxX, minZ, maxZ, wallList, false),
		bottomWall(WALL_XZ_PLANE, minY, minX, maxX, minZ, maxZ, wallList, true) {}

/*
 * getters for the 6 walls to access their properties
 */

RoomWall* Room::getLeftWall()   { return &this->leftWall; }
RoomWall* Room::getRightWall()  { return &this->rightWall; }
RoomWall* Room::getTopWall()    { return &this->topWall; }
RoomWall* Room::getBottomWall() { return &this->bottomWall; }
RoomWall* Room::getFrontWall()  { return &this->frontWall; }
RoomWall* Room::getBackWall()   { return &this->backWall; }

/*
 * add and remove light to the RenderableList for the lights
 */

void Room::addLight(RoomLight* light) {
	this->lights.add(light);
}

void Room::removeLight(RoomLight* light) {
	this->lights.remove(light);
}

/*
 * the implementation of the Renderable-hooks
 */

void Room::initScene() {
	// init all the walls and all the lights
	this->topWall.initScene();
	this->frontWall.initScene();
	this->leftWall.initScene();
	this->backWall.initScene();
	this->rightWall.initScene();
	this->bottomWall.initScene();
	this->lights.initScene();
}

void Room::renderScene() {
	// render all the walls and all the lights
	this->topWall.renderScene();
	this->frontWall.renderScene();
	this->leftWall.renderScene();
	this->backWall.renderScene();
	this->rightWall.renderScene();
	this->bottomWall.renderScene();
	this->lights.initScene();
}

void Room::disposeScene() {
	// dispose all the walls and all the lights
	this->topWall.disposeScene();
	this->frontWall.disposeScene();
	this->leftWall.disposeScene();
	this->backWall.disposeScene();
	this->rightWall.disposeScene();
	this->bottomWall.disposeScene();
	this->lights.disposeScene();
}


/***************************************************************************************
 ************      CAMERA                                                   ************
 ***************************************************************************************/


/**********************************************************
 *                   class Camera
 * the camera object holds the properties of the
 * scene-Camera, being the camera position (eye), the 
 * look-at-point (focus) and the up-vector (up). 
 * there are methods to move and turn the camera and methods
 * to activate auto-moving/-turning by a specific step-
 * value (delta). If using auto-moving/-turning, the method
 * step() must be called everytime a step has to be done.
 * using the method updateWindow() one can set the openGL-
 * options according to the current camera parameters.
 * The camera can be connected to a WallList for automatic
 * collision testing.
 **********************************************************/

class Camera {
	private:
		Point eye;
		Point focus;
		Point up;
		float deltaMoveForward;
		float deltaMoveUp;
		float deltaMoveLeft;
		float deltaTurnUp;
		float deltaTurnLeft;
		WallList* wallList;
		Point getNormalizedForward();
		Point getNormalizedUp();
		Point getNormalizedLeft();
		static Point rotateAroundVector(Point axis, Point vector, float angle) {
			Point re = vector;
			re.x =    ( cos(angle) + pow(axis.x,2) * (1-cos(angle))        ) * vector.x
					+ ( axis.x*axis.y*(1-cos(angle)) - axis.z * sin(angle) ) * vector.y
					+ ( axis.x*axis.z*(1-cos(angle)) + axis.y * sin(angle) ) * vector.z;
			re.y =    ( axis.y*axis.x*(1-cos(angle)) + axis.z * sin(angle) ) * vector.x
					+ ( cos(angle) + pow(axis.y,2) * (1-cos(angle))        ) * vector.y
					+ ( axis.y*axis.z*(1-cos(angle)) - axis.x * sin(angle) ) * vector.z;
			re.z =    ( axis.z*axis.x*(1-cos(angle)) - axis.y * sin(angle) ) * vector.x
					+ ( axis.z*axis.y*(1-cos(angle)) + axis.x * sin(angle) ) * vector.y
					+ ( cos(angle) + pow(axis.z,2) * (1-cos(angle))        ) * vector.z;
			return re;
		};
	public:
		Camera();
		Camera(Point eye, Point focus, Point up);
		void set(Point eye, Point focus, Point up);
		void moveForward(float distance);
		void moveForwardAuto(float distance);
		void moveUp(float distance);
		void moveUpAuto(float distance);
		void moveLeft(float distance);
		void moveLeftAuto(float distance);
		void turnUp(float angle);
		void turnUpAuto(float angle);
		void turnLeft(float angle);
		void turnLeftAuto(float angle);
		void setWallList(WallList* wallList);
		bool step();
		void updateWindow();
		Point getEye();
		Point getFocus();
		Point getUp();
		WallList* getWallList();
};

Camera::Camera(): wallList(NULL) {}

Camera::Camera(Point eye, Point focus, Point up):
	eye(eye), focus(focus), up(up), wallList(NULL) {}

/*
 * get the normalized forward vector. the forward vector
 * is the vector between the camera position (eye) and the
 * look-at-point (focus)
 */

Point Camera::getNormalizedForward() {
	Point re;
	re.x = (this->focus.x - this->eye.x);
	re.y = (this->focus.y - this->eye.y);
	re.z = (this->focus.z - this->eye.z);
	float norm = sqrt(pow(re.x,2) + pow(re.y,2) + pow(re.z,2));
	re.x /= norm; re.y /= norm; re.z /= norm;
	return re;
}

/*
 * get the normalized up vector
 */

Point Camera::getNormalizedUp() {
	Point re = this->up;
	float norm = sqrt(pow(re.x,2) + pow(re.y,2) + pow(re.z,2));
	re.x /= norm; re.y /= norm; re.z /= norm;
	return re;
}

/*
 * get the normalized left vector, which can be calculated using
 * the cross product between the up and the forward vector
 */

Point Camera::getNormalizedLeft() {
	Point forward = this->getNormalizedForward();
	Point up = this->getNormalizedUp();
	// left is cross product of up and forward
	Point re;
	re.x = up.y * forward.z - up.z * forward.y;
	re.y = up.z * forward.x - up.x * forward.z;
	re.z = up.x * forward.y - up.y * forward.x;
	// normalize
	float norm = sqrt(pow(re.x,2) + pow(re.y,2) + pow(re.z,2));
	re.x /= norm; re.y /= norm; re.z /= norm;
	return re;
}

/*
 * set eye, focus and up of the camera
 */

void Camera::set(Point eye, Point focus, Point up) {
	this->eye = eye;
	this->focus = focus;
	this->up = up;
}

/*
 * move the camera distance units into the direction of the forward vector.
 */

void Camera::moveForward(float distance) {
	// moving forward means doing "distance" steps into forward direction vector.
	Point forward = this->getNormalizedForward();
	Point neweye, newfocus;
	neweye.x = this->eye.x + distance * forward.x; newfocus.x = this->focus.x + distance * forward.x;
	neweye.y = this->eye.y + distance * forward.y; newfocus.y = this->focus.y + distance * forward.y;
	neweye.z = this->eye.z + distance * forward.z; newfocus.z = this->focus.z + distance * forward.z;
	// test if wall is in the way
	if (this->wallList == NULL || this->wallList->canIMove(focus, newfocus)) {
		this->eye = neweye; this->focus = newfocus;
	}
}

/*
 * move the camera distance units into the direction of the up vector.
 */

void Camera::moveUp(float distance) {
	// moving up means doing "distance" steps into direction of up.
	Point up = this->getNormalizedUp();
	Point neweye, newfocus;
	neweye.x = this->eye.x + distance * up.x; newfocus.x = this->focus.x + distance * up.x;
	neweye.y = this->eye.y + distance * up.y; newfocus.y = this->focus.y + distance * up.y;
	neweye.z = this->eye.z + distance * up.z; newfocus.z = this->focus.z + distance * up.z;
	// test if wall is in the way
	if (this->wallList == NULL || this->wallList->canIMove(focus, newfocus)) {
		this->eye = neweye; this->focus = newfocus;
	}
}

/*
 * move the camera distance units into the direction of the left vector.
 */

void Camera::moveLeft(float distance) {
	// moving left means doing "distance" steps into Left directiovector.
	Point left = this->getNormalizedLeft();
	Point neweye, newfocus;
	neweye.x = this->eye.x + distance * left.x; newfocus.x = this->focus.x + distance * left.x;
	neweye.y = this->eye.y + distance * left.y; newfocus.y = this->focus.y + distance * left.y;
	neweye.z = this->eye.z + distance * left.z; newfocus.z = this->focus.z + distance * left.z;
	// test if wall is in the way
	if (this->wallList == NULL || this->wallList->canIMove(focus, newfocus)) {
		this->eye = neweye; this->focus = newfocus;
	}
}

/*
 * turn the camera by angle radians around the up vector to the left
 */

void Camera::turnLeft(float angle) {
	// rotate around up-vector - this only changes the forward-vector.
	// (left vector is also changed, but we do not save this vector)
	Point newforward = Camera::rotateAroundVector(this->getNormalizedUp(), this->getNormalizedForward(), angle);
	Point newfocus;
	newfocus.x = this->eye.x + newforward.x;
	newfocus.y = this->eye.y + newforward.y;
	newfocus.z = this->eye.z + newforward.z;
	// test if wall is in the way
	if (this->wallList == NULL || this->wallList->canIMove(focus, newfocus))
		this->focus = newfocus;
}

/*
 * turn the camera up by angle radians around the left vector 
 */

void Camera::turnUp(float angle) {
	// rotating up (around left-vector) changes both up and forward vector
	Point left = this->getNormalizedLeft();
	Point newforward = Camera::rotateAroundVector(left, this->getNormalizedForward(), -angle);
	Point newup      = Camera::rotateAroundVector(left, this->getNormalizedUp(),      -angle);
	Point newfocus;
	newfocus.x = this->eye.x + newforward.x;
	newfocus.y = this->eye.y + newforward.y;
	newfocus.z = this->eye.z + newforward.z;
	// test if wall is in the way
	if (this->wallList == NULL || this->wallList->canIMove(focus, newfocus)) {
		this->focus = newfocus;
		this->up = newup;
	}
}

/*
 * enables auto-moving or -turning. give 0.0 to deactivate it.
 * for every step of auto-moving/-turning, you have to call the step() function.
 */

void Camera::moveForwardAuto(float distance) {
	this->deltaMoveForward = distance;
}

void Camera::moveLeftAuto(float distance) {
	this->deltaMoveLeft = distance;
}

void Camera::moveUpAuto(float distance) {
	this->deltaMoveUp = distance;
}

void Camera::turnLeftAuto(float angle) {
	this->deltaTurnLeft = angle;
}

void Camera::turnUpAuto(float angle) {
	this->deltaTurnUp = angle;
}

/*
 * does one step of every delta value, which is set when activating auto-moving or -turning
 * will return true, if auto-moving or -turning was activated.
 */

bool Camera::step() {
	if (this->deltaMoveForward || this->deltaMoveLeft || this->deltaMoveUp ||
			this->deltaTurnLeft ||	this->deltaTurnUp) {
		if (this->deltaMoveForward) this->moveForward(this->deltaMoveForward);
		if (this->deltaMoveLeft)    this->moveLeft(this->deltaMoveLeft);
		if (this->deltaMoveUp)      this->moveUp(this->deltaMoveUp);
		if (this->deltaTurnLeft)    this->turnLeft(this->deltaTurnLeft);
		if (this->deltaTurnUp)      this->turnUp(this->deltaTurnUp);
		return true;
	} else {
		return false;
	}
}

/*
 * sets the openGL-view matrix to the current camera properties
 */

void Camera::updateWindow() {
	glLoadIdentity();
	gluLookAt(this->eye.x,   this->eye.y,   this->eye.z, 
		      this->focus.x, this->focus.y, this->focus.z,
			  this->up.x,    this->up.y,    this->up.z);
}

/*
 * setter for the WallList and getter for the camera properties
 */

void Camera::setWallList(WallList* wallList) {
	this->wallList = wallList;
}

Point Camera::getEye() {
	return this->eye;
}

Point Camera::getFocus() {
	return this->focus;
}

Point Camera::getUp() {
	return this->up;
}

WallList* Camera::getWallList() {
	return this->wallList; 
}


/***************************************************************************************
 ************      OBJECTS                                                  ************
 ***************************************************************************************/


/**********************************************************
 *                     class Object
 * a Object is a Renderable which can be transformed, 
 * scaled and rotated. For this reason, we have to override
 * the renderScene() function. All sub-classes of Object
 * will implement the hook drawObject() instead. drawObject
 * is purely virtual, hence Object is an abstract class.
 **********************************************************/

class Object : public Renderable {
	private:
		float translateX, translateY, translateZ;
		float scaleX, scaleY, scaleZ;
		float rotateOmega, rotatePhi, rotateKappa;
		static bool classInitialized;
	public: 
		Object();
		void setTranslate(float x, float y, float z);
		void setScale(float x, float y, float z);
		void setRotate(float omega, float phi, float kappa);
		virtual void drawObject() = 0;
		virtual void renderScene();
		static void initializeClass() {
			// initialize depth test if we create the first Object
			if (!Object::classInitialized) {
				glEnable(GL_DEPTH_TEST);
			}
		}
};

bool Object::classInitialized = false;

Object::Object():
		translateX(0.0), translateY(0.0), translateZ(0.0),
		scaleX(1.0), scaleY(1.0), scaleZ(1.0),
		rotateOmega(0.0), rotatePhi(0.0), rotateKappa(0.0) {
	Object::initializeClass();
}

/*
 * set the transform properties
 */

void Object::setTranslate(float x, float y, float z) {
	this->translateX = x;
	this->translateY = y;
	this->translateZ = z;
}

void Object::setScale(float x, float y, float z) {
	this->scaleX = x;
	this->scaleY = y;
	this->scaleZ = z;
}

void Object::setRotate(float omega, float phi, float kappa) {
	this->rotateOmega = omega;
	this->rotatePhi = phi;
	this->rotateKappa = kappa;
}

/*
 * the implemented Renderable hooks.
 */

void Object::renderScene() {
	glPushMatrix();
		glScalef(this->scaleX, this->scaleY, this->scaleZ);
		glRotatef(this->rotateOmega, 1.0, 0.0, 0.0);
		glRotatef(this->rotatePhi,   0.0, 1.0, 0.0);
		glRotatef(this->rotateKappa, 0.0, 0.0, 1.0);
		glTranslatef(this->translateX, this->translateY, this->translateZ);
		this->drawObject();
	glPopMatrix();
}


/**********************************************************
 *                   class TableObject
 * TableObject is a table with a transparent plate, 
 * and a foot, consisting of a ring below the plate and a
 * number of sticks.
 **********************************************************/

class TableObject: public Object {
	private:
		static bool classInitialized;
		static const float TABLE_PLATE_THICKNESS;
		static const float TABLE_PLATE_RADIUS;
		static const float TABLE_PLATE_HEIGHT;
		static const float TABLE_FOOT_RADIUS;
		static const float TABLE_FOOT_WIDTH;
		static const float TABLE_FOOT_HEIGHT;
		static const int TABLE_FOOT_STICKS = 5;
	public:
		TableObject();
		virtual void drawObject();
		static void classInitialize() {
			if (!TableObject::classInitialized) {
				TableObject::classInitialized = true;
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
				glEnable(GL_BLEND);
			}
		}
};

bool TableObject::classInitialized = false;
const float TableObject::TABLE_PLATE_THICKNESS = 0.1;
const float TableObject::TABLE_PLATE_RADIUS = 3.0;
const float TableObject::TABLE_PLATE_HEIGHT = 1.0;
const float TableObject::TABLE_FOOT_WIDTH = 0.1;
const float TableObject::TABLE_FOOT_RADIUS = 2.5;
const float TableObject::TABLE_FOOT_HEIGHT = 0.8;

TableObject::TableObject(): Object() {
	TableObject::classInitialize();
}

void TableObject::drawObject() {
	// foot
	GLUquadric* foot = gluNewQuadric();
	float footDiffuseColour[4] = { 0.2, 0.2, 0.0, 1.0 };
	float footSpecularColour[4] = { 0.4, 0.4, 0.1, 1.0 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, footDiffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, footSpecularColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, footSpecularColour);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.2);
	glPushMatrix();
		glRotatef(90, 1.0, 0.0, 0.0);
		glTranslatef(0.0, 0.0, -TableObject::TABLE_FOOT_HEIGHT);
		gluQuadricDrawStyle(foot, GLU_FILL);
		gluQuadricOrientation(foot, GLU_OUTSIDE);
		gluQuadricNormals(foot, GLU_SMOOTH);
		// ring outside
		gluCylinder(foot, TableObject::TABLE_FOOT_RADIUS, TableObject::TABLE_FOOT_RADIUS,
			TableObject::TABLE_PLATE_HEIGHT-TableObject::TABLE_FOOT_HEIGHT, 36, 2);
		// ring inside
		gluCylinder(foot, TableObject::TABLE_FOOT_RADIUS-TableObject::TABLE_FOOT_WIDTH, TableObject::TABLE_FOOT_RADIUS-TableObject::TABLE_FOOT_WIDTH,
			TableObject::TABLE_PLATE_HEIGHT-TableObject::TABLE_FOOT_HEIGHT, 36, 2);
		// ring upper cap 
		glTranslatef(0.0, 0.0, +(TableObject::TABLE_PLATE_HEIGHT-TableObject::TABLE_FOOT_HEIGHT));
		gluDisk(foot, TableObject::TABLE_FOOT_RADIUS-TableObject::TABLE_FOOT_WIDTH, TableObject::TABLE_FOOT_RADIUS, 36, 3); 
		// ring lower cap
		glTranslatef(0.0, 0.0, -(TableObject::TABLE_PLATE_HEIGHT-TableObject::TABLE_FOOT_HEIGHT));
		gluDisk(foot, TableObject::TABLE_FOOT_RADIUS-TableObject::TABLE_FOOT_WIDTH, TableObject::TABLE_FOOT_RADIUS, 36, 3); 
		// sticks
		glTranslatef(0.0, 0.0, +TableObject::TABLE_FOOT_HEIGHT-(TableObject::TABLE_FOOT_HEIGHT)/2-(TableObject::TABLE_PLATE_HEIGHT-TableObject::TABLE_FOOT_HEIGHT));
		for (int i = 0; i < TableObject::TABLE_FOOT_STICKS; i++) {
			float centerX = cos(2*i*3.1415/TableObject::TABLE_FOOT_STICKS);
			float centerY = sin(2*i*3.1415/TableObject::TABLE_FOOT_STICKS);
			glTranslatef(centerX*(TableObject::TABLE_FOOT_RADIUS-TableObject::TABLE_FOOT_WIDTH/2),
						 centerY*(TableObject::TABLE_FOOT_RADIUS-TableObject::TABLE_FOOT_WIDTH/2),
						 0.0);
			gluCylinder(foot, TableObject::TABLE_FOOT_WIDTH, TableObject::TABLE_FOOT_WIDTH,
				TableObject::TABLE_FOOT_HEIGHT, 10, 10);
			glTranslatef(-centerX*(TableObject::TABLE_FOOT_RADIUS-TableObject::TABLE_FOOT_WIDTH/2),
						 -centerY*(TableObject::TABLE_FOOT_RADIUS-TableObject::TABLE_FOOT_WIDTH/2),
						 0.0);
		}
	glPopMatrix();
	// Plate
	GLUquadric* plate = gluNewQuadric();
	float plateDiffuseColour[4] = { 1.0, 1.0, 1.0, 0.5 };
	float plateSpecularColour[4] = { 1.0, 1.0, 1.0, 1.0 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, plateDiffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, plateDiffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, plateSpecularColour);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0);
	glPushMatrix();
		glRotatef(90, 1.0, 0.0, 0.0);
		glTranslatef(0.0, 0.0, -TableObject::TABLE_PLATE_HEIGHT+TableObject::TABLE_PLATE_THICKNESS);
		gluQuadricDrawStyle(plate, GLU_FILL);
		gluQuadricOrientation(plate, GLU_OUTSIDE);
		gluQuadricNormals(plate, GLU_SMOOTH);
		// outline
		gluCylinder(plate, TableObject::TABLE_PLATE_RADIUS, TableObject::TABLE_PLATE_RADIUS,
			TableObject::TABLE_PLATE_THICKNESS, 36, 2);
		// upper cap
		glTranslatef(0.0, 0.0, TableObject::TABLE_PLATE_THICKNESS);
		gluDisk(plate, 0.0, TableObject::TABLE_PLATE_RADIUS, 36, 3); 
		// lower cap
		glTranslatef(0.0, 0.0, -TableObject::TABLE_PLATE_THICKNESS);
		gluDisk(plate, 0.0, TableObject::TABLE_PLATE_RADIUS, 36, 3);
	glPopMatrix();
	gluDeleteQuadric(plate);
}


/**********************************************************
 *                   class LampObject
 * LampObject is a lamp with a bulb in the center, a 
 * shield above it and a rope on top.
 **********************************************************/

class LampObject: public Object {
	private:
		static const float LAMP_BULB_RADIUS;
		static const float LAMP_SHIELD_RADIUS;
		static const float LAMP_SHIELD_HEIGHT;
		static const float LAMP_ROPE_HEIGHT;
		static const float LAMP_ROPE_RADIUS;
	public:
		virtual void drawObject();
};

const float LampObject::LAMP_BULB_RADIUS = 0.5;
const float LampObject::LAMP_SHIELD_RADIUS = 1.0;
const float LampObject::LAMP_SHIELD_HEIGHT = 0.5;
const float LampObject::LAMP_ROPE_HEIGHT = 1.8;
const float LampObject::LAMP_ROPE_RADIUS = 0.2;

void LampObject::drawObject() {
	// bulk
	GLUquadric* bulk = gluNewQuadric();
	float bulkDiffuseColour[4] = { 1.0, 1.0, 1.0, 1.0 };
	float bulkSpecularColour[4] = { 1.0, 1.0, 1.0, 1.0 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, bulkDiffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, bulkDiffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, bulkSpecularColour);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0);
	glPushMatrix();
		glRotatef(90, 1.0, 0.0, 0.0);
		gluQuadricDrawStyle(bulk, GLU_FILL);
		gluQuadricOrientation(bulk, GLU_OUTSIDE);
		gluQuadricNormals(bulk, GLU_SMOOTH);
		// sphere
		glTranslatef(0.0, 0.0, -LampObject::LAMP_BULB_RADIUS);
		gluSphere(bulk, LampObject::LAMP_BULB_RADIUS, 10, 10);
		glTranslatef(0.0, 0.0, +LampObject::LAMP_BULB_RADIUS);
		// cone
		glTranslatef(0.0, 0.0, -LampObject::LAMP_BULB_RADIUS*2);
		gluCylinder(bulk, LampObject::LAMP_BULB_RADIUS/10, LampObject::LAMP_BULB_RADIUS, LampObject::LAMP_BULB_RADIUS, 10, 10);
		glTranslatef(0.0, 0.0, +LampObject::LAMP_BULB_RADIUS*2);
	glPopMatrix();
	// shield
	GLUquadric* shield = gluNewQuadric();
	float shieldDiffuseColour[4] = { 0.8, 0.2, 0.2, 1.0 };
	float shieldSpecularColour[4] = { 0.8, 0.2, 0.2, 1.0 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, shieldDiffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, shieldDiffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, shieldSpecularColour);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 20.0);
	glPushMatrix();
		gluQuadricDrawStyle(shield, GLU_FILL);
		gluQuadricOrientation(shield, GLU_OUTSIDE);
		gluQuadricNormals(shield, GLU_SMOOTH);
		glRotatef(90, 1.0, 0.0, 0.0);
		glTranslatef(0.0, 0.0, -LampObject::LAMP_BULB_RADIUS*2.8+LampObject::LAMP_SHIELD_HEIGHT/2);
		gluCylinder(bulk, 0.0, LampObject::LAMP_SHIELD_RADIUS, LampObject::LAMP_SHIELD_HEIGHT, 10, 10);
	glPopMatrix();
	// rope
	GLUquadric* rope = gluNewQuadric();
	float ropeDiffuseColour[4] = { 0.2, 0.2, 0.2, 1.0 };
	float ropeSpecularColour[4] = { 0.2, 0.2, 0.2, 1.0 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ropeDiffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, ropeDiffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, ropeSpecularColour);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.1);
	glPushMatrix();
		gluQuadricDrawStyle(rope, GLU_FILL);
		gluQuadricOrientation(rope, GLU_OUTSIDE);
		gluQuadricNormals(rope, GLU_SMOOTH);
		glRotatef(90, 1.0, 0.0, 0.0);
		glTranslatef(0.0, 0.0, -LampObject::LAMP_BULB_RADIUS*2.8-LampObject::LAMP_SHIELD_HEIGHT-LampObject::LAMP_ROPE_HEIGHT/2);
		gluCylinder(rope, LampObject::LAMP_ROPE_RADIUS, LampObject::LAMP_ROPE_RADIUS, LampObject::LAMP_ROPE_HEIGHT, 10, 10);
	glPopMatrix();
}


/**********************************************************
 *                   class WellObject
 * WellObject is a well with a tube and a set of stakes,
 * one stake left, one right, on top of both stakes every
 * having a ring, holding the top-stake, which contains
 * a crank on the right. in the middle of the top-stake,
 * there is a rope.
 **********************************************************/

class WellObject: public Object {
	private:
		static const float WELL_TUBE_INNRADIUS;
		static const float WELL_TUBE_OUTRADIUS;
		static const float WELL_TUBE_HEIGHT;
		static const float WELL_STAKES_HEIGHT;
		static const float WELL_STAKES_RADIUS;
		static const float WELL_CRANK_HEIGHT;
		static const float WELL_CRANK_WIDTH;
		static const float WELL_ROPE_RADIUS;
	public:
		virtual void drawObject();
};

const float WellObject::WELL_TUBE_INNRADIUS = 2.0;
const float WellObject::WELL_TUBE_OUTRADIUS = 3.0;
const float WellObject::WELL_TUBE_HEIGHT = 2.0;
const float WellObject::WELL_STAKES_HEIGHT = 1.0;
const float WellObject::WELL_STAKES_RADIUS = 0.1;
const float WellObject::WELL_CRANK_HEIGHT = 1.0;
const float WellObject::WELL_CRANK_WIDTH = 0.5;
const float WellObject::WELL_ROPE_RADIUS = 0.05;

void WellObject::drawObject() {
	// tube
	GLUquadric* tube = gluNewQuadric();
	float tubeDiffuseColour[4] = { 0.1, 0.2, 0.4, 1.0 };
	float tubeSpecularColour[4] = { 0.3, 0.6, 0.9, 1.0 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, tubeDiffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, tubeDiffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tubeSpecularColour);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 20.0);
	glPushMatrix();
		gluQuadricDrawStyle(tube, GLU_FILL);
		gluQuadricOrientation(tube, GLU_OUTSIDE);
		gluQuadricNormals(tube, GLU_SMOOTH);
		glRotatef(90, 1.0, 0.0, 0.0);
		glTranslatef(0.0, 0.0, -WellObject::WELL_TUBE_HEIGHT);
		gluCylinder(tube, WellObject::WELL_TUBE_INNRADIUS, WellObject::WELL_TUBE_INNRADIUS, WellObject::WELL_TUBE_HEIGHT, 36, 10);
		gluCylinder(tube, WellObject::WELL_TUBE_OUTRADIUS, WellObject::WELL_TUBE_OUTRADIUS, WellObject::WELL_TUBE_HEIGHT, 36, 10);
		gluDisk(tube, WellObject::WELL_TUBE_INNRADIUS, WellObject::WELL_TUBE_OUTRADIUS, 36, 10);
		glTranslatef(0.0, 0.0, +WellObject::WELL_TUBE_HEIGHT-0.2);
		gluDisk(tube, 0.0, WellObject::WELL_TUBE_INNRADIUS, 36, 10);
	glPopMatrix();
	// stakes
	GLUquadric* stakes = gluNewQuadric();
	float stakesDiffuseColour[4] = { 0.6, 0.4, 0.2, 1.0 };
	float stakesSpecularColour[4] = { 0.9, 0.6, 0.3, 1.0 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, stakesDiffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, stakesDiffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, stakesSpecularColour);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0);
	glPushMatrix();
		gluQuadricDrawStyle(stakes, GLU_FILL);
		gluQuadricOrientation(stakes, GLU_OUTSIDE);
		gluQuadricNormals(stakes, GLU_SMOOTH);
		glRotatef(90, 1.0, 0.0, 0.0);
		glTranslatef(0.0, 0.0, -WellObject::WELL_TUBE_HEIGHT-WellObject::WELL_STAKES_HEIGHT);
		// right stake
		glTranslatef(  -(WellObject::WELL_TUBE_INNRADIUS+(WellObject::WELL_TUBE_OUTRADIUS-WellObject::WELL_TUBE_INNRADIUS)/2), 0.0, 0.0);
			gluCylinder(stakes, WellObject::WELL_STAKES_RADIUS, WellObject::WELL_STAKES_RADIUS, WellObject::WELL_STAKES_HEIGHT, 10, 10);
			// right stake: clamp
			glTranslatef(  0.0, 0.0, -WellObject::WELL_STAKES_RADIUS); glRotatef(+90, 0.0, 1.0, 0.0);
				glTranslatef(0.0, 0.0,  -WellObject::WELL_STAKES_RADIUS);
					gluCylinder(stakes, 2*WellObject::WELL_STAKES_RADIUS, 2*WellObject::WELL_STAKES_RADIUS, 2*WellObject::WELL_STAKES_RADIUS, 36, 10);
					gluDisk(stakes, 0.0, 2*WellObject::WELL_STAKES_RADIUS, 36, 10);
				glTranslatef(0.0, 0.0, +2*WellObject::WELL_STAKES_RADIUS);
					gluDisk(stakes, 0.0, 2*WellObject::WELL_STAKES_RADIUS, 36, 10);
				glTranslatef(0.0, 0.0,  -WellObject::WELL_STAKES_RADIUS);
			glRotatef(-90, 0.0, 1.0, 0.0); glTranslatef(  0.0, 0.0, +WellObject::WELL_STAKES_RADIUS); 
		// left stake
		glTranslatef(+2*(WellObject::WELL_TUBE_INNRADIUS+(WellObject::WELL_TUBE_OUTRADIUS-WellObject::WELL_TUBE_INNRADIUS)/2), 0.0, 0.0);
			gluCylinder(stakes, WellObject::WELL_STAKES_RADIUS, WellObject::WELL_STAKES_RADIUS, WellObject::WELL_STAKES_HEIGHT, 10, 10);
			// left stake: clamp
			glTranslatef(  0.0, 0.0, -WellObject::WELL_STAKES_RADIUS); glRotatef(+90, 0.0, 1.0, 0.0);
				glTranslatef(0.0, 0.0,  -WellObject::WELL_STAKES_RADIUS);
					gluCylinder(stakes, 2*WellObject::WELL_STAKES_RADIUS, 2*WellObject::WELL_STAKES_RADIUS, 2*WellObject::WELL_STAKES_RADIUS, 36, 10);
					gluDisk(stakes, 0.0, 2*WellObject::WELL_STAKES_RADIUS, 36, 10);
				glTranslatef(0.0, 0.0, +2*WellObject::WELL_STAKES_RADIUS);
					gluDisk(stakes, 0.0, 2*WellObject::WELL_STAKES_RADIUS, 36, 10);
				glTranslatef(0.0, 0.0,  -WellObject::WELL_STAKES_RADIUS);
			glRotatef(-90, 0.0, 1.0, 0.0); glTranslatef(  0.0, 0.0, +WellObject::WELL_STAKES_RADIUS); 
			// top stake
			glTranslatef(+4*WellObject::WELL_STAKES_RADIUS, 0.0, -WellObject::WELL_STAKES_RADIUS);
				glRotatef(-90, 0.0, 1.0, 0.0); 
					gluCylinder(stakes, WellObject::WELL_STAKES_RADIUS, WellObject::WELL_STAKES_RADIUS,
						2*(WellObject::WELL_TUBE_OUTRADIUS-WellObject::WELL_TUBE_INNRADIUS) +
						2*WellObject::WELL_TUBE_INNRADIUS,
						36, 10);
					glTranslatef(0.0, 0.0, +(2*(WellObject::WELL_TUBE_OUTRADIUS-WellObject::WELL_TUBE_INNRADIUS) + 2*WellObject::WELL_TUBE_INNRADIUS));
						gluDisk(stakes, 0.0, WellObject::WELL_STAKES_RADIUS, 16, 2);
					glTranslatef(0.0, 0.0, -(2*(WellObject::WELL_TUBE_OUTRADIUS-WellObject::WELL_TUBE_INNRADIUS) + 2*WellObject::WELL_TUBE_INNRADIUS));
				glRotatef(90, 0.0, 1.0, 0.0); 
				// top stake: crank
				glTranslatef(0.0, 0.0, -2*WellObject::WELL_STAKES_RADIUS); glRotatef(45, 0.0, 0.0, 1.0);
					gluCylinder(stakes, sqrtf(2)*WellObject::WELL_STAKES_RADIUS, sqrtf(2)*WellObject::WELL_STAKES_RADIUS, WellObject::WELL_CRANK_HEIGHT, 4, 4);
					gluDisk(stakes, 0.0, sqrtf(2)*WellObject::WELL_STAKES_RADIUS, 4, 4);
					glTranslatef(0.0, 0.0, +WellObject::WELL_CRANK_HEIGHT);
						gluDisk(stakes, 0.0, sqrtf(2)*WellObject::WELL_STAKES_RADIUS, 4, 4);
					glTranslatef(0.0, 0.0, -WellObject::WELL_CRANK_HEIGHT);
				glRotatef(-45, 0.0, 0.0, 1.0); glTranslatef(0.0, 0.0, +2*WellObject::WELL_STAKES_RADIUS);
				// top stakes: crank, lower part
				glTranslatef(+WellObject::WELL_CRANK_WIDTH, 0.0, +WellObject::WELL_CRANK_HEIGHT-4*WellObject::WELL_STAKES_RADIUS); glRotatef(-90, 0.0, 1.0, 0.0); 
					gluCylinder(stakes, WellObject::WELL_STAKES_RADIUS, WellObject::WELL_STAKES_RADIUS, WellObject::WELL_CRANK_WIDTH, 10, 2);
					gluDisk(stakes, 0.0, WellObject::WELL_STAKES_RADIUS, 10, 2);
				glRotatef(+90, 0.0, 1.0, 0.0); glTranslatef(-WellObject::WELL_CRANK_WIDTH, 0.0, -WellObject::WELL_CRANK_HEIGHT+4*WellObject::WELL_STAKES_RADIUS);
			glTranslatef(-4*WellObject::WELL_STAKES_RADIUS, 0.0, +WellObject::WELL_STAKES_RADIUS);
		glTranslatef(-(WellObject::WELL_TUBE_INNRADIUS+(WellObject::WELL_TUBE_OUTRADIUS-WellObject::WELL_TUBE_INNRADIUS)/2), 0.0, 0.0);
	glPopMatrix();
	// rope
	GLUquadric* rope = gluNewQuadric();
	float ropeDiffuseColour[4] = { 0.2, 0.2, 0.2, 1.0 };
	float ropeSpecularColour[4] = { 0.2, 0.2, 0.2, 1.0 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ropeDiffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, ropeDiffuseColour);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, ropeSpecularColour);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0);
	glPushMatrix();
		gluQuadricDrawStyle(stakes, GLU_FILL);
		gluQuadricOrientation(stakes, GLU_OUTSIDE);
		gluQuadricNormals(stakes, GLU_SMOOTH);
		glTranslatef(0.0, +WellObject::WELL_TUBE_HEIGHT+WellObject::WELL_STAKES_HEIGHT+WellObject::WELL_STAKES_RADIUS, 0.0);
		gluSphere(rope, 2*WellObject::WELL_STAKES_RADIUS, 36, 36);
		glRotatef(90, 1, 0, 0);
		gluCylinder(rope, WellObject::WELL_ROPE_RADIUS, WellObject::WELL_ROPE_RADIUS, 
			WellObject::WELL_TUBE_HEIGHT+WellObject::WELL_STAKES_HEIGHT+WellObject::WELL_STAKES_RADIUS, 10, 10);
	glPopMatrix();
}


/***************************************************************************************
 ************      GLOBAL SCOPE                                             ************
 ***************************************************************************************/


/**********************************************************
 *                  scene functions
 * this functions are called upon the initialization, each
 * rendering and the disposing of the scene. they are
 * responsible for creating the Renderables and redirecting
 * the init/render/dispose function call to there.
 **********************************************************/

Camera sceneCamera;
RenderableList sceneRenderables;

void sceneInit() {
	// Initialize generic OpenGL options
    glClearColor(0.0,0.0,0.0,0.0);
	// Initialize camera
	Point eye   = {-6.0, 2.5, 5.0};
	Point focus = {-6.0, 2.5, 6.0};
	Point up    = { 0.0, 1.0, 0.0};
	sceneCamera.set(eye, focus, up);
	// --- Initialize rooms ---
	WallList* wallList = new WallList();
	Room* roomNW = new Room(-12.0, -1.0,   0.0,  0.0, 6.0, 20.0, wallList);
	Room* roomNE = new Room(  0.0, -1.0,   0.0, 10.0, 6.0, 10.0, wallList);
	Room* roomSE = new Room(  0.0, -1.0, -10.0, 15.0, 6.0,  0.0, wallList);
	// Initialize doors
	roomNW->getRightWall()->setDoor(-1.0, 2.8, 4.2, 7.3);
	roomNE->getLeftWall()->setDoor(-1.0, 2.8, 4.2, 7.3);
	roomNE->getBackWall()->setDoor(5.0, -1.0, 9.0, 4.0);
	roomSE->getFrontWall()->setDoor(5.0, -1.0, 9.0, 4.0);
	// Initialize NW room
	roomNW->getFrontWall()->setTextureMap("wall_nw_frontback_map.bmp");
	roomNW->getBackWall()->setTextureMap("wall_nw_frontback_map.bmp");
	roomNW->getLeftWall()->setTextureMap("wall_nw_left_map.bmp");
	roomNW->getRightWall()->setTextureMap("wall_nw_right_map.bmp");
	roomNW->getBottomWall()->setColor(0.2, 0.4, 0.6);
	roomNW->getTopWall()->setColor(0.8, 0.8, 0.8);
	// Initialize NE room
	roomNE->getFrontWall()->setTextureMap("wall_ne_front_map.bmp");
	roomNE->getBackWall()->setTextureMap("wall_ne_back_map.bmp");
	roomNE->getRightWall()->setTextureMap("wall_ne_right_map.bmp");
	roomNE->getLeftWall()->setTextureMap("wall_ne_left_map.bmp");
	roomNE->getBottomWall()->setColor(0.6, 0.2, 0.2);
	roomNE->getTopWall()->setColor(0.8, 0.8, 0.8);
	// Initialize SE room
	roomSE->getFrontWall()->setTextureMap("wall_se_front_map.bmp");
	roomSE->getBackWall()->setTextureMap("wall_se_back_map.bmp");
	roomSE->getLeftWall()->setTextureMap("wall_se_leftright_map.bmp");
	roomSE->getRightWall()->setTextureMap("wall_se_leftright_map.bmp");
	roomSE->getBottomWall()->setColor(0.6, 0.4, 0.2);
	roomSE->getTopWall()->setColor(0.8, 0.8, 0.8);
	// Initializing lights
	RoomLight* light0 = new RoomLight(GL_LIGHT0); 	RoomLight* light1 = new RoomLight(GL_LIGHT1);
	light0->setColors(1.0, 1.0, 1.0);               light1->setColors(1.0, 1.0, 1.0); 
	light0->setPosition(-9.0, 4.5, 19.0);           light1->setPosition(-3.0, 4.5, 19.0); 
	light0->setAttenuation(0.001, 0.001);           light1->setAttenuation(0.001, 0.001);
	RoomLight* light2 = new RoomLight(GL_LIGHT2);
	light2->setColors(0.6, 0.6, 0.6);
	light2->setPosition(5.0, 4.0, 5.0);
	light2->setAttenuation(0.001, 0.001);
	RoomLight* light3 = new RoomLight(GL_LIGHT3);
	light3->setColors(1.0, 1.0, 1.0);
	light3->setPosition(-14.0, 4.5, -5.0);
	light3->setAttenuation(0.001, 0.001);
	// add lights to rooms
	roomNW->addLight(light0);
	roomNW->addLight(light1);
	roomNE->addLight(light2);
	roomSE->addLight(light3);
	// --- Initialize objects ---
	WellObject*  wellObject  = new WellObject();
	LampObject*  lampObject  = new LampObject();
	TableObject* tableObject = new TableObject();
	tableObject->setTranslate(5.0, -1.0, -5.0);
	lampObject->setTranslate(5.0, 4.0, 5.0);
	wellObject->setTranslate(-6.0, -1.0, 14.0);
	// --- add rooms & objects to scene and WallList to camera for collision control ---
	sceneCamera.setWallList(wallList);
	sceneRenderables.add(roomNW);
	sceneRenderables.add(roomNE);
	sceneRenderables.add(roomSE);
	sceneRenderables.add(wellObject);
	sceneRenderables.add(lampObject);
	sceneRenderables.add(tableObject);
	sceneRenderables.initScene();
}

void sceneRender(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	sceneCamera.updateWindow();     // Orientate camera
	sceneRenderables.renderScene(); // Draw rooms and objects
	glutSwapBuffers();
}

void sceneDispose() {
	sceneRenderables.disposeScene();
}


/**********************************************************
 *                  control functions
 * this functions are called by openGL in case of user 
 * activity or when ideling.
 **********************************************************/

void controlIdle(void) {
	if (sceneCamera.step()) 
		sceneRender();
}

void controlReshape(int w, int h) {
	float ratio;
	if (h == 0) h = 1;
	ratio = 1.0f * w / h;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, w, h);
	gluPerspective(45, ratio, 1, 1000);
	glMatrixMode(GL_MODELVIEW);
	sceneRender();
}

void controlKeyboardUp(unsigned char key, int x, int y) {
	switch (key) {
		case 'i': case 'k': sceneCamera.moveForwardAuto(0.0); break; // stop moving forward/back
		case 'l': case 'j': sceneCamera.moveLeftAuto(0.0);    break; // stop moving left/right
		case 'z': case 'h': sceneCamera.moveUpAuto(0.0);      break; // stop moving up/down
		case 'd': case 'a': sceneCamera.turnLeftAuto(0.0);    break; // stop turning left/right
		case 'w': case 's': sceneCamera.turnUpAuto(0.0);      break; // stop turning up/down
	}
}

void controlKeyboard(unsigned char key, int x, int y) {
	switch (key) {
		case 'i': sceneCamera.moveForwardAuto(+0.2); break; // move forward
		case 'k': sceneCamera.moveForwardAuto(-0.2); break; // move back
		case 'j': sceneCamera.moveLeftAuto(+0.2);    break; // move left
		case 'l': sceneCamera.moveLeftAuto(-0.2);    break; // move right
		case 'y': sceneCamera.moveUpAuto(+0.2);      break; // move up
		case 'h': sceneCamera.moveUpAuto(-0.2);      break; // move down
		case 'a': sceneCamera.turnLeftAuto(+0.05);   break; // turn left
		case 'd': sceneCamera.turnLeftAuto(-0.05);   break; // turn right
		case 'w': sceneCamera.turnUpAuto(+0.05);     break; // turn up
		case 's': sceneCamera.turnUpAuto(-0.05);     break; // turn down
		case 'q': sceneDispose(); exit(0);
	}
}


/**********************************************************
 *                   main function
 * the main function initializes openGL and the GLUT library
 * and sets up the connection between the glut-hooks and 
 * the scene and control functions
 **********************************************************/

int main(int argc, char* argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(640,360);
	glutCreateWindow("OHJ-2706 Computer Graphics - Assignment 3 - Benjamin Söllner (206342)");
	sceneInit();
	glutDisplayFunc(sceneRender);
	glutIdleFunc(controlIdle);
	glutReshapeFunc(controlReshape);
	glutIgnoreKeyRepeat(true);
	glutKeyboardFunc(controlKeyboard);
	glutKeyboardUpFunc(controlKeyboardUp);
	glutMainLoop();
	sceneDispose();
	return(0);
}

