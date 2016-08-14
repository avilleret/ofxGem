#include "ofxGem.h"

ofxGem :: ofxGem() :
  shm_addr(NULL),
#ifdef _WIN32
#else
  shm_id(0),
#endif
  m_size(0)
{}

ofxGem :: ~ofxGem(){
  ofxGem :: freeShm();
}

void ofxGem :: setup(float id, int width, int height, int color)
{
#ifndef _WIN32
  memset(&shm_desc, 0, sizeof(shm_desc));
#endif

  char buf[MAXPDSTRING];
  int fake = 0;
  snprintf(buf, MAXPDSTRING-1, "%g", id);
  buf[MAXPDSTRING-1]=0;
  fake = hash_str2us(buf);
  // fake = hash_str2us(atom_getsymbol(argv)->s_name);

  int err  = getShm(fake, width, height, color);

  switch(err){
  case 0:
    ofLog(OF_LOG_VERBOSE) << "shared memomry segment well done";
    break;
  case 1:
    ofLog(OF_LOG_ERROR) << "no valid size given";
    break;
  case 2:
    ofLog(OF_LOG_ERROR) <<  "given size < 0";
    break;
  case 3:
    ofLog(OF_LOG_ERROR) << "no valid dimensions given";
    break;
  case 4:
    ofLog(OF_LOG_ERROR) << "<color> must be one of: 4,2,1,RGBA,YUV,Grey";
    break;
  case 6:
    ofLog(OF_LOG_ERROR) << "couldn't get shared memory";
    break;
  case 7:
    ofLog(OF_LOG_ERROR) << "no ID given";
    break;
  case 8:
    ofLog(OF_LOG_ERROR) << "invalid ID...";
    break;
  default:
    ofLog(OF_LOG_ERROR) << "unknown error";
    break;
  }
}

void ofxGem :: setup(std::string id, int width, int height, int color)
{
#ifndef _WIN32
  memset(&shm_desc, 0, sizeof(shm_desc));
#endif

  int fake = 0;
  fake = hash_str2us(id.c_str());

  int err  = getShm(fake, width, height, color);

  switch(err){
  case 0:
    ofLog(OF_LOG_VERBOSE) << "shared memomry segment well done";
    break;
  case 1:
    ofLog(OF_LOG_ERROR) << "no valid size given";
    break;
  case 2:
    ofLog(OF_LOG_ERROR) <<  "given size < 0";
    break;
  case 3:
    ofLog(OF_LOG_ERROR) << "no valid dimensions given";
    break;
  case 4:
    ofLog(OF_LOG_ERROR) << "<color> must be one of: 4,2,1,RGBA,YUV,Grey";
    break;
  case 6:
    ofLog(OF_LOG_ERROR) << "couldn't get shared memory";
    break;
  case 7:
    ofLog(OF_LOG_ERROR) << "no ID given";
    break;
  case 8:
    ofLog(OF_LOG_ERROR) << "invalid ID...";
    break;
  default:
    ofLog(OF_LOG_ERROR) << "unknown error";
    break;
  }
}

int ofxGem :: getShm(int fake, int _width, int _height, int _color)
{
#ifdef _WIN32
  if ( shm_addr ) UnmapViewOfFile( shm_addr );
  if ( m_MapFile ) CloseHandle( m_MapFile );
  snprintf(m_fileMappingName, MAXPDSTRING-1, "gem_pix_share-FileMappingObject_%g", _id);
  // } else if (A_SYMBOL==argv->a_type){
  // snprintf(m_fileMappingName, MAXPDSTRING-1, "gem_pix_share-FileMappingObject_%s", atom_getsymbol(argv)->s_name);
  // }

#else
  if(shm_id>0)freeShm();

  if(fake<=0)return 8;
#endif /* _WIN32 */

  ofPixelFormat color;
  switch(_color)
  {
    case 1:
      color = OF_PIXELS_MONO;
      break;
    case 2:
      color = OF_PIXELS_YUY2;
      break;
    case 4:
      color = OF_PIXELS_RGBA;
      break;
    default:
      return 4;
      break;
  }

  ofPixels dummy;
  dummy.allocate(_width, _height, color);

  m_size = dummy.size();

  ofLog(OF_LOG_VERBOSE) << "pix_share segment : " << _width << "x" << _height << "x" << dummy.getBytesPerPixel() << " " << m_size;

#ifdef _WIN32
  int segmentSize=m_size+sizeof(t_pixshare_header);

  m_MapFile = CreateFileMapping(
               INVALID_HANDLE_VALUE,    // use paging file
               NULL,                    // default security
               PAGE_READWRITE,          // read/write access
               (segmentSize & 0xFFFFFFFF00000000) >> 32,         // maximum object size (high-order DWORD)
               segmentSize & 0xFFFFFFFF,         // maximum object size (low-order DWORD)
               m_fileMappingName);      // name of mapping object

  if (m_MapFile == NULL)
  {
    ofLog(OF_LOG_ERROR) << "Could not create file mapping object " << m_fileMappingName << "- error " << GetLastError() << ".";
    return -1;
  }

  shm_addr = (unsigned char*) MapViewOfFile(m_MapFile,   // handle to map object
                      FILE_MAP_ALL_ACCESS, // read/write permission
                      0,
                      0,
                      segmentSize);

  if ( !shm_addr ){
    ofLog(OF_LOG_ERROR) << "Could not get a view of file " << m_fileMappingName << " - error " << GetLastError() << ".";
    return -1;
  } else {
    verbose(0,"File mapping object %s successfully created.",m_fileMappingName);
  }

#else

  /* get a new segment with the size specified by the user
   * OR an old segment with the size specified in its header
   * why: if somebody has already created the segment with our key
   * we want to reuse it, even if its size is smaller than we requested
   */
  errno=0;
  shm_id = shmget(fake,m_size+sizeof(t_pixshare_header), IPC_CREAT | 0666);

  if((shm_id<0) && (EINVAL==errno)){
    errno=0;
    // the segment already exists, but is smaller than we thought!
    int id = shmget(fake,sizeof(t_pixshare_header),0666);
    if(id>0){ /* yea, we got it! */
      t_pixshare_header*h=(t_pixshare_header*)shmat(id,NULL,0666);
      if (!shm_addr || shm_addr==(void *)-1){
  shm_addr=NULL;
  return 8;
      }
      /* read the size of the blob from the shared segment */
      if(h&&h->size){
        ofLog(OF_LOG_ERROR) << "someone was faster: only got "<< h->size << " bytes instead of " << m_size << ".";
        m_size=h->size;

        /* so free this shm-segment before we re-try with a smaller size */
        shmdt(h);

        /* now get the shm-segment with the correct size */
        shm_id = shmget(fake,m_size+sizeof(t_pixshare_header), IPC_CREAT | 0666);
      }
    }
  }

  if(shm_id>0){
    /* now that we have a shm-segment, get the pointer to the data */
    shm_addr = (unsigned char*)shmat(shm_id,NULL,0666);
    if (!shm_addr || shm_addr==(void *)-1){
      shm_addr=NULL;
      return 8;
    }

    if(shmctl(shm_id,IPC_STAT,&shm_desc)<0) {
      return 8;
    }
    /* write the size into the shm-segment */
    t_pixshare_header *h=(t_pixshare_header *)shm_addr;
    h->size = (shm_desc.shm_segsz-sizeof(t_pixshare_header));

    ofLog(OF_LOG_VERBOSE) << "shm:: id(" << shm_id << "segsz(" << shm_desc.shm_segsz << " cpid (" << shm_desc.shm_cpid << " mem(" << std::hex << shm_addr << ")";
  } else {
    ofLog(OF_LOG_ERROR) << "couldn't get shm_id: error " << errno << ".";
    return -1; // AV : added because i'm usure of what value is returned when we get this error...
  }
#endif /* _WIN32 */
  return 0;
}

void ofxGem :: freeShm()
{
#ifdef _WIN32
  if ( shm_addr ) UnmapViewOfFile( shm_addr );
  shm_addr = NULL;
  if ( m_MapFile ) CloseHandle( m_MapFile );
  m_MapFile = NULL;
#else
  if(shm_addr){
    if (shmdt(shm_addr) == -1) ofLog(OF_LOG_ERROR) << "shmdt failed at " << std::hex << shm_addr;
  }
  shm_addr=NULL;

  if(shm_id>0){
    if (shmctl(shm_id,IPC_STAT, &shm_desc) != -1){
      if(shm_desc.shm_nattch<=0){
        if (shmctl(shm_id,IPC_RMID, &shm_desc) == -1) ofLog(OF_LOG_ERROR) << "shmctl remove failed for " << shm_id;
      }
    }
  }
  shm_id=0;
#endif /* _WIN32 */
}

int ofxGem :: setPixels(ofPixels pix){

  img.setFromPixels(pix);

#ifndef _WIN32
  if(shm_id>0){
#else
  if(m_MapFile){
#endif /* _WIN32 */

    if (!shm_addr){
      ofLog(OF_LOG_ERROR) << "no shmaddr";
      return -1;
    }

    if (pix.size()<=m_size) {
      t_pixshare_header *h=(t_pixshare_header *)shm_addr;
      h->size =pix.size();
      h->xsize=pix.getWidth();
      h->ysize=pix.getHeight();
      switch (pix.getPixelFormat()){
        case OF_PIXELS_MONO:
          h->format = GL_LUMINANCE;
          break;
        case OF_PIXELS_RGBA:
          h->format = GL_RGBA;
          break;
        case OF_PIXELS_YUY2:
          h->format = GL_YCBCR_422_APPLE;
          break;
        default:
          ofLog(OF_LOG_ERROR) << "format " << pix.getPixelFormat() << " is not supported (only OF_PIXELS_MONO, OF_PIXELS_RGBA or OF_PIXELS_YUY2)";
          return -1;
      }

      h->upsidedown=GL_TRUE;
      memcpy(shm_addr+sizeof(t_pixshare_header),pix.getData(),pix.size());
    }
    else{
      ofLog(OF_LOG_ERROR) << "input image too large: " << pix.getWidth() << "x" << pix.getHeight() << "x" << pix.getBytesPerPixel() << "=" << pix.size() << ">" << m_size;
    }
  }
  return 0;
}


ofPixels ofxGem :: getPixels(){
    ofPixels pix;
    if (shm_addr) {
      t_pixshare_header *h=(t_pixshare_header *)shm_addr;
      unsigned char* data=shm_addr+sizeof(t_pixshare_header);
      int imgsize=h->format*h->xsize*h->ysize;
      if(imgsize){

        ofPixelFormat color;
        switch (h->format){
          case GL_LUMINANCE:
            color=OF_PIXELS_MONO;
            break;
          case GL_RGBA:
            color = OF_PIXELS_RGBA;
            break;
          case GL_YCBCR_422_APPLE:
            color = OF_PIXELS_YUY2;
            break;
          default:
            ofLog(OF_LOG_ERROR) << "pixel format not supported : " << h->format;
            return pix;
        }
        pix.setFromPixels(data, h->xsize, h->ysize, color);
        img.setFromPixels(pix);
      }
    } else {
      ofLog(OF_LOG_ERROR) << "no shmaddr";
    }
    return pix;
}

void ofxGem ::  draw( float x, float y, float w, float h ) const {
    img.draw(x,y,w,h);
}

int ofxGem :: hash_str2us(std::string s) {
  /*
  def self.rs( str, len=str.length )
    a,b = 63689,378551
    hash = 0
    len.times{ |i|
      hash = hash*a + str[i]
      a *= b
    }
    hash & SIGNEDSHORT
  end
  */

  int result=0;
  int a=63689;
  int b=378551;


  if(s.length()<1)return -1;

  unsigned int i=0;
  for(i=0; i<s.length(); i++) {
    result=result*a+s[i];
    a *= b;
  }

  return ((unsigned short)(result) & 0x7FFFFFFF);
}
