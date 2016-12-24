#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include "defs.h"

void draw(struct state *state,struct base *base){
	float x=base->x;
	float y=base->y;
	glUniform4f(state->uniform.texcoords,0.0f,1.0f,0.0f,1.0f);
	glUniform2f(state->uniform.vector,xcorrect(x),ycorrect(y));
	glUniform2f(state->uniform.size,base->w,base->h);
	glUniform1f(state->uniform.rot,base->rot);

	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}

void uidraw(struct state *state,struct base *base){
	glUniform4f(state->uniform.texcoords,0.0f,1.0f,0.0f,1.0f);
	glUniform2f(state->uniform.vector,base->x,base->y);
	glUniform2f(state->uniform.size,base->w,base->h);
	glUniform1f(state->uniform.rot,base->rot);

	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}

