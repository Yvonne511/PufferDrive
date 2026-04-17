// Headless EGL GPU rendering for PufferDrive.
//
// This implementation intentionally avoids build-time EGL dependencies because
// our cluster nodes may expose libEGL at runtime without installing the EGL
// development headers or linker symlinks. We resolve the required entry points
// from libEGL.so.1 dynamically.

#ifndef EGL_HEADLESS_H
#define EGL_HEADLESS_H

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int GLenum;
typedef unsigned char GLubyte;
typedef struct GLFWwindow GLFWwindow;
GLFWwindow *glfwGetCurrentContext(void);
void glfwMakeContextCurrent(GLFWwindow *window);

#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02

typedef void *EGLDisplay;
typedef void *EGLContext;
typedef void *EGLSurface;
typedef void *EGLConfig;
typedef void *EGLDeviceEXT;
typedef intptr_t EGLAttrib;
typedef unsigned int EGLenum;
typedef int EGLint;
typedef unsigned int EGLBoolean;

#define EGL_FALSE 0
#define EGL_TRUE 1

#define EGL_DEFAULT_DISPLAY ((void *)0)
#define EGL_NO_DISPLAY ((EGLDisplay)0)
#define EGL_NO_CONTEXT ((EGLContext)0)
#define EGL_NO_SURFACE ((EGLSurface)0)

#define EGL_NONE 0x3038
#define EGL_VENDOR 0x3053
#define EGL_SURFACE_TYPE 0x3033
#define EGL_PBUFFER_BIT 0x0001
#define EGL_RENDERABLE_TYPE 0x3040
#define EGL_OPENGL_BIT 0x0008
#define EGL_RED_SIZE 0x3024
#define EGL_GREEN_SIZE 0x3023
#define EGL_BLUE_SIZE 0x3022
#define EGL_ALPHA_SIZE 0x3021
#define EGL_DEPTH_SIZE 0x3025
#define EGL_WIDTH 0x3057
#define EGL_HEIGHT 0x3056
#define EGL_OPENGL_API 0x30A2
#define EGL_CONTEXT_MAJOR_VERSION 0x3098
#define EGL_CONTEXT_MINOR_VERSION 0x30FB
#define EGL_CONTEXT_OPENGL_PROFILE_MASK 0x30FD
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT 0x00000002
#define EGL_PLATFORM_DEVICE_EXT 0x313F

typedef void *(*eglGetProcAddressFunc)(const char *procname);
typedef EGLDisplay (*eglGetPlatformDisplayEXTFunc)(EGLenum platform, void *native_display, const EGLAttrib *attrib_list);
typedef EGLBoolean (*eglQueryDevicesEXTFunc)(EGLint max_devices, EGLDeviceEXT *devices, EGLint *num_devices);
typedef EGLBoolean (*eglInitializeFunc)(EGLDisplay dpy, EGLint *major, EGLint *minor);
typedef const char *(*eglQueryStringFunc)(EGLDisplay dpy, EGLint name);
typedef EGLBoolean (*eglTerminateFunc)(EGLDisplay dpy);
typedef EGLBoolean (*eglChooseConfigFunc)(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs,
                                          EGLint config_size, EGLint *num_config);
typedef EGLSurface (*eglCreatePbufferSurfaceFunc)(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
typedef EGLBoolean (*eglBindAPIFunc)(EGLenum api);
typedef EGLContext (*eglCreateContextFunc)(EGLDisplay dpy, EGLConfig config, EGLContext share_context,
                                           const EGLint *attrib_list);
typedef EGLBoolean (*eglMakeCurrentFunc)(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
typedef EGLint (*eglGetErrorFunc)(void);
typedef EGLBoolean (*eglDestroySurfaceFunc)(EGLDisplay dpy, EGLSurface surface);
typedef EGLBoolean (*eglDestroyContextFunc)(EGLDisplay dpy, EGLContext ctx);
typedef const GLubyte *(*glGetStringFunc)(GLenum name);

typedef struct {
    void *libegl;
    eglGetProcAddressFunc eglGetProcAddress;
    eglGetPlatformDisplayEXTFunc eglGetPlatformDisplayEXT;
    eglQueryDevicesEXTFunc eglQueryDevicesEXT;
    eglInitializeFunc eglInitialize;
    eglQueryStringFunc eglQueryString;
    eglTerminateFunc eglTerminate;
    eglChooseConfigFunc eglChooseConfig;
    eglCreatePbufferSurfaceFunc eglCreatePbufferSurface;
    eglBindAPIFunc eglBindAPI;
    eglCreateContextFunc eglCreateContext;
    eglMakeCurrentFunc eglMakeCurrent;
    eglGetErrorFunc eglGetError;
    eglDestroySurfaceFunc eglDestroySurface;
    eglDestroyContextFunc eglDestroyContext;
    glGetStringFunc glGetString;
    void *libgl;
    int loaded;
} EGLFns;

typedef struct {
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
    EGLConfig config;
    int width;
    int height;
    int active;
    int handoff_complete;
} EGLHeadlessContext;

static EGLFns g_egl = {0};
static EGLHeadlessContext g_egl_ctx = {0};

static int egl_headless_load(void) {
    if (g_egl.loaded) {
        return 1;
    }

    g_egl.libegl = dlopen("libEGL.so.1", RTLD_NOW | RTLD_LOCAL);
    if (g_egl.libegl == NULL) {
        fprintf(stderr, "[egl_headless] Failed to load libEGL.so.1: %s\n", dlerror());
        return 0;
    }

    g_egl.eglGetProcAddress = (eglGetProcAddressFunc)dlsym(g_egl.libegl, "eglGetProcAddress");
    g_egl.eglInitialize = (eglInitializeFunc)dlsym(g_egl.libegl, "eglInitialize");
    g_egl.eglQueryString = (eglQueryStringFunc)dlsym(g_egl.libegl, "eglQueryString");
    g_egl.eglTerminate = (eglTerminateFunc)dlsym(g_egl.libegl, "eglTerminate");
    g_egl.eglChooseConfig = (eglChooseConfigFunc)dlsym(g_egl.libegl, "eglChooseConfig");
    g_egl.eglCreatePbufferSurface = (eglCreatePbufferSurfaceFunc)dlsym(g_egl.libegl, "eglCreatePbufferSurface");
    g_egl.eglBindAPI = (eglBindAPIFunc)dlsym(g_egl.libegl, "eglBindAPI");
    g_egl.eglCreateContext = (eglCreateContextFunc)dlsym(g_egl.libegl, "eglCreateContext");
    g_egl.eglMakeCurrent = (eglMakeCurrentFunc)dlsym(g_egl.libegl, "eglMakeCurrent");
    g_egl.eglGetError = (eglGetErrorFunc)dlsym(g_egl.libegl, "eglGetError");
    g_egl.eglDestroySurface = (eglDestroySurfaceFunc)dlsym(g_egl.libegl, "eglDestroySurface");
    g_egl.eglDestroyContext = (eglDestroyContextFunc)dlsym(g_egl.libegl, "eglDestroyContext");

    if (g_egl.eglGetProcAddress == NULL || g_egl.eglInitialize == NULL || g_egl.eglQueryString == NULL ||
        g_egl.eglTerminate == NULL || g_egl.eglChooseConfig == NULL || g_egl.eglCreatePbufferSurface == NULL ||
        g_egl.eglBindAPI == NULL || g_egl.eglCreateContext == NULL || g_egl.eglMakeCurrent == NULL ||
        g_egl.eglGetError == NULL || g_egl.eglDestroySurface == NULL || g_egl.eglDestroyContext == NULL) {
        fprintf(stderr, "[egl_headless] Missing required EGL entry points\n");
        dlclose(g_egl.libegl);
        memset(&g_egl, 0, sizeof(g_egl));
        return 0;
    }

    g_egl.eglQueryDevicesEXT =
        (eglQueryDevicesEXTFunc)g_egl.eglGetProcAddress("eglQueryDevicesEXT");
    g_egl.eglGetPlatformDisplayEXT =
        (eglGetPlatformDisplayEXTFunc)g_egl.eglGetProcAddress("eglGetPlatformDisplayEXT");

    if (g_egl.eglQueryDevicesEXT == NULL || g_egl.eglGetPlatformDisplayEXT == NULL) {
        fprintf(stderr, "[egl_headless] EGL device extensions not available\n");
        dlclose(g_egl.libegl);
        memset(&g_egl, 0, sizeof(g_egl));
        return 0;
    }

    g_egl.libgl = dlopen("libGL.so.1", RTLD_NOW | RTLD_LOCAL);
    if (g_egl.libgl != NULL) {
        g_egl.glGetString = (glGetStringFunc)dlsym(g_egl.libgl, "glGetString");
    }

    g_egl.loaded = 1;
    return 1;
}

// Create an EGL context on an NVIDIA GPU. Does NOT make it current yet.
static int egl_headless_init(int width, int height) {
    if (!egl_headless_load()) {
        return 0;
    }

    EGLDeviceEXT devices[8];
    EGLint numDevices = 0;
    g_egl.eglQueryDevicesEXT(8, devices, &numDevices);
    if (numDevices == 0) {
        fprintf(stderr, "[egl_headless] No EGL devices found\n");
        return 0;
    }

    EGLDisplay display = EGL_NO_DISPLAY;
    for (int i = 0; i < numDevices; i++) {
        display = g_egl.eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, devices[i], NULL);
        if (display != EGL_NO_DISPLAY) {
            EGLint major, minor;
            if (g_egl.eglInitialize(display, &major, &minor)) {
                const char *vendor = g_egl.eglQueryString(display, EGL_VENDOR);
                if (vendor && strstr(vendor, "NVIDIA")) {
                    fprintf(stderr, "[egl_headless] Using NVIDIA EGL device %d (EGL %d.%d)\n", i, major, minor);
                    break;
                }
                g_egl.eglTerminate(display);
                display = EGL_NO_DISPLAY;
            }
        }
    }

    if (display == EGL_NO_DISPLAY) {
        fprintf(stderr, "[egl_headless] No NVIDIA EGL device found\n");
        return 0;
    }

    EGLint configAttribs[] = {EGL_SURFACE_TYPE,
                              EGL_PBUFFER_BIT,
                              EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_BIT,
                              EGL_RED_SIZE,
                              8,
                              EGL_GREEN_SIZE,
                              8,
                              EGL_BLUE_SIZE,
                              8,
                              EGL_ALPHA_SIZE,
                              8,
                              EGL_DEPTH_SIZE,
                              24,
                              EGL_NONE};
    EGLConfig config;
    EGLint numConfigs = 0;
    g_egl.eglChooseConfig(display, configAttribs, &config, 1, &numConfigs);
    if (numConfigs == 0) {
        fprintf(stderr, "[egl_headless] No suitable EGL config\n");
        g_egl.eglTerminate(display);
        return 0;
    }

    EGLint pbufferAttribs[] = {EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE};
    EGLSurface surface = g_egl.eglCreatePbufferSurface(display, config, pbufferAttribs);
    if (surface == EGL_NO_SURFACE) {
        fprintf(stderr, "[egl_headless] Failed to create pbuffer: 0x%x\n", g_egl.eglGetError());
        g_egl.eglTerminate(display);
        return 0;
    }

    g_egl.eglBindAPI(EGL_OPENGL_API);
    EGLint contextAttribs[] = {EGL_CONTEXT_MAJOR_VERSION,
                               3,
                               EGL_CONTEXT_MINOR_VERSION,
                               3,
                               EGL_CONTEXT_OPENGL_PROFILE_MASK,
                               EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT,
                               EGL_NONE};
    EGLContext context = g_egl.eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        fprintf(stderr, "[egl_headless] Failed to create GL context: 0x%x\n", g_egl.eglGetError());
        g_egl.eglDestroySurface(display, surface);
        g_egl.eglTerminate(display);
        return 0;
    }

    g_egl_ctx.display = display;
    g_egl_ctx.context = context;
    g_egl_ctx.surface = surface;
    g_egl_ctx.config = config;
    g_egl_ctx.width = width;
    g_egl_ctx.height = height;
    fprintf(stderr, "[egl_headless] GPU context created (%dx%d), ready to activate\n", width, height);
    return 1;
}

static int egl_headless_release_glfw_context(void) {
    GLFWwindow *current = glfwGetCurrentContext();
    if (current == NULL) {
        return 1;
    }

    glfwMakeContextCurrent(NULL);
    fprintf(stderr, "[egl_headless] Released GLX context\n");
    return 1;
}

static int egl_headless_restore_glfw_context(void *window_handle) {
    if (window_handle == NULL) {
        return 0;
    }

    glfwMakeContextCurrent((GLFWwindow *)window_handle);
    return 1;
}

static int egl_headless_make_current(void) {
    if (g_egl_ctx.active) {
        return 1;
    }

    if (!g_egl.eglMakeCurrent(g_egl_ctx.display, g_egl_ctx.surface, g_egl_ctx.surface, g_egl_ctx.context)) {
        fprintf(stderr, "[egl_headless] eglMakeCurrent failed: 0x%x\n", g_egl.eglGetError());
        return 0;
    }

    g_egl_ctx.active = 1;
    return 1;
}

// Recreate the pbuffer surface at (width, height) if the current one is too
// small for the requested dimensions.
static int egl_headless_resize(int width, int height) {
    if (!g_egl_ctx.active || g_egl_ctx.surface == EGL_NO_SURFACE) {
        return 0;
    }
    if (width <= g_egl_ctx.width && height <= g_egl_ctx.height) {
        return 1;
    }

    int new_w = width > g_egl_ctx.width ? width : g_egl_ctx.width;
    int new_h = height > g_egl_ctx.height ? height : g_egl_ctx.height;

    if (!g_egl.eglMakeCurrent(g_egl_ctx.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
        fprintf(stderr, "[egl_headless] resize unbind failed: 0x%x\n", g_egl.eglGetError());
        return 0;
    }
    g_egl.eglDestroySurface(g_egl_ctx.display, g_egl_ctx.surface);

    EGLint pbufferAttribs[] = {EGL_WIDTH, new_w, EGL_HEIGHT, new_h, EGL_NONE};
    EGLSurface new_surface = g_egl.eglCreatePbufferSurface(g_egl_ctx.display, g_egl_ctx.config, pbufferAttribs);
    if (new_surface == EGL_NO_SURFACE) {
        fprintf(stderr, "[egl_headless] resize create pbuffer failed: 0x%x\n", g_egl.eglGetError());
        return 0;
    }
    if (!g_egl.eglMakeCurrent(g_egl_ctx.display, new_surface, new_surface, g_egl_ctx.context)) {
        fprintf(stderr, "[egl_headless] resize rebind failed: 0x%x\n", g_egl.eglGetError());
        g_egl.eglDestroySurface(g_egl_ctx.display, new_surface);
        return 0;
    }

    g_egl_ctx.surface = new_surface;
    g_egl_ctx.width = new_w;
    g_egl_ctx.height = new_h;
    g_egl_ctx.active = 1;
    fprintf(stderr, "[egl_headless] pbuffer resized to %dx%d\n", new_w, new_h);
    return 1;
}

// Switch the current GL context from Xvfb/GLX to the EGL/NVIDIA GPU exactly
// once. Subsequent calls are no-ops, which avoids repeated GLFW/GLX handoffs.
static int egl_switch_to_gpu(void) {
    if (g_egl_ctx.active) {
        return 1;
    }

    if (!g_egl_ctx.handoff_complete) {
        if (!egl_headless_release_glfw_context()) {
            return 0;
        }
        g_egl_ctx.handoff_complete = 1;
    }

    if (!egl_headless_make_current()) {
        return 0;
    }

    const char *renderer = g_egl.glGetString ? (const char *)g_egl.glGetString(GL_RENDERER) : NULL;
    const char *version = g_egl.glGetString ? (const char *)g_egl.glGetString(GL_VERSION) : NULL;
    fprintf(stderr, "[egl_headless] GPU active: %s (%s)\n", renderer ? renderer : "unknown", version ? version : "?");
    return 1;
}

static void egl_headless_cleanup(void) {
    if (g_egl_ctx.context) {
        g_egl.eglMakeCurrent(g_egl_ctx.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        g_egl.eglDestroyContext(g_egl_ctx.display, g_egl_ctx.context);
    }
    if (g_egl_ctx.surface) {
        g_egl.eglDestroySurface(g_egl_ctx.display, g_egl_ctx.surface);
    }

    g_egl_ctx.display = EGL_NO_DISPLAY;
    g_egl_ctx.context = EGL_NO_CONTEXT;
    g_egl_ctx.surface = EGL_NO_SURFACE;
    g_egl_ctx.config = NULL;
    g_egl_ctx.width = 0;
    g_egl_ctx.height = 0;
    g_egl_ctx.active = 0;
    g_egl_ctx.handoff_complete = 0;
}

#endif // EGL_HEADLESS_H
