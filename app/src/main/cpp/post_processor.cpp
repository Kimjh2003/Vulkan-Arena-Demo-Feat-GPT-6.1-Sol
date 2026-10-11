#include "post_processor.h"
#include "gl_program.h"
#include <android/log.h>
#include <stdexcept>
#include <string>

PostProcessor::PostProcessor() {
    try { initialize(); } catch (...) { release(); throw; }
}
PostProcessor::~PostProcessor() { release(); }
void PostProcessor::release() noexcept {
    if (program) glDeleteProgram(program);
    if (vao) glDeleteVertexArrays(1, &vao);
    if (sceneFramebuffer) glDeleteFramebuffers(1, &sceneFramebuffer);
    if (outputFramebuffer) glDeleteFramebuffers(1, &outputFramebuffer);
    if (sceneTexture) glDeleteTextures(1, &sceneTexture);
    if (outputTexture) glDeleteTextures(1, &outputTexture);
    if (depthBuffer) glDeleteRenderbuffers(1, &depthBuffer);
}
void PostProcessor::initialize() {
    caps = astc::queryCapabilities();
    auto preferred = astc::chooseFormat(caps, astc::Semantic::Color, {{6,6},{4,4},{8,8}});
    __android_log_print(ANDROID_LOG_INFO, "Arena", "Selected GLES %d.%d; ASTC sRGB: %s; postprocess: %s",
                        caps.major, caps.minor, preferred ? "available" : "unavailable",
                        caps.compute ? "compute" : "fragment");
    if (caps.compute) {
        program = astc::createCopyProgram(caps);
    } else {
        const char* copyVertex = R"(#version 300 es
void main() {
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
})";
        const char* copyFragment = R"(#version 300 es
precision highp float;
precision highp int;
uniform highp sampler2D sourceTexture;
out vec4 fragment;
void main() { fragment = texelFetch(sourceTexture, ivec2(gl_FragCoord.xy), 0); }
)";
        program = graphics::createProgram({{GL_VERTEX_SHADER, copyVertex}, {GL_FRAGMENT_SHADER, copyFragment}});
        glUseProgram(program);
        glUniform1i(glGetUniformLocation(program, "sourceTexture"), 0);
        glGenVertexArrays(1, &vao);
    }
}
void PostProcessor::resizeTargets() {
    if (targetWidth == width && targetHeight == height) return;
    if (width <= 0 || height <= 0) throw std::runtime_error("Invalid render extent");
    if (sceneTexture) glDeleteTextures(1, &sceneTexture);
    if (outputTexture) glDeleteTextures(1, &outputTexture);
    sceneTexture = outputTexture = 0;
    if (!sceneFramebuffer) glGenFramebuffers(1, &sceneFramebuffer);
    if (!outputFramebuffer) glGenFramebuffers(1, &outputFramebuffer);
    if (!depthBuffer) glGenRenderbuffers(1, &depthBuffer);
    auto allocate = [&](GLuint& texture, GLuint framebuffer) {
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    };
    allocate(sceneTexture, sceneFramebuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Scene framebuffer incomplete");
    allocate(outputTexture, outputFramebuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Postprocess framebuffer incomplete");
    targetWidth = width; targetHeight = height;
}

void PostProcessor::beginScene(int w, int h) {
    width = w; height = h;
    resizeTargets();
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFramebuffer);
}
void PostProcessor::present() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (caps.compute) {
        astc::copyTexture(program, caps, sceneTexture, outputTexture, width, height);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, outputFramebuffer);
        glDisable(GL_DEPTH_TEST);
        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneTexture);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, outputFramebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}
