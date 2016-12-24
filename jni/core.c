#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include <stdio.h>
#include "defs.h"

int core(struct state *state){
	return true;
}

void render(struct state *state){
	glClear(GL_COLOR_BUFFER_BIT);
	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	/*glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BACKGROUND].object);
	draw(state,&state->background);*/

	glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_PLAYER].object);
	draw(state,&state->player.base);
	
	glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
	glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
	drawtext(state->font.main,state->rect.left+0.05f,state->rect.top+0.05f,"ace [alpha]");
}

void init(struct state *state){
	memset(state->pointer,0,sizeof(struct crosshair)*2);

	state->rect.left=-8.0f;
	state->rect.right=8.0f;
	state->rect.bottom=4.5f;
	state->rect.top=-4.5f;

	state->background.x=state->rect.left;
	state->background.y=state->rect.top;
	state->background.w=state->rect.right*2.0f;
	state->background.h=state->rect.bottom*2.0f;
	state->background.rot=0.0f;

	state->player.base.x=0.0f;
	state->player.base.y=0.0f;
	state->player.base.w=PLAYER_WIDTH;
	state->player.base.h=PLAYER_HEIGHT;
	state->player.base.rot=0.0f;
}

void reset(struct state *state){
}

