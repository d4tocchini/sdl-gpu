

// https://github.com/memononen/nanovg/issues/234
//  NANOVG_GL_USE_UNIFORMBUFFER  ?
#include "gfx/gfx.h"
#include "gfx/ren.h"

#define  STB_IMAGE_WRITE_IMPLEMENTATION
#include STB_IMAGE_WRITE__H
#undef   STB_IMAGE_WRITE_IMPLEMENTATION

// TODO:
#undef GLAPI
#define GLAPI extern

#define SDL_GPU_DISABLE_GLEW
// #define GLEW_STATIC // needed for windows only(?)
// #include <GL/glew.h>

// #include <SDL2/SDL.h>
// #include <SDL2/SDL_gpu.h>
#define SDL_GPU_USE_BUFFER_RESET
#define SDL_GPU_DISABLE_GLES

#define SDL_GPU_DISABLE_OPENGL_1_BASE
#define SDL_GPU_DISABLE_OPENGL_1
#define SDL_GPU_DISABLE_OPENGL_2
#define SDL_GPU_DISABLE_OPENGL_4
#include "sdl_gpu/src/SDL_gpu.c"
#include "sdl_gpu/src/SDL_gpu_matrix.c"
#include "sdl_gpu/src/SDL_gpu_shapes.c"
#include "sdl_gpu/src/SDL_gpu_renderer.c"
#include "sdl_gpu/src/renderer_OpenGL_3.c"


#include <math.h>
#include "sdl_gpu/common/common.c"



// #define NANOVG_GL3_IMPLEMENTATION
// #include "nanovg/nanovg.h"
// #include "nanovg/nanovg_gl.h"
// #include "nanovg/nanovg_gl_utils.h"

/* Create a GPU_Image from a NanoVG Framebuffer */
GPU_Image* generateFBO(NVGcontext* vg,
    const float _w, const float _h,
    void (*draw)(NVGcontext*, const float, const float, const float, const float)) {
    // GPU_FlushBlitBuffer(); // call GPU_FlushBlitBuffer if you're doing this in the middle of SDL_gpu blitting
    NVGLUframebuffer* fb = nvgluCreateFramebuffer(vg, _w, _h,
        NVG_IMAGE_NODELETE
    );

    // https://github.com/memononen/nanovg/issues/410
    // if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    //     printf("! GL_FRAMEBUFFER_UNSUPPORTED.\n");
    //     return -1;
    // }
        // IMPORTANT: NVG_IMAGE_NODELETE allows us to run nvgluDeleteFramebuffer without freeing the GPU_Image data
    if (fb == NULL) {
		printf("Could not create FBO.\n");
		return -1;
	}
    nvgluBindFramebuffer(fb);
    glViewport(0, 0, _w, _h);
    glClearColor(0, 0, 0, 0);
            glClearColor(0x00, 0x00, 0xFF, 0x1F);
        glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    // glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    nvgBeginFrame(vg, _w, _h, 1.0f);
    draw(vg, 0, 0, _w, _h); // call the drawing function that was passed as parameter
    nvgBeginPath(vg);
        nvgRect(vg, 0.0f, 0.0f, 30.0f, 30.0f);
        nvgStrokeColor(vg, nvgRGBA(0xFF, 0xFF, 0x00, 0xFF));
        nvgStroke(vg);
    nvgEndFrame(vg);
    nvgluBindFramebuffer(NULL); // official documentation says to unbind, but I haven't had issues not doing it */
    GPU_ResetRendererState(); // not calling GPU_ResetRendererState can cause problems with SDL_gpu depending on your order of operations
    // IMPORTANT: don't run nvgluDeleteFramebuffer, GPU_CreateImageUsingTexture takes the handle
    GPU_Image* return_image = GPU_CreateImageUsingTexture(fb->texture, false); // should take_ownership be true?
    // nvgluDeleteFramebuffer(fb);
    return return_image;
}

/* Simple Drawing Example */
void drawNVG(NVGcontext* _vg, const float _x, const float _y, const float _w, const float _h) {
    const float square_r = 5.0f;
    nvgBeginPath(_vg);
    nvgRoundedRect(_vg, _x, _y, _w, _h, square_r);
    NVGpaint bg_paint = nvgLinearGradient(_vg, _x, _y, _x+_w, _y+_h, nvgRGBA(255, 255, 255, 255), nvgRGBA(255, 255, 255, 155));
    nvgFillPaint(_vg, bg_paint);
    nvgFill(_vg);
}

/* draw something that takes some awhile */
void drawComplexNVG(NVGcontext* _vg, const float _x, const float _y, const float _w, const float _h) {
    float x = _x;
    float y = _y;
    nvgBeginPath(_vg);
	nvgMoveTo(_vg, x, y);
	for (unsigned i = 1; i < 50000; i++) {
		nvgBezierTo(_vg, x-10.0f, y+10.0f, x+25, y+25, x,y);
        x += 10.0f;
        y += 5.0f;
        if (x > _w)
            x = 0.0f;
        if (y > _h)
            y = 0.0f;
    }
    NVGpaint stroke_paint = nvgLinearGradient(_vg, _x, _y, _w, _h, nvgRGBA(255, 255, 255, 20), nvgRGBA(0, 255, 255, 10));
    nvgStrokePaint(_vg, stroke_paint);
    nvgStroke(_vg);
}

/* Can help show STENCIL problems when _arc_radius because concave (convex?) */
void drawPie(NVGcontext* _vg, const float _x, const float _y, const float _arc_radius) {
    const float pie_radius = 100.0f;
    nvgBeginPath(_vg);
    nvgMoveTo(_vg, _x, _y);
    nvgArc(_vg, _x, _y, pie_radius, 0.0f, nvgDegToRad(_arc_radius), NVG_CW);
    nvgLineTo(_vg, _x, _y);
    nvgFillColor(_vg, nvgRGBA(0xFF,0xFF,0xFF,0xFF));
    nvgFill(_vg);
}


void renderPattern(NVGcontext* vg, NVGLUframebuffer* fb, float t, float px_ratio)
{
	int win_w, win_h;
	int fboWidth, fboHeight;
	int pw, ph, x, y;
	float s = 20.0f;
	float sr = (cosf(t)+1)*0.5f;
	float r = s * 0.6f * (0.2f + 0.8f * sr);

	if (fb == NULL) return;

	nvgImageSize(vg, fb->image, &fboWidth, &fboHeight);
	win_w = (int)(fboWidth / px_ratio);
	win_h = (int)(fboHeight / px_ratio);

	// Draw some stuff to an FBO as a test
	nvgluBindFramebuffer(fb);
	glViewport(0, 0, fboWidth, fboHeight);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	nvgBeginFrame(vg, win_w, win_h, px_ratio);

	pw = (int)ceilf(win_w / s);
	ph = (int)ceilf(win_h / s);

	nvgBeginPath(vg);
	for (y = 0; y < ph; y++) {
		for (x = 0; x < pw; x++) {
			float cx = (x+0.5f) * s;
			float cy = (y+0.5f) * s;
			nvgCircle(vg, cx,cy, r);
		}
	}
	nvgFillColor(vg, nvgRGBA(220,160,0,200));
	nvgFill(vg);

	nvgEndFrame(vg);
	nvgluBindFramebuffer(NULL);
    GPU_ResetRendererState();
}


void main_loop(GPU_Target* _screen, NVGcontext* vg, const Uint16 win_w, const Uint16 win_h) {
    NVGLUframebuffer* fb = NULL;
    int fbWidth = win_w;
    int fbHeight = win_h;
    const float px_ratio = (float)fbWidth/(float)win_w; // (float)win_w / (float)win_w; // spoilers: it's 1.0f

    fb = nvgluCreateFramebuffer(vg, (int)(100*px_ratio), (int)(100*px_ratio), NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY);
    if (fb == NULL) {
		printf("Could not create FBO.\n");
		return ;
	}

    // GPU_FlushBlitBuffer();
    GPU_Rect fbo_simple_rect = { 65.0f, 10.0f, 50.0f, 50.0f}; // Blitting Destination
    GPU_Image* fbo_simple = generateFBO(vg, fbo_simple_rect.w*2.0f, fbo_simple_rect.h*2.0f, drawNVG);

    GPU_Rect fbo_complex_rect = { 0.0f, 0.0f, win_w, win_h}; // Blitting Destination
    GPU_Image* fbo_complex = generateFBO(vg, fbo_complex_rect.w, fbo_complex_rect.h, drawPie);

    // GPU_LoadTarget(fbo_simple);
    // GPU_LoadTarget(fbo_complex);

    float arc_radius = 0.0f;

    Uint32 startTime = SDL_GetTicks();;
    long frameCount = 0;

    GPU_Image* image = GPU_LoadImage("data/d5.png");
    float t = 0.0f;
    renderPattern(vg, fb, t, px_ratio);
    GPU_Image* fb_img = GPU_CreateImageUsingTexture(fb->texture, false);
    if (!fb_img) {
        printf("! GPU_CreateImageUsingTexture.\n");
		return ;
    }

    SDL_Event event;
    bool loop = true;
    do {
        /* SDL Event Handling */
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT)
                loop = false;
            else if(event.type == SDL_KEYDOWN) {
                if(event.key.keysym.sym == SDLK_ESCAPE)
                    loop = false;
            }
        }
        int i, n;
        t = SDL_GetTicks()/2000.0f;



		// Update and render
		glViewport(0, 0, fbWidth, fbHeight);
		// glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
		// glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);



        /* Animation Pass */
        arc_radius += 1.0f;

        //  GPU_Clear(_screen);
        GPU_ClearRGBA(_screen, 0x00, 0x00, 0x00, 0xFF); // GPU_ClearRGBA clears GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT
        glClear(GL_STENCIL_BUFFER_BIT); // IMPORTANT: GPU_ClearRGBA does not clear GL_STENCIL_BUFFER_BIT


        // GPU_Blit(fb_img, NULL, _screen, 0, 0);

        GPU_BlitRectX(fbo_complex, NULL, _screen, &fbo_complex_rect, 0.0f, 0.0f, 0.0f, GPU_FLIP_VERTICAL); // IMPORTANT: GPU_BlitRectX is required to use GPU_FLIP_VERTICAL which is required for NVGLUframebuffer data (why???)
            GPU_BlitRectX(fbo_simple, NULL, _screen, &fbo_simple_rect, 0.0f, 0.0f, 0.0f, GPU_FLIP_VERTICAL); // IMPORTANT: GPU_BlitRectX is required to use GPU_FLIP_VERTICAL which is required for NVGLUframebuffer data (why???)
            GPU_BlitTransform(
                fbo_simple,
                 NULL, _screen, _screen->w/2, _screen->h/2, 360*sin(SDL_GetTicks()/2000.0f), 2.5*sin(SDL_GetTicks()/1000.0f), 2.5*sin(SDL_GetTicks()/1200.0f));



        GPU_FlushBlitBuffer(); // IMPORTANT: run GPU_FlushBlitBuffer before nvgBeginFrame
		// nvgBeginFrame(vg, win_w, win_h, px_ratio);

		// // Use the FBO as image pattern.
		// if (fb != NULL) {
		// 	NVGpaint img = nvgImagePattern(vg, 0, 0, 100, 100, 0, fb->image, 1.0f);
		// 	nvgSave(vg);

		// 	for (i = 0; i < 20; i++) {
		// 		nvgBeginPath(vg);
		// 		nvgRect(vg, 10 + i*30,10, 10, win_h-20);
		// 		nvgFillColor(vg, nvgHSLA(i/19.0f, 0.5f, 0.5f, 1.0f));
		// 		nvgFill(vg);
		// 	}

		// 	nvgBeginPath(vg);
		// 	nvgRoundedRect(vg, 140 + sinf(t*1.3f)*100, 140 + cosf(t*1.71244f)*100, 250, 250, 20);
		// 	nvgFillPaint(vg, img);
		// 	nvgFill(vg);
		// 	nvgStrokeColor(vg, nvgRGBA(220,160,0,255));
		// 	nvgStrokeWidth(vg, 3.0f);
		// 	nvgStroke(vg);

		// 	nvgRestore(vg);
		// }

        // nvgEndFrame(vg);
        GPU_ResetRendererState();





        /* SDL_gpu + NanoVG Rendering */
        // GPU_ClearRGBA(_screen, 0x00, 0x00, 0x00, 0xFF); // GPU_ClearRGBA clears GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT
        // glClear(GL_STENCIL_BUFFER_BIT); // IMPORTANT: GPU_ClearRGBA does not clear GL_STENCIL_BUFFER_BIT

        /* SDL_gpu Blitting */
        // GPU_BlitRectX(fbo_complex, NULL, _screen, &fbo_complex_rect, 0.0f, 0.0f, 0.0f, GPU_FLIP_VERTICAL); // IMPORTANT: GPU_BlitRectX is required to use GPU_FLIP_VERTICAL which is required for NVGLUframebuffer data (why???)
        // GPU_BlitRectX(fbo_simple, NULL, _screen, &fbo_simple_rect, 0.0f, 0.0f, 0.0f, GPU_FLIP_VERTICAL); // IMPORTANT: GPU_BlitRectX is required to use GPU_FLIP_VERTICAL which is required for NVGLUframebuffer data (why???)

        // // /* NanoVG Section */
        // GPU_FlushBlitBuffer(); // IMPORTANT: run GPU_FlushBlitBuffer before nvgBeginFrame
        // nvgBeginFrame(vg, win_w, win_h, px_ratio); // Do your normal NanoVG stuff
        // // drawNVG(vg, 10.0f, 10.0f, fbo_simple_rect.w, fbo_simple_rect.h); // run our simple drawing code directly
        // drawPie(vg, win_w/2, win_h/2, arc_radius); // drawing the pie chart will break if you don't have a stencil buffer
        // // drawComplexNVG(vg, 0.0f,0.0f,win_w,win_h);
        // nvgEndFrame(vg); // Finish our NanoVG pass
        // GPU_ResetRendererState(); // IMPORTANT: run GPU_ResetRendererState after nvgEndFrame

        /* Finish */
        GPU_Flip(_screen); // Render to screen


        frameCount++;
        if(frameCount%500 == 0)
            printf("Average FPS: %.2f\n", 1000.0f*frameCount/(SDL_GetTicks() - startTime));

    } while (loop);

    /* Loops over, Cleanup */
    GPU_FreeImage(fbo_simple);
    // GPU_FreeImage(fbo_complex);
}

int main() {
    const Uint16 screen_w = 500;
    const Uint16 screen_h = 500;

    /* Init SDL and our renderer, no error handling! */
    // SDL_Init(SDL_INIT_EVERYTHING); // Init SDL
    // SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8); // NanoVG _REQUIRES_ a stencil buffer
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
    gfx_init();



    GPU_SetDebugLevel(GPU_DEBUG_LEVEL_MAX);
    GPU_Target* target = GPU_InitRenderer(GPU_RENDERER_OPENGL_3, screen_w, screen_h, 0); // Init SDL_gpu
    NVGcontext* vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG); // Init NanoVG
    if (vg == NULL) {
		printf("Could not init nanovg.\n");
		return -1;
	}
    // printCurrentRenderer();

    main_loop(target, vg, screen_w, screen_h);

    return 0;
}





// int main(int argc, char* argv[])
// {
//     gfx_init();
// 	GPU_Target* screen;

// 	printRenderers();

// 	screen = GPU_Init(800, 600, GPU_DEFAULT_INIT_FLAGS);
// 	if(screen == NULL)
// 		return -1;

// 	printCurrentRenderer();

// 	{
// 		Uint32 startTime;
// 		long frameCount;
// 		Uint8 done;
// 		SDL_Event event;

//         GPU_Image* image = GPU_LoadImage("data/d5.png");
//         if(image == NULL)
//             return -1;


//         startTime = SDL_GetTicks();
//         frameCount = 0;

//         done = 0;
//         while(!done)
//         {
//             while(SDL_PollEvent(&event))
//             {
//                 if(event.type == SDL_QUIT)
//                     done = 1;
//                 else if(event.type == SDL_KEYDOWN)
//                 {
//                     if(event.key.keysym.sym == SDLK_ESCAPE)
//                         done = 1;
//                 }
//             }

//             GPU_Clear(screen);

//             GPU_BlitTransform(image, NULL, screen, screen->w/2, screen->h/2, 360*sin(SDL_GetTicks()/2000.0f), 2.5*sin(SDL_GetTicks()/1000.0f), 2.5*sin(SDL_GetTicks()/1200.0f));

//             GPU_Flip(screen);

//             frameCount++;
//             if(frameCount%500 == 0)
//                 printf("Average FPS: %.2f\n", 1000.0f*frameCount/(SDL_GetTicks() - startTime));
//         }

//         printf("Average FPS: %.2f\n", 1000.0f*frameCount/(SDL_GetTicks() - startTime));

//         GPU_FreeImage(image);
// 	}

// 	GPU_Quit();

// 	return 0;
// }


