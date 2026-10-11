#include "compressed_texture.h"
#include <algorithm>
#include <limits>
namespace graphics {
GLuint uploadCompressed(GLenum format,int bw,int bh,int bb,const std::vector<CompressedMip>& mips,float anisotropy) {
    if(mips.empty() || bw<=0 || bh<=0 || bb<=0) return 0;
    GLint maximum=0; glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maximum);
    int w=mips[0].width,h=mips[0].height;
    if(w<=0 || h<=0 || w>maximum || h>maximum) return 0;
    for(size_t i=0;i<mips.size();++i) {
        size_t bytes=((static_cast<size_t>(w)+bw-1)/bw)*((static_cast<size_t>(h)+bh-1)/bh)*bb;
        const auto& mip=mips[i];
        if(mip.width!=w || mip.height!=h || mip.bytes.size()!=bytes ||
           bytes>static_cast<size_t>(std::numeric_limits<GLsizei>::max())) return 0;
        if(w==1 && h==1) { if(i+1!=mips.size()) return 0; }
        else if(i+1==mips.size()) return 0;
        w=std::max(1,w/2); h=std::max(1,h/2);
    }
    GLint previous=0; glGetIntegerv(GL_TEXTURE_BINDING_2D,&previous);
    GLuint texture=0; glGenTextures(1,&texture); glBindTexture(GL_TEXTURE_2D,texture);
    for(size_t i=0;i<mips.size();++i) {
        const auto& mip=mips[i];
        glCompressedTexImage2D(GL_TEXTURE_2D,static_cast<GLint>(i),format,mip.width,mip.height,0,
                              static_cast<GLsizei>(mip.bytes.size()),mip.bytes.data());
    }
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,static_cast<GLint>(mips.size()-1));
    if(anisotropy>1.f) glTexParameterf(GL_TEXTURE_2D,0x84FE,anisotropy);
    GLenum error=glGetError(); glBindTexture(GL_TEXTURE_2D,previous);
    if(error!=GL_NO_ERROR) { glDeleteTextures(1,&texture); return 0; }
    return texture;
}
}
