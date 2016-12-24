#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "defs.h"

int core(struct state *state){
	// joysticks
	state->joy_top.x=state->joy_base.x+(JOYBASE_SIZE/2.0f)-(JOYTOP_SIZE/2.0f);
	state->joy_top.y=state->joy_base.y+(JOYBASE_SIZE/2.0f)-(JOYTOP_SIZE/2.0f);
	for(int i=0;i<2;++i){
		if(!state->pointer[i].active||state->pointer[i].x>0.0f)
			continue;

		state->joy_top.x=state->pointer[i].x-(JOYTOP_SIZE/2.0f);
		state->joy_top.y=state->pointer[i].y-(JOYTOP_SIZE/2.0f);
		float angle=atan2f((state->joy_base.y+(JOYBASE_SIZE/2.0f))-(state->joy_top.y+(JOYTOP_SIZE/2.0f)),
				(state->joy_base.x+(JOYBASE_SIZE/2.0f))-(state->joy_top.x+(JOYTOP_SIZE/2.0f)));
		state->player.base.rot=angle;

		if(distance(state->joy_top.x+(JOYTOP_SIZE/2.0f),state->joy_base.x+(JOYBASE_SIZE/2.0f),
					state->joy_top.y+(JOYTOP_SIZE/2.0f),state->joy_base.y+(JOYBASE_SIZE/2.0f))>JOYTOP_DIST){
			state->joy_top.x=(state->joy_base.x+(JOYBASE_SIZE/2.0f)-(JOYTOP_SIZE/2.0f))-cosf(angle)*JOYTOP_DIST;
			state->joy_top.y=(state->joy_base.y+(JOYBASE_SIZE/2.0f)-(JOYTOP_SIZE/2.0f))-sinf(angle)*JOYTOP_DIST;
		}
	}
	
	return true;
}

void render(struct state *state){
	glClear(GL_COLOR_BUFFER_BIT);
	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	/*glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BACKGROUND].object);
	draw(state,&state->background);*/

	glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_PLAYER].object);
	draw(state,&state->player.base);

	glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_JOYBASE].object);
	draw(state,&state->joy_base);
	glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_JOYTOP].object);
	draw(state,&state->joy_top);
	
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

	state->joy_base.w=JOYBASE_SIZE;
	state->joy_base.h=JOYBASE_SIZE;
	state->joy_base.x=-6.5f;
	state->joy_base.y=1.25f;
	state->joy_base.rot=0.0f;
	state->joy_top.w=JOYTOP_SIZE;
	state->joy_top.h=JOYTOP_SIZE;
	state->joy_top.x=state->joy_base.x+(JOYBASE_SIZE/2.0f)-(JOYTOP_SIZE/2.0f);
	state->joy_top.y=state->joy_base.y+(JOYBASE_SIZE/2.0f)-(JOYTOP_SIZE/2.0f);
	state->joy_top.rot=0.0f;

	state->player.base.x=-PLAYER_WIDTH/2.0f;
	state->player.base.y=-PLAYER_HEIGHT/2.0f;
	state->player.base.w=PLAYER_WIDTH;
	state->player.base.h=PLAYER_HEIGHT;
	state->player.base.rot=0.0f;
}

void reset(struct state *state){
}

