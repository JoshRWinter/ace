#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include "defs.h"

int button_process(struct crosshair *p,struct button *b){
	if(p[0].x>b->base.x&&p[0].x<b->base.x+BUTTON_WIDTH&&p[0].y>b->base.y&&p[0].y<b->base.y+BUTTON_HEIGHT){
		if(p[0].active){
			b->active=true;
			return BUTTON_PRESS;
		}
		else if(b->active){
			b->active=false;
			return BUTTON_ACTIVATE;
		}
	}
	b->active=false;
	return 0;
}

void button_draw(struct state *state,struct button *button){
	float yoff;
	struct button b=*button;
	if(b.active){
		yoff=0.075f;
		b.base.y+=yoff;
	}
	else
		yoff=0.0f;

	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
	uidraw(state,&b.base);

	glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
	glBindTexture(GL_TEXTURE_2D,state->font.button->atlas);
	drawtextcentered(state->font.button,b.base.x+(BUTTON_WIDTH/2.0f),b.base.y+(BUTTON_HEIGHT/2.0f)-(state->font.button->fontsize/2.0f),b.label);
}

int collide(struct base *a,struct base *b,float tolerance){
	return a->x+a->w>b->x+tolerance&&a->x<b->x+(b->w-tolerance)&&a->y+a->h>b->y+tolerance&&a->y<b->y+(b->h-tolerance);
}

void draw(struct state *state,struct base *base){
	// texcoords
	float size=1.0f/base->count;
	float left=size*base->frame;
	float right=left+size;

	float x=base->x;
	float y=base->y;
	glUniform4f(state->uniform.texcoords,left,right,0.0f,1.0f);
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

