/* ofGem.h
 *
 * This header and its implementation
 * file are highly inspired by Gem's pix_share* object.
 * You may find them here :
 * https://github.com/umlaeute/Gem/tree/master/src/Pixes
 *
 * Antoine Villeret 2016
 */

#pragma once

#include "ofMain.h"
#include <sys/types.h>
#ifndef _WIN32
# include <sys/ipc.h>
# include <sys/shm.h>
#else
# include <windows.h>
# include <stdio.h>
# include <conio.h>
# include <tchar.h>
#endif

#define MAXPDSTRING 1000

// this is the header of the shared-memory segment
typedef struct _pixshare_header {
  size_t    size;      // total size of the shared-memory segment (without header)
  GLint     xsize;     // width of the image in the shm-segment
  GLint     ysize;     // height of the image in the shm-segment
  GLenum    format;    // format of the image (calculate csize,... from that)
  GLboolean upsidedown;// is the stored image swapped?
} t_pixshare_header;

class ofxGem : public ofBaseDraws {
public:
  ofxGem();
  virtual ~ofxGem();
  void setup(float id, int width, int height, int color);
  void setup(std::string id, int width, int height, int color);

  virtual void  draw() const { draw(0,0, m_width, m_height); };
  virtual void  draw( float x, float y ) const { draw(x,y, m_width, m_height); };
  virtual void  draw( float x, float y, float w, float h ) const;

  virtual float getWidth() const { return m_width; };
  virtual float getHeight() const { return m_height; };
  virtual float getColor() const { return m_color; };
  virtual float getShmaddr() const { return m_fake; };

  ofPixels getPixels();
  int setPixels(ofPixels);
protected:
  int m_width;
  int m_height;
  int m_color;
  int m_fake;

  int getShm(int _id, int _width, int _height, int _color);
  int hash_str2us(std::string s);

  void freeShm();

  ofImage m_img;

  unsigned char *m_shm_addr;
#ifndef _WIN32
  int m_shm_id;
  struct shmid_ds m_shm_desc;
#else
  HANDLE m_MapFile;
  char m_fileMappingName[MAXPDSTRING];
#endif
  size_t m_size;
};
