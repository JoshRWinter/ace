#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include <math.h>
#include "defs.h"

void newenemy(struct state *state){
	int count=0;
	for(struct enemy *enemy=state->enemylist;enemy!=NULL;enemy=enemy->next)
		++count;
	if(count>2)
		return;

	struct enemy *enemy=malloc(sizeof(struct enemy));
	enemy->base.w=ENEMY_WIDTH;
	enemy->base.h=ENEMY_HEIGHT;
	enemy->base.x=state->player.base.x;
	enemy->base.y=state->player.base.y;
	enemy->base.rot=randomint(1,360)*(M_PI/180.0f);
	enemy->target.w=ENEMY_WIDTH;
	enemy->target.h=ENEMY_HEIGHT;
	enemy->target.x=enemy->base.x;
	enemy->target.y=enemy->base.y;
	enemy->target.rot=0.0f;
	enemy->timer_smoke=0;
	enemy->next=state->enemylist;
	state->enemylist=enemy;
}
struct enemy *deleteenemy(struct state *state,struct enemy *enemy,struct enemy *prev){
	if(prev!=NULL)prev->next=enemy->next;
	else state->enemylist=enemy->next;
	void *temp=enemy->next;
	free(temp);
	return temp;
}

void newmissile(struct state *state,struct enemy *enemy){
	int count=0;
	for(struct missile *missile=state->missilelist;missile!=NULL;missile=missile->next)
		++count;
	if(count>1)
		return;
	struct missile *missile=malloc(sizeof(struct missile));
	missile->base.w=MISSILE_WIDTH;
	missile->base.h=MISSILE_HEIGHT;
	missile->base.x=enemy->base.x+(ENEMY_WIDTH/2.0f)-(MISSILE_WIDTH/2.0f);
	missile->base.y=enemy->base.y+(ENEMY_HEIGHT/2.0f)-(MISSILE_HEIGHT/2.0f);
	missile->base.rot=enemy->base.rot;
	missile->timer_smoke=0;
	missile->dead=false;
	missile->next=state->missilelist;
	state->missilelist=missile;
}
struct missile *deletemissile(struct state *state,struct missile *missile,struct missile *prev){
	if(prev!=NULL)prev->next=missile->next;
	else state->missilelist=missile->next;
	void *temp=missile->next;
	free(missile);
	return temp;
}

void newbullet(struct state *state,struct base *owner){
	const float DISPLACE=0.2f;
	const float ANGLE_OFFSET=0.9f;

	struct bullet *b1=malloc(sizeof(struct bullet));
	struct bullet *b2=malloc(sizeof(struct bullet));

	b1->base.w=BULLET_WIDTH;
	b1->base.h=BULLET_HEIGHT;
	b1->base.x=(owner->x+(owner->w/2.0f)-(BULLET_WIDTH/2.0f))-(cosf(owner->rot+ANGLE_OFFSET)*DISPLACE);
	b1->base.y=(owner->y+(owner->h/2.0f)-(BULLET_HEIGHT/2.0f))-(sinf(owner->rot+ANGLE_OFFSET)*DISPLACE);
	b1->base.rot=owner->rot;
	b1->xv=-cosf(owner->rot)*BULLET_SPEED;
	b1->yv=-sinf(owner->rot)*BULLET_SPEED;
	b1->owner=owner;
	b1->next=state->bulletlist;
	state->bulletlist=b1;

	b2->base.w=BULLET_WIDTH;
	b2->base.h=BULLET_HEIGHT;
	b2->base.x=(owner->x+(owner->w/2.0f)-(BULLET_WIDTH/2.0f))-(cosf(owner->rot-ANGLE_OFFSET)*DISPLACE);
	b2->base.y=(owner->y+(owner->h/2.0f)-(BULLET_HEIGHT/2.0f))-(sinf(owner->rot-ANGLE_OFFSET)*DISPLACE);
	b2->base.rot=owner->rot;
	b2->xv=-cosf(owner->rot)*BULLET_SPEED;
	b2->yv=-sinf(owner->rot)*BULLET_SPEED;
	b2->owner=owner;
	b2->next=state->bulletlist;
	state->bulletlist=b2;
}
struct bullet *deletebullet(struct state *state,struct bullet *bullet,struct bullet *prev){
	if(prev!=NULL)prev->next=bullet->next;
	else state->bulletlist=bullet->next;
	void *temp=bullet->next;
	free(bullet);
	return temp;
}

void newsmoke(struct state *state,struct base *base,float size,float alpha){
	const float SMOKE_SPEED=0.01f;
	struct smoke *smoke=malloc(sizeof(struct smoke));
	smoke->base.w=size;
	smoke->base.h=size;
	smoke->base.x=base->x+(base->w/2.0f)-(size/2.0f);
	smoke->base.y=base->y+(base->h/2.0f)-(size/2.0f);
	smoke->base.rot=randomint(1,360)*(M_PI/180.0f);
	smoke->alpha=alpha;
	smoke->xv=cosf(base->rot)*SMOKE_SPEED;
	smoke->yv=sinf(base->rot)*SMOKE_SPEED;
	smoke->next=state->smokelist;
	state->smokelist=smoke;
}
struct smoke *deletesmoke(struct state *state,struct smoke *smoke,struct smoke *prev){
	if(prev!=NULL)prev->next=smoke->next;
	else state->smokelist=smoke->next;
	void *temp=smoke->next;
	free(smoke);
	return temp;
}

void newcloud(struct state *state){
	int count=0;
	for(struct cloud *cloud=state->cloudlist;cloud!=NULL;cloud=cloud->next)
		++count;
	if(count>12)
		return;

	while(count<5){
		struct cloud *cloud=malloc(sizeof(struct cloud));
		cloud->base.w=CLOUD_SIZE;
		cloud->base.h=CLOUD_SIZE;
		cloud->base.rot=0.0f;
		float xoffset=randomint(8.0f,11.0f)*(onein(2)?1.0f:-1.0f);
		float yoffset=randomint(8.0f,11.0f)*(onein(2)?1.0f:-1.0f);
		cloud->base.x=(state->player.base.x+(PLAYER_WIDTH/2.0f))+xoffset;
		cloud->base.y=(state->player.base.y+(PLAYER_HEIGHT/2.0f))+yoffset;
		cloud->next=state->cloudlist;
		state->cloudlist=cloud;
		++count;
	}
}
struct cloud *deletecloud(struct state *state,struct cloud *cloud,struct cloud *prev){
	if(prev!=NULL)prev->next=cloud->next;
	else state->cloudlist=cloud->next;
	void *temp=cloud->next;
	free(cloud);
	return temp;
}

void newexplosion(struct state *state,float x,float y,float size){
	struct explosion *explosion=malloc(sizeof(struct explosion));
	for(int i=0;i<EXPLOSION_FLASH_COUNT;++i){
		explosion->flash[i].base.x=randomint((x-size)*10.0f,(x+size)*10.0f)/10.0f;
		explosion->flash[i].base.y=randomint((y-size)*10.0f,(y+size)*10.0f)/10.0f;
		explosion->flash[i].base.w=0.0f;
		explosion->flash[i].base.h=0.0f;
		explosion->flash[i].base.rot=0.0f;
		explosion->flash[i].maxsize=randomint(EXPLOSION_FLASH_MIN_SIZE*10.0f,EXPLOSION_FLASH_MAX_SIZE*10.0f)/10.0f;
		explosion->flash[i].growing=true;
		explosion->flash[i].timer_delay=randomint(EXPLOSION_FLASH_MIN_TIMER,EXPLOSION_FLASH_MAX_TIMER);
		int color=randomint(0,2);
		switch(color){
			case 0:
				explosion->flash[i].rgb[0]=1.0f;
				explosion->flash[i].rgb[1]=0.0f;
				explosion->flash[i].rgb[2]=0.0f;
				break;
			case 1:
				explosion->flash[i].rgb[0]=1.0f;
				explosion->flash[i].rgb[1]=1.0f;
				explosion->flash[i].rgb[2]=0.0f;
				break;
			case 2:
				explosion->flash[i].rgb[0]=1.0f;
				explosion->flash[i].rgb[1]=0.6f;
				explosion->flash[i].rgb[2]=0.1f;
				break;
		}
	}
	explosion->next=state->explosionlist;
	state->explosionlist=explosion;
}
struct explosion *deleteexplosion(struct state *state,struct explosion *explosion,struct explosion *prev){
	if(prev!=NULL)prev->next=explosion->next;
	else state->explosionlist=explosion->next;
	void *temp=explosion->next;
	free(explosion);
	return temp;
}

