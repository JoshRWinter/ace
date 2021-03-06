#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "defs.h"

extern const char *vertexshader,*fragmentshader;

void init_display(struct state *state){
	state->running=true;
	initextensions();
	getdims(&state->device,state->app->window,DIMS_LAND);
	state->screen.w=state->device.w>1920?1920:state->device.w;
	state->screen.h=state->device.h>1080?1080:state->device.h;

	state->display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(state->display,NULL,NULL);
	EGLConfig config;
	int configcount;
	eglChooseConfig(state->display,(int[]){EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE},&config,1,&configcount);
	ANativeWindow_setBuffersGeometry(state->app->window,state->screen.w,state->screen.h,0);

	state->surface=eglCreateWindowSurface(state->display,config,state->app->window,NULL);
	state->context=eglCreateContext(state->display,config,NULL,(int[]){EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE});

	eglMakeCurrent(state->display,state->surface,state->surface,state->context);

	// texture packs
	if(!loadpack(&state->assets,state->app->activity->assetManager,"assets",NULL))logcat("texture init error");
	if(!loadpack(&state->uiassets,state->app->activity->assetManager,"uiassets",NULL))logcat("ui texture init error");

	// set up shaders
	state->program=initshaders(vertexshader,fragmentshader);
	glUseProgram(state->program);
	state->uniform.vector=glGetUniformLocation(state->program,"vector");
	state->uniform.size=glGetUniformLocation(state->program,"size");
	state->uniform.texcoords=glGetUniformLocation(state->program,"texcoords");
	state->uniform.rot=glGetUniformLocation(state->program,"rot");
	state->uniform.rgba=glGetUniformLocation(state->program,"rgba");
	state->uniform.projection=glGetUniformLocation(state->program,"projection");
	glUniform1f(state->uniform.rot,0.0f);
	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	float matrix[16];
	initortho(matrix,state->rect.left,state->rect.right,state->rect.bottom,state->rect.top,-1.0f,1.0f);
	glUniformMatrix4fv(state->uniform.projection,1,false,matrix);

	// VBOs and VAOs
	glGenVertexArrays(1,&state->vao);
	glGenBuffers(1,&state->vbo);
	glBindVertexArray(state->vao);
	glBindBuffer(GL_ARRAY_BUFFER,state->vbo);
	float verts[]={-0.5f,-0.5f,-0.5f,0.5f,0.5f,-0.5f,0.5f,0.5f};
	glBufferData(GL_ARRAY_BUFFER,sizeof(float)*8,verts,GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,2,GL_FLOAT,false,0,NULL);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0f,0.65882f,0.83137f,1.0f);

	// set up sounds
	loadapack(&state->aassets,state->app->activity->assetManager,"aassets");
	state->soundengine=initOpenSL(sound_config_fn);
	if(state->music)
		sl_play_loop(state->soundengine,state->aassets.sound+SID_THEME);
	else
		sl_play_loop(state->soundengine,state->aassets.sound+SID_SILENCE);

	// set up fonts
	set_ftfont_params(state->screen.w,state->screen.h,state->rect.right*2.0f,state->rect.bottom*2.0f,state->uniform.vector,state->uniform.size,state->uniform.texcoords);
	state->font.main=create_ftfont(state->app->activity->assetManager,0.4f,"corbel.ttf");
	state->font.main_gothic=create_ftfont(state->app->activity->assetManager,0.4,"GOTHIC.TTF");
	state->font.button=create_ftfont(state->app->activity->assetManager,0.6f,"GOTHIC.TTF");
	state->font.header=create_ftfont(state->app->activity->assetManager,0.8f,"GOTHIC.TTF");
}

void term_display(struct state *state){
	if(state->player.engine!=0){
		sl_stop(state->soundengine,state->player.engine);
		state->player.engine=0;
	}
	termOpenSL(state->soundengine);
	destroyapack(&state->aassets);
	state->running=false;
	destroy_ftfont(state->font.main);
	destroy_ftfont(state->font.main_gothic);
	destroy_ftfont(state->font.button);
	destroy_ftfont(state->font.header);
	glDeleteBuffers(1,&state->vbo);
	glDeleteVertexArrays(1,&state->vao);
	glDeleteProgram(state->program);
	destroypack(&state->assets);
	destroypack(&state->uiassets);
	eglMakeCurrent(state->display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
	eglDestroyContext(state->display,state->context);
	eglDestroySurface(state->display,state->surface);
	eglTerminate(state->display);
}

int32_t inputproc(struct android_app *app, AInputEvent *event){
	struct state *state=app->userData;
	int32_t type=AInputEvent_getType(event);
	if(type==AINPUT_EVENT_TYPE_MOTION){
		return retrieve_touchscreen_input(event,state->pointer,state->device.w,state->device.h,state->rect.right*2.0,state->rect.bottom*2.0f);
	}
	else if(type==AINPUT_EVENT_TYPE_KEY){
		int action=AKeyEvent_getAction(event);
		int key=AKeyEvent_getKeyCode(event);
		if(action==AKEY_EVENT_ACTION_UP&&key==AKEYCODE_MENU){
			reset(app->userData);
			return true;
		}
		else if(action==AKEY_EVENT_ACTION_UP&&key==AKEYCODE_BACK){
			state->back=true;
			return true;
		}
	}
	return false;
}

void cmdproc(struct android_app *app,int32_t cmd){
	struct state *state=app->userData;
	switch(cmd){
		case APP_CMD_RESUME:
			hidenavbars(&state->jni_info);
			break;
		case APP_CMD_INIT_WINDOW:
			init_display(state);
			break;
		case APP_CMD_TERM_WINDOW:
			term_display(state);
			break;
		case APP_CMD_DESTROY:
			reset(state);
			break;
	}
}

int process(struct android_app *app){
	int ident,events;
	struct android_poll_source *source;
	while((ident=ALooper_pollAll(((struct state*)app->userData)->running?0:-1,NULL,&events,(void**)&source))>=0){
		if(source)source->process(app,source);
		if(app->destroyRequested)
			return false;
	}
	return true;
}

static const struct state *global_state;
void sound_config_fn(const struct apacksound *sound,const struct sl_entity_position *listener,const struct sl_entity_position *source,float *stereo,float *intensity){
	// set the stereo position
	*stereo=(source->x-listener->x)/10.0f;

	// set the intensity
	float atten;
	float d=distance(listener->x,source->x,listener->y,source->y);
	if(sound==global_state->aassets.sound+SID_HUGEEXPLOSION)
		atten=1.0f-(d/100.0f);
	else
		atten=1.0f-(d/15.0f);
	// clamp
	if(atten<0.0f)
		atten=0.0f;
	else if(atten>1.0f)
		atten=1.0f;
	*intensity=atten;
}

void android_main(struct android_app *app){
	logcat("--- BEGIN NEW LOG ---");
	app_dummy();
	srand48(time(NULL));
	struct state state;
	global_state=&state;
	state.running=false;
	state.app=app;
	app->userData=&state;
	app->onAppCmd=cmdproc;
	app->onInputEvent=inputproc;
	init(&state);
	reset(&state);
	init_jni(state.app,&state.jni_info);
	while(process(app)&&core(&state)){
		render(&state);
		eglSwapBuffers(state.display,state.surface);
	}
	term_jni(&state.jni_info);
	logcat("--- END LOG ---");
}

