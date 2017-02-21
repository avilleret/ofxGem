#include "ofxGem.h"

ofxGem :: ofxGem() :
  m_shm_addr(NULL),
#ifdef _WIN32
#else
  m_shm_id(0),
#endif
  m_size(0),
  m_fake(0)
{}

ofxGem :: ~ofxGem(){
  ofxGem :: freeShm();
}

int ofxGem :: setup(float id, int width, int height, int color){
  setup(id);
  return getShm(m_fake, width, height, color);
}

int ofxGem :: setup(std::string id, int width, int height, int color){
  setup(id);
  return getShm(m_fake, width, height, color);
}


void ofxGem :: setup(float id)
{
#ifndef _WIN32
  memset(&m_shm_desc, 0, sizeof(m_shm_desc));
#endif

  char buf[MAXPDSTRING];
  snprintf(buf, MAXPDSTRING-1, "%g", id);
  buf[MAXPDSTRING-1]=0;
  m_fake = hash_str2us(buf);
}

void ofxGem :: setup(std::string id)
{
#ifndef _WIN32
  memset(&m_shm_desc, 0, sizeof(m_shm_desc));
#endif

  m_fake = hash_str2us(id.c_str());
}

int ofxGem :: getShm(int fake, int w, int h, int c)
{
#ifdef _WIN32
  if ( shm_addr ) UnmapViewOfFile( shm_addr );
  if ( m_MapFile ) CloseHandle( m_MapFile );
  snprintf(m_fileMappingName, MAXPDSTRING-1, "gem_pix_share-FileMappingObject_%g", _id);
  // } else if (A_SYMBOL==argv->a_type){
  // snprintf(m_fileMappingName, MAXPDSTRING-1, "gem_pix_share-FileMappingObject_%s", atom_getsymbol(argv)->s_name);
  // }

#else
  if(m_shm_id>0)freeShm();

  if(fake<=0){
    ofLog(OF_LOG_ERROR) << "invalid ID...";
    return 8;
  }
#endif /* _WIN32 */

  m_size = w*h*c;

  ofLog(OF_LOG_VERBOSE) << "pix_share segment : " << w << "x" << h << "x" << c << " " << m_size;

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
  m_shm_id = shmget(fake,m_size+sizeof(t_pixshare_header), IPC_CREAT | 0666);

  if((m_shm_id<0) && (EINVAL==errno)){
    errno=0;
    // the segment already exists, but is smaller than we thought!
    int id = shmget(fake,sizeof(t_pixshare_header),0666);
    if(id>0){ /* yea, we got it! */
      t_pixshare_header*h=(t_pixshare_header*)shmat(id,NULL,0666);
      if (!m_shm_addr || m_shm_addr==(void *)-1){
        m_shm_addr=NULL;
        m_size=0;
        ofLog(OF_LOG_ERROR) << "couldn't get shared memory";
        return 8;
      }
      /* read the size of the blob from the shared segment */
      if(h&&h->size){
        ofLog(OF_LOG_ERROR) << "someone was faster: only got "<< h->size << " bytes instead of " << m_size << ".";
        m_size=h->size;

        /* so free this shm-segment before we re-try with a smaller size */
        shmdt(h);

        /* now get the shm-segment with the correct size */
        m_shm_id = shmget(fake,m_size+sizeof(t_pixshare_header), IPC_CREAT | 0666);
      }
    }
  }

  if(m_shm_id>0){
    /* now that we have a shm-segment, get the pointer to the data */
    m_shm_addr = (unsigned char*)shmat(m_shm_id,NULL,0666);
    if (!m_shm_addr || m_shm_addr==(void *)-1){
      m_shm_addr=NULL;
      m_size=0;
      ofLog(OF_LOG_ERROR) << "couldn't get shared memory";
      return 8;
    }

    if(shmctl(m_shm_id,IPC_STAT,&m_shm_desc)<0) {
      m_shm_addr=NULL;
      m_size=0;
      ofLog(OF_LOG_ERROR) << "couldn't get shared memory stats (IPC_STAT)";
      return 8;
    }

    /* write the size into the shm-segment */
    t_pixshare_header *h=(t_pixshare_header *)m_shm_addr;
    h->size = (m_shm_desc.shm_segsz-sizeof(t_pixshare_header));

    ofLog(OF_LOG_VERBOSE) << "shm:: id(" << m_shm_id << "segsz(" << m_shm_desc.shm_segsz << " cpid (" << m_shm_desc.shm_cpid << " mem(" << std::hex << m_shm_addr << ")";
  } else {
    ofLog(OF_LOG_ERROR) << "couldn't get shm_id: error " << errno << ".";
    m_shm_addr=NULL;
    m_size=0;
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
  if(m_shm_addr){
    if (shmdt(m_shm_addr) == -1) ofLog(OF_LOG_ERROR) << "shmdt failed at " << std::hex << m_shm_addr;
  }
  m_shm_addr=NULL;

  if(m_shm_id>0){
    if (shmctl(m_shm_id,IPC_STAT, &m_shm_desc) != -1){
      if(m_shm_desc.shm_nattch<=0){
        if (shmctl(m_shm_id,IPC_RMID, &m_shm_desc) == -1) ofLog(OF_LOG_ERROR) << "shmctl remove failed for " << m_shm_id;
      }
    }
  }
  m_shm_id=0;
#endif /* _WIN32 */
}

int ofxGem :: setPixels(ofPixels pix){

  if (!pix.isAllocated()) return -1;

  m_img.setFromPixels(pix);

#ifndef _WIN32
  if(m_shm_id>0){
#else
  if(m_MapFile){
#endif /* _WIN32 */

  if (pix.size()>m_size) {
    ofLog(OF_LOG_VERBOSE) << "pixels data is bigger than shared memory, so we reallocate it";
    getShm(m_fake, pix.getWidth(), pix.getHeight(), pix.getBytesPerPixel());
  }

  if (!m_shm_addr || m_shm_addr==(void *)-1){
    ofLog(OF_LOG_ERROR) << "no shmaddr";
    return -1;
  }

  t_pixshare_header *h=(t_pixshare_header *)m_shm_addr;
  h->size =pix.size();
  h->xsize=pix.getWidth();
  h->ysize=pix.getHeight();
  h->format=convertPixelFormat2Gem(pix.getPixelFormat());

  h->upsidedown=GL_TRUE;
  memcpy(m_shm_addr+sizeof(t_pixshare_header),pix.getData(),pix.size());
  }
  return 0;
}


ofPixels& ofxGem :: getPixels(){
    return m_pix;
}

void ofxGem::update(){
    if (m_shm_addr) {
      t_pixshare_header *h=(t_pixshare_header *)m_shm_addr;
      unsigned char* data=m_shm_addr+sizeof(t_pixshare_header);
      int imgsize=h->format*h->xsize*h->ysize;
      ofLogVerbose(__func__) << "size: " << h->xsize << "x" << h->ysize << " " << h->format;
      if(imgsize){

        ofPixelFormat fmt = convertPixelFormat2of(h->format);
        m_pix.setFromPixels(data, h->xsize, h->ysize, fmt);
        m_img.setFromPixels(m_pix);
      }
    } else {
      ofLog(OF_LOG_ERROR) << "no shmaddr";
    }
}

void ofxGem ::  draw( float x, float y, float w, float h ) const {
    m_img.draw(x,y,w,h);
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

int ofxGem :: convertPixelFormat2Gem (ofPixelFormat format){
  int gemFormat = -1;
  switch (format){
    case OF_PIXELS_MONO:
      gemFormat = GL_LUMINANCE;
      break;
    case OF_PIXELS_RGBA:
      gemFormat = GL_RGBA;
      break;
    case OF_PIXELS_YUY2:
      gemFormat = GL_YCBCR_422_APPLE;
      break;
    default:
      ofLog(OF_LOG_ERROR) << "format " << format << " is not supported (only OF_PIXELS_MONO, OF_PIXELS_RGBA or OF_PIXELS_YUY2)";
  }
  return gemFormat;
}

ofPixelFormat ofxGem :: convertPixelFormat2of(int gemFormat){
  ofPixelFormat format;
  switch (gemFormat){
    case GL_LUMINANCE:
      format=OF_PIXELS_MONO;
      break;
    case GL_RGBA:
      format = OF_PIXELS_RGBA;
      break;
    case GL_YCBCR_422_APPLE:
      format = OF_PIXELS_YUY2;
      break;
    default:
      ofLog(OF_LOG_ERROR) << "pixel format not supported : " << gemFormat;
  }
  return format;
}
