#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "defs.h"

int core(struct state *state){
	// clouds
	if(!state->cloudlist||onein(140))newcloud(state);
	for(struct cloud *cloud=state->cloudlist,*prevcloud=NULL;cloud!=NULL;){
		if(distance(cloud->base.x+(CLOUD_SIZE/2.0f),state->player.base.x+(PLAYER_WIDTH/2.0f),
		cloud->base.y+(CLOUD_SIZE/2.0f),state->player.base.y+(PLAYER_WIDTH/2.0f))>CLOUD_RMDIST){
				cloud=deletecloud(state,cloud,prevcloud);
				continue;
		}

		prevcloud=cloud;
		cloud=cloud->next;
	}

	// enemies
	if(onein(140))newenemy(state);
	for(struct enemy *enemy=state->enemylist,*prevenemy=NULL;enemy!=NULL;){
		float angle=atan2f((enemy->base.y+(ENEMY_HEIGHT/2.0f))-(enemy->target.y+(ENEMY_HEIGHT/2.0f)),
				(enemy->base.x+(ENEMY_WIDTH/2.0f))-(enemy->target.x+(ENEMY_WIDTH/2.0f)));
		align(&enemy->base.rot,PLAYER_TURN_SPEED,angle);
		enemy->base.x-=cosf(enemy->base.rot)*ENEMY_SPEED;
		enemy->base.y-=sinf(enemy->base.rot)*ENEMY_SPEED;

		if(collide(&enemy->base,&enemy->target)){
			enemy->target.x=randomint((state->player.base.x-10.0f)*10.0f,(state->player.base.x+10.0f)*10.0f)/10.0f;
			enemy->target.y=randomint((state->player.base.y-10.0f)*10.0f,(state->player.base.y+10.0f)*10.0f)/10.0f;
		}

		if(enemy->timer_smoke==0){
			enemy->timer_smoke=PLAYER_SMOKE;
			newsmoke(state,&enemy->base,0.4f,0.3f);
		}
		else --enemy->timer_smoke;
		
		prevenemy=enemy;
		enemy=enemy->next;
	}

	// bullets
	if(state->fire&&state->player.reload==0){
		newbullet(state,&state->player.base);
		state->player.reload=PLAYER_RELOAD;
	}
	for(struct bullet *bullet=state->bulletlist,*prevbullet=NULL;bullet!=NULL;){
		if(bullet->base.x<state->player.base.x-20.0f||bullet->base.x>state->player.base.x+20.0f||
				bullet->base.y<state->player.base.y-20.0f||state->player.base.y>state->player.base.y+20.0f){
			bullet=deletebullet(state,bullet,prevbullet);
			continue;
		}
		bullet->base.x+=bullet->xv;
		bullet->base.y+=bullet->yv;
		prevbullet=bullet;
		bullet=bullet->next;
	}

	// smoke
	for(struct smoke *smoke=state->smokelist,*prevsmoke=NULL;smoke!=NULL;){
		smoke->base.x+=smoke->xv;
		smoke->base.y+=smoke->yv;
		smoke->alpha-=SMOKE_FADE;
		if(smoke->alpha<0.0f){
			smoke=deletesmoke(state,smoke,prevsmoke);
			continue;
		}
		smoke->base.w-=SMOKE_SHRINK;
		smoke->base.h=smoke->base.w;
		smoke->base.x+=SMOKE_SHRINK/2.0f;
		smoke->base.y+=SMOKE_SHRINK/2.0f;
		prevsmoke=smoke;
		smoke=smoke->next;
	}

	// player
	state->player.base.x+=state->player.xv;
	state->player.base.y+=state->player.yv;
	if(state->player.reload>0)--state->player.reload;
	if(state->player.timer_smoke==0){
		newsmoke(state,&state->player.base,0.4f,0.3f);
		state->player.timer_smoke=PLAYER_SMOKE;
	}
	else --state->player.timer_smoke;

	// joysticks
	state->joy_top.x=state->joy_base.x+(JOYBASE_SIZE/2.0f)-(JOYTOP_SIZE/2.0f);
	state->joy_top.y=state->joy_base.y+(JOYBASE_SIZE/2.0f)-(JOYTOP_SIZE/2.0f);
	state->fire=false;
	state->player.xv=-cosf(state->player.base.rot)*PLAYER_SPEED;
	state->player.yv=-sinf(state->player.base.rot)*PLAYER_SPEED;
	align(&state->player.base.rot,PLAYER_TURN_SPEED,state->player.targetrot);
	for(int i=0;i<2;++i){
		if(!state->pointer[i].active)
			continue;
		if(state->pointer[i].x>0.0f){
			// fire button
			if(state->pointer[i].x>state->joy_fire.x&&state->pointer[i].x<state->joy_fire.x+JOYFIRE_SIZE&&
					state->pointer[i].y>state->joy_fire.y&&state->pointer[i].y<state->joy_fire.y+JOYFIRE_SIZE){
				state->fire=true;
			}
		}
		else{
			// joystick
			state->joy_top.x=state->pointer[i].x-(JOYTOP_SIZE/2.0f);
			state->joy_top.y=state->pointer[i].y-(JOYTOP_SIZE/2.0f);
			float angle=atan2f((state->joy_base.y+(JOYBASE_SIZE/2.0f))-(state->joy_top.y+(JOYTOP_SIZE/2.0f)),
					(state->joy_base.x+(JOYBASE_SIZE/2.0f))-(state->joy_top.x+(JOYTOP_SIZE/2.0f)));
			state->player.targetrot=angle;

			if(distance(state->joy_top.x+(JOYTOP_SIZE/2.0f),state->joy_base.x+(JOYBASE_SIZE/2.0f),
						state->joy_top.y+(JOYTOP_SIZE/2.0f),state->joy_base.y+(JOYBASE_SIZE/2.0f))>JOYTOP_DIST){
				state->joy_top.x=(state->joy_base.x+(JOYBASE_SIZE/2.0f)-(JOYTOP_SIZE/2.0f))-cosf(angle)*JOYTOP_DIST;
				state->joy_top.y=(state->joy_base.y+(JOYBASE_SIZE/2.0f)-(JOYTOP_SIZE/2.0f))-sinf(angle)*JOYTOP_DIST;
			}
		}
	}
	
	return true;
}

void render(struct state *state){
	glClear(GL_COLOR_BUFFER_BIT);
	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	/*glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BACKGROUND].object);
	draw(state,&state->background);*/

	// clouds
	if(state->cloudlist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_CLOUD].object);
		for(struct cloud *cloud=state->cloudlist;cloud!=NULL;cloud=cloud->next)
			draw(state,&cloud->base);
	}

	// smoke
	if(state->smokelist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_SMOKE].object);
		for(struct smoke *smoke=state->smokelist;smoke!=NULL;smoke=smoke->next){
			glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,smoke->alpha);
			draw(state,&smoke->base);
		}
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	}
	
	// bullets
	if(state->bulletlist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BULLET].object);
		for(struct bullet *bullet=state->bulletlist;bullet!=NULL;bullet=bullet->next)
			draw(state,&bullet->base);
	}

	// player
	glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_PLAYER].object);
	draw(state,&state->player.base);

	//enemies
	if(state->enemylist){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_ENEMY].object);
		for(struct enemy *enemy=state->enemylist;enemy!=NULL;enemy=enemy->next)
			draw(state,&enemy->base);
	}

	// fire button
	if(state->fire)
		glUniform4f(state->uniform.rgba,0.6f,0.6f,0.6f,1.0f);
	else
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_JOYFIRE].object);
	uidraw(state,&state->joy_fire);

	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	// joysticks
	glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_JOYBASE].object);
	uidraw(state,&state->joy_base);
	glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_JOYTOP].object);
	uidraw(state,&state->joy_top);
	
	{
		static char statustext[50];
		static int lasttime,fps;
		int currenttime=time(NULL);
		if(currenttime!=lasttime){
			lasttime=currenttime;
			sprintf(statustext,"[ace] %d fps",fps);
			fps=0;
		}
		else ++fps;
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		drawtext(state->font.main,state->rect.left+0.05f,state->rect.top+0.05f,statustext);
	}
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
	state->joy_fire.x=5.0f;
	state->joy_fire.y=1.6f;
	state->joy_fire.rot=0.0f;
	state->joy_fire.w=JOYFIRE_SIZE;
	state->joy_fire.h=JOYFIRE_SIZE;

	state->player.base.w=PLAYER_WIDTH;
	state->player.base.h=PLAYER_HEIGHT;

	state->cloudlist=NULL;
	state->enemylist=NULL;
	state->bulletlist=NULL;
	state->smokelist=NULL;
}

void reset(struct state *state){
	for(struct cloud *cloud=state->cloudlist;cloud!=NULL;cloud=deletecloud(state,cloud,NULL));
	state->cloudlist=NULL;
	for(struct bullet *bullet=state->bulletlist;bullet!=NULL;bullet=deletebullet(state,bullet,NULL));
	state->bulletlist=NULL;
	for(struct smoke *smoke=state->smokelist;smoke!=NULL;smoke=deletesmoke(state,smoke,NULL));
	state->smokelist=NULL;
	for(struct enemy *enemy=state->enemylist;enemy!=NULL;enemy=deleteenemy(state,enemy,NULL));
	state->enemylist=NULL;
	
	state->fire=false;
	state->player.base.x=-PLAYER_WIDTH/2.0f;
	state->player.base.y=-PLAYER_HEIGHT/2.0f;
	state->player.xv=0.0f;
	state->player.yv=PLAYER_SPEED;
	state->player.targetrot=0.0f;
	state->player.base.rot=0.0f;
	state->player.reload=0;
	state->player.timer_smoke=0;
}

