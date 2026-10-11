#include "gl_capabilities.h"
#include <algorithm>
namespace graphics {
bool GlCapabilities::has(const char* name) const {
    return std::find(extensions.begin(),extensions.end(),name)!=extensions.end();
}
GlCapabilities queryGlCapabilities() {
    GlCapabilities caps;
    glGetIntegerv(GL_MAJOR_VERSION,&caps.major);glGetIntegerv(GL_MINOR_VERSION,&caps.minor);
    GLint count=0;glGetIntegerv(GL_NUM_EXTENSIONS,&count);
    caps.extensions.reserve(std::max(0,count));
    for(GLint i=0;i<count;++i) {
        const auto* text=glGetStringi(GL_EXTENSIONS,i);
        if(text) caps.extensions.emplace_back(reinterpret_cast<const char*>(text));
    }
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS,&count);
    caps.compressedFormats.resize(std::max(0,count));
    if(count>0) glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS,caps.compressedFormats.data());
    if(caps.has("GL_EXT_texture_filter_anisotropic")) {
        GLfloat maximum=1;glGetFloatv(0x84FF,&maximum);caps.anisotropy=std::clamp(maximum,1.f,8.f);
    }
    if(caps.major>3 || caps.minor>=1) {
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,&caps.workgroupInvocations);
        for(GLuint i=0;i<3;++i) glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE,i,&caps.workgroupSize[i]);
    }
    return caps;
}
}
