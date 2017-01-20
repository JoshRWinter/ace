#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include "defs.h"

int menu_main(struct state *state){
	struct base title={state->rect.left,state->rect.top,state->rect.right*2.0f,state->rect.bottom*2.0f,0.0f,1.0f,0.0f};
	while(process(state->app)){
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_TITLE].object);
		draw(state,&title);

		eglSwapBuffers(state->display,state->surface);
	}
	return false;
}

